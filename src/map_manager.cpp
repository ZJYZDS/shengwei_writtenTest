#include "map_manager.h"
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace std;

// ============================================================================
// resolve_range — 单参数度感知解析
// ============================================================================
//
// 决策树:
//   r < 0     → [warn] abs → 递归重新判断
//   r >= 180  → [warn] clamp to 180
//   r > 90    → [warn] 超纬度上限(90°) → lon_range=r, lat_range=0
//   r <= 90   → [info] 合法经纬度 → lon_range=r, lat_range=r
//
// 返回 (lon_range, lat_range) — 先经度后纬度
// ============================================================================

pair<double, double> MapManager::resolve_range(double r) {
    // 负数 → 取绝对值后重新判断
    if (r < 0) {
        cerr << "[warn] range = " << r << " < 0, taking abs" << endl;
        return resolve_range(-r);
    }
    // >= 180° → 截断到全球最大经度跨度 180°
    if (r >= 180) {
        cerr << "[warn] range = " << r << " >= 180, clamped to 180" << endl;
        r = 180;
    }
    // > 90° 且 < 180° → 超过纬度最大值(90°)，只能作为经度范围
    if (r > 90) {
        cerr << "[warn] range = " << r << " > 90 (lat max=90), lon_range=" << r
             << ", lat_range=0" << endl;
        return {r, 0.0};
    }
    // <= 90° → 经纬度均可接受
    cout << "[info] range = " << r << " <= 90, set as both lon_range and lat_range" << endl;
    return {r, r};
}

// ============================================================================
// set_bounds — 设置地图全局经纬度边界
// ============================================================================

void MapManager::set_bounds(double min_lat, double max_lat,
                            double min_lon, double max_lon) {
    min_lat_ = min_lat;
    max_lat_ = max_lat;
    min_lon_ = min_lon;
    max_lon_ = max_lon;
}

// ============================================================================
// Jw2Coord — 经纬度 → 网格坐标 (row, col)
// ============================================================================
//
// 线性映射: (lat - min_lat) / lat_range * height → row
//           (lon - min_lon) / lon_range * width  → col
// 边界 clamp 到 [0, size-1] 防止越界
// ============================================================================

pair<uint32_t, uint32_t> MapManager::Jw2Coord(double lat, double lon,
                                               const Resolution& res) const {
    double lat_range = max_lat_ - min_lat_;
    double lon_range = max_lon_ - min_lon_;
    uint32_t row = static_cast<uint32_t>((lat - min_lat_) / lat_range * res.height);
    uint32_t col = static_cast<uint32_t>((lon - min_lon_) / lon_range * res.width);
    // 边界值 clamp: 坐标恰好在 max 边界时算入最后一格
    row = min(row, res.height - 1);
    col = min(col, res.width  - 1);
    return {row, col};
}

// ============================================================================
// coordEncoder — 网格坐标 → Key (uint64_t)
// ============================================================================
//
// 编码规则: Key = (row << 32) | col
// row 占高 32 位, col 占低 32 位
// row/col 各 ≤ 2^32-1，实际场景远小于此值
// ============================================================================

MapManager::Key MapManager::coordEncoder(uint32_t row, uint32_t col) {
    return (static_cast<uint64_t>(row) << 32) | col;
}

// ============================================================================
// in_tile — 点是否在 tile 矩形内
// ============================================================================
//
// 纬度: [min_lat, max_lat] 区间判断 — 不再假定 top_left 一定是北边
//
// 经度: 先计算经度跨度 lon_max - lon_min
//   正常 tile (span <= 180°): 点在 [lon_min, lon_max] 内
//   日期线 tile (span > 180°): 点覆盖相反一侧, 在 [lon_max, 180°] ∪ [-180°, lon_min] 内
//   例如 tl=175, br=-175 → span=350 > 180 → 实际跨 10°, 点在 >=175 OR <=-175
//   例如 tl=117, br=100  → span=17 <= 180 → 用户仅交换东西角, 正常区间 [100, 117]
// ============================================================================

bool MapManager::in_tile(const Tile& tile, double lat, double lon) {
    double lat_min = min(tile.top_left.latitude, tile.bottom_right.latitude);
    double lat_max = max(tile.top_left.latitude, tile.bottom_right.latitude);
    bool lat_ok = lat >= lat_min && lat <= lat_max;

    double lon_min = min(tile.top_left.longitude, tile.bottom_right.longitude);
    double lon_max = max(tile.top_left.longitude, tile.bottom_right.longitude);

    bool lon_ok;
    if (lon_max - lon_min <= 180.0) {
        // 正常 tile: 连续经度区间 [lon_min, lon_max]
        lon_ok = lon >= lon_min && lon <= lon_max;
    } else {
        // 日期线 tile: 覆盖 [lon_max, 180°] ∪ [-180°, lon_min]
        lon_ok = lon >= lon_max || lon <= lon_min;
    }
    return lat_ok && lon_ok;
}

// ============================================================================
// overlaps_rect — 查询矩形与 tile 是否相交
// ============================================================================
//
// 纬度: 标准区间相交判断 (不再假定 top_left 一定是北边)
//
// 经度: 先归一化两端点, 计算跨度 lon_max - lon_min
//   正常 tile (span <= 180°): 连续区间 [lon_min, lon_max]
//   日期线 tile (span > 180°): 反向区间 [lon_max, lon_min+360]
// 然后用三窗口法 (-360/0/+360) 与查询矩形测试交集
// ============================================================================

bool MapManager::overlaps_rect(const Tile& tile,
                                double q_min_lat, double q_max_lat,
                                double q_min_lon, double q_max_lon) {
    double tile_min_lat = min(tile.top_left.latitude, tile.bottom_right.latitude);
    double tile_max_lat = max(tile.top_left.latitude, tile.bottom_right.latitude);
    if (q_max_lat < tile_min_lat || q_min_lat > tile_max_lat)
        return false;

    double lon_min = min(tile.top_left.longitude, tile.bottom_right.longitude);
    double lon_max = max(tile.top_left.longitude, tile.bottom_right.longitude);

    double a, b;
    if (lon_max - lon_min <= 180.0) {
        // 正常 tile: 连续区间 [lon_min, lon_max]
        a = lon_min;
        b = lon_max;
    } else {
        // 日期线 tile: 区间 [lon_max, lon_min+360] (如 175→185, 代表 175°→-175°)
        a = lon_max;
        b = lon_min + 360.0;
    }

    // 三窗口: -360° / 0° / +360° 偏移
    for (double shift : { -360.0, 0.0, 360.0 }) {
        if (max(a + shift, q_min_lon) <= min(b + shift, q_max_lon))
            return true;
    }
    return false;
}

// ============================================================================
// Q1: insert — 插入 tile
// ============================================================================
//
// 1. 分配一次 shared_ptr<Tile>
// 2. 归一化经纬度, 计算覆盖的 cell 范围
// 3. 遍历覆盖的所有 cell, 将指针写入每个 cell 的 vector
//
// 经度归一化: 计算 lon_min, lon_max, 跨度 = lon_max - lon_min
//   span <= 180°: 正常 tile, 连续列区间 [c_min, c_max]
//   span > 180°:  日期线 tile, 分两段 [c_max, width-1] 和 [0, c_min]
// 例如 tl=117,br=100 → span=17≤180 → 列范围 [Jw2Coord(100), Jw2Coord(117)]
// ============================================================================

bool MapManager::insert(const Tile& tile, const Resolution& res) {
    auto& grid = grids_[res];
    auto ptr = make_shared<const Tile>(tile);

    double lat_min = min(tile.top_left.latitude, tile.bottom_right.latitude);
    double lat_max = max(tile.top_left.latitude, tile.bottom_right.latitude);
    double lon_min = min(tile.top_left.longitude, tile.bottom_right.longitude);
    double lon_max = max(tile.top_left.longitude, tile.bottom_right.longitude);

    auto [r1, c1] = Jw2Coord(lat_min, lon_min, res);
    auto [r2, c2] = Jw2Coord(lat_max, lon_max, res);
    uint32_t r_begin = r1, r_end = r2;

    auto insert_columns = [&](uint32_t r, uint32_t c_begin, uint32_t c_end) {
        for (uint32_t c = c_begin; c <= c_end; ++c)
            grid[coordEncoder(r, c)].push_back(ptr);
    };

    for (uint32_t r = r_begin; r <= r_end; ++r) {
        if (lon_max - lon_min <= 180.0) {
            // 正常 tile: 连续经度区间 [lon_min, lon_max]
            insert_columns(r, c1, c2);
        } else {
            // 跨日期线: 分西段 [lon_max→180°] 和东段 [-180°→lon_min]
            insert_columns(r, c2, res.width - 1);
            insert_columns(r, 0, c1);
        }
    }
    return true;
}

// ============================================================================
// Q1: remove — 删除 tile
// ============================================================================
//
// 1. 归一化经纬度, 计算覆盖的 cell 范围 (同 insert)
// 2. 在每个 cell 的 vector 中按 6 字段精确匹配找到对应 shared_ptr
// 3. erase 该指针 (shared_ptr 引用计数 -1)
//    — 若其他 cell 不再引用, 内存自动释放
// ============================================================================

bool MapManager::remove(const Tile& tile, const Resolution& res) {
    auto it = grids_.find(res);
    if (it == grids_.end()) return false;
    auto& grid = it->second;

    double lat_min = min(tile.top_left.latitude, tile.bottom_right.latitude);
    double lat_max = max(tile.top_left.latitude, tile.bottom_right.latitude);
    double lon_min = min(tile.top_left.longitude, tile.bottom_right.longitude);
    double lon_max = max(tile.top_left.longitude, tile.bottom_right.longitude);

    auto [r1, c1] = Jw2Coord(lat_min, lon_min, res);
    auto [r2, c2] = Jw2Coord(lat_max, lon_max, res);
    uint32_t r_begin = r1, r_end = r2;

    auto remove_from_cell = [&](uint32_t r, uint32_t c) -> bool {
        auto& list = grid[coordEncoder(r, c)];
        auto erase_it = find_if(list.begin(), list.end(),
            [&](const TilePtr& p) {
                return p->top_left.latitude     == tile.top_left.latitude     &&
                       p->top_left.longitude    == tile.top_left.longitude    &&
                       p->top_left.altitude     == tile.top_left.altitude     &&
                       p->bottom_right.latitude == tile.bottom_right.latitude &&
                       p->bottom_right.longitude == tile.bottom_right.longitude &&
                       p->bottom_right.altitude == tile.bottom_right.altitude;
            });
        if (erase_it == list.end()) return false;
        list.erase(erase_it);
        return true;
    };

    bool removed = false;
    for (uint32_t r = r_begin; r <= r_end; ++r) {
        if (lon_max - lon_min <= 180.0) {
            for (uint32_t c = c1; c <= c2; ++c)
                removed |= remove_from_cell(r, c);
        } else {
            for (uint32_t c = c2; c < res.width; ++c)
                removed |= remove_from_cell(r, c);
            for (uint32_t c = 0; c <= c1; ++c)
                removed |= remove_from_cell(r, c);
        }
    }
    return removed;
}

// ============================================================================
// Q1: get_all — 获取某分辨率层全部 tile
// ============================================================================
//
// 同一 shared_ptr 可能出现在多个 cell → 按裸指针地址去重
// 每个 insert 调用创建一个 shared_ptr, 地址唯一
// ============================================================================

vector<Tile> MapManager::get_all(const Resolution& res) const {
    auto it = grids_.find(res);
    if (it == grids_.end()) return {};
    vector<Tile> result;
    // 用裸指针集合去重: 同一次 insert 的 shared_ptr 地址相同
    vector<const Tile*> seen;
    for (const auto& [key, tiles] : it->second) {
        for (const auto& ptr : tiles) {
            if (find(seen.begin(), seen.end(), ptr.get()) == seen.end()) {
                seen.push_back(ptr.get());
                result.push_back(*ptr);   // 拷贝 Tile 值返回
            }
        }
    }
    return result;
}

// ============================================================================
// Q2: query_point — 点查询
// ============================================================================
//
// 1. 经纬度 → Jw2Coord → (row, col) → coordEncoder → Key
// 2. Grid[Key] 拿到该 cell 的 tile 列表
// 3. 逐个 in_tile 精确判断, 返回第一个命中的
// ============================================================================

optional<Tile> MapManager::query_point(double latitude, double longitude,
                                       const Resolution& res) const {
    auto it = grids_.find(res);
    if (it == grids_.end()) return nullopt;
    auto [r, c] = Jw2Coord(latitude, longitude, res);
    auto cell_it = it->second.find(coordEncoder(r, c));
    if (cell_it == it->second.end()) return nullopt;
    for (const auto& ptr : cell_it->second) {
        if (in_tile(*ptr, latitude, longitude)) return *ptr;
    }
    return nullopt;
}

// ============================================================================
// rect_query_impl — 矩形范围查询核心实现
// ============================================================================
//
// 1. 查询矩形 → 算出覆盖的 cell 区间
// 2. 遍历 cell → 收集候选 tile (按裸指针去重)
// 3. overlaps_rect 精确过滤 (处理日期线)
// 4. 返回拷贝 (脱离 shared_ptr)
// ============================================================================

vector<Tile> MapManager::rect_query_impl(
    const Grid& grid, const Resolution& res,
    double q_min_lat, double q_max_lat,
    double q_min_lon, double q_max_lon) const
{
    // 查询矩形的四角 → cell 坐标
    auto [r1, c1] = Jw2Coord(q_min_lat, q_min_lon, res);
    auto [r2, c2] = Jw2Coord(q_max_lat, q_max_lon, res);
    uint32_t r_begin = min(r1, r2), r_end = max(r1, r2);
    uint32_t c_begin = min(c1, c2), c_end = max(c1, c2);

    vector<Tile> result;
    vector<const Tile*> seen;  // 裸指针去重: 同一次 insert 的 tile 只出现一次

    for (uint32_t r = r_begin; r <= r_end; ++r) {
        for (uint32_t c = c_begin; c <= c_end; ++c) {
            auto cell_it = grid.find(coordEncoder(r, c));
            if (cell_it == grid.end()) continue;
            for (const auto& ptr : cell_it->second) {
                // 精确相交判定: 不只是"沾 cell 就算"
                if (overlaps_rect(*ptr, q_min_lat, q_max_lat, q_min_lon, q_max_lon)) {
                    if (find(seen.begin(), seen.end(), ptr.get()) == seen.end()) {
                        seen.push_back(ptr.get());
                        result.push_back(*ptr);
                    }
                }
            }
        }
    }
    return result;
}

// ============================================================================
// Q2: query_range — 单参数 (度感知)
// ============================================================================
//
// 调用 resolve_range 将度数解析为 (lon_range, lat_range)
// 然后走 rect_query_impl
// ============================================================================

vector<Tile> MapManager::query_range(double latitude, double longitude,
                                     double range,
                                     const Resolution& res) const {
    auto it = grids_.find(res);
    if (it == grids_.end()) return {};

    auto [lon_range, lat_range] = resolve_range(range);

    double q_min_lat = latitude - lat_range;
    double q_max_lat = latitude + lat_range;
    double q_min_lon = longitude - lon_range;
    double q_max_lon = longitude + lon_range;

    return rect_query_impl(it->second, res,
                           q_min_lat, q_max_lat, q_min_lon, q_max_lon);
}

// ============================================================================
// Q2: query_range — 双参数 (lon_range, lat_range)
// ============================================================================
//
// lat_range == -inf 时为哨兵值 → 回退单参数度感知逻辑 [warn]
// 否则显式构造矩形后走 rect_query_impl
// ============================================================================

vector<Tile> MapManager::query_range(double latitude, double longitude,
                                     double lon_range, double lat_range,
                                     const Resolution& res) const {
    auto it = grids_.find(res);
    if (it == grids_.end()) return {};

    if (lat_range == -numeric_limits<double>::infinity()) {
        cerr << "[warn] lat_range not provided, "
                "falling back to single-number degree-aware logic" << endl;
        return query_range(latitude, longitude, lon_range, res);
    }

    double q_min_lat = latitude - lat_range;
    double q_max_lat = latitude + lat_range;
    double q_min_lon = longitude - lon_range;
    double q_max_lon = longitude + lon_range;

    return rect_query_impl(it->second, res,
                           q_min_lat, q_max_lat, q_min_lon, q_max_lon);
}

// ============================================================================
// Q2: contains — 检查坐标是否在任意 tile 内
// ============================================================================
//
// 直接复用 query_point 的 optional 返回值
// ============================================================================

bool MapManager::contains(double latitude, double longitude,
                          const Resolution& res) const {
    return query_point(latitude, longitude, res).has_value();
}

// ============================================================================
// Q3: supported_resolutions — 返回所有已注册分辨率
// ============================================================================

vector<Resolution> MapManager::supported_resolutions() const {
    vector<Resolution> result;
    for (const auto& [res, grid] : grids_) {
        result.push_back(res);
    }
    return result;
}
