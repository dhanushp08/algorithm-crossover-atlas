#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

#include "../harness/bench.hpp"
#include "../harness/rng.hpp"
#include "dijkstra.hpp"

using harness::Rng;
using namespace dijkstra;

struct Input {
    Graph g;
    Vertex source = 0;
    std::vector<Dist> dist;
    Stats stats;
};

// Build CSR from an edge list given as per-vertex adjacency.
static Graph build_csr(size_t n, const std::vector<std::vector<std::pair<Vertex, Weight>>>& adj) {
    Graph g;
    g.n = n;
    g.offsets.resize(n + 1);
    g.offsets[0] = 0;
    for (size_t u = 0; u < n; ++u) g.offsets[u + 1] = g.offsets[u] + adj[u].size();
    g.adj.resize(g.offsets[n]);
    g.w.resize(g.offsets[n]);
    for (size_t u = 0; u < n; ++u) {
        uint64_t o = g.offsets[u];
        for (auto& [v, w] : adj[u]) {
            g.adj[o] = v;
            g.w[o] = w;
            ++o;
        }
    }
    return g;
}

// Distributions (graph structure):
//   random   directed, every vertex has `param` random out-edges (density axis)
//   grid     2-D grid, 4-neighbour, bidirectional (road-network-like, long paths)
//   dense    complete directed graph, m = n(n-1) (param ignored; n is capped)
// Weights are uniform integers in [1, 1000].
static Input generate(size_t n, const std::string& dist, const std::string& param, uint64_t seed) {
    Rng rng(seed);
    Input in;
    std::vector<std::vector<std::pair<Vertex, Weight>>> adj(n);
    auto weight = [&]() -> Weight { return (Weight)(1 + rng.below(1000)); };

    if (dist == "random") {
        size_t d = (size_t)std::atoi(param.c_str());
        if (d == 0) d = 4;
        if (d >= n) d = n - 1;
        for (size_t u = 0; u < n; ++u) {
            adj[u].reserve(d);
            for (size_t k = 0; k < d; ++k) {
                Vertex v = (Vertex)rng.below(n);
                if (v == u) v = (Vertex)((u + 1) % n);
                adj[u].push_back({v, weight()});
            }
        }
    } else if (dist == "grid") {
        size_t side = (size_t)std::llround(std::sqrt((double)n));
        if (side < 2) side = 2;
        n = side * side;
        adj.assign(n, {});
        for (size_t r = 0; r < side; ++r)
            for (size_t c = 0; c < side; ++c) {
                Vertex u = (Vertex)(r * side + c);
                if (c + 1 < side) {
                    Weight w = weight();
                    adj[u].push_back({u + 1, w});
                    adj[u + 1].push_back({u, w});
                }
                if (r + 1 < side) {
                    Weight w = weight();
                    adj[u].push_back({(Vertex)(u + side), w});
                    adj[u + side].push_back({u, w});
                }
            }
    } else if (dist == "dense") {
        for (size_t u = 0; u < n; ++u) {
            adj[u].reserve(n - 1);
            for (size_t v = 0; v < n; ++v)
                if (v != u) adj[u].push_back({(Vertex)v, weight()});
        }
    } else {
        std::fprintf(stderr, "unknown distribution %s\n", dist.c_str());
        std::exit(2);
    }
    in.g = build_csr(n, adj);
    in.source = 0;
    return in;
}

int main(int argc, char** argv) {
    harness::Options opt;
    opt.params = {"4"};
    if (!harness::parse_options(argc, argv, opt, "random,grid,dense",
                                "average out-degree for 'random' graphs (default 4; try 2,4,16,64,256)"))
        return 2;
    if (opt.sizes.empty()) opt.sizes = harness::parse_sizes("100:10000000:2");

    harness::Suite<Input> suite;
    suite.pair = "dijkstra";
    suite.generate = generate;
    suite.algos.push_back({"binary_heap",
                           [](Input& in, const std::string&) {
                               in.stats = dijkstra_binary(in.g, in.source, in.dist);
                           },
                           true});
    suite.algos.push_back({"lazy_heap",
                           [](Input& in, const std::string&) {
                               in.stats = dijkstra_lazy(in.g, in.source, in.dist);
                           },
                           true});
    suite.algos.push_back({"fib_heap",
                           [](Input& in, const std::string&) {
                               in.stats = dijkstra_fib(in.g, in.source, in.dist);
                           },
                           true});
    suite.same_result = [](const Input& a, const Input& b) { return a.dist == b.dist; };
    suite.dist_uses_param = [](const std::string& dist) { return dist == "random"; };
    suite.applicable = [](size_t n, const std::string& dist, const std::string& param) {
        if (dist == "dense") return n <= 8192;  // 67M edges, ~540 MB
        if (dist == "random") {
            size_t d = (size_t)std::atoi(param.c_str());
            return n * (d ? d : 4) <= 400000000ULL;  // cap edge count at 4e8
        }
        return true;
    };
    suite.describe = [](const Input& in) {
        // Run the binary-heap version once (untimed) to report operation counts.
        std::vector<Dist> d;
        Stats st = dijkstra_binary(in.g, in.source, d);
        char buf[160];
        std::snprintf(buf, sizeof(buf), "vertices=%zu edges=%zu pops=%llu pushes=%llu decrease_keys=%llu",
                      in.g.n, in.g.m(), (unsigned long long)st.pops, (unsigned long long)st.pushes,
                      (unsigned long long)st.decrease_keys);
        return std::string(buf);
    };
    return harness::run_suite(suite, opt);
}
