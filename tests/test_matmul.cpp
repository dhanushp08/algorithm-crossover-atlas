// Strassen (all cutoffs, odd and even n) must agree with the i-j-k product
// to within floating-point tolerance; integer inputs must agree exactly.
#include <cmath>
#include <cstdio>
#include <vector>

#include "../src/harness/rng.hpp"
#include "../src/matmul/matmul.hpp"

int main() {
    harness::Rng rng(2);
    int failures = 0, cases = 0;
    const size_t cutoffs[] = {1, 2, 3, 8, 16, 64};
    const size_t sizes[] = {1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 50, 64, 65, 100, 128, 129};
    for (size_t n : sizes) {
        for (int integer = 0; integer < 2; ++integer) {
            std::vector<double> A(n * n), B(n * n), C1(n * n), C2(n * n), C3(n * n);
            for (auto& x : A) x = integer ? (double)((int64_t)rng.below(21) - 10) : 2 * rng.uniform01() - 1;
            for (auto& x : B) x = integer ? (double)((int64_t)rng.below(21) - 10) : 2 * rng.uniform01() - 1;
            matmul::naive_textbook(n, A.data(), B.data(), C1.data());
            matmul::naive(n, A.data(), B.data(), C2.data());
            ++cases;
            for (size_t i = 0; i < n * n; ++i)
                if (C1[i] != C2[i] && std::fabs(C1[i] - C2[i]) > 1e-12 * n) {
                    ++failures;
                    std::printf("FAIL ikj vs ijk n=%zu\n", n);
                    break;
                }
            for (size_t cut : cutoffs) {
                matmul::strassen(n, A.data(), B.data(), C3.data(), cut);
                ++cases;
                double maxabs = 0;
                for (double x : C1) maxabs = std::max(maxabs, std::fabs(x));
                double tol = integer ? 0.0 : 1e-11 * n * (1 + maxabs);
                for (size_t i = 0; i < n * n; ++i)
                    if (std::fabs(C1[i] - C3[i]) > tol) {
                        ++failures;
                        std::printf("FAIL strassen n=%zu cutoff=%zu integer=%d diff=%g\n", n, cut,
                                    integer, std::fabs(C1[i] - C3[i]));
                        break;
                    }
            }
        }
    }
    std::printf("matmul: %d/%d cases passed\n", cases - failures, cases);
    return failures ? 1 : 0;
}
