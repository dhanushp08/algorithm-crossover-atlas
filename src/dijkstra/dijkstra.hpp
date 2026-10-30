// Shortest-paths pair: Dijkstra with a Fibonacci heap O(m + n log n) vs
// Dijkstra with a binary heap O((n + m) log n).
//
// Three priority-queue variants are provided:
//   binary_heap   indexed binary heap with true decrease-key (the textbook
//                 O((n+m) log n) algorithm)
//   lazy_heap     std::priority_queue with lazy deletion (what practitioners
//                 actually write; same bound but no decrease-key support,
//                 so stale entries are popped and skipped)
//   fib_heap      Fibonacci heap with O(1) amortised decrease-key
//
// Graphs are stored in CSR form. Vertices are inserted into the heap when
// first discovered rather than all up front, for every variant alike.
#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

namespace dijkstra {

using Vertex = uint32_t;
using Weight = uint32_t;
using Dist = uint64_t;
constexpr Dist INF = std::numeric_limits<Dist>::max();

struct Graph {
    size_t n = 0;
    std::vector<uint64_t> offsets;  // n+1
    std::vector<Vertex> adj;        // m
    std::vector<Weight> w;          // m
    size_t m() const { return adj.size(); }
};

struct Stats {
    uint64_t pops = 0, pushes = 0, decrease_keys = 0;
};

// ---------------------------------------------------------------------------
// Indexed binary min-heap with decrease-key
// ---------------------------------------------------------------------------
class BinaryHeap {
public:
    explicit BinaryHeap(size_t n) : pos_(n, NONE) { heap_.reserve(n); }
    bool empty() const { return heap_.empty(); }
    bool contains(Vertex v) const { return pos_[v] != NONE; }

    void push(Vertex v, Dist k) {
        heap_.push_back({k, v});
        pos_[v] = heap_.size() - 1;
        sift_up(heap_.size() - 1);
    }
    void decrease_key(Vertex v, Dist k) {
        size_t i = pos_[v];
        heap_[i].first = k;
        sift_up(i);
    }
    std::pair<Dist, Vertex> pop() {
        auto top = heap_[0];
        pos_[top.second] = NONE;
        auto last = heap_.back();
        heap_.pop_back();
        if (!heap_.empty()) {
            heap_[0] = last;
            pos_[last.second] = 0;
            sift_down(0);
        }
        return top;
    }

private:
    static constexpr size_t NONE = ~(size_t)0;
    std::vector<std::pair<Dist, Vertex>> heap_;
    std::vector<size_t> pos_;

    void sift_up(size_t i) {
        auto item = heap_[i];
        while (i > 0) {
            size_t p = (i - 1) / 2;
            if (heap_[p].first <= item.first) break;
            heap_[i] = heap_[p];
            pos_[heap_[i].second] = i;
            i = p;
        }
        heap_[i] = item;
        pos_[item.second] = i;
    }
    void sift_down(size_t i) {
        const size_t n = heap_.size();
        auto item = heap_[i];
        while (true) {
            size_t l = 2 * i + 1, r = l + 1, best = i;
            Dist bk = item.first;
            if (l < n && heap_[l].first < bk) { best = l; bk = heap_[l].first; }
            if (r < n && heap_[r].first < bk) { best = r; }
            if (best == i) break;
            heap_[i] = heap_[best];
            pos_[heap_[i].second] = i;
            i = best;
        }
        heap_[i] = item;
        pos_[item.second] = i;
    }
};

// ---------------------------------------------------------------------------
// Fibonacci heap (Fredman & Tarjan 1987), nodes pre-allocated per vertex
// ---------------------------------------------------------------------------
class FibHeap {
public:
    explicit FibHeap(size_t n) : nodes_(n), in_heap_(n, 0) {
        for (size_t i = 0; i < n; ++i) nodes_[i].v = (Vertex)i;
        // Max degree is O(log_phi n); 64 is safe for any n < 2^64.
        degree_table_.assign(64, NIL);
    }
    bool empty() const { return min_ == NIL; }
    bool contains(Vertex v) const { return in_heap_[v] != 0; }

    void push(Vertex v, Dist k) {
        Node& x = nodes_[v];
        x.key = k;
        x.degree = 0;
        x.mark = false;
        x.parent = NIL;
        x.child = NIL;
        in_heap_[v] = 1;
        add_to_roots(v);
        ++count_;
    }

    std::pair<Dist, Vertex> pop() {
        uint32_t z = min_;
        Node& zn = nodes_[z];
        // promote children to roots
        if (zn.child != NIL) {
            uint32_t c = zn.child;
            do {
                uint32_t next = nodes_[c].right;
                nodes_[c].parent = NIL;
                nodes_[c].mark = false;
                add_to_roots(c);
                c = next;
            } while (c != zn.child);
        }
        // remove z from root list
        remove_from_list(z);
        --count_;
        in_heap_[z] = 0;
        if (z == zn.right) {
            min_ = NIL;
        } else {
            min_ = zn.right;
            consolidate();
        }
        return {zn.key, (Vertex)z};
    }

    void decrease_key(Vertex v, Dist k) {
        Node& x = nodes_[v];
        x.key = k;
        uint32_t p = x.parent;
        if (p != NIL && nodes_[p].key > k) {
            cut(v, p);
            cascading_cut(p);
        }
        if (k < nodes_[min_].key) min_ = v;
    }

private:
    static constexpr uint32_t NIL = 0xFFFFFFFFu;
    struct Node {
        Dist key = 0;
        uint32_t parent = NIL, child = NIL, left = NIL, right = NIL;
        Vertex v = 0;
        uint32_t degree = 0;
        bool mark = false;
    };
    std::vector<Node> nodes_;
    std::vector<uint8_t> in_heap_;
    std::vector<uint32_t> degree_table_;
    uint32_t min_ = NIL;
    size_t count_ = 0;

    void add_to_roots(uint32_t x) {
        if (min_ == NIL) {
            nodes_[x].left = nodes_[x].right = x;
            min_ = x;
        } else {
            insert_after(min_, x);
            if (nodes_[x].key < nodes_[min_].key) min_ = x;
        }
    }
    void insert_after(uint32_t a, uint32_t x) {
        uint32_t b = nodes_[a].right;
        nodes_[x].left = a;
        nodes_[x].right = b;
        nodes_[a].right = x;
        nodes_[b].left = x;
    }
    void remove_from_list(uint32_t x) {
        uint32_t l = nodes_[x].left, r = nodes_[x].right;
        nodes_[l].right = r;
        nodes_[r].left = l;
    }
    void link(uint32_t y, uint32_t x) {  // make y a child of x
        remove_from_list(y);
        nodes_[y].parent = x;
        nodes_[y].mark = false;
        if (nodes_[x].child == NIL) {
            nodes_[x].child = y;
            nodes_[y].left = nodes_[y].right = y;
        } else {
            insert_after(nodes_[x].child, y);
        }
        nodes_[x].degree++;
    }
    void consolidate() {
        // Collect roots first, because linking mutates the circular list.
        std::vector<uint32_t>& roots = roots_scratch_;
        roots.clear();
        uint32_t r = min_;
        do {
            roots.push_back(r);
            r = nodes_[r].right;
        } while (r != min_);

        for (uint32_t x : roots) {
            uint32_t d = nodes_[x].degree;
            while (degree_table_[d] != NIL) {
                uint32_t y = degree_table_[d];
                if (nodes_[y].key < nodes_[x].key) std::swap(x, y);
                link(y, x);
                degree_table_[d] = NIL;
                ++d;
            }
            degree_table_[d] = x;
        }
        min_ = NIL;
        for (uint32_t& slot : degree_table_) {
            if (slot == NIL) continue;
            uint32_t x = slot;
            slot = NIL;
            if (min_ == NIL) {
                nodes_[x].left = nodes_[x].right = x;
                min_ = x;
            } else {
                insert_after(min_, x);
                if (nodes_[x].key < nodes_[min_].key) min_ = x;
            }
        }
    }
    void cut(uint32_t x, uint32_t p) {
        if (nodes_[x].right == x)
            nodes_[p].child = NIL;
        else {
            remove_from_list(x);
            if (nodes_[p].child == x) nodes_[p].child = nodes_[x].right;
        }
        nodes_[p].degree--;
        nodes_[x].parent = NIL;
        nodes_[x].mark = false;
        insert_after(min_, x);
    }
    void cascading_cut(uint32_t y) {
        uint32_t p = nodes_[y].parent;
        while (p != NIL) {
            if (!nodes_[y].mark) {
                nodes_[y].mark = true;
                return;
            }
            cut(y, p);
            y = p;
            p = nodes_[y].parent;
        }
    }
    std::vector<uint32_t> roots_scratch_;
};

// ---------------------------------------------------------------------------
// Dijkstra drivers
// ---------------------------------------------------------------------------
template <class Heap>
inline Stats dijkstra_with_decrease_key(const Graph& g, Vertex s, std::vector<Dist>& dist) {
    Stats st;
    dist.assign(g.n, INF);
    std::vector<uint8_t> done(g.n, 0);
    Heap heap(g.n);
    dist[s] = 0;
    heap.push(s, 0);
    ++st.pushes;
    while (!heap.empty()) {
        auto [d, u] = heap.pop();
        ++st.pops;
        done[u] = 1;
        for (uint64_t e = g.offsets[u]; e < g.offsets[u + 1]; ++e) {
            Vertex v = g.adj[e];
            if (done[v]) continue;
            Dist nd = d + g.w[e];
            if (nd < dist[v]) {
                if (dist[v] == INF) {
                    dist[v] = nd;
                    heap.push(v, nd);
                    ++st.pushes;
                } else {
                    dist[v] = nd;
                    heap.decrease_key(v, nd);
                    ++st.decrease_keys;
                }
            }
        }
    }
    return st;
}

inline Stats dijkstra_binary(const Graph& g, Vertex s, std::vector<Dist>& dist) {
    return dijkstra_with_decrease_key<BinaryHeap>(g, s, dist);
}
inline Stats dijkstra_fib(const Graph& g, Vertex s, std::vector<Dist>& dist) {
    return dijkstra_with_decrease_key<FibHeap>(g, s, dist);
}

inline Stats dijkstra_lazy(const Graph& g, Vertex s, std::vector<Dist>& dist) {
    Stats st;
    dist.assign(g.n, INF);
    using Item = std::pair<Dist, Vertex>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> pq;
    dist[s] = 0;
    pq.push({0, s});
    ++st.pushes;
    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        ++st.pops;
        if (d > dist[u]) continue;  // stale
        for (uint64_t e = g.offsets[u]; e < g.offsets[u + 1]; ++e) {
            Vertex v = g.adj[e];
            Dist nd = d + g.w[e];
            if (nd < dist[v]) {
                if (dist[v] != INF) ++st.decrease_keys;  // would have been a decrease-key
                dist[v] = nd;
                pq.push({nd, v});
                ++st.pushes;
            }
        }
    }
    return st;
}

}  // namespace dijkstra
