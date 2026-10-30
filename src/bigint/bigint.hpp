// Integer multiplication pair: Karatsuba O(n^1.585) vs schoolbook O(n^2).
//
// Numbers are little-endian arrays of 32-bit limbs. The schoolbook routine
// is the standard row-by-row multiply-accumulate with 64-bit intermediates.
// Karatsuba splits balanced operands at the midpoint and recurses until the
// operand length drops to `cutoff` limbs, where it hands off to the same
// schoolbook kernel. A caller-provided scratch buffer avoids heap traffic
// inside the recursion, matching how production libraries (GMP) do it.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace bigint {

using Limb = uint32_t;
using DLimb = uint64_t;

// r[0..na+nb) = a * b. r must be zero-initialised.
inline void schoolbook(const Limb* a, size_t na, const Limb* b, size_t nb, Limb* r) {
    for (size_t i = 0; i < na; ++i) {
        DLimb carry = 0;
        const DLimb ai = a[i];
        for (size_t j = 0; j < nb; ++j) {
            DLimb t = ai * b[j] + r[i + j] + carry;
            r[i + j] = (Limb)t;
            carry = t >> 32;
        }
        r[i + nb] = (Limb)carry;
    }
}

namespace detail {

// r[0..n) = x[0..nx) + y[0..ny), returns carry; n = max(nx,ny)
inline Limb add_n(const Limb* x, size_t nx, const Limb* y, size_t ny, Limb* r) {
    size_t n = nx > ny ? nx : ny;
    DLimb carry = 0;
    for (size_t i = 0; i < n; ++i) {
        DLimb t = carry + (i < nx ? x[i] : 0) + (i < ny ? y[i] : 0);
        r[i] = (Limb)t;
        carry = t >> 32;
    }
    return (Limb)carry;
}

// x[0..nx) -= y[0..ny), ny <= nx, assumes x >= y
inline void sub_in_place(Limb* x, size_t nx, const Limb* y, size_t ny) {
    int64_t borrow = 0;
    for (size_t i = 0; i < nx; ++i) {
        int64_t t = (int64_t)x[i] - (i < ny ? (int64_t)y[i] : 0) - borrow;
        borrow = t < 0;
        if (t < 0) t += ((int64_t)1 << 32);
        x[i] = (Limb)t;
        if (i >= ny && borrow == 0) break;
    }
}

// x[0..nx) += y[0..ny), carry propagates through x; caller guarantees no overflow
inline void add_in_place(Limb* x, size_t nx, const Limb* y, size_t ny) {
    DLimb carry = 0;
    for (size_t i = 0; i < nx; ++i) {
        if (i >= ny && carry == 0) break;
        DLimb t = carry + x[i] + (i < ny ? y[i] : 0);
        x[i] = (Limb)t;
        carry = t >> 32;
    }
}

// Equal-length Karatsuba: r[0..2n) = a[0..n) * b[0..n). r must be zeroed.
inline void karatsuba_rec(const Limb* a, const Limb* b, size_t n, Limb* r, size_t cutoff,
                          Limb* scratch) {
    // For n < 4 the "sum" sub-product has hi+1 >= n limbs, so the recursion
    // would not shrink; schoolbook is the base case there regardless of cutoff.
    if (n <= cutoff || n < 4) {
        schoolbook(a, n, b, n, r);
        return;
    }
    const size_t m = n / 2;      // low half length
    const size_t hi = n - m;     // high half length (>= m)

    // z0 = a0*b0 in r[0..2m), z2 = a1*b1 in r[2m..2n)
    karatsuba_rec(a, b, m, r, cutoff, scratch);
    karatsuba_rec(a + m, b + m, hi, r + 2 * m, cutoff, scratch);

    // sa = a0 + a1, sb = b0 + b1 (hi+1 limbs each)
    const size_t s = hi + 1;
    Limb* sa = scratch;
    Limb* sb = sa + s;
    Limb* z1 = sb + s;  // 2s limbs
    Limb* next = z1 + 2 * s;
    sa[hi] = add_n(a, m, a + m, hi, sa);
    sb[hi] = add_n(b, m, b + m, hi, sb);
    std::memset(z1, 0, 2 * s * sizeof(Limb));
    karatsuba_rec(sa, sb, s, z1, cutoff, next);

    // z1 -= z0 + z2
    sub_in_place(z1, 2 * s, r, 2 * m);
    sub_in_place(z1, 2 * s, r + 2 * m, 2 * hi);

    // r[m..2n) += z1 (top limbs of z1 are zero so this cannot overflow 2n)
    add_in_place(r + m, 2 * n - m, z1, 2 * s);
}

}  // namespace detail

// Scratch limbs needed by karatsuba() for operands of n limbs.
inline size_t karatsuba_scratch_size(size_t n) {
    size_t total = 0;
    while (n >= 4) {  // mirrors the recursion's base case
        size_t hi = n - n / 2 + 1;
        total += 4 * hi;
        n = hi;
    }
    return total + 64;
}

// r[0..2n) = a*b, both n limbs. Result buffer must be zeroed.
inline void karatsuba(const Limb* a, const Limb* b, size_t n, Limb* r, size_t cutoff,
                      std::vector<Limb>& scratch) {
    if (cutoff < 1) cutoff = 1;
    size_t need = karatsuba_scratch_size(n);
    if (scratch.size() < need) scratch.resize(need);
    detail::karatsuba_rec(a, b, n, r, cutoff, scratch.data());
}

}  // namespace bigint
