#include "graph.h"
#include <queue>
#include <limits>
#include <algorithm>
#include <cmath>

using namespace std;

// ============================================================================
// add_edge — 添加无向边
// ============================================================================
//
// 邻接表自动扩容到 max(u,v)+1 大小
// 双向推入: adj_[u] += (v, weight), adj_[v] += (u, weight)
// ============================================================================

void Graph::add_edge(int u, int v, double weight) {
    int n = max(u, v) + 1;
    if (static_cast<int>(adj_.size()) < n) adj_.resize(n);
    adj_[u].emplace_back(v, weight);
    adj_[v].emplace_back(u, weight);
}

// ============================================================================
// node_count — 节点数
// ============================================================================

int Graph::node_count() const {
    return static_cast<int>(adj_.size());
}

// ============================================================================
// find_path — Dijkstra 最短路
// ============================================================================
//
// 优先队列做贪心扩张: 每次取 dist 最小节点, 松弛其邻居
// 已有更短记录的跳过 (abs(d - dist[u]) > 1e-9)
// 所有边权重为 1.0 时退化为 BFS
//
// 复杂度: O((V + E) log V)
// ============================================================================

optional<vector<int>> Graph::find_path(int start, int end) const {
    int n = node_count();
    if (start >= n || end >= n) return nullopt;

    // dist: 起点到各节点的最短距离
    // prev: 前驱节点, -1 表示无前驱
    vector<double> dist(n, numeric_limits<double>::infinity());
    vector<int> prev(n, -1);

    // 优先队列: 小顶堆, pair<dist, node>
    using State = pair<double, int>;
    priority_queue<State, vector<State>, greater<State>> pq;

    dist[start] = 0.0;
    pq.emplace(0.0, start);

    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        // 已有更短记录 → 跳过此过期状态
        if (abs(d - dist[u]) > 1e-9) continue;
        if (u == end) break;   // 终点已确定最短路, 提前终止

        for (const auto& [v, w] : adj_[u]) {
            double nd = d + w;
            if (nd < dist[v]) {
                dist[v] = nd;
                prev[v] = u;
                pq.emplace(nd, v);
            }
        }
    }

    // 终点不可达 (且起点 != 终点)
    if (prev[end] == -1 && start != end) return nullopt;

    // 回溯前驱链构建路径: end → ... → start → 反转
    vector<int> path;
    for (int u = end; u != -1; u = prev[u]) path.push_back(u);
    reverse(path.begin(), path.end());
    return path;
}
