// BFS, Dijkstra and A* for any graph type that provides
//   num_nodes() and for_each_neighbor(u, f(v, w)).
//
// Dijkstra and A* share ONE engine (best_first): Dijkstra is exactly A* with the
// zero heuristic. This is deliberate -- the only difference between the
// algorithms is then the heuristic, which is the thing the experiments vary.
// (tests/ verifies the engine against an independent std::set-based Dijkstra.)
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "pf/common.hpp"
#include "pf/graph.hpp"

namespace pf {

struct SearchStats {
    bool found = false;
    double cost = kInf;              // path cost (BFS: number of edges)
    std::uint64_t expanded = 0;      // nodes popped and expanded (re-expansions count again)
    std::uint64_t generated = 0;     // open-list insertions / decrease-keys
    std::uint64_t stale_pops = 0;    // popped entries skipped because a better one existed
    std::size_t peak_open = 0;       // max entries simultaneously in the open list
    std::size_t container_bytes = 0; // bytes owned by the open list / queue at its peak
    std::size_t reached = 0;         // nodes with finite distance when the search stopped
};

struct SearchResult {
    SearchStats stats;
    std::vector<double> dist;    // best known cost from the source (kInf = never reached)
    std::vector<NodeId> parent;  // predecessor on the best known path
};

inline std::vector<NodeId> extract_path(const SearchResult& r, NodeId source, NodeId target) {
    std::vector<NodeId> path;
    if (!r.stats.found) return path;
    for (NodeId v = target; v != kNoNode; v = r.parent[v]) {
        path.push_back(v);
        if (v == source) break;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

// ------------------------------------------------------------------ BFS
// Unweighted: ignores edge weights and counts edges. Stops when `target` is
// DEQUEUED (same stopping rule as Dijkstra/A*, so expansion counts are comparable).
// target == kNoNode explores the whole component.
template <class Graph>
SearchResult bfs(const Graph& g, NodeId source, NodeId target = kNoNode) {
    const std::size_t n = g.num_nodes();
    SearchResult r;
    r.dist.assign(n, kInf);
    r.parent.assign(n, kNoNode);
    std::vector<NodeId> queue;  // grows geometrically, like the heaps (no up-front reserve)
    r.dist[source] = 0;
    queue.push_back(source);
    r.stats.generated = 1;
    r.stats.peak_open = 1;
    for (std::size_t head = 0; head < queue.size(); ++head) {
        const NodeId u = queue[head];
        ++r.stats.expanded;
        if (u == target) {
            r.stats.found = true;
            r.stats.cost = r.dist[u];
            break;
        }
        g.for_each_neighbor(u, [&](NodeId v, double) {
            if (r.dist[v] == kInf) {
                r.dist[v] = r.dist[u] + 1;
                r.parent[v] = u;
                queue.push_back(v);
                ++r.stats.generated;
            }
        });
        r.stats.peak_open = std::max(r.stats.peak_open, queue.size() - head - 1);
    }
    r.stats.reached = queue.size();  // every discovered node was enqueued once
    r.stats.container_bytes = queue.capacity() * sizeof(NodeId);
    return r;
}

// ------------------------------------------------------------------ best-first engine
// Heap : one of the open lists in heaps.hpp
// H    : callable  double h(NodeId)  -- estimate of the remaining cost to `target`
// target == kNoNode -> single-source shortest paths (h must then be zero).
template <class Heap, class Graph, class H>
SearchResult best_first(const Graph& g, NodeId source, NodeId target, H&& h) {
    const std::size_t n = g.num_nodes();
    SearchResult r;
    r.dist.assign(n, kInf);
    r.parent.assign(n, kNoNode);
    Heap open(n);

    r.dist[source] = 0;
    open.push_or_update({h(source), 0.0, source});
    r.stats.generated = 1;

    while (!open.empty()) {
        const Item it = open.pop();
        if (it.g > r.dist[it.node]) {  // a cheaper path to this node was found after this entry was pushed
            ++r.stats.stale_pops;
            continue;
        }
        ++r.stats.expanded;
        if (it.node == target) {
            r.stats.found = true;
            r.stats.cost = it.g;
            break;
        }
        g.for_each_neighbor(it.node, [&](NodeId v, double w) {
            const double ng = it.g + w;
            if (ng < r.dist[v]) {
                r.dist[v] = ng;
                r.parent[v] = it.node;
                open.push_or_update({ng + h(v), ng, v});
                ++r.stats.generated;
            }
        });
    }
    r.stats.peak_open = open.peak_size();
    r.stats.container_bytes = open.peak_bytes();
    if (target == kNoNode)
        for (double d : r.dist) r.stats.reached += (d != kInf);
    return r;
}

struct ZeroHeuristic {
    double operator()(NodeId) const { return 0.0; }
};

template <class Heap, class Graph>
SearchResult dijkstra(const Graph& g, NodeId source, NodeId target = kNoNode) {
    return best_first<Heap>(g, source, target, ZeroHeuristic{});
}

// ------------------------------------------------------------------ grid heuristics
enum class Heuristic { Zero, Manhattan, Euclidean, Octile };

inline const char* to_string(Heuristic h) {
    switch (h) {
        case Heuristic::Zero: return "zero";
        case Heuristic::Manhattan: return "manhattan";
        case Heuristic::Euclidean: return "euclidean";
        case Heuristic::Octile: return "octile";
    }
    return "?";
}

// Admissible = never overestimates the true remaining cost.
//   4-connected, unit cost : zero, Manhattan, Euclidean, Octile are all admissible
//                            (Manhattan is exact on an empty grid, Euclidean is weaker)
//   8-connected, diag=sqrt2: Manhattan OVERESTIMATES (diagonal step covers dx=dy=1 at cost 1.41 < 2)
//                            -> A* may return suboptimal paths; Octile is exact on an empty grid
inline bool is_admissible(Heuristic h, Connectivity c) {
    return !(h == Heuristic::Manhattan && c == Connectivity::Eight);
}

struct GridHeuristic {
    Heuristic kind;
    int width;
    int tx, ty;
    double operator()(NodeId u) const {
        const double dx = std::abs(static_cast<int>(u % static_cast<NodeId>(width)) - tx);
        const double dy = std::abs(static_cast<int>(u / static_cast<NodeId>(width)) - ty);
        switch (kind) {
            case Heuristic::Zero: return 0.0;
            case Heuristic::Manhattan: return dx + dy;
            case Heuristic::Euclidean: return std::sqrt(dx * dx + dy * dy);
            case Heuristic::Octile: return (dx + dy) + (kSqrt2 - 2.0) * std::min(dx, dy);
        }
        return 0.0;
    }
};

template <class Heap>
SearchResult astar_grid(const GridGraph& g, NodeId source, NodeId target, Heuristic kind) {
    return best_first<Heap>(g, source, target,
                            GridHeuristic{kind, g.width(), g.x_of(target), g.y_of(target)});
}

}  // namespace pf
