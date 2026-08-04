#pragma once

#include <vector>
#include <optional>
#include <utility>

// ============================================================================
// Graph: 无向图 + 最短路径
// ============================================================================
//
// 数据结构:
//   邻接表 adj_[u] = [(v, weight), ...]
//   每添加一条无向边, 两端各推一个 (邻居, 权重)
//
// 寻路算法:
//   Dijkstra 优先队列 — 权重为 1.0 时退化为 BFS
//   O((V+E) log V)
// ============================================================================

class Graph {
public:
    Graph() = default;

    // 添加无向边 (u, v) 带权重 weight
    void add_edge(int u, int v, double weight = 1.0);

    // 返回节点数 (最大节点编号 + 1)
    int node_count() const;

    // 查找 start → end 的最短路径
    // 返回节点序列; 不可达返回 nullopt
    std::optional<std::vector<int>> find_path(int start, int end) const;

private:
    // 邻接表: adj_[u] = [(v, weight), (w, weight), ...]
    std::vector<std::vector<std::pair<int, double>>> adj_;
};
