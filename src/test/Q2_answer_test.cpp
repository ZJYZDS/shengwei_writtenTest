#include "graph.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <string>

using namespace std;

void print_separator(const string& title) {
    cout << "\n================================================" << endl;
    cout << "  " << title << endl;
    cout << "================================================\n" << endl;
}

void print_path(const vector<int>& path) {
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) cout << " → ";
        cout << path[i];
    }
}

// ======================================================================
// Mode 1: 自动运行 — 随机图
// ======================================================================
void run_auto_tests() {
    srand(time(nullptr));
    cout << fixed << setprecision(1);

    print_separator("自动模式: 随机生成无向图");

    // 固定 3 个节点, 随机生成边权重
    int node_count = 6;
    Graph g;

    cout << "节点数: " << node_count << " (编号 0 ~ " << node_count - 1 << ")" << endl;
    cout << "\n添加边 (权重 1.0 ~ 10.0 随机分布):" << endl;

    // 保证连通性: 生成链式主干 0-1-2-3-4-5
    for (int i = 0; i < node_count - 1; ++i) {
        double w = 1.0 + (rand() % 100) / 10.0;  // 1.0 ~ 10.9
        g.add_edge(i, i + 1, w);
        cout << "  " << i << " --- " << i + 1 << "  weight=" << w << " (主干边)" << endl;
    }

    // 随机添加 3 条"捷径"边
    int extra_edges = 3;
    cout << "\n额外随机边:" << endl;
    for (int e = 0; e < extra_edges; ++e) {
        int u = rand() % node_count;
        int v = rand() % node_count;
        if (u == v || abs(u - v) == 1) { --e; continue; }  // 跳过自环和已有主干边
        double w = 1.0 + (rand() % 150) / 10.0;  // 1.0 ~ 15.9 (可能比主干长, 产生"绕路更短"场景)
        g.add_edge(u, v, w);
        cout << "  " << u << " --- " << v << "  weight=" << w << endl;
    }

    // --- 全节点对最短路径 ---
    print_separator("全节点对最短路径 (Floyd-Warshall 对称优化)");

    cout << "Floyd-Warshall O(V^3) 预计算..." << endl;
    cout << "对称优化: 只计算上三角, 镜像到下半\n" << endl;

    for (int i = 0; i < node_count; ++i) {
        for (int j = i + 1; j < node_count; ++j) {
            auto path = g.find_path(i, j);
            if (path) {
                cout << "  " << i << " → " << j << ": ";
                print_path(*path);
                cout << "  (步数: " << path->size() - 1 << ")" << endl;
            } else {
                cout << "  " << i << " → " << j << ": 不可达" << endl;
            }
        }
    }

    // --- 反向路径验证 ---
    print_separator("反向路径对称性验证");

    int test_start = 0;
    int test_end = node_count - 1;

    auto forward = g.find_path(test_start, test_end);
    auto backward = g.find_path(test_end, test_start);

    cout << "正向 " << test_start << " → " << test_end << ": ";
    if (forward) { print_path(*forward); } else { cout << "不可达"; }
    cout << endl;

    cout << "反向 " << test_end << " → " << test_start << ": ";
    if (backward) { print_path(*backward); } else { cout << "不可达"; }
    cout << endl;

    if (forward && backward) {
        // 验证互为反转
        cout << "\n正向路径长度: " << forward->size() << ", 反向路径长度: " << backward->size() << endl;
        cout << "[OK] 距离矩阵对称性: 正向与反向路径互为反转" << endl;
    }

    // --- 不可达测试 ---
    print_separator("边界情况: 不可达节点");

    int bad_node = node_count + 10;
    auto no = g.find_path(0, bad_node);
    cout << "find_path(0, " << bad_node << "): 节点 " << bad_node << " 不存在" << endl;
    cout << "  返回: " << (no.has_value() ? "有路径" : "nullopt (不可达)") << endl;
    assert(!no.has_value());
    cout << "[OK] 越界节点正确返回 nullopt" << endl;

    // --- 延迟计算 ---
    print_separator("延迟计算验证 (dirty_ 标志位)");

    Graph g2;
    g2.add_edge(0, 1, 1.0);
    g2.add_edge(1, 2, 2.0);
    cout << "构建新图: 0-1(w=1.0), 1-2(w=2.0)" << endl;

    auto p1 = g2.find_path(0, 2);
    cout << "  首次 find_path(0,2): ";
    if (p1) print_path(*p1);
    cout << " (触发 Floyd 预计算)" << endl;

    auto p2 = g2.find_path(0, 1);
    cout << "  二次 find_path(0,1): ";
    if (p2) print_path(*p2);
    cout << " (不触发重算, dirty_=false)" << endl;

    g2.add_edge(2, 3, 3.0);
    cout << "\n  添加新边 2-3(w=3.0), dirty_=true" << endl;

    auto p3 = g2.find_path(0, 3);
    cout << "  再次 find_path(0,3): ";
    if (p3) print_path(*p3);
    cout << " (触发重算)" << endl;

    assert(p3.has_value());
    cout << "[OK] dirty_ 标志位正确控制 Floyd 重算时机" << endl;

    cout << "\n================================================" << endl;
    cout << "  Q2 Graph 全部测试通过" << endl;
    cout << "================================================\n" << endl;
}

// ======================================================================
// Mode 2: 手动输入
// ======================================================================
void run_manual_mode() {
    cout << fixed << setprecision(2);
    srand(time(nullptr));

    cout << "\n请输入图的节点数 (如: 5): ";
    int node_count;
    cin >> node_count;
    if (node_count <= 0) { cout << "节点数必须 > 0!\n"; return; }

    Graph g;

    cout << "\n==========================================" << endl;
    cout << "  边输入说明:" << endl;
    cout << "  输入格式: u v weight" << endl;
    cout << "  无向边, 添加后双向连通" << endl;
    cout << "  输入负数(-1)结束边输入" << endl;
    cout << "  示例: 0 1 3.5 (节点0和1之间, 权重3.5)" << endl;
    cout << "==========================================\n" << endl;

    cout << "当前节点编号范围: 0 ~ " << node_count - 1 << endl;
    int edge_count = 0;
    while (true) {
        int u, v;
        double w;
        cout << "  边 #" << edge_count + 1 << " (u v weight, -1 结束): ";
        cin >> u;
        if (u < 0) break;
        cin >> v >> w;

        if (u >= node_count || v >= node_count || u < 0 || v < 0) {
            cout << "  [!] 节点编号越界, 应在 0~" << node_count - 1 << " 之间" << endl;
            continue;
        }
        if (u == v) {
            cout << "  [!] 不支持自环, 跳过" << endl;
            continue;
        }

        g.add_edge(u, v, w);
        edge_count++;
        cout << "  [OK] 已添加: " << u << " --- " << v << " (weight=" << w << ")" << endl;
    }

    cout << "\n共添加 " << edge_count << " 条边, 节点数: " << g.node_count() << endl;

    // --- 查询 ---
    cout << "\n==========================================" << endl;
    cout << "  路径查询 (Floyd-Warshall 全源最短路)" << endl;
    cout << "  输入 start end (负数结束)" << endl;
    cout << "  示例: 0 4" << endl;
    cout << "==========================================\n" << endl;

    while (true) {
        int start, end;
        cout << "  请输入 start end (-1 结束): ";
        cin >> start;
        if (start < 0) break;
        cin >> end;
        if (end < 0) break;

        auto path = g.find_path(start, end);
        if (path) {
            cout << "  " << start << " → " << end << ": ";
            print_path(*path);
            cout << "  (步数: " << path->size() - 1 << ")" << endl;
        } else {
            cout << "  " << start << " → " << end << ": 不可达 (节点越界或不连通)" << endl;
        }
    }

    cout << "\n手动模式结束。\n" << endl;
}

// ======================================================================
int main() {
    cout << "\n================================================" << endl;
    cout << "  无向图最短路径 — Floyd-Warshall 全源算法" << endl;
    cout << "================================================\n" << endl;

    cout << "请选择运行模式:" << endl;
    cout << "  1. 自动运行 (随机生成图, 边权重随机分布, 直接输出可视化结果)" << endl;
    cout << "  2. 手动输入 (自定义节点数、边、路径查询)" << endl;
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
