// Matrix multiplication pair: Strassen O(n^2.807) vs naive O(n^3).
//
// All matrices are square, row-major, double precision. The naive baseline
// uses the i-k-j loop order, which streams through B and C rows contiguously
// and is what a careful programmer writes without blocking or intrinsics.
// The textbook i-j-k order is kept as a second baseline because its
// stride-n access to B makes it a vivid demonstration of cache effects.
//
// Strassen recurses on quadrant views until the sub-problem is no larger
// than `cutoff` (or has odd dimension, in which case one zero row/column of
// padding is added at that level). The base case is the same i-k-j kernel,
// so the comparison isolates the recursive structure itself.
#pragma once
#include <cstddef>
#include <cstring>
#include <vector>

namespace matmul {

struct View {
    double* p;
    size_t stride;
    double& operator()(size_t i, size_t j) const { return p[i * stride + j]; }
    double* row(size_t i) const { return p + i * stride; }
};
struct CView {
    const double* p;
    size_t stride;
    double operator()(size_t i, size_t j) const { return p[i * stride + j]; }
    const double* row(size_t i) const { return p + i * stride; }
};

// C = A * B, i-k-j order. C must be zeroed by the caller if accumulate=false.
inline void naive_ikj(size_t n, CView A, CView B, View C) {
    for (size_t i = 0; i < n; ++i) {
        double* c = C.row(i);
        for (size_t j = 0; j < n; ++j) c[j] = 0.0;
        for (size_t k = 0; k < n; ++k) {
            const double a = A(i, k);
            const double* b = B.row(k);
            for (size_t j = 0; j < n; ++j) c[j] += a * b[j];
        }
    }
}

inline void naive_ijk(size_t n, CView A, CView B, View C) {
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j) {
            double s = 0.0;
            for (size_t k = 0; k < n; ++k) s += A(i, k) * B(k, j);
            C(i, j) = s;
        }
}

namespace detail {

inline void add(size_t n, CView X, CView Y, View Z) {
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j) Z(i, j) = X(i, j) + Y(i, j);
}
inline void sub(size_t n, CView X, CView Y, View Z) {
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j) Z(i, j) = X(i, j) - Y(i, j);
}
inline CView quad(CView M, size_t h, int r, int c) { return {M.p + r * h * M.stride + c * h, M.stride}; }
inline View quad(View M, size_t h, int r, int c) { return {M.p + r * h * M.stride + c * h, M.stride}; }

inline void strassen_rec(size_t n, CView A, CView B, View C, size_t cutoff);

// Handles odd n by copying into (n+1)x(n+1) zero-padded buffers.
inline void strassen_padded(size_t n, CView A, CView B, View C, size_t cutoff) {
    const size_t m = n + 1;
    std::vector<double> Ap(m * m, 0.0), Bp(m * m, 0.0), Cp(m * m, 0.0);
    for (size_t i = 0; i < n; ++i) {
        std::memcpy(&Ap[i * m], A.row(i), n * sizeof(double));
        std::memcpy(&Bp[i * m], B.row(i), n * sizeof(double));
    }
    strassen_rec(m, {Ap.data(), m}, {Bp.data(), m}, {Cp.data(), m}, cutoff);
    for (size_t i = 0; i < n; ++i) std::memcpy(C.row(i), &Cp[i * m], n * sizeof(double));
}

inline void strassen_rec(size_t n, CView A, CView B, View C, size_t cutoff) {
    if (n <= cutoff) { naive_ikj(n, A, B, C); return; }
    if (n & 1) { strassen_padded(n, A, B, C, cutoff); return; }

    const size_t h = n / 2;
    CView A11 = quad(A, h, 0, 0), A12 = quad(A, h, 0, 1), A21 = quad(A, h, 1, 0), A22 = quad(A, h, 1, 1);
    CView B11 = quad(B, h, 0, 0), B12 = quad(B, h, 0, 1), B21 = quad(B, h, 1, 0), B22 = quad(B, h, 1, 1);
    View C11 = quad(C, h, 0, 0), C12 = quad(C, h, 0, 1), C21 = quad(C, h, 1, 0), C22 = quad(C, h, 1, 1);

    // Two temporaries for operand sums and seven for products (standard
    // Strassen; Winograd's variant would save a few additions but the
    // point here is the canonical algorithm).
    std::vector<double> T1(h * h), T2(h * h);
    std::vector<double> M(7 * h * h);
    View t1{T1.data(), h}, t2{T2.data(), h};
    auto Mi = [&](int i) { return View{M.data() + (size_t)i * h * h, h}; };
    auto cMi = [&](int i) { return CView{M.data() + (size_t)i * h * h, h}; };
    CView ct1{T1.data(), h}, ct2{T2.data(), h};

    add(h, A11, A22, t1); add(h, B11, B22, t2); strassen_rec(h, ct1, ct2, Mi(0), cutoff);  // M1=(A11+A22)(B11+B22)
    add(h, A21, A22, t1);                       strassen_rec(h, ct1, B11, Mi(1), cutoff);  // M2=(A21+A22)B11
    sub(h, B12, B22, t2);                       strassen_rec(h, A11, ct2, Mi(2), cutoff);  // M3=A11(B12-B22)
    sub(h, B21, B11, t2);                       strassen_rec(h, A22, ct2, Mi(3), cutoff);  // M4=A22(B21-B11)
    add(h, A11, A12, t1);                       strassen_rec(h, ct1, B22, Mi(4), cutoff);  // M5=(A11+A12)B22
    sub(h, A21, A11, t1); add(h, B11, B12, t2); strassen_rec(h, ct1, ct2, Mi(5), cutoff);  // M6=(A21-A11)(B11+B12)
    sub(h, A12, A22, t1); add(h, B21, B22, t2); strassen_rec(h, ct1, ct2, Mi(6), cutoff);  // M7=(A12-A22)(B21+B22)

    for (size_t i = 0; i < h; ++i)
        for (size_t j = 0; j < h; ++j) {
            double m1 = cMi(0)(i, j), m2 = cMi(1)(i, j), m3 = cMi(2)(i, j), m4 = cMi(3)(i, j),
                   m5 = cMi(4)(i, j), m6 = cMi(5)(i, j), m7 = cMi(6)(i, j);
            C11(i, j) = m1 + m4 - m5 + m7;
            C12(i, j) = m3 + m5;
            C21(i, j) = m2 + m4;
            C22(i, j) = m1 - m2 + m3 + m6;
        }
}

}  // namespace detail

inline void strassen(size_t n, const double* A, const double* B, double* C, size_t cutoff = 64) {
    detail::strassen_rec(n, {A, n}, {B, n}, {C, n}, cutoff < 1 ? 1 : cutoff);
}
inline void naive(size_t n, const double* A, const double* B, double* C) {
    naive_ikj(n, {A, n}, {B, n}, {C, n});
}
inline void naive_textbook(size_t n, const double* A, const double* B, double* C) {
    naive_ijk(n, {A, n}, {B, n}, {C, n});
}

}  // namespace matmul
