#include "map_manager.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <cassert>
#include <string>

using namespace std;

void print_separator(const string& title) {
    cout << "\n================================================" << endl;
    cout << "  " << title << endl;
    cout << "================================================\n" << endl;
}

// ======================================================================
// Mode 1: 自动运行全部测试
// ======================================================================
void run_auto_tests() {
    cout << fixed << setprecision(4);

    // --- Q1: 基础增删查 ---
    print_separator("Q1: 插入 / 检索 / 删除");

    MapManager mgr;
    Resolution low{10, 10}, high{100, 100};

    Tile tile_a = { {40.0, 116.0, 0.0}, {39.0, 117.0, 0.0} };
    Tile tile_b = { {41.0, 116.0, 0.0}, {40.5, 116.5, 0.0} };
    Tile date_tile = { {42.0, 175.0, 0.0}, {40.0, -175.0, 0.0} };

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

    auto all_low = mgr.get_all(low);
    cout << "\nget_all(low): 共 " << all_low.size() << " 个 tile (期望 2)" << endl;
    for (size_t i = 0; i < all_low.size(); ++i) {
        cout << "   [" << i << "] tl(" << all_low[i].top_left.latitude << ", "
             << all_low[i].top_left.longitude << ") br("
             << all_low[i].bottom_right.latitude << ", "
             << all_low[i].bottom_right.longitude << ")" << endl;
    }
    assert(all_low.size() == 2);

    cout << "\n删除 Tile A (low 层)..." << endl;
    assert(mgr.remove(tile_a, low));
    auto after_remove = mgr.get_all(low);
    cout << "删除后 get_all(low): 共 " << after_remove.size() << " 个 tile (期望 1)" << endl;
    assert(after_remove.size() == 1);
    cout << "[OK] remove 成功, shared_ptr 引用计数递减" << endl;

    mgr.insert(tile_a, low);

    // --- Q2: 空间查询 ---
    print_separator("Q2: 点查询 / 范围查询 / 包含判断");

    double q_lat = 39.5, q_lon = 116.5;
    auto found = mgr.query_point(q_lat, q_lon, low);
    cout << "query_point(" << q_lat << "°, " << q_lon << "°, low):" << endl;
    if (found) {
        cout << "  命中 Tile: tl(" << found->top_left.latitude << ", " << found->top_left.longitude
             << ") br(" << found->bottom_right.latitude << ", " << found->bottom_right.longitude << ")" << endl;
    }
    assert(found.has_value());
    cout << "[OK] 点 (" << q_lat << "°, " << q_lon << "°) 在 Tile A 内" << endl;

    cout << "\ncontains 测试:" << endl;
    cout << "   (40.5°, 116.3°) -> " << (mgr.contains(40.5, 116.3, low) ? "YES" : "NO") << " (期望 YES)" << endl;
    cout << "   (0.0°, 0.0°)    -> " << (mgr.contains(0.0, 0.0, low) ? "YES" : "NO") << " (期望 NO)" << endl;
    assert(mgr.contains(40.5, 116.3, low));
    assert(!mgr.contains(0.0, 0.0, low));
    cout << "[OK] contains 正确" << endl;

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

    cout << "\n--- query_range 双参数 ---" << endl;
    cout << "输入: lat=39.5°, lon=116.5°, lon_range=1.0°, lat_range=0.5°" << endl;
    auto tiles2 = mgr.query_range(39.5, 116.5, 1.0, 0.5, low);
    cout << "  输出: " << tiles2.size() << " 个 tile (期望 >=1)" << endl;
    assert(tiles2.size() >= 1);
    cout << "[OK] 双参数范围查询可用" << endl;

    // --- 度感知边界 ---
    print_separator("Q2 续: 度感知边界测试");

    cout << "输入 range=120° (>90, 超纬度上限):" << endl;
    auto tiles3 = mgr.query_range(39.5, 116.5, 120.0, low);
    cout << "  输出: " << tiles3.size() << " 个 tile (lon_range=120°, lat_range=0°)" << endl;
    assert(tiles3.size() >= 1);

    cout << "\n输入 range=-30° (负数, 自动 abs):" << endl;
    auto tiles4 = mgr.query_range(39.5, 116.5, -30.0, low);
    cout << "  输出: " << tiles4.size() << " 个 tile (等价于 range=30°)" << endl;
    assert(tiles4.size() >= 1);

    cout << "\n输入 range=200° (>=180, clamp to 180):" << endl;
    auto tiles5 = mgr.query_range(39.5, 116.5, 200.0, low);
    cout << "  输出: " << tiles5.size() << " 个 tile (lon_range=180°, lat_range=0° 全球扫描)" << endl;

    cout << "\n--- 双参数回退到单参数 (lat_range 未提供, 哨兵 -inf) ---" << endl;
    auto tiles6 = mgr.query_range(39.5, 116.5, 1.0, -std::numeric_limits<double>::infinity(), low);
    cout << "  输出: " << tiles6.size() << " 个 tile (自动回退到单参数逻辑, range=1° -> 1°×1°)" << endl;
    assert(tiles6.size() >= 1);

    cout << "\n[OK] 度感知边界: 负数→abs | >=180→clamp | >90→纯经度 | <=90→经纬同范围 | 双参数可回退" << endl;

    // --- 日期线 ---
    print_separator("Q2 续: 国际日期线 (±180°) 穿越测试");

    cout << "插入跨日期线 Tile: tl(" << date_tile.top_left.latitude << "°, "
         << date_tile.top_left.longitude << "°) br("
         << date_tile.bottom_right.latitude << "°, "
         << date_tile.bottom_right.longitude << "°)" << endl;
    cout << "  实际经度跨度: 175° → -175° = 10°" << endl;
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

    cout << "\n[OK] 日期线穿越: in_tile || 判断 / overlaps_rect 三窗口法 / insert 分两段写" << endl;

    // --- Q3: 多分辨率 ---
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
    cout << "\n[OK] 多分辨率同时管理, 不同分辨率层数据完全隔离" << endl;

    cout << "\n================================================" << endl;
    cout << "  Q1-Q3 MapManager 全部测试通过" << endl;
    cout << "================================================\n" << endl;
}

// ======================================================================
// Mode 2: 手动交互式测试
// ======================================================================
void run_manual_mode() {
    cout << fixed << setprecision(4);
    MapManager mgr;

    cout << "\n请输入地图全局网格分辨率 (height width, 如: 10 10): ";
    uint32_t h, w;
    cin >> h >> w;
    Resolution res{h, w};

    cout << "\n已创建分辨率层 (" << res.height << "×" << res.width << ")" << endl;

    int choice = 0;
    while (choice != 8) {
        cout << "\n------------------------------------------" << endl;
        cout << "  操作菜单:" << endl;
        cout << "  1. 插入 Tile" << endl;
        cout << "  2. 删除 Tile" << endl;
        cout << "  3. 查看全部 Tile (get_all)" << endl;
        cout << "  4. 点查询 (query_point)" << endl;
        cout << "  5. 范围查询 (query_range)" << endl;
        cout << "  6. 包含判断 (contains)" << endl;
        cout << "  7. 查看已注册分辨率" << endl;
        cout << "  8. 退出" << endl;
        cout << "------------------------------------------" << endl;
        cout << "请输入操作编号: ";
        cin >> choice;

        if (choice == 1) {
            double lat1, lon1, lat2, lon2;
            cout << "  请输入 top_left(lat lon), 如: 40.0 116.0" << endl;
            cout << "  > ";
            cin >> lat1 >> lon1;
            cout << "  请输入 bottom_right(lat lon), 如: 39.0 117.0" << endl;
            cout << "  > ";
            cin >> lat2 >> lon2;
            Tile tile = { {lat1, lon1, 0.0}, {lat2, lon2, 0.0} };
            mgr.insert(tile, res);
            cout << "  [OK] Tile 已插入" << endl;

        } else if (choice == 2) {
            double lat1, lon1, lat2, lon2;
            cout << "  请输入要删除的 Tile 坐标 (同插入时):" << endl;
            cout << "  top_left(lat lon): ";
            cin >> lat1 >> lon1;
            cout << "  bottom_right(lat lon): ";
            cin >> lat2 >> lon2;
            Tile tile = { {lat1, lon1, 0.0}, {lat2, lon2, 0.0} };
            bool ok = mgr.remove(tile, res);
            cout << "  " << (ok ? "[OK] 已删除" : "[FAIL] 未找到匹配 Tile") << endl;

        } else if (choice == 3) {
            auto all = mgr.get_all(res);
            cout << "  当前层共 " << all.size() << " 个 Tile:" << endl;
            for (size_t i = 0; i < all.size(); ++i) {
                cout << "    [" << i << "] tl(" << all[i].top_left.latitude << ", "
                     << all[i].top_left.longitude << ") br("
                     << all[i].bottom_right.latitude << ", "
                     << all[i].bottom_right.longitude << ")" << endl;
            }

        } else if (choice == 4) {
            double lat, lon;
            cout << "  请输入坐标(lat lon), 如: 39.5 116.5" << endl;
            cout << "  > ";
            cin >> lat >> lon;
            auto found = mgr.query_point(lat, lon, res);
            if (found) {
                cout << "  命中 Tile: tl(" << found->top_left.latitude << ", "
                     << found->top_left.longitude << ") br("
                     << found->bottom_right.latitude << ", "
                     << found->bottom_right.longitude << ")" << endl;
            } else {
                cout << "  未命中任何 Tile" << endl;
            }

        } else if (choice == 5) {
            cout << "  范围查询模式:" << endl;
            cout << "    1) 单参数 (度数感知自动解析)" << endl;
            cout << "    2) 双参数 (显式指定经度/纬度半跨度)" << endl;
            cout << "  请选择: ";
            int sub;
            cin >> sub;
            double lat, lon;
            cout << "  请输入中心坐标(lat lon), 如: 39.5 116.5" << endl;
            cout << "  > ";
            cin >> lat >> lon;

            vector<Tile> result;
            if (sub == 1) {
                double range;
                cout << "  请输入范围(度数), 如: 1.0 (支持负数和>90的边界值)" << endl;
                cout << "  > ";
                cin >> range;
                result = mgr.query_range(lat, lon, range, res);
            } else {
                double lon_r, lat_r;
                cout << "  请输入经度半跨度和纬度半跨度, 如: 1.0 0.5" << endl;
                cout << "  > ";
                cin >> lon_r >> lat_r;
                result = mgr.query_range(lat, lon, lon_r, lat_r, res);
            }
            cout << "  查询结果: " << result.size() << " 个 Tile" << endl;
            for (size_t i = 0; i < result.size(); ++i) {
                cout << "    [" << i << "] tl(" << result[i].top_left.latitude << ", "
                     << result[i].top_left.longitude << ") br("
                     << result[i].bottom_right.latitude << ", "
                     << result[i].bottom_right.longitude << ")" << endl;
            }

        } else if (choice == 6) {
            double lat, lon;
            cout << "  请输入坐标(lat lon), 如: 40.5 116.3" << endl;
            cout << "  > ";
            cin >> lat >> lon;
            bool in = mgr.contains(lat, lon, res);
            cout << "  坐标 (" << lat << "°, " << lon << "°) " << (in ? "在地图范围内" : "不在范围内") << endl;

        } else if (choice == 7) {
            auto ress = mgr.supported_resolutions();
            cout << "  已注册分辨率: " << ress.size() << " 层" << endl;
            for (auto& r : ress) {
                cout << "    Resolution(" << r.height << "×" << r.width << ")" << endl;
            }
        }
    }
    cout << "\n手动模式结束。\n" << endl;
}

// ======================================================================
int main() {
    cout << "\n================================================" << endl;
    cout << "  Q1-Q3: 多分辨率地图块管理器" << endl;
    cout << "================================================\n" << endl;

    cout << "请选择运行模式:" << endl;
    cout << "  1. 自动运行 (直接输出可视化测试结果)" << endl;
    cout << "  2. 手动输入 (交互式操作地图块)" << endl;
    cout << "请输入 (1/2, 默认 1): ";

    int mode;
    string input;
    cin >> input;
    try {
        mode = stoi(input);
    } catch (...) {
        mode = 1;
    }

    if (mode == 2) {
        run_manual_mode();
    } else {
        if (mode != 1) cout << "无效输入, 使用默认模式 1 (自动).\n" << endl;
        run_auto_tests();
    }

    return 0;
}
