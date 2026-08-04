#pragma once

#include <vector>
#include <optional>
#include <utility>
using namespace std;

// ============================================================================
// Graph: 无向图 + 全源最短路径
// ============================================================================
//
// 数据结构:
//   邻接表 adj_[u] = [(v, weight), ...]
//   距离矩阵 dist_[i][j] 和前驱矩阵 next_[i][j] — Floyd-Warshall 预计算
//
// 寻路算法:
//   Floyd-Warshall 全源最短路 — O(V^3) 一次预计算，每次查询 O(1) 查表 + O(P) 路径重建
//   适配任意 start/end 不固定的场景
// ============================================================================

class Graph {
public:
    Graph() = default;

    // 添加无向边 (u, v) 带权重 weight（无向图中每个边的weight set as 1）
    void add_edge(int u, int v, double weight = 1.0);

    // 返回节点数 (最大节点编号 + 1)
    int node_count() const;

    // 查找 start → end 的最短路径
    // 返回节点序列; 不可达返回 nullopt
    optional<vector<int>> find_path(int start, int end);

private:
    // 若邻接表有更新则重新运行 Floyd-Warshall
    void ensure_floyd();

    // 邻接表: adj_[u] = [(v, weight), (w, weight), ...]
    vector<vector<pair<int, double>>> adj_;

    // Floyd 距离矩阵: dist_[i][j] = i→j 最短距离
    vector<vector<double>> dist_;

    // Floyd 前驱矩阵: next_[i][j] = i 出发到 j 的下一步节点，-1 表示不可达
    vector<vector<int>> next_;

    bool dirty_ = true;
};
