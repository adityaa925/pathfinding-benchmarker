// Shared basic types for the pathfinding benchmarker.
#pragma once
#include <cstdint>
#include <limits>

namespace pf {

using NodeId = std::uint32_t;
inline constexpr NodeId kNoNode = std::numeric_limits<NodeId>::max();
inline constexpr double kInf = std::numeric_limits<double>::infinity();
inline constexpr double kSqrt2 = 1.4142135623730950488;

// One entry of the open list.
//   f    = g + h(node)  (for Dijkstra h == 0, so f == g)
//   g    = cost of the best known path to `node` when the entry was created
struct Item {
    double f;
    double g;
    NodeId node;
};

// Strict total order used by EVERY heap implementation:
//   1) smaller f first
//   2) on ties, larger g first (deeper nodes first -> fewer A* expansions)
//   3) on ties, smaller node id first
// Because the order is total, all heaps pop items in exactly the same
// sequence, so "nodes expanded" is identical across heaps and the heap
// comparison measures only data-structure cost, not search behaviour.
inline constexpr bool better(const Item& a, const Item& b) noexcept {
    if (a.f != b.f) return a.f < b.f;
    if (a.g != b.g) return a.g > b.g;
    return a.node < b.node;
}

}  // namespace pf
