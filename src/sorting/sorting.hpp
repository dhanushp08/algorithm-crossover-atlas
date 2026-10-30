// Sorting pair: LSD radix sort O(n*k) vs introsort O(n log n).
//
// Radix sort here is a textbook least-significant-digit counting sort with a
// configurable digit width and two engineering touches that any competent
// implementation would have:
//   * a single histogram pass computes all digit histograms at once,
//   * passes in which every key has the same digit are skipped entirely
//     (this is what makes narrow keys, e.g. 16-bit values in 64-bit slots,
//     cheap and is the mechanism behind the "key width" variable).
// Introsort is libstdc++'s std::sort (Musser's introsort: median-of-3
// quicksort, heapsort fallback, insertion sort for small partitions).
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

namespace sorting {

using Key = uint64_t;

inline void introsort(std::vector<Key>& v) { std::sort(v.begin(), v.end()); }

// LSD radix sort with `digit_bits` per pass (8, 11 or 16 are sensible).
inline void radix_sort(std::vector<Key>& v, int digit_bits = 8) {
    const size_t n = v.size();
    if (n < 2) return;
    const int passes = (64 + digit_bits - 1) / digit_bits;
    const size_t buckets = (size_t)1 << digit_bits;
    const Key mask = (Key)buckets - 1;

    // Histograms for every pass in one sweep over the data.
    std::vector<size_t> hist((size_t)passes * buckets, 0);
    for (size_t i = 0; i < n; ++i) {
        Key x = v[i];
        for (int p = 0; p < passes; ++p) {
            hist[(size_t)p * buckets + ((x >> (p * digit_bits)) & mask)]++;
        }
    }

    std::vector<Key> tmp(n);
    Key* src = v.data();
    Key* dst = tmp.data();
    for (int p = 0; p < passes; ++p) {
        size_t* h = &hist[(size_t)p * buckets];
        // Skip a pass if all keys fall into one bucket.
        bool trivial = false;
        for (size_t b = 0; b < buckets; ++b) {
            if (h[b] == n) { trivial = true; break; }
            if (h[b] != 0) break;
        }
        if (trivial) continue;
        // exclusive prefix sum -> starting offsets
        size_t sum = 0;
        for (size_t b = 0; b < buckets; ++b) {
            size_t c = h[b];
            h[b] = sum;
            sum += c;
        }
        const int shift = p * digit_bits;
        for (size_t i = 0; i < n; ++i) {
            Key x = src[i];
            dst[h[(x >> shift) & mask]++] = x;
        }
        std::swap(src, dst);
    }
    if (src != v.data()) std::memcpy(v.data(), src, n * sizeof(Key));
}

}  // namespace sorting
