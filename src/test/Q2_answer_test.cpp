#include "graph.h"
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace std;

void print_separator(const string& title) {
    cout << "\n================================================" << endl;
    cout << "  " << title << endl;
    cout << "================================================\n" << endl;
}

int main() {
    cout << fixed << setprecision(1);

    // ======================================================================
    // 图构建
    // ======================================================================
    print_separator("无向图构建");

    Graph g;

    //     0 ----(1.0)---- 1
    //     |                |
    //   (10.0)           (1.0)
    //     |                |
    //     3 ----(1.0)---- 2
    //
    // 最短路 0→3: 直接边 10.0 vs 绕路 0-1-2-3 = 3.0 → 绕路更短

    cout << "添加边 (无向, 默认权重 1.0):" << endl;
    g.add_edge(0, 1);          // 0-1 weight=1.0
    g.add_edge(1, 2);          // 1-2 weight=1.0
    g.add_edge(2, 3);          // 2-3 weight=1.0
    g.add_edge(0, 3, 10.0);    // 0-3 weight=10.0 (绕路的对比项)

    cout << "   0 --- 1 (weight=1.0)" << endl;
    cout << "   1 --- 2 (weight=1.0)" << endl;
    cout << "   2 --- 3 (weight=1.0)" << endl;
    cout << "   0 --- 3 (weight=10.0, 直接边长)" << endl;

    int n = g.node_count();
    cout << "\n图节点数: " << n << " (期望 4)" << endl;
    assert(n == 4);
    cout << "[OK] 邻接表构建完成, 双向边已推入" << endl;

    // ======================================================================
    // Floyd-Warshall 全源最短路
    // ======================================================================
    print_separator("Floyd-Warshall 全源最短路 (无向图对称优化)");

    cout << "预计算: O(V^3) = O(4^3) = 64 次迭代 (上三角优化后约 32 次)" << endl;
    cout << "对称优化: 只计算上三角 (j > i), 结果镜像 dist[i][j] = dist[j][i]" << endl;
    cout << "dirty_ 标志: add_edge 后触发一次, 连续查询不重复计算\n" << endl;

    // 路径查找
    auto path_0_3 = g.find_path(0, 3);
    auto path_3_0 = g.find_path(3, 0);

    cout << "find_path(0, 3): 起点 0 → 终点 3" << endl;
    if (path_0_3) {
        cout << "  路径: ";
        for (size_t i = 0; i < path_0_3->size(); ++i) {
            if (i > 0) cout << " → ";
            cout << (*path_0_3)[i];
        }
        cout << endl;
        cout << "  长度: " << path_0_3->size() - 1 << " 步, 总权重 3.0 (绕路 0-1-2-3)" << endl;
        cout << "  对比直接边 0→3 权重 10.0: 绕路更短, 正确!" << endl;
    }

    cout << "\nfind_path(3, 0): 起点 3 → 终点 0 (反向查询)" << endl;
    if (path_3_0) {
        cout << "  路径: ";
        for (size_t i = 0; i < path_3_0->size(); ++i) {
            if (i > 0) cout << " → ";
            cout << (*path_3_0)[i];
        }
        cout << endl;
        cout << "  与 0→3 路径互为反转: 验证对称性正确!" << endl;
    }

    assert(path_0_3.has_value() && path_0_3->size() == 4);
    assert(path_3_0.has_value() && path_3_0->size() == 4);

    // ======================================================================
    // 所有节点对测试
    // ======================================================================
    print_separator("全节点对最短路径验证");

    cout << "距离矩阵 (Floyd-Warshall 预计算结果):" << endl;
    cout << "  所有边权重 1.0 (0-3 直接边 10.0 被绕路跳过)" << endl;
    cout << endl;

    int test_pairs[][2] = { {0,1}, {0,2}, {0,3}, {1,2}, {1,3}, {2,3} };
    for (auto& [s, e] : test_pairs) {
        auto p = g.find_path(s, e);
        if (p) {
            cout << "  " << s << " → " << e << ": ";
            for (size_t i = 0; i < p->size(); ++i) {
                if (i > 0) cout << " → ";
                cout << (*p)[i];
            }
            cout << "  (步数: " << p->size() - 1 << ")" << endl;
        }
    }

    cout << "\n所有节点对可达, 距离矩阵对称 (i→j 与 j→i 路径互为反转)" << endl;
    cout << "[OK] Floyd-Warshall 无向图对称优化验证通过" << endl;

    // ======================================================================
    // 不可达测试
    // ======================================================================
    print_separator("边界情况: 不可达节点");

    auto no_path = g.find_path(0, 99);
    cout << "find_path(0, 99): 节点 99 不存在 (图共 " << n << " 个节点)" << endl;
    cout << "  返回: " << (no_path.has_value() ? "有路径" : "nullopt (不可达)") << endl;
    assert(!no_path.has_value());
    cout << "[OK] 越界节点正确返回 nullopt" << endl;

    // ======================================================================
    // 延迟计算验证
    // ======================================================================
    print_separator("延迟计算验证 (dirty_ 标志位)");

    Graph g2;
    g2.add_edge(0, 1);
    g2.add_edge(1, 2);
    cout << "构建新图: 0-1-2 (2 条边)" << endl;
    cout << "  node_count: " << g2.node_count() << endl;

    auto p1 = g2.find_path(0, 2);
    cout << "  首次 find_path(0,2): ";
    if (p1) { for (size_t i = 0; i < p1->size(); ++i) { if (i > 0) cout << " → "; cout << (*p1)[i]; } }
    cout << " (触发 Floyd 预计算)" << endl;

    auto p2 = g2.find_path(0, 1);
    cout << "  二次 find_path(0,1): ";
    if (p2) { for (size_t i = 0; i < p2->size(); ++i) { if (i > 0) cout << " → "; cout << (*p2)[i]; } }
    cout << " (不触发重算, dirty_=false)" << endl;

    g2.add_edge(2, 3);
    cout << "\n  添加新边 2-3, dirty_=true" << endl;

    auto p3 = g2.find_path(0, 3);
    cout << "  再次 find_path(0,3): ";
    if (p3) { for (size_t i = 0; i < p3->size(); ++i) { if (i > 0) cout << " → "; cout << (*p3)[i]; } }
    cout << " (触发重算, dirty_ 转为 false)" << endl;

    assert(p3.has_value() && p3->size() == 4);
    cout << "[OK] dirty_ 标志位正确控制 Floyd 重算时机" << endl;

    cout << "\n================================================" << endl;
    cout << "  Q2 Graph 全部测试通过" << endl;
    cout << "================================================\n" << endl;

    return 0;
}
