#include <cstdlib>
#include <string>
#include <vector>

#include "../harness/bench.hpp"
#include "../harness/rng.hpp"
#include "bigint.hpp"

using harness::Rng;

struct Input {
    size_t n = 0;  // operand length in 32-bit limbs
    std::vector<bigint::Limb> a, b, r;
    std::vector<bigint::Limb> scratch;
};

// Distributions:
//   uniform   random limbs (typical)
//   ones      all limbs 0xFFFFFFFF (maximal carry propagation)
//   sparse    90% zero limbs (no effect on these algorithms' work; included
//             so the atlas can show that the pair is input-oblivious)
static Input generate(size_t n, const std::string& dist, const std::string&, uint64_t seed) {
    Rng rng(seed);
    Input in;
    in.n = n;
    in.a.resize(n);
    in.b.resize(n);
    in.r.assign(2 * n, 0);
    auto draw = [&]() -> bigint::Limb {
        if (dist == "ones") return 0xFFFFFFFFu;
        if (dist == "sparse") return rng.uniform01() < 0.9 ? 0u : (bigint::Limb)rng.next();
        return (bigint::Limb)rng.next();
    };
    for (auto& x : in.a) x = draw();
    for (auto& x : in.b) x = draw();
    in.scratch.resize(bigint::karatsuba_scratch_size(n));
    return in;
}

int main(int argc, char** argv) {
    harness::Options opt;
    opt.params = {"32"};
    if (!harness::parse_options(argc, argv, opt, "uniform",
                                "Karatsuba cutoff in limbs (default 32; try 8,16,32,64,128)"))
        return 2;
    if (opt.sizes.empty()) opt.sizes = harness::parse_sizes("8:1048576:1.5");

    harness::Suite<Input> suite;
    suite.pair = "bigint";
    suite.generate = generate;
    suite.algos.push_back({"schoolbook",
                           [](Input& in, const std::string&) {
                               bigint::schoolbook(in.a.data(), in.n, in.b.data(), in.n, in.r.data());
                           },
                           false});
    suite.algos.push_back({"karatsuba",
                           [](Input& in, const std::string& p) {
                               size_t cutoff = (size_t)std::atoi(p.c_str());
                               if (cutoff == 0) cutoff = 32;
                               bigint::karatsuba(in.a.data(), in.b.data(), in.n, in.r.data(), cutoff,
                                                 in.scratch);
                           },
                           true});
    suite.same_result = [](const Input& a, const Input& b) { return a.r == b.r; };
    return harness::run_suite(suite, opt);
}
