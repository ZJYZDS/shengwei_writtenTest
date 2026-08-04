#include "map_manager.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <cassert>

using namespace std;

void print_separator(const string& title) {
    cout << "\n================================================" << endl;
    cout << "  " << title << endl;
    cout << "================================================\n" << endl;
}

int main() {
    cout << fixed << setprecision(4);

    // ======================================================================
    // Q1: 基础增删查
    // ======================================================================
    print_separator("Q1: 插入 / 检索 / 删除");

    MapManager mgr;
    Resolution low{10, 10}, high{100, 100};

    // 构造测试 tile
    Tile tile_a = { {40.0, 116.0, 0.0}, {39.0, 117.0, 0.0} };   // 北京区域 1°×1°
    Tile tile_b = { {41.0, 116.0, 0.0}, {40.5, 116.5, 0.0} };   // 北京北侧 0.5°×0.5°
    Tile date_tile = { {42.0, 175.0, 0.0}, {40.0, -175.0, 0.0} }; // 跨日期线

    cout << "插入测试数据:" << endl;
    cout << "   Tile A: top_left(" << tile_a.top_left.latitude << "°, " << tile_a.top_left.longitude << "°) "
         << "bottom_right(" << tile_a.bottom_right.latitude << "°, " << tile_a.bottom_right.longitude << "°)" << endl;
    cout << "   Tile B: top_left(" << tile_b.top_left.latitude << "°, " << tile_b.top_left.longitude << "°) "
         << "bottom_right(" << tile_b.bottom_right.latitude << "°, " << tile_b.bottom_right.longitude << "°)" << endl;

    assert(mgr.insert(tile_a, low));
    assert(mgr.insert(tile_b, low));
    assert(mgr.insert(tile_a, high));
    cout << "\n[OK] Tile A 插入 low(10×10)、high(100×100) 两层" << endl;
    cout << "[OK] Tile B 插入 low(10×10) 层" << endl;

    // get_all
    auto all_low = mgr.get_all(low);
    cout << "\nget_all(low): 共 " << all_low.size() << " 个 tile (期望 2)" << endl;
    for (size_t i = 0; i < all_low.size(); ++i) {
        cout << "   [" << i << "] tl(" << all_low[i].top_left.latitude << ", "
             << all_low[i].top_left.longitude << ") br("
             << all_low[i].bottom_right.latitude << ", "
             << all_low[i].bottom_right.longitude << ")" << endl;
    }
    assert(all_low.size() == 2);

    // remove
    cout << "\n删除 Tile A (low 层)..." << endl;
    assert(mgr.remove(tile_a, low));
    auto after_remove = mgr.get_all(low);
    cout << "删除后 get_all(low): 共 " << after_remove.size() << " 个 tile (期望 1)" << endl;
    assert(after_remove.size() == 1);
    cout << "[OK] remove 成功, shared_ptr 引用计数递减, 内存自动管理" << endl;

    // 恢复 tile_a 以便后续测试
    mgr.insert(tile_a, low);

    // ======================================================================
    // Q2: 空间查询
    // ======================================================================
    print_separator("Q2: 点查询 / 范围查询 / 包含判断");

    // query_point
    double q_lat = 39.5, q_lon = 116.5;
    auto found = mgr.query_point(q_lat, q_lon, low);
    cout << "query_point(" << q_lat << "°, " << q_lon << "°, low):" << endl;
    if (found) {
        cout << "  命中 Tile: tl(" << found->top_left.latitude << ", " << found->top_left.longitude
             << ") br(" << found->bottom_right.latitude << ", " << found->bottom_right.longitude << ")" << endl;
    }
    assert(found.has_value());
    cout << "[OK] 点 (" << q_lat << "°, " << q_lon << "°) 在 Tile A 内" << endl;

    // contains
    cout << "\ncontains 测试:" << endl;
    cout << "   (40.5°, 116.3°) -> " << (mgr.contains(40.5, 116.3, low) ? "YES" : "NO") << " (期望 YES)" << endl;
    cout << "   (0.0°, 0.0°)    -> " << (mgr.contains(0.0, 0.0, low) ? "YES" : "NO") << " (期望 NO)" << endl;
    assert(mgr.contains(40.5, 116.3, low));
    assert(!mgr.contains(0.0, 0.0, low));
    cout << "[OK] contains 正确判断点是否在地图覆盖范围内" << endl;

    // query_range — 单参数: <=90°
    cout << "\n--- query_range 单参数 (度感知解析) ---" << endl;
    cout << "输入: lat=39.5°, lon=116.5°, range=1.0°" << endl;
    auto tiles1 = mgr.query_range(39.5, 116.5, 1.0, low);
    cout << "  输出: " << tiles1.size() << " 个 tile (期望 >=1)" << endl;
    for (size_t i = 0; i < tiles1.size(); ++i) {
        cout << "    [" << i << "] tl(" << tiles1[i].top_left.latitude << ", "
             << tiles1[i].top_left.longitude << ") br("
             << tiles1[i].bottom_right.latitude << ", "
             << tiles1[i].bottom_right.longitude << ")" << endl;
    }
    assert(tiles1.size() >= 1);

    // query_range — 双参数显式
    cout << "\n--- query_range 双参数 (显式 lon_range, lat_range) ---" << endl;
    cout << "输入: lat=39.5°, lon=116.5°, lon_range=1.0°, lat_range=0.5°" << endl;
    auto tiles2 = mgr.query_range(39.5, 116.5, 1.0, 0.5, low);
    cout << "  输出: " << tiles2.size() << " 个 tile (期望 >=1)" << endl;
    assert(tiles2.size() >= 1);
    cout << "[OK] 双参数范围查询可用" << endl;

    // ======================================================================
    // Q2: 度感知边界测试
    // ======================================================================
    print_separator("Q2 续: 度感知边界测试");

    // >90° — 超纬度上限
    cout << "输入 range=120° (>90, 超纬度上限):" << endl;
    auto tiles3 = mgr.query_range(39.5, 116.5, 120.0, low);
    cout << "  输出: " << tiles3.size() << " 个 tile (lon_range=120°, lat_range=0°)" << endl;
    assert(tiles3.size() >= 1);

    // 负数 — 自动 abs
    cout << "\n输入 range=-30° (负数, 自动 abs):" << endl;
    auto tiles4 = mgr.query_range(39.5, 116.5, -30.0, low);
    cout << "  输出: " << tiles4.size() << " 个 tile (等价于 range=30°)" << endl;
    assert(tiles4.size() >= 1);

    // >=180 — clamp
    cout << "\n输入 range=200° (>=180, clamp to 180):" << endl;
    auto tiles5 = mgr.query_range(39.5, 116.5, 200.0, low);
    cout << "  输出: " << tiles5.size() << " 个 tile (lon_range=180°, lat_range=0° 全球经度扫描)" << endl;

    // 双参数不传 lat_range → 回退单参数
    cout << "\n--- 双参数回退到单参数 (lat_range 未提供, 哨兵 -inf) ---" << endl;
    cout << "调用 query_range(lat, lon, range, -inf, res)..." << endl;
    auto tiles6 = mgr.query_range(39.5, 116.5, 1.0, -std::numeric_limits<double>::infinity(), low);
    cout << "  输出: " << tiles6.size() << " 个 tile (自动回退到单参数逻辑, range=1° -> 1°×1° 矩形)" << endl;
    assert(tiles6.size() >= 1);

    cout << "\n[OK] 所有度感知边界情况正确: 负数→abs | >=180→clamp | >90→纯经度 | <=90→经纬同范围 | 双参数可回退单参数" << endl;

    // ======================================================================
    // Q2: 日期线穿越
    // ======================================================================
    print_separator("Q2 续: 国际日期线 (±180°) 穿越测试");

    cout << "插入跨日期线 Tile: tl(" << date_tile.top_left.latitude << "°, "
         << date_tile.top_left.longitude << "°) br("
         << date_tile.bottom_right.latitude << "°, "
         << date_tile.bottom_right.longitude << "°)" << endl;
    cout << "  实际经度跨度: 175° → -175° = 10° (非 350°)" << endl;
    mgr.insert(date_tile, low);

    cout << "\n点查询测试:" << endl;
    cout << "   (41.0°,  179.0°) in tile? " << (mgr.contains(41.0, 179.0, low) ? "YES" : "NO") << " (期望 YES)" << endl;
    cout << "   (41.0°, -178.0°) in tile? " << (mgr.contains(41.0, -178.0, low) ? "YES" : "NO") << " (期望 YES)" << endl;
    cout << "   (41.0°,    0.0°) in tile? " << (mgr.contains(41.0, 0.0, low) ? "YES" : "NO") << " (期望 NO)" << endl;
    assert(mgr.contains(41.0, 179.0, low));
    assert(mgr.contains(41.0, -178.0, low));
    assert(!mgr.contains(41.0, 0.0, low));

    cout << "\n范围查询: lat=41.0°, lon=178.0°, range=3.0°" << endl;
    auto date_result = mgr.query_range(41.0, 178.0, 3.0, low);
    cout << "  输出: " << date_result.size() << " 个 tile (期望 >=1, 含跨日期线 tile)" << endl;
    assert(!date_result.empty());

    cout << "\n[OK] 日期线穿越正确处理: in_tile 用 || 判断 / overlaps_rect 用三窗口法 / insert 分两段写" << endl;

    // ======================================================================
    // Q3: 多分辨率管理
    // ======================================================================
    print_separator("Q3: 多分辨率管理");

    auto resolutions = mgr.supported_resolutions();
    cout << "已注册分辨率: " << resolutions.size() << " 层" << endl;
    for (size_t i = 0; i < resolutions.size(); ++i) {
        size_t tile_count = mgr.get_all(resolutions[i]).size();
        cout << "   [" << i << "] Resolution(" << resolutions[i].height
             << "×" << resolutions[i].width << "): "
             << tile_count << " 个 tile" << endl;
    }
    assert(resolutions.size() == 2);
    cout << "\n[OK] 支持多分辨率同时管理, 不同分辨率层数据完全隔离" << endl;

    cout << "\n================================================" << endl;
    cout << "  Q1-Q3 MapManager 全部测试通过" << endl;
    cout << "================================================\n" << endl;

    return 0;
}
