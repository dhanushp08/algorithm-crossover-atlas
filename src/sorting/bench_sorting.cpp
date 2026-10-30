#include <cstdlib>
#include <string>
#include <vector>

#include "../harness/bench.hpp"
#include "../harness/rng.hpp"
#include "sorting.hpp"

using harness::Rng;
using Input = std::vector<sorting::Key>;

// Distributions:
//   uniform8/16/32/64   uniform random keys of that bit width (key-width axis)
//   nearly_sorted       sorted 64-bit keys with 1% random adjacent-ish swaps
//   reverse             strictly decreasing 64-bit keys
//   few_unique          64-bit keys drawn from 100 distinct values
//   organ_pipe          ascending then descending (classic quicksort stressor)
static Input generate(size_t n, const std::string& dist, const std::string&, uint64_t seed) {
    Rng rng(seed);
    Input v(n);
    if (dist.rfind("uniform", 0) == 0) {
        int bits = std::atoi(dist.c_str() + 7);
        if (bits <= 0 || bits > 64) bits = 64;
        sorting::Key mask = bits == 64 ? ~0ULL : ((1ULL << bits) - 1);
        for (auto& x : v) x = rng.next() & mask;
    } else if (dist == "nearly_sorted") {
        for (size_t i = 0; i < n; ++i) v[i] = (sorting::Key)i * 7919ULL;
        size_t swaps = n / 100 + 1;
        for (size_t s = 0; s < swaps; ++s) {
            size_t i = rng.below(n), j = rng.below(n);
            std::swap(v[i], v[j]);
        }
    } else if (dist == "reverse") {
        for (size_t i = 0; i < n; ++i) v[i] = (sorting::Key)(n - i) * 7919ULL;
    } else if (dist == "few_unique") {
        sorting::Key vals[100];
        for (auto& x : vals) x = rng.next();
        for (auto& x : v) x = vals[rng.below(100)];
    } else if (dist == "organ_pipe") {
        for (size_t i = 0; i < n; ++i) v[i] = (sorting::Key)(i < n / 2 ? i : n - i) * 7919ULL;
    } else {
        std::fprintf(stderr, "unknown distribution %s\n", dist.c_str());
        std::exit(2);
    }
    return v;
}

int main(int argc, char** argv) {
    harness::Options opt;
    opt.params = {"8"};
    if (!harness::parse_options(argc, argv, opt,
                                "uniform64,uniform32,uniform16,uniform8,nearly_sorted,reverse,few_unique",
                                "radix digit bits (default 8; try 8,11,16)"))
        return 2;
    if (opt.sizes.empty()) opt.sizes = harness::parse_sizes("16:100000000:2");

    harness::Suite<Input> suite;
    suite.pair = "sorting";
    suite.generate = generate;
    suite.algos.push_back(
        {"introsort", [](Input& v, const std::string&) { sorting::introsort(v); }, false});
    suite.algos.push_back({"radix",
                           [](Input& v, const std::string& p) {
                               int bits = std::atoi(p.c_str());
                               if (bits < 1 || bits > 16) bits = 8;
                               sorting::radix_sort(v, bits);
                           },
                           true});
    suite.same_result = [](const Input& a, const Input& b) { return a == b; };
    return harness::run_suite(suite, opt);
}
