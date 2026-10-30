// (1) FibHeap and BinaryHeap are exercised directly against a brute-force
//     "scan for minimum" oracle under random push / decrease-key / pop.
// (2) All three Dijkstra variants must agree with Bellman-Ford on random
//     graphs, including graphs with unreachable vertices.
#include <cstdio>
#include <vector>

#include "../src/dijkstra/dijkstra.hpp"
#include "../src/harness/rng.hpp"

using namespace dijkstra;

template <class Heap>
static int heap_test(const char* name, harness::Rng& rng) {
    int failures = 0;
    for (int trial = 0; trial < 200; ++trial) {
        size_t n = 1 + rng.below(300);
        Heap heap(n);
        std::vector<Dist> key(n, INF);
        std::vector<uint8_t> in(n, 0);
        for (int op = 0; op < 2000; ++op) {
            int kind = (int)rng.below(3);
            if (kind == 0) {  // push
                Vertex v = (Vertex)rng.below(n);
                if (in[v]) continue;
                key[v] = rng.below(1000000);
                in[v] = 1;
                heap.push(v, key[v]);
            } else if (kind == 1) {  // decrease-key
                Vertex v = (Vertex)rng.below(n);
                if (!in[v] || key[v] == 0) continue;
                key[v] -= 1 + rng.below(key[v]);
                heap.decrease_key(v, key[v]);
            } else {  // pop
                if (heap.empty()) continue;
                Dist best = INF;
                for (size_t v = 0; v < n; ++v)
                    if (in[v] && key[v] < best) best = key[v];
                auto [k, v] = heap.pop();
                if (k != best || !in[v] || key[v] != k) {
                    ++failures;
                    std::printf("FAIL %s pop: got (%llu,%u) expected key %llu\n", name,
                                (unsigned long long)k, v, (unsigned long long)best);
                    break;
                }
                in[v] = 0;
            }
        }
    }
    return failures;
}

static std::vector<Dist> bellman_ford(const Graph& g, Vertex s) {
    std::vector<Dist> d(g.n, INF);
    d[s] = 0;
    for (size_t it = 0; it < g.n; ++it) {
        bool changed = false;
        for (size_t u = 0; u < g.n; ++u) {
            if (d[u] == INF) continue;
            for (uint64_t e = g.offsets[u]; e < g.offsets[u + 1]; ++e) {
                Dist nd = d[u] + g.w[e];
                if (nd < d[g.adj[e]]) { d[g.adj[e]] = nd; changed = true; }
            }
        }
        if (!changed) break;
    }
    return d;
}

int main() {
    harness::Rng rng(4);
    int failures = 0, cases = 0;
    failures += heap_test<BinaryHeap>("BinaryHeap", rng);
    failures += heap_test<FibHeap>("FibHeap", rng);
    cases += 400;

    for (int trial = 0; trial < 300; ++trial) {
        size_t n = 1 + rng.below(120);
        size_t m = rng.below(n * 6);
        std::vector<std::vector<std::pair<Vertex, Weight>>> adj(n);
        for (size_t e = 0; e < m; ++e) {
            Vertex u = (Vertex)rng.below(n), v = (Vertex)rng.below(n);
            adj[u].push_back({v, (Weight)(1 + rng.below(50))});
        }
        Graph g;
        g.n = n;
        g.offsets.resize(n + 1);
        g.offsets[0] = 0;
        for (size_t u = 0; u < n; ++u) g.offsets[u + 1] = g.offsets[u] + adj[u].size();
        for (size_t u = 0; u < n; ++u)
            for (auto& [v, w] : adj[u]) { g.adj.push_back(v); g.w.push_back(w); }

        auto ref = bellman_ford(g, 0);
        std::vector<Dist> d1, d2, d3;
        dijkstra_binary(g, 0, d1);
        dijkstra_lazy(g, 0, d2);
        dijkstra_fib(g, 0, d3);
        cases += 3;
        if (d1 != ref) { ++failures; std::printf("FAIL binary_heap n=%zu m=%zu\n", n, m); }
        if (d2 != ref) { ++failures; std::printf("FAIL lazy_heap n=%zu m=%zu\n", n, m); }
        if (d3 != ref) { ++failures; std::printf("FAIL fib_heap n=%zu m=%zu\n", n, m); }
    }
    std::printf("dijkstra: %d/%d cases passed\n", cases - failures, cases);
    return failures ? 1 : 0;
}
