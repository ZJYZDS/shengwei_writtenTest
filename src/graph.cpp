#include "../include/graph.h"
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
// 标记 dirty_=true 触发下次 find_path 时重新 Floyd
// ============================================================================

void Graph::add_edge(int u, int v, double weight) {
    int n = max(u, v) + 1;
    if (static_cast<int>(adj_.size()) < n) adj_.resize(n);
    adj_[u].emplace_back(v, weight);
    adj_[v].emplace_back(u, weight);
    dirty_ = true;
}

// ============================================================================
// node_count — 节点数
// ============================================================================

int Graph::node_count() const {
    return static_cast<int>(adj_.size());
}

// ============================================================================
// ensure_floyd — Floyd-Warshall 全源最短路预计算
// ============================================================================
//
// O(V^3) 三重循环:
//   初始化: dist[i][i]=0, 有边的 dist[i][j]=weight, next[i][j]=j
//   松弛:   dist[i][k] + dist[k][j] < dist[i][j] 时更新
//   不可达: dist[i][j]=inf, next[i][j]=-1
// ============================================================================

void Graph::ensure_floyd() {
    if (!dirty_) return;
    int n = node_count();
    if (n == 0) { dirty_ = false; return; }

    const double INF = numeric_limits<double>::infinity();

    // 初始化距离矩阵和前驱矩阵
    dist_.assign(n, vector<double>(n, INF));
    next_.assign(n, vector<int>(n, -1));

    for (int i = 0; i < n; ++i) {
        dist_[i][i] = 0.0;
        next_[i][i] = i;
    }

    for (int u = 0; u < n; ++u) {
        for (const auto& [v, w] : adj_[u]) {
            if (w < dist_[u][v]) {
                dist_[u][v] = w;
                next_[u][v] = v;
            }
        }
    }

    // Floyd 三重循环: k 为中间节点
    for (int k = 0; k < n; ++k) {
        for (int i = 0; i < n; ++i) {
            if (dist_[i][k] == INF) continue;
            for (int j = 0; j < n; ++j) {
                if (dist_[k][j] == INF) continue;
                double nd = dist_[i][k] + dist_[k][j];
                if (nd < dist_[i][j]) {
                    dist_[i][j] = nd;
                    next_[i][j] = next_[i][k];
                }
            }
        }
    }

    dirty_ = false;
}

// ============================================================================
// find_path — 查询 start → end 最短路径
// ============================================================================
//
// 触发 ensure_floyd() 保证矩阵最新
// 查 next_ 表重建路径: start 开始逐步跳 next_[cur][end] 直到达到 end
// 不可达(next_[start][end]==-1 且 start!=end) → 返回 nullopt
// ============================================================================

optional<vector<int>> Graph::find_path(int start, int end) {
    ensure_floyd();
    int n = node_count();
    if (start >= n || end >= n) return nullopt;

    if (next_[start][end] == -1) return nullopt;

    vector<int> path;
    int cur = start;
    while (true) {
        path.push_back(cur);
        if (cur == end) break;
        cur = next_[cur][end];
    }
    return path;
}
