#include "map_manager.h"
#include "graph.h"
#include <iostream>
#include <cassert>

using namespace std;

int main() {
    // ---- MapManager ----
    cout << "=== MapManager ===" << endl;

    MapManager mgr;

    Tile a = {
        {40.0, 116.0, 0.0},
        {39.0, 117.0, 0.0}
    };

    Tile b = {
        {41.0, 116.0, 0.0},
        {40.5, 116.5, 0.0}
    };

    Resolution low  = {10, 10};
    Resolution high = {100, 100};

    // Q1: insert / get_all
    assert(mgr.insert(a, low));
    assert(mgr.insert(b, low));
    assert(mgr.insert(a, high));
    assert(mgr.get_all(low).size() == 2);
    cout << "Q1 insert/get_all: OK" << endl;

    // Q1: remove
    assert(mgr.remove(a, low));
    assert(mgr.get_all(low).size() == 1);
    cout << "Q1 remove: OK" << endl;

    mgr.insert(a, low);

    // Q2: query_point
    auto found = mgr.query_point(39.5, 116.5, low);
    assert(found.has_value());
    cout << "Q2 query_point: OK" << endl;

    // Q2: contains
    assert(mgr.contains(40.5, 116.3, low));
    cout << "Q2 contains: OK" << endl;

    // Q2: query_range — 单个数 (<=90°, 经纬同范围)
    auto tiles = mgr.query_range(39.5, 116.5, 1.0, low);
    assert(tiles.size() >= 1);
    cout << "Q2 query_range (single <=90): OK" << endl;

    // Q2: query_range — 两个数 (lon_range=1.0, lat_range=0.5)
    auto tiles2 = mgr.query_range(39.5, 116.5, 1.0, 0.5, low);
    assert(tiles2.size() >= 1);
    cout << "Q2 query_range (two args): OK" << endl;

    // Q2: query_range — 单个数 (>90°, 仅经度有范围)
    cout << "\n--- degree-aware tests ---" << endl;
    auto tiles3 = mgr.query_range(39.5, 116.5, 120.0, low);
    // 120° > 90 → lon_range=120, lat_range=0 → 仅纬度方向无扩展
    cout << "Q2 query_range (single >90): " << tiles3.size() << " tiles" << endl;

    // Q2: query_range — negative (auto abs)
    auto tiles4 = mgr.query_range(39.5, 116.5, -30.0, low);
    assert(tiles4.size() >= 1);
    cout << "Q2 query_range (negative -> abs): OK" << endl;

    // Q2: query_range — >=180 (auto %180)
    auto tiles5 = mgr.query_range(39.5, 116.5, 200.0, low);
    cout << "Q2 query_range ( >=180 -> %180): " << tiles5.size() << " tiles" << endl;

    // Q2: date-line crossing
    Tile date_tile = {
        {42.0, 175.0,  0.0},
        {40.0, -175.0, 0.0}
    };
    assert(mgr.insert(date_tile, low));
    assert(mgr.contains(41.0, 179.0, low));
    assert(mgr.contains(41.0, -178.0, low));
    assert(!mgr.contains(41.0, 0.0, low));

    auto date_query = mgr.query_range(41.0, 178.0, 3.0, low);
    assert(!date_query.empty());
    cout << "Q2 date-line wrap: OK" << endl;

    // Q3
    auto ress = mgr.supported_resolutions();
    assert(ress.size() == 2);
    cout << "Q3 resolutions: " << ress.size() << " levels" << endl;

    // ---- Graph ----
    cout << "\n=== Graph ===" << endl;

    Graph g;
    g.add_edge(0, 1);
    g.add_edge(1, 2);
    g.add_edge(2, 3);
    g.add_edge(0, 3, 10.0);

    assert(g.node_count() == 4);
    cout << "Graph node_count: OK" << endl;

    auto path = g.find_path(0, 3);
    assert(path.has_value());
    assert(path->size() == 4);
    cout << "Path 0->3: ";
    for (int v : *path) cout << v << " ";
    cout << "(expected 0 1 2 3)" << endl;

    cout << "\nAll tests passed." << endl;
    return 0;
}
