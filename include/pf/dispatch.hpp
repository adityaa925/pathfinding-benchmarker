// Runtime selection of the heap type (the heaps are compile-time template parameters).
#pragma once
#include <array>
#include <stdexcept>
#include <string>

#include "pf/algorithms.hpp"
#include "pf/heaps.hpp"

namespace pf {

enum class HeapKind { StdPQ, Binary, Quaternary, IndexedBinary };

inline constexpr std::array<HeapKind, 4> kAllHeaps = {HeapKind::StdPQ, HeapKind::Binary,
                                                      HeapKind::Quaternary, HeapKind::IndexedBinary};

inline const char* to_string(HeapKind k) {
    switch (k) {
        case HeapKind::StdPQ: return StdPriorityQueue::name();
        case HeapKind::Binary: return BinaryHeap::name();
        case HeapKind::Quaternary: return QuaternaryHeap::name();
        case HeapKind::IndexedBinary: return IndexedBinaryHeap::name();
    }
    return "?";
}

inline HeapKind parse_heap(const std::string& s) {
    for (HeapKind k : kAllHeaps)
        if (s == to_string(k)) return k;
    throw std::runtime_error("unknown heap '" + s + "' (std_pq|binary|quaternary|indexed_binary)");
}

template <class Graph>
SearchResult run_dijkstra(HeapKind k, const Graph& g, NodeId s, NodeId t = kNoNode) {
    switch (k) {
        case HeapKind::StdPQ: return dijkstra<StdPriorityQueue>(g, s, t);
        case HeapKind::Binary: return dijkstra<BinaryHeap>(g, s, t);
        case HeapKind::Quaternary: return dijkstra<QuaternaryHeap>(g, s, t);
        case HeapKind::IndexedBinary: return dijkstra<IndexedBinaryHeap>(g, s, t);
    }
    throw std::logic_error("bad HeapKind");
}

inline SearchResult run_astar_grid(HeapKind k, const GridGraph& g, NodeId s, NodeId t, Heuristic h) {
    switch (k) {
        case HeapKind::StdPQ: return astar_grid<StdPriorityQueue>(g, s, t, h);
        case HeapKind::Binary: return astar_grid<BinaryHeap>(g, s, t, h);
        case HeapKind::Quaternary: return astar_grid<QuaternaryHeap>(g, s, t, h);
        case HeapKind::IndexedBinary: return astar_grid<IndexedBinaryHeap>(g, s, t, h);
    }
    throw std::logic_error("bad HeapKind");
}

}  // namespace pf
