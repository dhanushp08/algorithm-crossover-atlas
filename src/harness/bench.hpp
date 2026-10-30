// Generic benchmark driver shared by all four algorithm pairs.
//
// Protocol for one (distribution, parameter, n) cell:
//   1. generate the input once from (seed, n, dist, param)    [not timed]
//   2. for r in 0 .. warmup+reps-1:
//        for each algorithm, in an order rotated by r:        <- interleaving
//            copy the input                                    [not timed]
//            start counters, run, stop counters               [timed]
//            emit a CSV row if r >= warmup
//
// Interleaving A and B inside the repetition loop (rather than running all
// of A then all of B) means slow drift of the machine state (thermal,
// background load, frequency) affects both algorithms equally, which is what
// matters for a ratio/crossover estimate.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

#include "affinity.hpp"
#include "cli.hpp"
#include "perf_counters.hpp"
#include "timer.hpp"

namespace harness {

template <class Input>
struct Algo {
    std::string name;
    // Runs the algorithm on a working copy of the input, given the current
    // pair-specific parameter string (e.g. "32" for a recursion cutoff).
    // Must leave a result the caller can inspect (see `same_result`).
    std::function<void(Input&, const std::string& param)> run;
    // Whether the pair-specific parameter (e.g. recursion cutoff) affects
    // this algorithm. If false, it is run only for the first parameter value
    // and its rows carry param="-".
    bool param_sensitive = true;
};

template <class Input>
struct Suite {
    std::string pair;
    std::function<Input(size_t n, const std::string& dist, const std::string& param, uint64_t seed)>
        generate;
    std::vector<Algo<Input>> algos;
    // Optional: verify that two algorithms produced the same answer on the
    // last repetition of each cell (cheap consistency check during benchmarking;
    // the full randomized tests live in tests/).
    std::function<bool(const Input& a, const Input& b)> same_result;
    // Optional: one-line description of a generated input (e.g. edge count,
    // decrease-key count), logged to stderr once per cell as "[cell] ...".
    std::function<std::string(const Input&)> describe;
    // Optional: whether a (n, dist, param) cell makes sense at all (e.g. a
    // complete graph on 10^7 vertices does not). Skipped cells emit no rows.
    std::function<bool(size_t n, const std::string& dist, const std::string& param)> applicable;
    // Optional: whether the parameter means anything for this distribution
    // (e.g. out-degree matters for random graphs but not for grids). When it
    // does not, the cell runs once and its rows carry param="-".
    std::function<bool(const std::string& dist)> dist_uses_param;
};

struct Row {
    std::string pair, algorithm, dist, param;
    size_t n;
    int rep;
    double time_ns;   // per iteration, includes one input copy
    double copy_ns;   // per iteration, the input copy alone
    size_t iters;     // iterations batched into this measurement
    std::vector<int64_t> counters;  // per iteration
};

inline void write_csv_header(FILE* f, const PerfCounters& pc) {
    std::fputs("pair,algorithm,dist,param,n,rep,time_ns,copy_ns,iters", f);
    for (const auto& s : pc.specs()) std::fprintf(f, ",%s", s.name);
    std::fputc('\n', f);
}

inline void write_csv_row(FILE* f, const Row& r) {
    std::fprintf(f, "%s,%s,%s,%s,%zu,%d,%.1f,%.1f,%zu", r.pair.c_str(), r.algorithm.c_str(),
                 r.dist.c_str(), r.param.c_str(), r.n, r.rep, r.time_ns, r.copy_ns, r.iters);
    for (int64_t v : r.counters) {
        if (v < 0)
            std::fputs(",", f);
        else
            std::fprintf(f, ",%lld", (long long)v);
    }
    std::fputc('\n', f);
}

template <class Input>
int run_suite(const Suite<Input>& suite, const Options& opt) {
    std::string pin_err = pin_to_core(opt.pin);
    if (!pin_err.empty()) std::fprintf(stderr, "[warn] %s\n", pin_err.c_str());

    PerfCounters pc;
    std::fprintf(stderr, "[info] pair=%s counters: %s\n", suite.pair.c_str(),
                 pc.availability_report().c_str());

    FILE* out = stdout;
    if (opt.out != "-") {
        out = std::fopen(opt.out.c_str(), "w");
        if (!out) {
            std::fprintf(stderr, "[error] cannot open %s\n", opt.out.c_str());
            return 2;
        }
    }
    write_csv_header(out, pc);

    const size_t A = suite.algos.size();
    const int total_runs = opt.warmup + opt.reps;

    for (const auto& dist : opt.dists) {
        const bool dist_param = !suite.dist_uses_param || suite.dist_uses_param(dist);
        for (size_t pi = 0; pi < opt.params.size(); ++pi) {
            if (!dist_param && pi > 0) break;
            const std::string& param = opt.params[pi];
            const std::string param_label = dist_param ? param : "-";
            // Which algorithms participate in this parameter value?
            std::vector<size_t> active;
            for (size_t a = 0; a < A; ++a)
                if (suite.algos[a].param_sensitive || pi == 0) active.push_back(a);
            if (active.empty()) continue;

            // Budget tracking: once an algorithm exceeds max_seconds on one
            // run, it is skipped for all larger n within this (dist,param).
            std::vector<bool> exhausted(A, false);

            for (size_t n : opt.sizes) {
                std::vector<size_t> todo;
                for (size_t a : active)
                    if (!exhausted[a]) todo.push_back(a);
                if (todo.empty()) break;
                if (suite.applicable && !suite.applicable(n, dist, param)) continue;

                Input base = suite.generate(n, dist, param, opt.seed ^ (uint64_t)n);
                if (suite.describe)
                    std::fprintf(stderr, "[cell] pair=%s dist=%s param=%s n=%zu %s\n",
                                 suite.pair.c_str(), dist.c_str(), param.c_str(), n,
                                 suite.describe(base).c_str());
                std::vector<Input> last(A);  // last result per algo for cross-check
                std::vector<bool> have_last(A, false);

                // Batching: tiny inputs run in a few nanoseconds, far below
                // the ~20 ns resolution of the clock, so each measurement
                // times K iterations of (copy input; run) and reports the
                // per-iteration mean. K is chosen from one untimed calibration
                // run so that a measurement lasts at least opt.min_time_ns.
                //
                // K == 1 (the common case above ~1e4 elements): the copy is
                // made *outside* the timed region and only the run is timed;
                // copy_ns is emitted as 0.
                //
                // K > 1: in-place algorithms need a fresh copy per iteration,
                // so the copy is inside the timed region; the cost of K copies
                // alone is measured first with the same, already-allocated
                // working buffer and emitted as copy_ns for the analysis to
                // subtract. (Allocating inside the copy loop would charge the
                // page faults of a fresh buffer to the baseline but not to the
                // timed loop, over-subtracting for large inputs.)
                std::vector<size_t> iters(A, 1);
                for (size_t a : todo) {
                    Input work = base;
                    suite.algos[a].run(work, param);  // cold run, discarded
                    do_not_optimize(work);
                    work = base;
                    uint64_t t0 = now_ns();
                    suite.algos[a].run(work, param);
                    uint64_t est = now_ns() - t0;
                    do_not_optimize(work);
                    if (est < 1) est = 1;
                    size_t K = (size_t)((opt.min_time_ns + est - 1) / est);
                    iters[a] = K < 1 ? 1 : (K > 10000000 ? 10000000 : K);
                }

                for (int r = 0; r < total_runs; ++r) {
                    for (size_t k = 0; k < todo.size(); ++k) {
                        size_t a = todo[(k + (size_t)r) % todo.size()];
                        if (exhausted[a]) continue;
                        const size_t K = iters[a];
                        Input work = base;  // allocates and fills the working buffer (untimed)
                        do_not_optimize(work);

                        double ns, copy_ns;
                        uint64_t t0, t1;
                        if (K == 1) {
                            clobber_memory();
                            pc.start();
                            t0 = now_ns();
                            suite.algos[a].run(work, param);
                            t1 = now_ns();
                            pc.stop();
                            do_not_optimize(work);
                            ns = (double)(t1 - t0);
                            copy_ns = 0.0;
                        } else {
                            // copy-only baseline on the already-allocated buffer
                            clobber_memory();
                            uint64_t c0 = now_ns();
                            for (size_t it = 0; it < K; ++it) {
                                work = base;
                                do_not_optimize(work);
                            }
                            uint64_t c1 = now_ns();

                            // copy + run
                            clobber_memory();
                            pc.start();
                            t0 = now_ns();
                            for (size_t it = 0; it < K; ++it) {
                                work = base;
                                suite.algos[a].run(work, param);
                                do_not_optimize(work);
                            }
                            t1 = now_ns();
                            pc.stop();
                            ns = (double)(t1 - t0) / (double)K;
                            copy_ns = (double)(c1 - c0) / (double)K;
                        }
                        if (r >= opt.warmup) {
                            std::vector<int64_t> ctr = pc.values();
                            for (auto& v : ctr)
                                if (v >= 0) v = (int64_t)((double)v / (double)K);
                            Row row{suite.pair, suite.algos[a].name, dist,
                                    suite.algos[a].param_sensitive ? param_label : "-",
                                    n, r - opt.warmup, ns, copy_ns, K, ctr};
                            write_csv_row(out, row);
                        }
                        if ((double)(t1 - t0) * 1e-9 > opt.max_seconds) {
                            exhausted[a] = true;
                            std::fprintf(stderr,
                                         "[info] %s/%s dist=%s param=%s n=%zu took %.2fs > budget; "
                                         "skipping larger n\n",
                                         suite.pair.c_str(), suite.algos[a].name.c_str(),
                                         dist.c_str(), param.c_str(), n, (double)(t1 - t0) * 1e-9);
                        }
                        if (r == total_runs - 1) {
                            last[a] = std::move(work);
                            have_last[a] = true;
                        }
                    }
                    std::fflush(out);
                }

                if (suite.same_result) {
                    size_t ref = A;
                    for (size_t a = 0; a < A; ++a)
                        if (have_last[a]) { ref = a; break; }
                    for (size_t a = 0; a < A; ++a) {
                        if (a == ref || !have_last[a]) continue;
                        if (!suite.same_result(last[ref], last[a])) {
                            std::fprintf(stderr,
                                         "[ERROR] result mismatch between %s and %s at n=%zu "
                                         "dist=%s param=%s\n",
                                         suite.algos[ref].name.c_str(),
                                         suite.algos[a].name.c_str(), n, dist.c_str(),
                                         param.c_str());
                            if (out != stdout) std::fclose(out);
                            return 3;
                        }
                    }
                }
                if (opt.verbose)
                    std::fprintf(stderr, "[progress] %s dist=%s param=%s n=%zu done\n",
                                 suite.pair.c_str(), dist.c_str(), param.c_str(), n);
            }
        }
    }
    if (out != stdout) std::fclose(out);
    return 0;
}

}  // namespace harness
