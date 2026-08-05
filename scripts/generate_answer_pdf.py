#!/usr/bin/env python3
"""Generate optimized answer PDF: cover page + original Q blocks + detailed answers."""

import os
from weasyprint import HTML

PROJ_DIR = "/home/zjy/shengwei_writtenTest"
GITHUB_URL = "https://github.com/ZJYZDS/shengwei_writtenTest"
SKILLS_DIR = os.path.join(PROJ_DIR, "scripts")

CSS = """
@page {
  size: A4;
  margin: 2cm 2.2cm 2.5cm 2.2cm;
  @bottom-center {
    content: counter(page);
    font-size: 8pt;
    color: #999;
    font-family: "Noto Sans CJK SC", sans-serif;
  }
}
@page :first {
  @bottom-center { content: none; }
  @top-center { content: none; }
  margin: 0;
}
body {
  font-family: "Noto Sans CJK SC", "Noto Serif CJK SC", sans-serif;
  font-size: 10pt;
  color: #2d2d2d;
  line-height: 1.75;
}

/* ===== Cover Page ===== */
.cover {
  width: 100%; height: 100%;
  display: flex; flex-direction: column; justify-content: center; align-items: center;
  text-align: center; page-break-after: always;
  background: linear-gradient(160deg, #f8f9fa 0%, #fff 40%, #f0f4f8 100%);
}
.cover-top { position: absolute; top: 0; left: 0; right: 0; height: 8px;
             background: linear-gradient(90deg, #1a56db, #0694a2); }
.cover-bottom { position: absolute; bottom: 0; left: 0; right: 0; height: 4px;
                background: linear-gradient(90deg, #0694a2, #1a56db); }
.cover h1 { font-size: 26pt; color: #1a1a2e; margin: 0 0 8pt 0; letter-spacing: 2pt; }
.cover .subtitle { font-size: 12pt; color: #555; margin: 0 0 28pt 0; }
.cover .meta-box {
  background: #fff; border: 1px solid #e0e5ec; border-radius: 8px;
  padding: 14pt 32pt; margin-bottom: 18pt;
  text-align: left; font-size: 9.5pt; color: #555; line-height: 2;
}
.cover .meta-box strong { color: #333; display: inline-block; width: 80pt; }
.cover .github-link {
  display: inline-block; background: #1a56db; color: #fff; text-decoration: none;
  padding: 8pt 24pt; border-radius: 6px; font-size: 11pt; font-weight: bold;
  margin-top: 10pt;
}
.cover .badge { display: inline-block; background: #e8f0fe; color: #1a56db;
                padding: 3pt 10pt; border-radius: 12px; font-size: 8pt; margin: 2pt; }

/* ===== Header ===== */
.section-header { padding-top: 8pt; }
.page-header { border-bottom: 2px solid #1a56db; padding-bottom: 4pt; margin-bottom: 16pt; }
.page-header .repo { font-size: 7.5pt; color: #1a56db; float: right; margin-top: 4pt; }

h1 { font-size: 16pt; color: #1a1a2e; margin: 20pt 0 12pt 0; padding-bottom: 4pt;
     border-bottom: 1.5px solid #e0e5ec; }
h2 { font-size: 12.5pt; color: #1a56db; margin: 16pt 0 8pt 0; }
h3 { font-size: 10.5pt; color: #333; margin: 12pt 0 6pt 0; }

p { margin: 5pt 0; }

/* ===== Question Block ===== */
.q-block {
  background: #f0f5ff; border-left: 4px solid #1a56db;
  padding: 8pt 14pt; margin: 10pt 0 14pt 0; border-radius: 0 6px 6px 0;
  font-size: 9.5pt; color: #1e3a5f;
}
.q-block .q-label {
  font-size: 8pt; font-weight: bold; color: #1a56db; text-transform: uppercase;
  letter-spacing: 1pt; margin-bottom: 4pt;
}

/* ===== Answer Block ===== */
.answer-label {
  font-size: 8pt; font-weight: bold; color: #0694a2; text-transform: uppercase;
  letter-spacing: 1pt; margin-bottom: 2pt;
}

/* ===== Code Block ===== */
pre {
  background: #1e293b; color: #e2e8f0; padding: 10pt 14pt; font-size: 7.8pt;
  line-height: 1.55; white-space: pre-wrap; word-break: break-all;
  border-radius: 6px; margin: 8pt 0; font-family: "DejaVu Sans Mono", monospace;
}
code { font-family: "DejaVu Sans Mono", monospace; font-size: 8.5pt;
       background: #f1f5f9; padding: 1pt 4pt; border-radius: 3px; color: #1a56db; }
pre code { background: none; padding: 0; color: inherit; font-size: inherit; }

/* ===== Lists ===== */
ol, ul { margin: 4pt 0; padding-left: 22pt; }
li { margin: 3pt 0; padding-left: 2pt; }
ol > li { margin: 6pt 0; }
ul > li { list-style-type: square; }
li strong { color: #1a1a2e; }

/* ===== Other ===== */
.divider { border: none; border-top: 1px dashed #d0d7e2; margin: 14pt 0; }
.note {
  background: #fefce8; border: 1px solid #fde68a; border-radius: 6px;
  padding: 8pt 14pt; font-size: 9pt; color: #713f12; margin: 10pt 0;
}
.footnote { font-size: 8pt; color: #999; margin-top: 18pt; }
.tag {
  display: inline-block; background: #e0e7ff; color: #3730a3;
  padding: 1pt 8pt; border-radius: 10px; font-size: 7.5pt; margin-right: 3pt;
}
"""

HTML_TEMPLATE = """<!DOCTYPE html>
<html lang="zh-CN">
<head><meta charset="utf-8"/><style>{css}</style></head>
<body>

<!-- ===== COVER PAGE ===== -->
<div class="cover">
  <div class="cover-top"></div>
  <h1>感知算法笔试</h1>
  <p class="subtitle">解答文档</p>
  <div class="meta-box">
    <p><strong>项目</strong> shengwei_writtenTest</p>
    <p><strong>语言</strong> C++17</p>
    <p><strong>依赖</strong> Eigen3, PCL, CUDA (可选)</p>
    <p><strong>作答时间</strong> 1天 &nbsp;|&nbsp; 允许使用大模型辅助</p>
    <p><strong>生成日期</strong> 2026-08-05</p>
  </div>
  <a class="github-link" href="{github}">GitHub: {github}</a>
  <p style="margin-top:14pt;">
    <span class="badge">工程算法 Q1-Q3</span>
    <span class="badge">最短路径</span>
    <span class="badge">SfM</span>
    <span class="badge">YOLO-depth</span>
  </p>
  <div class="cover-bottom"></div>
</div>

<!-- ===== CONTENT ===== -->
<div class="page-header">
  <span class="repo">GitHub: {github}</span>
</div>

<!-- ============================================================ -->
<!-- 一、工程算法题 -->
<!-- ============================================================ -->
<h1>一、工程算法题：多分辨率地图块管理器</h1>

<div class="q-block">
  <div class="q-label">Q1 题目</div>
  实现一个可以随意插入，删除，检索地图块的地图管理功能。<br/>
  <strong>数据结构</strong>: GpsCoordinate = &#123; latitude, longitude, altitude &#125;<br/>
  Tile = &#123; GpsCoordinate top_left, bottom_right &#125;
</div>

<div class="q-block">
  <div class="q-label">Q2 题目</div>
  在 Q1 的基础上增添以下功能：<br/>
  (1) 任意给出坐标，可以返回该坐标所在的地图块<br/>
  (2) 任意给出坐标及范围，可以返回该范围内所有地图块<br/>
  (3) 任意给一个坐标，检查当前坐标是否在地图范围内
</div>

<div class="q-block">
  <div class="q-label">Q3 题目</div>
  在 Q2 的基础上增加功能，支持不同分辨率地图同时管理。
</div>

<div class="answer-label">解答</div>

<h2>数据结构设计</h2>
<p>采用<strong>两层空间索引</strong>结构：</p>
<pre><code>grids_: map&lt;Resolution, Grid&gt;
  Resolution = (height, width)  -- 控制网格切分粒度
  Grid       = unordered_map&lt;Key, vector&lt;shared_ptr&lt;Tile&gt;&gt;&gt;
  Key        = (row &lt;&lt; 32) | col  -- 2D网格坐标编码为uint64_t，O(1)查找</code></pre>

<p><strong>内存策略</strong>：使用 <code>shared_ptr</code> 共享同一 Tile。大 Tile 可能覆盖数百个 cell，按值存储会导致同一份数据复制数百次。每个 <code>insert</code> 只分配一次堆内存，各 cell 共享同一指针。<code>remove</code> 时 erase 指针，引用计数自动递减，无其他 cell 引用时自动释放内存。</p>

<p><strong>Tile 坐标约定</strong>：<code>top_left</code> = 西北角 (纬度大，经度小)；<code>bottom_right</code> = 东南角 (纬度小，经度大)。违反约定（tl.lon &gt; br.lon 但跨度 &le; 180&deg;）的输入视为日期线穿越，不会自动纠错。</p>

<p><strong>去重策略</strong>：<code>get_all</code> / <code>rect_query</code> 中维护按裸指针地址去重的集合，避免按浮点字段比较的不稳定性。</p>

<h2>Q1: 基础增删查</h2>
<ul>
  <li><strong>insert</strong>: 计算 Tile 覆盖的 cell 范围，将 <code>shared_ptr</code> 写入对应 cell 列表</li>
  <li><strong>remove</strong>: 找到 cell 中对应指针并 erase，引用计数自动 -1</li>
  <li><strong>get_all</strong>: 遍历所有 cell，按裸指针地址去重，返回唯一切片列表</li>
</ul>

<h2>Q2: 空间查询</h2>
<ol>
  <li><strong>query_point(lat, lon, res)</strong>: 经纬度 &rarr; Jw2Coord &rarr; (row, col) &rarr; coordEncoder &rarr; Key &rarr; O(1) cell 查找 &rarr; in_tile 精确判定，返回第一个命中的 Tile</li>
  <li><strong>query_range(lat, lon, range, res) [单参数]</strong>: 度感知解析 <code>resolve_range(range)</code>，自动判断用户意图：
    <ul>
      <li>range &lt; 0: 取绝对值后递归判断</li>
      <li>range &ge; 180&deg;: clamp 到 180&deg;，按全球经度扫描处理</li>
      <li>90&deg; &lt; range &lt; 180&deg;: 纬度归零，纯经度方向带状查询</li>
      <li>range &le; 90&deg;: 经纬同范围，正方形查询区域</li>
    </ul>
    <strong>设计理由</strong>：实际使用中用户可能随手传入任意数值，程序需要合理推测意图而非直接报错。
  </li>
  <li><strong>query_range(lat, lon, lon_range, lat_range, res) [双参数]</strong>: 用户显式指定经度和纬度方向半跨度。lat_range 默认 <code>-inf</code>，自动回退到单参数度感知逻辑</li>
  <li><strong>contains(lat, lon, res)</strong>: 复用 <code>query_point</code> 的 <code>optional</code> 返回值判断</li>
</ol>

<h3>国际日期线处理 (tl.lon &gt; br.lon 判定跨 &plusmn;180&deg;)</h3>
<ul>
  <li><strong>insert / remove</strong>: 分两段写入 cell &mdash; [c1, width-1] (西段) 和 [0, c2] (东段)</li>
  <li><strong>in_tile</strong>: 用 <code>||</code> 判断 &mdash; 点落在 tl.lon 以右 <em>或</em> br.lon 以左即命中</li>
  <li><strong>overlaps_rect</strong>: 三窗口法 &mdash; tile 经度区间分别偏移 -360&deg;/0&deg;/+360&deg;，与查询区间测试交集，任一命中即判定相交</li>
</ul>

<h2>Q3: 多分辨率管理</h2>
<p><code>grids_</code> 为 <code>map&lt;Resolution, Grid&gt;</code>，不同 Resolution 的 Tile 存储在不同 Grid 中，完全隔离。<code>supported_resolutions()</code> 返回所有已注册的分辨率层级。各层独立增删查，同一 Tile 可在不同分辨率层中同时存在，互不干扰。</p>

<p><strong>核心实现文件</strong>: <code>include/map_manager.h</code>, <code>src/map_manager.cpp</code>, <code>src/test/Q1_answer_test.cpp</code></p>

<hr class="divider">

<!-- ============================================================ -->
<!-- 二、基础算法题 -->
<!-- ============================================================ -->
<h1>二、基础算法题：无向图最短路径</h1>

<div class="q-block">
  <div class="q-label">题目</div>
  设计数据结构用于存储一个无向图。用户输入任意起点和终点节点，输出起点到终点在无向图中的路径。
</div>

<div class="answer-label">解答</div>

<h2>数据结构</h2>
<p>邻接表：<code>adj_[u] = [(v, weight), (v2, weight2), ...]</code>，添加无向边时双向推入以保证对称性。</p>

<h2>算法选择：Floyd-Warshall (非 Dijkstra)</h2>
<p>题目要求"任意起点和终点"，即 start/end 不固定。Dijkstra 每次查询需 O((V+E)log V)，多次查询需反复运行，效率低。Floyd-Warshall 一次 O(V&sup3;) 预计算，之后每次查询 O(1) 查表 + O(P) 路径重建，适合多次任意点对查询场景。</p>

<h2>无向图对称优化</h2>
<p>无向图距离矩阵对称 (<code>dist[i][j] == dist[j][i]</code>)，最短路径可逆 (i&rarr;j 反转即 j&rarr;i)。只计算上三角 (j &gt; i)，镜像到下半部分，节省约一半计算量：</p>
<pre><code>for k in 0..n-1:
  for i in 0..n-1:
    for j in i+1..n-1:               // 仅上三角
      if dist[i][k] + dist[k][j] &lt; dist[i][j]:
        dist[i][j] = dist[j][i] = new_dist
        next[i][j] = next[i][k]      // i&rarr;j: 先去 k
        next[j][i] = next[j][k]      // j&rarr;i: 先去 k (反向路径第一步)</code></pre>

<h2>延迟计算</h2>
<p><code>dirty_</code> 标志位跟踪邻接表变更：<code>add_edge</code> 后置 <code>true</code>，下次 <code>find_path</code> 触发 <code>ensure_floyd()</code> 重算。连续查询不重复计算 Floyd，仅在图结构变化后才重新预计算。访问越界节点返回 <code>nullopt</code>。</p>

<p><strong>核心实现文件</strong>: <code>include/graph.h</code>, <code>src/graph.cpp</code>, <code>src/test/Q2_answer_test.cpp</code></p>

<hr class="divider">

<!-- ============================================================ -->
<!-- 三、算法原理 -->
<!-- ============================================================ -->
<h1>三、算法原理</h1>

<h2>Q1: 2D 点集恢复 3D 信息与位姿</h2>

<div class="q-block">
  <div class="q-label">Q1 题目</div>
  现在有前后帧点集 Set<sub>k-1</sub>(point(x, y)), Set<sub>k</sub>(point(x, y)) 及前后帧点集对应关系 Reflection(point<sub>k-1</sub>[i], point<sub>k</sub>[j])，如何从点的二维信息获取三维信息及前后帧位姿？
</div>

<div class="answer-label">解答</div>

<h3>方法一：多视图几何 (2D-2D 对极几何)</h3>
<p>已知前后帧 2D 点集及对应关系，本质是 <strong>2D-2D 对极几何</strong>问题。核心流程：</p>
<ol>
  <li><strong>本质矩阵估计</strong>: 利用 &ge;8 组匹配点对，通过归一化八点法求解基础矩阵 F。若已知相机内参 K，可由 <code>E = K<sup>T</sup> F K</code> 将基础矩阵 F 转为本质矩阵。</li>
  <li><strong>SVD 分解恢复位姿</strong>: 对 E 做 SVD：<code>E = U &Sigma; V<sup>T</sup></code>，恢复旋转 R 和平移 t (共 4 组解)，通过三角化后的正深度约束 (Chierality Check) 筛选唯一解。</li>
  <li><strong>三角化恢复 3D 点</strong>: 已知 R, t 后，对每组匹配点通过 DLT 线性三角化求解 3D 坐标。</li>
  <li><strong>BA 优化</strong>: 将恢复的 3D 点和相机位姿作为初值，最小化重投影误差进行 Bundle Adjustment。</li>
</ol>
<p><strong>关键假设</strong>: 相机内参 K 已知 (已标定)，场景为静态刚体 (无动态物体干扰)。</p>

<h3>方法二：单目深度估计 (YOLO-depth)</h3>
<p>除上述多视图几何方法外，还可通过<strong>单目深度估计模型</strong>从单帧图像直接获取 3D 信息：</p>
<ol>
  <li><strong>模型</strong>: 使用 YOLO-depth 模型 (<code>yolo26s-depth.pt</code>)，在 YOLO 检测框架基础上增加深度估计头，单次推理同时输出目标检测框和逐像素深度图 (单位: 米)。</li>
  <li><strong>3D 反投影</strong>: 给定像素 (u, v) 及其深度 d，结合相机内参 K：<br/>
    <code>X = d * inv(K) * [u, v, 1]<sup>T</sup></code><br/>
    即可恢复该像素对应的 3D 坐标。</li>
  <li><strong>优势</strong>: 单帧即可获取稠密 3D 信息，无需多帧匹配或特征点对应，对纹理稀疏区域更鲁棒。</li>
  <li><strong>局限</strong>: 深度精度依赖模型泛化能力，绝对尺度可能漂移；无法直接恢复相机位姿，需配合 PnP 或 ICP 等方法。</li>
</ol>
<p><strong>相关脚本</strong>: <code>scripts/Q3_1/Depth_model_estimation.py</code>, <code>scripts/Q3_1/example_depth_model.py</code></p>

<p class="note"><strong>补充说明</strong>: 本项目同时实现了以上两种方法。方法一对应 C++ SfM 流水线 (<code>src/sfm.cpp</code>)，方法二对应 Python 深度估计脚本。两种方法互补：多视图几何可同时恢复位姿和稀疏 3D 结构，单目深度估计可获取稠密深度但需额外位姿估计。</p>

<h2>Q2: 点云管理与局部规划空间生成</h2>

<div class="q-block">
  <div class="q-label">Q2 题目</div>
  假如你现在可以通过传感器获取空间中的点云，如何对传感器数据进行管理。现在需要在此基础上生成规划空间用于局部避障和路径规划，请详细思考并告诉我解决思路和潜在问题。
</div>

<div class="answer-label">解答</div>

<h3>点云管理</h3>
<ul>
  <li><strong>空间索引</strong>: 使用八叉树 (Octree) 或 KD-Tree 对点云进行空间划分，支持快速近邻搜索和体素降采样。</li>
  <li><strong>多层分辨率</strong>: 类似 Q3 的多分辨率管理思路。近处精细 (如 0.1m 体素)，远处粗糙 (如 0.5m 体素)，平衡精度与计算开销。</li>
  <li><strong>动态更新</strong>: 使用环形缓冲区或滑动窗口维护最近 N 帧点云，淘汰过期数据。对静态场景可累积融合 (TSDF / occupancy grid)。</li>
</ul>

<h3>规划空间生成</h3>
<p><strong>流程</strong>: 地面分割 &rarr; 占栅格地图 &rarr; 膨胀处理 &rarr; 通行区域提取</p>
<ol>
  <li><strong>地面分割</strong>: 使用 RANSAC 或 Cloth Simulation Filter 分离地面点与非地面点。</li>
  <li><strong>占栅格地图</strong>: 将非地面点投影到水平面，统计每个栅格内的点云高度范围，生成 2.5D 高程图或 OctoMap 占栅格。</li>
  <li><strong>膨胀处理</strong>: 对障碍物栅格按机器人半径膨胀，生成 costmap (代价地图)。</li>
  <li><strong>通行区域提取</strong>: costmap 中 cost 低于阈值的区域即为可通行空间，可直接用于 A* / DWA 等规划算法。</li>
</ol>

<h3>潜在问题</h3>
<ul>
  <li><strong>遮挡与视野外区域</strong>: 传感器只能看到前方一定范围，被障碍物遮挡的后方区域状态未知。需区分"已知空闲""已知占用""未知"三种状态，规划时对未知区域采用保守策略 (视作占用或给予高 cost)。</li>
  <li><strong>动态障碍物</strong>: 运动中的行人/车辆在点云中留下拖影或残影，需通过跟踪/预测滤除，或使用 occupancy grid 的 probabilistic update 衰减历史观测。</li>
  <li><strong>传感器噪声与稀疏性</strong>: 远距离点云稀疏，地面分割可能失败 (地面点过少或噪声过大)。可通过多帧融合或引入 IMU 约束提高鲁棒性。</li>
  <li><strong>计算效率</strong>: 大范围场景的点云存储和查询开销大。八叉树 + 体素降采样 + 局部地图 (sliding window) 是常用的平衡策略。</li>
  <li><strong>地形适应性</strong>: 纯几何方法在斜坡、台阶、草地等场景可能误判。可结合法向量分析和 traversability estimation 增强判断。</li>
</ul>

<hr class="divider">

<!-- ============================================================ -->
<!-- 四、SfM 流水线 -->
<!-- ============================================================ -->
<h1>四、SfM 完整流水线实现 (附加)</h1>

<div class="answer-label">实现概览</div>
<p><strong>实现文件</strong>: <code>src/sfm.cpp</code> &mdash; 包含 Step 0-5 完整 SfM 流水线 + 合成数据自验证。</p>

<p><strong>流水线</strong>:</p>
<pre><code>set_k_minus_1 / set_k / matches
    &rarr; extractAlignedPoints()  &rarr;  pts1[i] &harr; pts2[i]  (&ge;8 对)
    &rarr; computeFundamentalMatrix()     [归一化八点法 + 秩-2 强制]
    &rarr; computeEssentialMatrix()       [E = K^T F K]
    &rarr; recoverPose()                  [SVD分解, 4候选正深度投票]
    &rarr; triangulateAll()               [DLT, A4x4 X = 0]
    &rarr; 3D points</code></pre>

<p><strong>内参 K</strong> (自标定 RealSense D435, 640x480): fx=456.563, fy=455.041, cx=345.191, cy=213.798</p>

<p><strong>合成数据自验证结果</strong> (30 随机 3D 点, Z: 2~8m, GT位姿绕Y轴5&deg;, 右移0.3m):</p>
<table style="border-collapse:collapse; margin:8pt 0; font-size:9pt;">
  <tr style="background:#f1f5f9;"><td style="padding:4pt 16pt;"><strong>指标</strong></td><td style="padding:4pt 16pt;"><strong>结果</strong></td></tr>
  <tr><td style="padding:3pt 16pt;">Rotation error</td><td style="padding:3pt 16pt;">0 deg</td></tr>
  <tr><td style="padding:3pt 16pt;">Translation error</td><td style="padding:3pt 16pt;">0 deg (方向完全一致)</td></tr>
  <tr><td style="padding:3pt 16pt;">3D RMSE (scale-aligned)</td><td style="padding:3pt 16pt;">6.27606e-06 m</td></tr>
</table>
<p class="footnote">精度解析：旋转0&deg;，平移方向0&deg;，3D重建RMSE约6.3e-6 m，已达浮点数值精度级别。</p>

<hr class="divider">

<!-- ============================================================ -->
<!-- 附录 -->
<!-- ============================================================ -->
<h1>附录：项目结构</h1>

<pre><code>shengwei_writtenTest/
├── include/
│   ├── common_types.h        # GpsCoordinate, Resolution, Tile 类型定义
│   ├── map_manager.h         # MapManager: 多分辨率地图块管理器
│   └── graph.h               # Graph: 无向图 + Floyd-Warshall
├── src/
│   ├── map_manager.cpp       # MapManager 实现
│   ├── graph.cpp             # Graph 实现
│   ├── sfm.cpp               # SfM 完整流水线 (F&rarr;E&rarr;R,t&rarr;三角化)
│   └── test/
│       ├── Q1_answer_test.cpp # Q1-Q3 测试 (MapManager)
│       └── Q2_answer_test.cpp # 图算法测试 (Graph)
├── scripts/
│   ├── Q3_1/
│   │   ├── Depth_model_estimation.py  # YOLO 深度估计 (nuScenes)
│   │   └── example_depth_model.py     # YOLO 深度估计 (COCO)
│   └── test_images/                   # 输入图 + 输出图
├── CMakeLists.txt            # CMake 构建配置 (C++17, Eigen, PCL, CUDA可选)
└── README.md</code></pre>

<p><strong>构建与运行</strong>:</p>
<pre><code>mkdir build &amp;&amp; cd build
cmake .. &amp;&amp; make
./Q1_answer_test   # Q1-Q3: 地图块管理器 (自动/手动两种模式)
./Q2_answer_test   # 无向图最短路径 (自动/手动两种模式)
./Q3_sfm           # SfM完整流水线 (合成数据自验证)</code></pre>

<p class="footnote" style="text-align:center; border-top: 1px solid #e0e5ec; padding-top: 8pt; margin-top: 24pt;">
  GitHub: <a href="{github}" style="color:#1a56db;">{github}</a>
  &nbsp;|&nbsp; 生成时间: 2026-08-05
  &nbsp;|&nbsp; Powered by WeasyPrint
</p>

</body></html>
"""


def main():
    html = HTML_TEMPLATE.format(css=CSS, github=GITHUB_URL)
    html_path = os.path.join(SKILLS_DIR, "answer_temp.html")
    output = os.path.join(PROJ_DIR, "感知算法笔试_解答.pdf")

    with open(html_path, "w", encoding="utf-8") as f:
        f.write(html)

    HTML(filename=html_path).write_pdf(output)
    os.remove(html_path)

    import subprocess
    info = subprocess.run(["pdfinfo", output], capture_output=True, text=True)
    pages = [l for l in info.stdout.split("\n") if "Pages" in l][0]
    size = subprocess.run(["du", "-h", output], capture_output=True, text=True).stdout.split()[0]
    print(f"Generated: {output}")
    print(f"  {pages}, {size}B")


if __name__ == "__main__":
    main()
