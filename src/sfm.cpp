#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/SVD>
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
#include <cassert>

using namespace Eigen;
using namespace std;

using Point2f = Vector2f;
using Point3f = Vector3f;

// ═══════════════════════════════════════════════════════════
// K: 自标定 RealSense D435 内参 (640x480)
// 来源: /home/zjy/.ros/camera_info_06/ost.yaml
// 仅在此处使用，非通用参数
// ═══════════════════════════════════════════════════════════
const Matrix3f K = (Matrix3f() << 456.56284f, 0.0f,       345.19053f,
                                 0.0f,       455.04096f, 213.79762f,
                                 0.0f,       0.0f,        1.0f).finished();

// ── 计算点集质心 ───────────────────────────────────────
Vector2f computeMean(const vector<Point2f>& pts)
{
    if (pts.empty()) return {0.0f, 0.0f};
    Vector2f sum = Vector2f::Zero();
    for (const auto& p : pts) sum += p;
    return sum / pts.size();
}

// ── Hartley 归一化 ─────────────────────────────────────
Matrix3f normalizePoints(const vector<Point2f>& pts, vector<Point2f>& norm_pts)
{
    Vector2f mean = computeMean(pts);

    float avg_dist = 0.0f;
    for (const auto& p : pts) avg_dist += (p - mean).norm();
    avg_dist /= pts.size();

    float s = sqrt(2.0f) / avg_dist;

    Matrix3f T = Matrix3f::Identity();
    T(0, 0) = s;
    T(1, 1) = s;
    T(0, 2) = -s * mean(0);
    T(1, 2) = -s * mean(1);

    norm_pts.clear();
    norm_pts.reserve(pts.size());
    for (const auto& p : pts)
    {
        Vector3f h(p(0), p(1), 1.0f);
        Vector3f h_norm = T * h;
        norm_pts.emplace_back(h_norm(0), h_norm(1));
    }
    return T;
}

// ── 归一化八点法求基础矩阵 F ────────────────────────────
Matrix3f computeFundamentalMatrix(const vector<Point2f>& pts1,
                                  const vector<Point2f>& pts2)
{
    assert(pts1.size() == pts2.size() && pts1.size() >= 8);

    vector<Point2f> n1, n2;
    Matrix3f T1 = normalizePoints(pts1, n1);
    Matrix3f T2 = normalizePoints(pts2, n2);

    int n = n1.size();
    MatrixXf A(n, 9);
    for (int i = 0; i < n; ++i)
    {
        float u1 = n1[i](0), v1 = n1[i](1);
        float u2 = n2[i](0), v2 = n2[i](1);
        A.row(i) << u1 * u2, v1 * u2, u2,
                     u1 * v2, v1 * v2, v2,
                     u1,      v1,      1.0f;
    }

    JacobiSVD<MatrixXf> svd_A(A, ComputeFullV);
    VectorXf f = svd_A.matrixV().col(8);
    Matrix3f F_hat;
    F_hat << f(0), f(1), f(2),
             f(3), f(4), f(5),
             f(6), f(7), f(8);

    JacobiSVD<Matrix3f> svd_F(F_hat, ComputeFullU | ComputeFullV);
    Vector3f S = svd_F.singularValues();
    S(2) = 0.0f;
    Matrix3f F_rank2 = svd_F.matrixU() * S.asDiagonal() * svd_F.matrixV().transpose();

    return T2.transpose() * F_rank2 * T1;
}

// ── 从 F 和内参 K 计算本质矩阵 E ──────────────────────
//    推导: x'^T F x = 0, x = K x_norm
//    => x_norm'^T (K^T F K) x_norm = 0
//    => E = K^T F K
Matrix3f computeEssentialMatrix(const Matrix3f& F, const Matrix3f& K_mat)
{
    return K_mat.transpose() * F * K_mat;
}

// ── 检查正深度 (Cheirality check) ────────────────────
//    统计两个相机前方可见的点的数量
int countPositiveDepth(const Matrix3f& R, const Vector3f& t,
                       const vector<Point2f>& pts1,
                       const vector<Point2f>& pts2)
{
    Matrix<float, 3, 4> P1, P2;
    P1 << 1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1, 0;
    P2 << R, t;

    int count = 0;
    for (size_t i = 0; i < pts1.size(); ++i)
    {
        // DLT 三角化 (见 triangulate 函数)
        float u1 = pts1[i](0), v1 = pts1[i](1);
        float u2 = pts2[i](0), v2 = pts2[i](1);

        Matrix4f A;
        A.row(0) = u1 * P1.row(2) - P1.row(0);
        A.row(1) = v1 * P1.row(2) - P1.row(1);
        A.row(2) = u2 * P2.row(2) - P2.row(0);
        A.row(3) = v2 * P2.row(2) - P2.row(1);

        JacobiSVD<Matrix4f> svd(A, ComputeFullV);
        Vector4f X = svd.matrixV().col(3);
        Vector3f X3D = X.head<3>() / X(3);

        // 相机1视角: Z > 0
        float z1 = X3D(2);
        // 相机2视角: 将点变换到相机2坐标系
        Vector3f X2 = R * X3D + t;
        float z2 = X2(2);

        if (z1 > 0 && z2 > 0) ++count;
    }
    return count;
}

// ── 从 E 分解 R, t, 并选择正深度解 ──────────────────────
//    返回完整投影矩阵 P2 = K * [R | t]
void recoverPose(const Matrix3f& E,
                 const vector<Point2f>& pts1_pixel,
                 const vector<Point2f>& pts2_pixel,
                 Matrix3f& R_out, Vector3f& t_out)
{
    // 像素坐标 → 归一化平面坐标 (x_norm = K^{-1} * x_pixel)
    Matrix3f K_inv = K.inverse();
    vector<Point2f> pts1_norm, pts2_norm;
    pts1_norm.reserve(pts1_pixel.size());
    pts2_norm.reserve(pts2_pixel.size());
    for (size_t i = 0; i < pts1_pixel.size(); ++i)
    {
        Vector3f n1 = K_inv * Vector3f(pts1_pixel[i](0), pts1_pixel[i](1), 1.0f);
        Vector3f n2 = K_inv * Vector3f(pts2_pixel[i](0), pts2_pixel[i](1), 1.0f);
        pts1_norm.emplace_back(n1(0), n1(1));
        pts2_norm.emplace_back(n2(0), n2(1));
    }

    // SVD 分解 E
    JacobiSVD<Matrix3f> svd_E(E, ComputeFullU | ComputeFullV);
    Matrix3f U = svd_E.matrixU();
    Matrix3f V = svd_E.matrixV();

    // 修正符号: det(U) > 0, det(V) > 0
    if (U.determinant() < 0) U.col(2) *= -1;
    if (V.determinant() < 0) V.col(2) *= -1;

    // 辅助矩阵
    Matrix3f W;
    W << 0, -1,  0,
         1,  0,  0,
         0,  0,  1;

    // 4 组候选解
    Matrix3f R_cand[4];
    Vector3f t_cand[4];

    R_cand[0] = U * W * V.transpose();
    t_cand[0] =  U.col(2);
    R_cand[1] = U * W * V.transpose();
    t_cand[1] = -U.col(2);
    R_cand[2] = U * W.transpose() * V.transpose();
    t_cand[2] =  U.col(2);
    R_cand[3] = U * W.transpose() * V.transpose();
    t_cand[3] = -U.col(2);

    // 选正深度点最多的解 (用归一化坐标 + 归一化相机矩阵)
    int best = 0, best_count = 0;
    for (int i = 0; i < 4; ++i)
    {
        int cnt = countPositiveDepth(R_cand[i], t_cand[i], pts1_norm, pts2_norm);
        if (cnt > best_count) { best = i; best_count = cnt; }
    }

    R_out = R_cand[best];
    t_out = t_cand[best];
}

// ── DLT 三角化：对一对匹配点计算三维坐标 ─────────────────
Point3f triangulate(const Matrix<float, 3, 4>& P1,
                    const Matrix<float, 3, 4>& P2,
                    const Point2f& pt1, const Point2f& pt2)
{
    float u1 = pt1(0), v1 = pt1(1);
    float u2 = pt2(0), v2 = pt2(1);

    Matrix4f A;
    A.row(0) = u1 * P1.row(2) - P1.row(0);
    A.row(1) = v1 * P1.row(2) - P1.row(1);
    A.row(2) = u2 * P2.row(2) - P2.row(0);
    A.row(3) = v2 * P2.row(2) - P2.row(1);

    JacobiSVD<Matrix4f> svd(A, ComputeFullV);
    Vector4f X = svd.matrixV().col(3);
    return X.head<3>() / X(3);
}

// ── 批量三角化 ─────────────────────────────────────────
void triangulateAll(const Matrix3f& R, const Vector3f& t,
                    const vector<Point2f>& pts1,
                    const vector<Point2f>& pts2,
                    vector<Point3f>& points3D)
{
    Matrix<float, 3, 4> P1, P2;
    P1 << 1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1, 0;
    P2 << R, t;

    points3D.clear();
    points3D.reserve(pts1.size());
    for (size_t i = 0; i < pts1.size(); ++i)
        points3D.push_back(triangulate(P1, P2, pts1[i], pts2[i]));
}

// ── 根据匹配关系提取对齐点对 ──────────────────────────
//    pts1[i] 与 pts2[i] 一一对应，构成第 i 个匹配对
void extractAlignedPoints(const vector<Point2f>& set_k_minus_1,
                          const vector<Point2f>& set_k,
                          const vector<pair<int, int>>& matches,
                          vector<Point2f>& pts1,
                          vector<Point2f>& pts2)
{
    pts1.clear();
    pts2.clear();
    pts1.reserve(matches.size());
    pts2.reserve(matches.size());

    for (const auto& m : matches)
    {
        int idx1 = m.first;
        int idx2 = m.second;

        if (idx1 >= 0 && idx1 < (int)set_k_minus_1.size() &&
            idx2 >= 0 && idx2 < (int)set_k.size())
        {
            pts1.push_back(set_k_minus_1[idx1]);
            pts2.push_back(set_k[idx2]);
        }
    }
}

// ── 生成合成测试数据 ─────────────────────────────────
//    已知 R_gt, t_gt, 生成 3D 点并投影到两帧
void generateSyntheticData(vector<Point2f>& set_k_minus_1,
                           vector<Point2f>& set_k,
                           vector<pair<int, int>>& matches,
                           Matrix3f& R_gt, Vector3f& t_gt,
                           vector<Point3f>& pts3D_gt,
                           int num_points = 30)
{
    // Ground truth 位姿: 右移 0.3m, 绕 Y 轴旋转 5°
    float angle = 5.0f * M_PI / 180.0f;
    R_gt = AngleAxisf(angle, Vector3f::UnitY()).toRotationMatrix();
    t_gt = Vector3f(0.3f, 0.05f, 0.1f);

    // 相机投影矩阵 P1 = K[I|0], P2 = K[R_gt|t_gt]
    Matrix<float, 3, 4> P1, P2;
    P1 << 1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1, 0;
    P2 << R_gt, t_gt;

    // 随机数生成器
    mt19937 rng(42); // 固定种子，结果可复现
    uniform_real_distribution<float> dist_x(-2.0f, 2.0f);
    uniform_real_distribution<float> dist_y(-1.5f, 1.5f);
    uniform_real_distribution<float> dist_z( 2.0f, 8.0f);

    pts3D_gt.clear();
    set_k_minus_1.clear();
    set_k.clear();
    matches.clear();

    for (int i = 0; i < num_points; ++i)
    {
        // 随机生成相机前方 3D 点
        Vector4f X3D(dist_x(rng), dist_y(rng), dist_z(rng), 1.0f);
        pts3D_gt.push_back(X3D.head<3>());

        // 投影到两帧 (归一化坐标 → 像素坐标)
        Vector3f x1_norm = P1 * X3D;
        Vector3f x2_norm = P2 * X3D;

        Vector3f px1 = K * (x1_norm / x1_norm(2));
        Vector3f px2 = K * (x2_norm / x2_norm(2));

        set_k_minus_1.emplace_back(px1(0), px1(1));
        set_k.emplace_back(px2(0), px2(1));

        // 一一对应映射 (i ↔ i)
        matches.emplace_back(i, i);
    }
}

// ═══════════════════════════════════════════════════════════
// 测试入口
// ═══════════════════════════════════════════════════════════
int main()
{
    cout << "K (RealSense D435 640x480, from ost.yaml):\n" << K << endl << endl;

    // ── 生成合成测试数据 ──
    vector<Point2f> set_k_minus_1, set_k;
    vector<pair<int, int>> matches;
    Matrix3f R_gt;
    Vector3f t_gt;
    vector<Point3f> pts3D_gt;

    generateSyntheticData(set_k_minus_1, set_k, matches, R_gt, t_gt, pts3D_gt);

    cout << "--- Ground Truth ---" << endl;
    cout << "R_gt =\n" << R_gt << endl;
    cout << "t_gt = " << t_gt.transpose() << endl << endl;

    // Step 0: 根据映射关系提取对齐点对
    vector<Point2f> pts1, pts2;
    extractAlignedPoints(set_k_minus_1, set_k, matches, pts1, pts2);

    if (pts1.size() < 8)
    {
        cerr << "Need at least 8 point correspondences, got " << pts1.size() << "." << endl;
        return 1;
    }

    cout << "Loaded " << pts1.size() << " aligned matches." << endl << endl;

    // Step 1-2: 归一化八点法 → F
    Matrix3f F = computeFundamentalMatrix(pts1, pts2);
    cout << "F =\n" << F << endl << endl;

    // Step 3: 算本质矩阵 E = K^T F K
    Matrix3f E = computeEssentialMatrix(F, K);
    cout << "E =\n" << E << endl << endl;

    // Step 4: 从 E 分解 R, t
    Matrix3f R_est;
    Vector3f t_est;
    recoverPose(E, pts1, pts2, R_est, t_est);

    // t 的尺度不确定，归一化后比较方向
    t_est.normalize();

    cout << "R_est =\n" << R_est << endl;
    cout << "t_est (normalized) = " << t_est.transpose() << endl << endl;

    // 与 ground truth 比较
    cout << "--- Error ---" << endl;
    float trace_val = (R_gt.transpose() * R_est).trace();
    float cos_ang = clamp((trace_val - 1.0f) / 2.0f, -1.0f, 1.0f);
    float ang_err = acos(cos_ang) * 180.0f / M_PI;
    cout << "Rotation error: " << ang_err << " deg" << endl;

    Vector3f t_gt_norm = t_gt.normalized();
    float cos_t = clamp(abs(t_gt_norm.dot(t_est)), 0.0f, 1.0f);
    float t_err = acos(cos_t) * 180.0f / M_PI;
    if (t_err > 90.0f) t_err = 180.0f - t_err;
    cout << "Translation error: " << t_err << " deg" << endl << endl;

    // Step 5: 三角化 — 需将像素坐标转为归一化坐标
    Matrix3f K_inv = K.inverse();
    vector<Point2f> pts1_norm, pts2_norm;
    for (size_t i = 0; i < pts1.size(); ++i)
    {
        Vector3f n1 = K_inv * Vector3f(pts1[i](0), pts1[i](1), 1.0f);
        Vector3f n2 = K_inv * Vector3f(pts2[i](0), pts2[i](1), 1.0f);
        pts1_norm.emplace_back(n1(0), n1(1));
        pts2_norm.emplace_back(n2(0), n2(1));
    }

    vector<Point3f> points3D;
    triangulateAll(R_est, t_est, pts1_norm, pts2_norm, points3D);
    cout << "Triangulated " << points3D.size() << " 3D points." << endl;

    // 与 GT 3D 点比较 (尺度需要对齐)
    if (!points3D.empty())
    {
        Vector3f gt_center = Vector3f::Zero(), est_center = Vector3f::Zero();
        for (size_t i = 0; i < pts3D_gt.size(); ++i) gt_center += pts3D_gt[i];
        gt_center /= pts3D_gt.size();
        for (size_t i = 0; i < points3D.size(); ++i) est_center += points3D[i];
        est_center /= points3D.size();

        // 对齐尺度
        float gt_scale = 0, est_scale = 0;
        for (size_t i = 0; i < pts3D_gt.size(); ++i)
        {
            gt_scale += (pts3D_gt[i] - gt_center).norm();
            est_scale += (points3D[i] - est_center).norm();
        }
        float scale = gt_scale / est_scale;

        float rmse = 0;
        for (size_t i = 0; i < pts3D_gt.size(); ++i)
            rmse += (pts3D_gt[i] - points3D[i] * scale).squaredNorm();
        rmse = sqrt(rmse / pts3D_gt.size());
        cout << "3D RMSE (scale-aligned): " << rmse << " m" << endl;
    }

    return 0;
}
