#pragma once

#include "common_types.h"
#include <vector>
#include <optional>
#include <unordered_map>
#include <map>
#include <memory>
#include <utility>

// ============================================================================
// MapManager: 多分辨率地图块管理器
// ============================================================================
//
// 数据结构设计:
//   grids_ : map<Resolution, Grid>
//     Resolution  = (height, width) — 第几层精度，控制网格切分粒度
//     Grid        = unordered_map<Key, vector<shared_ptr<Tile>>>
//     Key         = (row << 32) | col — 将 2D 网格坐标编码为 uint64_t
//
//   两层索引:
//     Layer 1 — 按分辨率分区: {10,10} 是粗精度层, {1000,1000} 是细精度层
//     Layer 2 — 每层内按 cell 分组: 每个 cell 存一份 shared_ptr<Tile> 列表
//              同一 Tile 跨越多个 cell，但只分配一次，各 cell 共享同一指针
//
//   查询流程:
//     1. 经纬度 → Jw2Coord → (row, col) → coordEncoder → Key
//     2. Grid[Key] → 该 cell 覆盖的所有 Tile 指针
//     3. in_tile / overlaps_rect → 精确判定
//
//   经度环绕(日期线)处理:
//     当 top_left.lon > bottom_right.lon (如 175° → -175°)，tile 跨 ±180°
//     in_tile 用 || 判断，insert/remove 分两段写入 cell
// ============================================================================

class MapManager {
public:
    MapManager() = default;

    // 设置地图全局经纬度边界，默认 lat[-90,90] lon[-180,180]
    void set_bounds(double min_lat, double max_lat,
                    double min_lon, double max_lon);

    // ========================================================================
    // Q1: 基础增删查
    // ========================================================================

    // 将 tile 加入指定分辨率层的网格索引
    // tile 会分配一次 shared_ptr，跨越的所有 cell 存储同一指针
    bool insert(const Tile& tile, const Resolution& res);

    // 从指定分辨率层删除 tile (按坐标值匹配，只删第一个匹配的)
    bool remove(const Tile& tile, const Resolution& res);

    // 获取指定分辨率层的全部 tile (已去重)
    std::vector<Tile> get_all(const Resolution& res) const;

    // ========================================================================
    // Q2: 空间查询
    // ========================================================================

    // 点查询: 返回包含该点的第一个 tile
    std::optional<Tile> query_point(double latitude, double longitude,
                                    const Resolution& res) const;

    // 范围查询(单参数, 度数感知):
    //   range < 0  → [warn] 取绝对值后重新判断
    //   range >=180 → [warn] clamp 到 180
    //   range > 90 → [warn] 超纬度上限 → lon_range=range, lat_range=0
    //   range <=90 → [info] lon_range=range, lat_range=range
    std::vector<Tile> query_range(double latitude, double longitude,
                                  double range,
                                  const Resolution& res) const;

    // 范围查询(双参数, 显式指定):
    //   lon_range  — 经度方向半跨度
    //   lat_range  — 纬度方向半跨度 (-inf 哨兵 → 回退单参数逻辑)
    std::vector<Tile> query_range(double latitude, double longitude,
                                  double lon_range, double lat_range,
                                  const Resolution& res) const;

    // 检查某坐标是否落入任意已注册 tile 内
    bool contains(double latitude, double longitude,
                  const Resolution& res) const;

    // ========================================================================
    // Q3: 多分辨率管理
    // ========================================================================

    // 返回当前所有已注册的分辨率层级
    std::vector<Resolution> supported_resolutions() const;

private:
    // ---- 类型别名 ----
    using Key     = uint64_t;                       // (row<<32)|col 编码
    using TilePtr = std::shared_ptr<const Tile>;    // 共享指针，避免跨 cell 拷贝
    using Grid    = std::unordered_map<Key, std::vector<TilePtr>>;

    // ---- 地图边界 ----
    double min_lat_ = -90.0, max_lat_ = 90.0;
    double min_lon_ = -180.0, max_lon_ = 180.0;

    // ---- 核心存储: 分辨率 → 网格 → tile 列表 ----
    std::map<Resolution, Grid> grids_;

    // ---- 内部辅助函数 ----

    // 经纬度 → 网格坐标 (row, col): 线性映射 + clamp
    std::pair<uint32_t, uint32_t> Jw2Coord(double lat, double lon,
                                           const Resolution& res) const;

    // 网格坐标 → Key: row 存高 32 位, col 存低 32 位
    static Key coordEncoder(uint32_t row, uint32_t col);

    // 点是否在 tile 矩形内 (处理日期线穿越)
    static bool in_tile(const Tile& tile, double lat, double lon);

    // 查询矩形与 tile 是否相交 (处理日期线穿越，三窗口法)
    static bool overlaps_rect(const Tile& tile,
                              double q_min_lat, double q_max_lat,
                              double q_min_lon, double q_max_lon);

    // 单参数度数 → (lon_range, lat_range)，含 warn/info 输出
    static std::pair<double, double> resolve_range(double degrees);

    // 给定矩形区域，扫描覆盖的 cell，由 overlaps_rect 精确过滤后返回
    std::vector<Tile> rect_query_impl(const Grid& grid, const Resolution& res,
                                      double q_min_lat, double q_max_lat,
                                      double q_min_lon, double q_max_lon) const;
};
