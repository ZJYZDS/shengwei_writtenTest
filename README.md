# 感知算法笔试题目

## 项目结构

```
shengwei_writtenTest/
├── include/
│   ├── common_types.h      # GpsCoordinate, Resolution, Tile 类型定义
│   ├── map_manager.h       # MapManager: 多分辨率地图块管理器
│   └── graph.h             # Graph: 无向图 + Floyd-Warshall 全源最短路径
├── src/
│   ├── map_manager.cpp     # MapManager 实现
│   ├── graph.cpp           # Graph 实现
│   └── main.cpp            # 测试用例
├── CMakeLists.txt          # CMake 构建配置 (C++17, Eigen, PCL, CUDA)
└── README.md
```

## 构建与运行

```bash
mkdir build && cd build
cmake .. && make
./written_test
```

## 运行结果

```
=== MapManager ===
Q1 insert/get_all: OK
Q1 remove: OK
Q2 query_point: OK
Q2 contains: OK
Q2 query_range (single <=90): OK
Q2 query_range (two args): OK

--- degree-aware tests ---
[warn] range = 120 > 90 (lat max=90), lon_range=120, lat_range=0
Q2 query_range (single >90): 1 tiles
[warn] range = -30 < 0, taking abs
[info] range = 30 <= 90, set as both lon_range and lat_range
Q2 query_range (negative -> abs): OK
[warn] range = 200 >= 180, clamped to 180
[warn] range = 180 > 90 (lat max=90), lon_range=180, lat_range=0
Q2 query_range ( >=180 -> clamp 180): 1 tiles
[info] range = 3 <= 90, set as both lon_range and lat_range
Q2 date-line wrap: OK
Q3 resolutions: 2 levels

=== Graph ===
Graph node_count: OK
Path 0->3: 0 1 2 3 (expected 0 1 2 3)

All tests passed.
```

---

## 一、工程算法题：多分辨率地图块管理器

### Q1: 基础增删查

**数据结构设计** — 两层空间索引：

```
grids_: map<Resolution, Grid>
  Resolution = (height, width)   — 第几层精度，控制网格切分粒度
  Grid       = unordered_map<Key, vector<shared_ptr<Tile>>>
  Key        = (row << 32) | col — 2D网格坐标编码为 uint64_t
```

- **Layer 1** — 按分辨率分区：`{10,10}` 是粗精度层，`{1000,1000}` 是细精度层
- **Layer 2** — 每层内按 cell 分组：每个 cell 存 `shared_ptr<Tile>` 列表，同一 Tile 跨越多个 cell 但只分配一次

**为什么选择 shared_ptr**：大 Tile 可能覆盖数百个 cell，按值存储会导致同一份数据复制数百次。使用 shared_ptr 保证每个 insert 只分配一次堆内存，各 cell 共享同一指针。remove 时 erase 指针，引用计数自动 -1，无其他 cell 引用时自动释放。

**去重策略**：get_all / rect_query 中维护 `vector<const Tile*> seen`，按裸指针地址去重。同一次 insert 的 shared_ptr 地址相同，避免按浮点字段比较的不稳定性。

**日期线处理**：当 `top_left.lon > bottom_right.lon`（如 175° → -175°），tile 跨 ±180°。insert 时分两段写入 cell：`[c1, width-1]`（西段）和 `[0, c2]`（东段）。in_tile 用 `||` 判断：点落在 tl.lon 以右 **或** br.lon 以左即命中。

### Q2: 空间查询

- **query_point**：经纬度 → Jw2Coord → (row, col) → coordEncoder → Key → O(1) cell 查找 → in_tile 精确判定，返回第一个命中的 tile
- **query_range**：度感知单参数解析 → 构造查询矩形 → rect_query_impl（cell 扫描 + overlaps_rect 精确过滤 + 指针去重）
- **contains**：直接复用 query_point 的 optional 返回值

**度感知范围解析 (resolve_range)**：

| 输入 r | 行为 | 输出 |
|--------|------|------|
| r < 0 | [warn] 取绝对值，递归 | — |
| r >= 180 | [warn] clamp 到 180 | — |
| 90 < r < 180 | [warn] 超纬度上限 | lon_range=r, lat_range=0 |
| r <= 90 | [info] 合法 | lon_range=r, lat_range=r |

**overlaps_rect 三窗口法**：将 tile 经度区间分别偏移 -360°/0°/+360°，与查询区间测试交集，任一命中即判定相交。这是处理圆形坐标轴（lon 在 ±180° 处闭环）的标准方法。

### Q3: 多分辨率管理

`grids_` 为 `map<Resolution, Grid>`，不同 Resolution 的 tile 存储在不同 Grid 中，完全隔离。`supported_resolutions()` 返回所有已注册的分辨率层级。

---

## 二、基础算法题：无向图最短路径

### 数据结构

邻接表：`adj_[u] = [(v, weight), (v2, weight2), ...]`，添加无向边时双向推入。

### 算法选择：Floyd-Warshall（非 Dijkstra）

题目要求"用户输入任意起点和终点"，即 start/end 不固定。Dijkstra 每次查询需 O((V+E)log V)，多次查询效率低。Floyd-Warshall 一次 O(V³) 预计算后每次查询 O(1) 查表 + O(P) 路径重建。

### 无向图对称优化

无向图的距离矩阵对称（`dist[i][j] == dist[j][i]`），最短路径可逆（i→j 路径反转即 j→i 路径）。因此只计算上三角（`j > i`），镜像到下半部分，节省约一半计算量：

```
for k in 0..n-1:
  for i in 0..n-1:
    for j in i+1..n-1:          // 上三角
      if dist[i][k] + dist[k][j] < dist[i][j]:
        dist[i][j] = dist[j][i] = nd
        next[i][j] = next[i][k]   // i→j: 先去 k
        next[j][i] = next[j][k]   // j→i: 先去 k (反向路径第一步)
```

**测试用例**：节点 0-1-2-3 链式连接，权重均为 1.0；另加直接边 0→3 权重 10.0。最短路径应绕过直接边走 0→1→2→3（总权重 3.0 < 10.0）。输出 `0 1 2 3`，验证正确。

**延迟计算**：`dirty_` 标志位跟踪邻接表变更。add_edge 后置 true，下次 find_path 触发 ensure_floyd() 重算。连续查询不重复计算。

---

## 三、算法原理

### Q1: 2D 点集恢复 3D 信息与位姿

**核心思路**：已知前后帧 2D 点集 `Set_{k-1}`, `Set_k` 及对应关系 `Reflection(i, j)`，本质是 2D-2D 对极几何问题。

1. **本质矩阵估计**：利用 >=8 组匹配点对，通过八点法或归一化八点法求解本质矩阵 E。若已知相机内参 K，可由 `E = K^T F K` 将基础矩阵 F 转为本质矩阵。

2. **SVD 分解恢复位姿**：对 E 做 SVD：`E = U Σ V^T`，恢复旋转 R 和平移 t（共 4 组解），通过三角化后的正深度约束筛选唯一解。

3. **三角化恢复 3D 点**：已知 R, t 后，对每组匹配点通过线性三角化（DLT）或中点法求解 3D 坐标。

4. **BA 优化**：将恢复的 3D 点和相机位姿作为初值，最小化重投影误差进行 Bundle Adjustment。

**关键假设**：相机内参 K 已知（已标定），场景为静态刚体（无动态物体干扰）。

### Q2: 点云管理与局部规划空间生成

**点云管理**：

- **空间索引**：使用八叉树（Octree）或 KD-Tree 对点云进行空间划分，支持快速近邻搜索和体素降采样。
- **多层分辨率**：类似 Q3 的地图管理，不同高度/距离使用不同体素分辨率。近处精细（如 0.1m 体素），远处粗糙（如 0.5m 体素）。
- **动态更新**：使用环形缓冲区或滑动窗口维护最近 N 帧点云，淘汰过期数据。对静态场景可累积融合（TSDF/ occupancy grid）。

**规划空间生成**：

1. **地面分割**：使用 RANSAC 或 Cloth Simulation Filter 分离地面点与非地面点。
2. **占栅格地图**：将非地面点投影到水平面，统计每个栅格内的点云高度范围，生成 2.5D 高程图或 OctoMap 占栅格。
3. **膨胀处理**：对障碍物栅格按机器人半径膨胀，生成 costmap。
4. **通行区域提取**：costmap 中 cost 低于阈值的区域即为可通行空间。

**潜在问题**：

- **遮挡与视野外区域**：传感器只能看到前方一定范围，被障碍物遮挡的后方区域状态未知。需区分"已知空闲"、"已知占用"、"未知"三种状态，规划时对未知区域采用保守策略（视作占用或给予高 cost）。
- **动态障碍物**：运动中的行人/车辆在点云中留下拖影或残影，需通过跟踪/预测滤除，或使用 occupancy grid 的 probabilistic update 衰减历史观测。
- **传感器噪声与稀疏性**：远距离点云稀疏，地面分割可能失败（地面点过少或噪声过大）。可通过多帧融合或引入 IMU 约束提高鲁棒性。
- **计算效率**：大范围场景的点云存储和查询开销大。八叉树 + 体素降采样 + 局部地图（sliding window）是常用平衡策略。
- **地形适应性**：纯几何方法在斜坡、台阶、草地等场景可能误判。可结合法向量分析和 traversability estimation 增强判断。
