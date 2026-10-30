// Minimal command-line parsing shared by all benchmark binaries.
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace harness {

struct Options {
    std::vector<size_t> sizes;
    std::vector<std::string> dists;
    std::vector<std::string> params{"-"};
    int reps = 5;
    int warmup = 1;
    int pin = -1;
    uint64_t seed = 20261029ULL;
    double max_seconds = 30.0;
    double min_time_ns = 2e6;  // batch tiny runs until a measurement lasts >= 2 ms
    std::string out = "-";
    bool verbose = false;
};

inline std::vector<std::string> split(const std::string& s, char sep = ',') {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        } else
            cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

// Accepts "10,100,1000" or a geometric range "10:1000000:2" (start:end:factor),
// or "10:1000000:x10" meaning 10 points per decade is NOT supported; keep it
// simple: factor may be fractional, e.g. 1.5.
inline std::vector<size_t> parse_sizes(const std::string& s) {
    std::vector<size_t> out;
    auto parts = split(s, ':');
    if (parts.size() == 3) {
        double start = std::atof(parts[0].c_str());
        double end = std::atof(parts[1].c_str());
        double factor = std::atof(parts[2].c_str());
        if (factor <= 1.0) factor = 2.0;
        size_t prev = 0;
        for (double x = start; x <= end * (1 + 1e-9); x *= factor) {
            size_t v = (size_t)std::llround(x);
            if (v != prev) out.push_back(v);
            prev = v;
        }
        return out;
    }
    for (const auto& p : split(s)) out.push_back((size_t)std::atof(p.c_str()));
    return out;
}

inline void usage(const char* prog, const char* default_dists, const char* param_help) {
    std::fprintf(stderr,
                 "usage: %s [options]\n"
                 "  --sizes LIST|start:end:factor   input sizes (default pair-specific)\n"
                 "  --dists LIST                    distributions (default: %s)\n"
                 "  --params LIST                   %s\n"
                 "  --reps N                        timed repetitions per cell (default 5)\n"
                 "  --warmup N                      untimed warmup runs per cell (default 1)\n"
                 "  --pin CORE                      pin to a CPU core (default: none)\n"
                 "  --seed S                        PRNG seed\n"
                 "  --max-seconds S                 skip larger n after a run exceeds S\n"
                 "  --min-time-ms T                 batch tiny inputs so a measurement lasts >= T ms (default 2)\n"
                 "  --out FILE                      CSV output (default stdout)\n"
                 "  --verbose                       progress on stderr\n",
                 prog, default_dists, param_help);
}

inline bool parse_options(int argc, char** argv, Options& o, const char* default_dists,
                          const char* param_help) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto need = [&](const char* flag) -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "missing value for %s\n", flag);
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "--sizes") o.sizes = parse_sizes(need("--sizes"));
        else if (a == "--dists") o.dists = split(need("--dists"));
        else if (a == "--params") o.params = split(need("--params"));
        else if (a == "--reps") o.reps = std::atoi(need("--reps"));
        else if (a == "--warmup") o.warmup = std::atoi(need("--warmup"));
        else if (a == "--pin") o.pin = std::atoi(need("--pin"));
        else if (a == "--seed") o.seed = std::strtoull(need("--seed"), nullptr, 10);
        else if (a == "--max-seconds") o.max_seconds = std::atof(need("--max-seconds"));
        else if (a == "--min-time-ms") o.min_time_ns = 1e6 * std::atof(need("--min-time-ms"));
        else if (a == "--out") o.out = need("--out");
        else if (a == "--verbose") o.verbose = true;
        else if (a == "--help" || a == "-h") {
            usage(argv[0], default_dists, param_help);
            return false;
        } else {
            std::fprintf(stderr, "unknown option %s\n", a.c_str());
            usage(argv[0], default_dists, param_help);
            return false;
        }
    }
    if (o.dists.empty()) o.dists = split(default_dists);
    if (o.params.empty()) o.params = {"-"};
    return true;
}

}  // namespace harness
