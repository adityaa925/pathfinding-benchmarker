// Deterministic instance generators.
//
// Rng is a hand-written splitmix64 on purpose: std::uniform_*_distribution is
// implementation-defined, so the same seed would give different instances on
// MSVC / MinGW / Linux. With splitmix64 the benchmark instances are bit-identical
// everywhere, which makes results reproducible and comparable.
#pragma once
#include <cstdint>
#include <utility>
#include <vector>

#include "pf/common.hpp"
#include "pf/graph.hpp"

namespace pf {

class Rng {
public:
    explicit Rng(std::uint64_t seed) : s_(seed) {}
    std::uint64_t next() {
        std::uint64_t z = (s_ += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    // uniform in [0, bound); modulo bias is ~bound/2^64, irrelevant here
    // (no unsigned __int128 so this also builds with MSVC)
    std::uint64_t below(std::uint64_t bound) { return next() % bound; }
    double real01() { return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0); }

private:
    std::uint64_t s_;
};

// Mixes several integers into one reproducible seed.
inline std::uint64_t derive_seed(std::uint64_t base, std::uint64_t a, std::uint64_t b = 0,
                                 std::uint64_t c = 0) {
    Rng r(base);
    r = Rng(r.next() ^ (a * 0x100000001B3ull));
    r = Rng(r.next() ^ (b * 0x100000001B3ull));
    r = Rng(r.next() ^ (c * 0x100000001B3ull));
    return r.next();
}

// ------------------------------------------------------------------ grids
struct GridInstance {
    GridGraph grid;
    NodeId start = kNoNode;
    NodeId goal = kNoNode;
    std::size_t free_cells = 0;
    std::size_t component_size = 0;  // size of the component containing start and goal
    double density = 0.0;            // requested obstacle probability
};

// Every cell is blocked independently with probability `density`.
// Start/goal are then chosen INSIDE the largest connected component, as the
// cells with the smallest / largest (x + y): near opposite corners, but
// guaranteed to be connected. (A square lattice percolates at ~59.3% open cells,
// so at 40% obstacles a random corner pair is often disconnected -- picking the
// endpoints inside the giant component avoids discarding instances.)
inline GridInstance make_random_grid(int w, int h, double density, Connectivity conn,
                                     std::uint64_t seed) {
    const std::size_t n = static_cast<std::size_t>(w) * h;
    Rng rng(seed);
    std::vector<std::uint8_t> blocked(n);
    for (auto& b : blocked) b = rng.real01() < density ? 1 : 0;

    GridInstance inst{GridGraph(w, h, conn, std::move(blocked)), kNoNode, kNoNode, 0, 0, density};
    const GridGraph& g = inst.grid;
    inst.free_cells = g.free_cells();

    // label connected components with an iterative BFS
    std::vector<std::int32_t> comp(n, -1);
    std::vector<NodeId> queue;
    std::int32_t best_id = -1;
    std::size_t best_size = 0;
    std::int32_t next_id = 0;
    for (NodeId s = 0; s < n; ++s) {
        if (g.is_blocked(s) || comp[s] != -1) continue;
        queue.clear();
        queue.push_back(s);
        comp[s] = next_id;
        for (std::size_t head = 0; head < queue.size(); ++head) {
            g.for_each_neighbor(queue[head], [&](NodeId v, double) {
                if (comp[v] == -1) {
                    comp[v] = next_id;
                    queue.push_back(v);
                }
            });
        }
        if (queue.size() > best_size) {
            best_size = queue.size();
            best_id = next_id;
        }
        ++next_id;
    }
    inst.component_size = best_size;
    if (best_id < 0) return inst;  // everything blocked

    int min_sum = w + h, max_sum = -1;
    for (NodeId u = 0; u < n; ++u) {
        if (comp[u] != best_id) continue;
        const int sum = g.x_of(u) + g.y_of(u);
        if (sum < min_sum) { min_sum = sum; inst.start = u; }
        if (sum >= max_sum) { max_sum = sum; inst.goal = u; }  // >= : last cell with the max sum
    }
    return inst;
}

// ------------------------------------------------------------------ sparse random graphs
// Undirected, connected, n nodes, about n * avg_degree / 2 edges, integer weights
// in [1, max_weight]. Connectivity is guaranteed by first building a random
// spanning tree (node i is attached to a random node j < i); the remaining edges
// are uniform random pairs (duplicates / parallel edges are allowed, self-loops are not).
inline CSRGraph make_random_sparse_graph(std::size_t n, double avg_degree, std::uint32_t max_weight,
                                         std::uint64_t seed) {
    Rng rng(seed);
    std::vector<Edge> edges;
    const std::size_t target = static_cast<std::size_t>(static_cast<double>(n) * avg_degree / 2.0);
    edges.reserve(target > n ? target : n);
    auto weight = [&] { return static_cast<std::uint32_t>(1 + rng.below(max_weight)); };
    for (std::size_t i = 1; i < n; ++i)
        edges.push_back({static_cast<NodeId>(rng.below(i)), static_cast<NodeId>(i), weight()});
    while (edges.size() < target) {
        NodeId a = static_cast<NodeId>(rng.below(n)), b = static_cast<NodeId>(rng.below(n));
        if (a != b) edges.push_back({a, b, weight()});
    }
    return CSRGraph::from_edges(n, edges, /*symmetrize=*/true);
}

}  // namespace pf
