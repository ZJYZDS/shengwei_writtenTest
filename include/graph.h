#pragma once

#include <vector>
#include <optional>
#include <utility>

class Graph {
public:
    Graph() = default;

    void add_edge(int u, int v, double weight = 1.0);

    int node_count() const;

    std::optional<std::vector<int>> find_path(int start, int end) const;

private:
    std::vector<std::vector<std::pair<int, double>>> adj_;
};
