#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

#include "../harness/bench.hpp"
#include "../harness/rng.hpp"
#include "matmul.hpp"

using harness::Rng;

struct Input {
    size_t n = 0;
    std::vector<double> A, B, C;
};

// Distributions:
//   uniform    entries uniform in [-1, 1)
//   integer    small integers in [-8, 8] (exact arithmetic, tests stability-free path)
//   sparse     90% zeros (no effect on dense algorithms' work, included for completeness)
static Input generate(size_t n, const std::string& dist, const std::string&, uint64_t seed) {
    Rng rng(seed);
    Input in;
    in.n = n;
    in.A.resize(n * n);
    in.B.resize(n * n);
    in.C.assign(n * n, 0.0);
    auto draw = [&]() -> double {
        if (dist == "integer") return (double)((int64_t)rng.below(17) - 8);
        if (dist == "sparse") return rng.uniform01() < 0.9 ? 0.0 : 2.0 * rng.uniform01() - 1.0;
        return 2.0 * rng.uniform01() - 1.0;
    };
    for (auto& x : in.A) x = draw();
    for (auto& x : in.B) x = draw();
    return in;
}

int main(int argc, char** argv) {
    harness::Options opt;
    opt.params = {"64"};
    if (!harness::parse_options(argc, argv, opt, "uniform",
                                "Strassen base-case cutoff (default 64; try 16,32,64,128,256)"))
        return 2;
    if (opt.sizes.empty())
        opt.sizes = {16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096};

    harness::Suite<Input> suite;
    suite.pair = "matmul";
    suite.generate = generate;
    suite.algos.push_back({"naive_ikj",
                           [](Input& in, const std::string&) {
                               matmul::naive(in.n, in.A.data(), in.B.data(), in.C.data());
                           },
                           false});
    suite.algos.push_back({"naive_ijk",
                           [](Input& in, const std::string&) {
                               matmul::naive_textbook(in.n, in.A.data(), in.B.data(), in.C.data());
                           },
                           false});
    suite.algos.push_back({"strassen",
                           [](Input& in, const std::string& p) {
                               size_t cutoff = (size_t)std::atoi(p.c_str());
                               if (cutoff == 0) cutoff = 64;
                               matmul::strassen(in.n, in.A.data(), in.B.data(), in.C.data(), cutoff);
                           },
                           true});
    suite.same_result = [](const Input& a, const Input& b) {
        // Strassen is numerically less stable than the naive product; allow
        // a tolerance that scales with n.
        double tol = 1e-9 * (double)a.n;
        double maxabs = 0.0;
        for (double x : a.C) maxabs = std::max(maxabs, std::fabs(x));
        for (size_t i = 0; i < a.C.size(); ++i)
            if (std::fabs(a.C[i] - b.C[i]) > tol * (1.0 + maxabs)) return false;
        return true;
    };
    return harness::run_suite(suite, opt);
}
