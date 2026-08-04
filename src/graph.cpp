#include "graph.h"
#include <queue>
#include <limits>
#include <algorithm>
#include <cmath>

using namespace std;

void Graph::add_edge(int u, int v, double weight) {
    int n = max(u, v) + 1;
    if (static_cast<int>(adj_.size()) < n) adj_.resize(n);
    adj_[u].emplace_back(v, weight);
    adj_[v].emplace_back(u, weight);
}

int Graph::node_count() const {
    return static_cast<int>(adj_.size());
}

optional<vector<int>> Graph::find_path(int start, int end) const {
    int n = node_count();
    if (start >= n || end >= n) return nullopt;

    vector<double> dist(n, numeric_limits<double>::infinity());
    vector<int> prev(n, -1);

    using State = pair<double, int>;
    priority_queue<State, vector<State>, greater<State>> pq;

    dist[start] = 0.0;
    pq.emplace(0.0, start);

    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (abs(d - dist[u]) > 1e-9) continue;
        if (u == end) break;

        for (const auto& [v, w] : adj_[u]) {
            double nd = d + w;
            if (nd < dist[v]) {
                dist[v] = nd;
                prev[v] = u;
                pq.emplace(nd, v);
            }
        }
    }

    if (prev[end] == -1 && start != end) return nullopt;

    vector<int> path;
    for (int u = end; u != -1; u = prev[u]) path.push_back(u);
    reverse(path.begin(), path.end());
    return path;
}
