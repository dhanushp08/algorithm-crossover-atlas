// Karatsuba (all cutoffs, odd/even lengths) must equal schoolbook exactly,
// and schoolbook itself is validated independently by a modular identity:
// (a*b) mod p == ((a mod p) * (b mod p)) mod p for p = 2^61 - 1.
#include <cstdio>
#include <vector>

#include "../src/bigint/bigint.hpp"
#include "../src/harness/rng.hpp"

using namespace bigint;

static const uint64_t P = (1ULL << 61) - 1;

static uint64_t mulmod(uint64_t a, uint64_t b) { return (uint64_t)(((__uint128_t)a * b) % P); }

static uint64_t limbs_mod_p(const Limb* x, size_t n) {
    uint64_t r = 0;
    for (size_t i = n; i-- > 0;) r = ((__uint128_t)r * (1ULL << 32) + x[i]) % P;
    return r;
}

int main() {
    harness::Rng rng(3);
    int failures = 0, cases = 0;
    const size_t cutoffs[] = {1, 2, 3, 5, 8, 16, 32};
    std::vector<Limb> scratch;
    for (int trial = 0; trial < 300; ++trial) {
        size_t n = trial < 10 ? (size_t)trial + 1 : 1 + rng.below(600);
        std::vector<Limb> a(n), b(n), r1(2 * n, 0), r2(2 * n);
        int mode = (int)rng.below(3);
        for (size_t i = 0; i < n; ++i) {
            a[i] = mode == 1 ? 0xFFFFFFFFu : (Limb)rng.next();
            b[i] = mode == 2 ? 0xFFFFFFFFu : (Limb)rng.next();
        }
        schoolbook(a.data(), n, b.data(), n, r1.data());
        ++cases;
        uint64_t lhs = limbs_mod_p(r1.data(), 2 * n);
        uint64_t rhs = mulmod(limbs_mod_p(a.data(), n), limbs_mod_p(b.data(), n));
        if (lhs != rhs) {
            ++failures;
            std::printf("FAIL schoolbook modular check n=%zu\n", n);
        }
        for (size_t cut : cutoffs) {
            std::fill(r2.begin(), r2.end(), 0);
            karatsuba(a.data(), b.data(), n, r2.data(), cut, scratch);
            ++cases;
            if (r1 != r2) {
                ++failures;
                std::printf("FAIL karatsuba n=%zu cutoff=%zu mode=%d\n", n, cut, mode);
            }
        }
    }
    std::printf("bigint: %d/%d cases passed\n", cases - failures, cases);
    return failures ? 1 : 0;
}
