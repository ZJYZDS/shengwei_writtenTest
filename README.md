# 感知算法笔试题目

## 项目结构

```
shengwei_writtenTest/
├── include/
│   ├── common_types.h        # GpsCoordinate, Resolution, Tile 类型定义
│   ├── map_manager.h         # MapManager: 多分辨率地图块管理器
│   └── graph.h               # Graph: 无向图 + Floyd-Warshall 全源最短路径
├── src/
│   ├── map_manager.cpp       # MapManager 实现
│   ├── graph.cpp             # Graph 实现
│   └── test/
│       ├── Q1_answer_test.cpp # Q1-Q3 测试 (MapManager)
│       └── Q2_answer_test.cpp # 图算法测试 (Graph)
├── CMakeLists.txt            # CMake 构建配置 (C++17, Eigen, PCL, CUDA)
└── README.md
```

## 构建与运行

```bash
mkdir build && cd build
cmake .. && make

# Q1-Q3: 地图块管理器
./Q1_answer_test

# 无向图最短路径
./Q2_answer_test
```

每个测试程序支持两种运行模式：
- **模式 1（自动）**：直接输出完整可视化测试结果，无需手动输入
- **模式 2（手动）**：交互式操作，附样例提示，可自定义输入

## 运行结果 (自动模式)

### Q1_answer_test

```
================================================
  Q1: 插入 / 检索 / 删除
================================================

插入测试数据:
   Tile A: top_left(40.0000°, 116.0000°) bottom_right(39.0000°, 117.0000°)
   Tile B: top_left(41.0000°, 116.0000°) bottom_right(40.5000°, 116.5000°)

[OK] Tile A 插入 low(10×10)、high(100×100) 两层
[OK] Tile B 插入 low(10×10) 层

get_all(low): 共 2 个 tile (期望 2)
   [0] tl(40.0000, 116.0000) br(39.0000, 117.0000)
   [1] tl(41.0000, 116.0000) br(40.5000, 116.5000)

删除 Tile A (low 层)...
删除后 get_all(low): 共 1 个 tile (期望 1)
[OK] remove 成功, shared_ptr 引用计数递减

================================================
  Q2: 点查询 / 范围查询 / 包含判断
================================================

query_point(39.5000°, 116.5000°, low):
  命中 Tile: tl(40.0000, 116.0000) br(39.0000, 117.0000)
[OK] 点 (39.5000°, 116.5000°) 在 Tile A 内

contains 测试:
   (40.5°, 116.3°) -> YES (期望 YES)
   (0.0°, 0.0°)    -> NO (期望 NO)
[OK] contains 正确

--- query_range 单参数 (度感知解析) ---
输入: lat=39.5°, lon=116.5°, range=1.0°
[info] range = 1.0000 <= 90, set as both lon_range and lat_range
  输出: 2 个 tile (期望 >=1)

--- query_range 双参数 ---
输入: lat=39.5°, lon=116.5°, lon_range=1.0°, lat_range=0.5°
  输出: 1 个 tile (期望 >=1)
[OK] 双参数范围查询可用

================================================
  Q2 续: 度感知边界测试
================================================

输入 range=120° (>90, 超纬度上限):
[warn] range = 120 > 90 (lat max=90), lon_range=120, lat_range=0
  输出: 1 个 tile (lon_range=120°, lat_range=0°)

输入 range=-30° (负数, 自动 abs):
[warn] range = -30 < 0, taking abs
[info] range = 30.0000 <= 90, set as both lon_range and lat_range
  输出: 2 个 tile (等价于 range=30°)

输入 range=200° (>=180, clamp to 180):
[warn] range = 200 >= 180, clamped to 180
[warn] range = 180 > 90 (lat max=90), lon_range=180, lat_range=0
  输出: 1 个 tile (lon_range=180°, lat_range=0° 全球扫描)

--- 双参数回退到单参数 (lat_range 未提供, 哨兵 -inf) ---
[warn] lat_range not provided, falling back to single-number degree-aware logic
[info] range = 1.0000 <= 90, set as both lon_range and lat_range
  输出: 2 个 tile (自动回退到单参数逻辑, range=1° -> 1°×1°)

[OK] 度感知边界: 负数→abs | >=180→clamp | >90→纯经度 | <=90→经纬同范围 | 双参数可回退

================================================
  Q2 续: 国际日期线 (±180°) 穿越测试
================================================

插入跨日期线 Tile: tl(42.0000°, 175.0000°) br(40.0000°, -175.0000°)
  实际经度跨度: 175° → -175° = 10°

点查询测试:
   (41.0°,  179.0°) in tile? YES (期望 YES)
   (41.0°, -178.0°) in tile? YES (期望 YES)
   (41.0°,    0.0°) in tile? NO (期望 NO)

范围查询: lat=41.0°, lon=178.0°, range=3.0°
  输出: 1 个 tile (期望 >=1, 含跨日期线 tile)

[OK] 日期线穿越: in_tile || 判断 / overlaps_rect 三窗口法 / insert 分两段写

================================================
  Q3: 多分辨率管理
================================================

已注册分辨率: 2 层
   [0] Resolution(10×10): 3 个 tile
   [1] Resolution(100×100): 1 个 tile

[OK] 多分辨率同时管理, 不同分辨率层数据完全隔离

================================================
  Q1-Q3 MapManager 全部测试通过
================================================
```

### Q2_answer_test

```
================================================
  自动模式: 随机生成无向图
================================================

节点数: 6 (编号 0 ~ 5)

添加边 (权重 1.0 ~ 10.0 随机分布):
  0 --- 1  weight=4.7 (主干边)
  1 --- 2  weight=4.8 (主干边)
  2 --- 3  weight=9.7 (主干边)
  3 --- 4  weight=9.5 (主干边)
  4 --- 5  weight=9.1 (主干边)

额外随机边:
  5 --- 1  weight=14.8
  3 --- 0  weight=1.6
  5 --- 3  weight=3.1

================================================
  全节点对最短路径 (Floyd-Warshall 对称优化)
================================================

Floyd-Warshall O(V^3) 预计算...
对称优化: 只计算上三角, 镜像到下半

  0 → 1: 0 → 1  (步数: 1)
  0 → 2: 0 → 1 → 2  (步数: 2)
  0 → 3: 0 → 3  (步数: 1)       ← 捷径边 0-3(1.6) 替代链式绕路
  0 → 4: 0 → 3 → 4  (步数: 2)
  0 → 5: 0 → 3 → 5  (步数: 2)   ← 捷径边 3-5(3.1)
  1 → 2: 1 → 2  (步数: 1)
  ...

================================================
  反向路径对称性验证
================================================

正向 0 → 5: 0 → 3 → 5
反向 5 → 0: 5 → 3 → 0

正向路径长度: 3, 反向路径长度: 3
[OK] 距离矩阵对称性: 正向与反向路径互为反转

================================================
  边界情况: 不可达节点
================================================

find_path(0, 16): 节点 16 不存在
  返回: nullopt (不可达)
[OK] 越界节点正确返回 nullopt

================================================
  延迟计算验证 (dirty_ 标志位)
================================================

构建新图: 0-1(w=1.0), 1-2(w=2.0)
  首次 find_path(0,2): 0 → 1 → 2 (触发 Floyd 预计算)
  二次 find_path(0,1): 0 → 1 (不触发重算, dirty_=false)

  添加新边 2-3(w=3.0), dirty_=true
  再次 find_path(0,3): 0 → 1 → 2 → 3 (触发重算)
[OK] dirty_ 标志位正确控制 Floyd 重算时机

================================================
  Q2 Graph 全部测试通过
================================================
```

---

## 一、工程算法题：多分辨率地图块管理器

### Tile 坐标约定

```
top_left  = 西北角 (纬度大, 经度小)
bottom_right = 东南角 (纬度小, 经度大)

正常 tile:  tl.lat > br.lat,  tl.lon < br.lon
日期线 tile: tl.lat > br.lat,  tl.lon > br.lon  (如 175° → -175°, 跨 10°)
```

违反约定（tl.lon > br.lon 但跨度 ≤ 180°）的输入视为日期线穿越，不会自动纠错。

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

**日期线处理**：当 `top_left.lon > bottom_right.lon`（如 175° → -175°），tile 跨 ±180°。insert/remove 时分两段写入 cell：`[c1, width-1]`（西段）和 `[0, c2]`（东段）。in_tile 用 `||` 判断：点落在 tl.lon 以右 **或** br.lon 以左即命中。

### Q2: 空间查询

- **query_point**：经纬度 → Jw2Coord → (row, col) → coordEncoder → Key → O(1) cell 查找 → in_tile 精确判定，返回第一个命中的 tile
- **query_range**：度感知单参数解析 → 构造查询矩形 → rect_query_impl（cell 扫描 + overlaps_rect 精确过滤 + 指针去重）
- **contains**：直接复用 query_point 的 optional 返回值

**范围查询提供两个重载**：

**1. 单参数版本** `query_range(lat, lon, range, res)` — 用户只给一个度数，内部调用 `resolve_range(range)` 自动判断意图：

```
输入 range 为负数
  → [warn] 取绝对值，递归回 resolve_range(|range|) 重新判断
  → 例如 -30° 变为 30°，最终按 r<=90° 处理为 30°×30° 矩形

输入 range >= 180°
  → [warn] "clamped to 180"，截断到 180°，继续后续判断
  → 180° > 90°，落入下一条，最终按 lon_range=180°, lat_range=0 处理（全球经度扫描）

输入 90° < range < 180°
  → [warn] "lat max=90"，经度方向取 range 全值，纬度方向归零
  → 例如 120° 输出 lon_range=120°, lat_range=0°（仅经度方向带状查询）

输入 range <= 90°
  → [info] 经纬度均可接受，lon_range=range, lat_range=range
  → 例如 1.0° 输出 1.0°×1.0° 正方形查询区域
```

**设计理由**：实际使用中用户可能随手传入任意数值，程序需要合理推测用户意图，而非直接报错。负数取绝对值是容错；超大值 clamp 到 180° 保证全球覆盖而非静默丢弃数据；超过纬度上限 90° 的值只可能是用户想表达纯经度方向的搜索。

**2. 双参数版本** `query_range(lat, lon, lon_range, lat_range, res)` — 用户显式指定经度和纬度方向的半跨度，不做任何度感知推测，直接用给定的 lon_range 和 lat_range 构造矩形查询。

**双参数回退到单参数**：当调用双参数版本时不传 lat_range（默认值 `-inf`），输出 `[warn]` 并自动回退到单参数版本，走上述完整的度感知决策树。

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
