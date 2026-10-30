// Randomized cross-check: radix_sort must agree with std::sort on every
// distribution and digit width, including edge cases (n=0,1, duplicates,
// narrow keys, all-equal keys).
#include <algorithm>
#include <cstdio>
#include <vector>

#include "../src/harness/rng.hpp"
#include "../src/sorting/sorting.hpp"

int main() {
    harness::Rng rng(1);
    int failures = 0, cases = 0;
    const int widths[] = {1, 3, 8, 11, 16};
    for (int trial = 0; trial < 400; ++trial) {
        size_t n = trial < 20 ? (size_t)trial : rng.below(5000);
        int bits = (int)(1 + rng.below(64));
        sorting::Key mask = bits == 64 ? ~0ULL : ((1ULL << bits) - 1);
        std::vector<sorting::Key> v(n);
        int mode = (int)rng.below(4);
        for (size_t i = 0; i < n; ++i) {
            if (mode == 0) v[i] = rng.next() & mask;
            else if (mode == 1) v[i] = (sorting::Key)i;
            else if (mode == 2) v[i] = 42;
            else v[i] = rng.below(5);
        }
        std::vector<sorting::Key> ref = v;
        std::sort(ref.begin(), ref.end());
        for (int w : widths) {
            std::vector<sorting::Key> got = v;
            sorting::radix_sort(got, w);
            ++cases;
            if (got != ref) {
                ++failures;
                std::printf("FAIL n=%zu bits=%d mode=%d digit_bits=%d\n", n, bits, mode, w);
            }
        }
    }
    std::printf("sorting: %d/%d cases passed\n", cases - failures, cases);
    return failures ? 1 : 0;
}
