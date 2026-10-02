// Dependency-free test runner (no gtest needed). Exit code != 0 on any failure.
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

#include "pf/algorithms.hpp"
#include "pf/dimacs.hpp"
#include "pf/dispatch.hpp"
#include "pf/generators.hpp"

using namespace pf;

static int g_failed = 0, g_checks = 0;
#define CHECK(cond)                                                                      \
    do {                                                                                 \
        ++g_checks;                                                                      \
        if (!(cond)) {                                                                   \
            ++g_failed;                                                                  \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  " #cond << "\n";   \
        }                                                                                \
    } while (0)

static bool close(double a, double b) {
    if (a == b) return true;  // also handles inf == inf (unreachable nodes)
    return std::abs(a - b) <= 1e-9 * std::max(1.0, std::abs(b));
}

// Independent reference: textbook Dijkstra on std::set (no shared code with the engine).
template <class Graph>
std::vector<double> reference_sssp(const Graph& g, NodeId s) {
    std::vector<double> d(g.num_nodes(), kInf);
    std::set<std::pair<double, NodeId>> q;
    d[s] = 0;
    q.insert({0.0, s});
    while (!q.empty()) {
        auto [du, u] = *q.begin();
        q.erase(q.begin());
        g.for_each_neighbor(u, [&](NodeId v, double w) {
            if (du + w < d[v]) {
                q.erase({d[v], v});
                d[v] = du + w;
                q.insert({d[v], v});
            }
        });
    }
    return d;
}

// ------------------------------------------------------------------ tests
static void test_dynamic_array() {
    DynamicArray<int> a;
    for (int i = 0; i < 1000; ++i) a.push_back(i);
    CHECK(a.size() == 1000);
    CHECK(a[0] == 0 && a[999] == 999 && a.back() == 999);
    a.push_back(a[5]);  // aliasing push during possible growth
    CHECK(a.back() == 5);
    DynamicArray<int> b = a;
    CHECK(b.size() == a.size() && b[500] == 500);
    DynamicArray<int> c = std::move(b);
    CHECK(c.size() == 1001 && b.size() == 0);
    c.pop_back();
    CHECK(c.size() == 1000);
}

template <class Heap>
void heap_sorted_order(const char* label) {
    Rng rng(42);
    std::vector<Item> items;
    for (NodeId i = 0; i < 2000; ++i) {
        double g = static_cast<double>(rng.below(50));  // many ties on purpose
        items.push_back({g + static_cast<double>(rng.below(5)), g, i});
    }
    Heap h(items.size());
    for (const Item& it : items) h.push_or_update(it);
    CHECK(h.size() == items.size());
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return better(a, b); });
    bool ok = true;
    for (const Item& expect : items) {
        Item got = h.pop();
        ok = ok && got.node == expect.node && got.f == expect.f && got.g == expect.g;
    }
    if (!ok) std::cerr << "  heap order mismatch in " << label << "\n";
    CHECK(ok);
    CHECK(h.empty());
    CHECK(h.peak_size() == items.size());
    CHECK(h.peak_bytes() >= items.size() * sizeof(Item));
}

static void test_heaps() {
    heap_sorted_order<StdPriorityQueue>("std_pq");
    heap_sorted_order<BinaryHeap>("binary");
    heap_sorted_order<QuaternaryHeap>("quaternary");
    heap_sorted_order<IndexedBinaryHeap>("indexed_binary");

    // decrease-key semantics of the indexed heap
    IndexedBinaryHeap h(10);
    h.push_or_update({10, 10, 3});
    h.push_or_update({20, 20, 4});
    h.push_or_update({5, 5, 4});    // decrease node 4
    h.push_or_update({50, 50, 3});  // worse -> ignored
    CHECK(h.size() == 2);
    Item a = h.pop();
    CHECK(a.node == 4 && a.f == 5);
    Item b = h.pop();
    CHECK(b.node == 3 && b.f == 10);
    h.push_or_update({7, 7, 4});  // a popped node can be inserted again (A* reopening)
    CHECK(h.size() == 1 && h.pop().node == 4);
}

static void test_generators() {
    auto a = make_random_grid(64, 64, 0.3, Connectivity::Four, 7);
    auto b = make_random_grid(64, 64, 0.3, Connectivity::Four, 7);
    bool same = true;
    for (NodeId u = 0; u < a.grid.num_nodes(); ++u) same = same && a.grid.is_blocked(u) == b.grid.is_blocked(u);
    CHECK(same && a.start == b.start && a.goal == b.goal);

    auto open = make_random_grid(32, 32, 0.0, Connectivity::Four, 1);
    CHECK(open.free_cells == 32 * 32 && open.component_size == 32 * 32);
    CHECK(open.start == 0 && open.goal == open.grid.id(31, 31));

    // density roughly matches the request
    auto d = make_random_grid(200, 200, 0.25, Connectivity::Four, 3);
    double blocked_frac = 1.0 - static_cast<double>(d.free_cells) / (200.0 * 200.0);
    CHECK(blocked_frac > 0.22 && blocked_frac < 0.28);

    // start and goal are always connected, even close to the percolation threshold
    for (std::uint64_t seed = 0; seed < 20; ++seed) {
        auto inst = make_random_grid(80, 80, 0.40, Connectivity::Four, seed);
        CHECK(inst.start != kNoNode && inst.goal != kNoNode);
        CHECK(bfs(inst.grid, inst.start, inst.goal).stats.found);
    }

    auto gr = make_random_sparse_graph(1000, 4.0, 100, 5);
    CHECK(gr.num_nodes() == 1000 && gr.num_edges() == 2 * 2000);
    CHECK(bfs(gr, 0).stats.reached == 1000);  // connected by construction
}

static void test_dijkstra_matches_reference() {
    // random graphs, all heaps, full SSSP
    for (std::uint64_t seed = 0; seed < 5; ++seed) {
        auto g = make_random_sparse_graph(500, 3.0, 50, seed);
        auto ref = reference_sssp(g, 0);
        for (HeapKind k : kAllHeaps) {
            auto r = run_dijkstra(k, g, 0);
            bool ok = true;
            for (std::size_t v = 0; v < ref.size(); ++v) ok = ok && close(r.dist[v], ref[v]);
            if (!ok) std::cerr << "  mismatch for heap " << to_string(k) << " seed " << seed << "\n";
            CHECK(ok);
        }
    }
    // 8-connected grids (non-integer weights)
    for (std::uint64_t seed = 0; seed < 5; ++seed) {
        auto inst = make_random_grid(60, 60, 0.2, Connectivity::Eight, seed);
        auto ref = reference_sssp(inst.grid, inst.start);
        for (HeapKind k : kAllHeaps) {
            auto r = run_dijkstra(k, inst.grid, inst.start);
            bool ok = true;
            for (std::size_t v = 0; v < ref.size(); ++v) ok = ok && close(r.dist[v], ref[v]);
            CHECK(ok);
        }
    }
}

static void test_heaps_expand_identically() {
    // total order => every heap must expand exactly the same number of nodes
    auto inst = make_random_grid(100, 100, 0.25, Connectivity::Eight, 11);
    for (Heuristic h : {Heuristic::Zero, Heuristic::Euclidean, Heuristic::Octile}) {
        auto base = run_astar_grid(HeapKind::Binary, inst.grid, inst.start, inst.goal, h);
        for (HeapKind k : kAllHeaps) {
            auto r = run_astar_grid(k, inst.grid, inst.start, inst.goal, h);
            CHECK(r.stats.expanded == base.stats.expanded);
            CHECK(close(r.stats.cost, base.stats.cost));
        }
    }
}

static void test_bfs_and_astar_on_grids() {
    std::uint64_t optimal_checked = 0;
    for (double density : {0.0, 0.1, 0.2, 0.3, 0.4}) {
        for (std::uint64_t seed = 0; seed < 6; ++seed) {
            auto inst = make_random_grid(70, 70, density, Connectivity::Four, seed);
            auto dij = dijkstra<BinaryHeap>(inst.grid, inst.start, inst.goal);
            auto bf = bfs(inst.grid, inst.start, inst.goal);
            CHECK(dij.stats.found && bf.stats.found);
            CHECK(close(dij.stats.cost, bf.stats.cost));  // unit costs: BFS hops == Dijkstra cost
            CHECK(close(dij.stats.cost, reference_sssp(inst.grid, inst.start)[inst.goal]));
            for (Heuristic h : {Heuristic::Zero, Heuristic::Manhattan, Heuristic::Euclidean, Heuristic::Octile}) {
                auto a = astar_grid<BinaryHeap>(inst.grid, inst.start, inst.goal, h);
                CHECK(a.stats.found && close(a.stats.cost, dij.stats.cost));  // admissible => optimal
                ++optimal_checked;
            }
            auto z = astar_grid<BinaryHeap>(inst.grid, inst.start, inst.goal, Heuristic::Zero);
            auto m = astar_grid<BinaryHeap>(inst.grid, inst.start, inst.goal, Heuristic::Manhattan);
            auto e = astar_grid<BinaryHeap>(inst.grid, inst.start, inst.goal, Heuristic::Euclidean);
            CHECK(z.stats.expanded == dij.stats.expanded);   // zero heuristic == Dijkstra
            CHECK(m.stats.expanded <= e.stats.expanded);      // stronger heuristic never expands more here
            CHECK(e.stats.expanded <= z.stats.expanded);
            // returned path is a real path of the claimed cost
            auto path = extract_path(m, inst.start, inst.goal);
            CHECK(!path.empty() && path.front() == inst.start && path.back() == inst.goal);
            CHECK(path.size() - 1 == static_cast<std::size_t>(std::llround(m.stats.cost)));
        }
    }
    CHECK(optimal_checked == 5 * 6 * 4);
}

static void test_8connected_heuristics() {
    // Octile / Euclidean admissible, Manhattan over-estimates on 8-connected grids.
    auto inst = make_random_grid(40, 40, 0.0, Connectivity::Eight, 1);
    auto dij = dijkstra<BinaryHeap>(inst.grid, inst.start, inst.goal);
    CHECK(close(dij.stats.cost, 39 * kSqrt2));
    GridHeuristic man{Heuristic::Manhattan, 40, inst.grid.x_of(inst.goal), inst.grid.y_of(inst.goal)};
    GridHeuristic oct{Heuristic::Octile, 40, inst.grid.x_of(inst.goal), inst.grid.y_of(inst.goal)};
    GridHeuristic euc{Heuristic::Euclidean, 40, inst.grid.x_of(inst.goal), inst.grid.y_of(inst.goal)};
    CHECK(man(inst.start) > dij.stats.cost + 1e-9);   // inadmissible
    CHECK(close(oct(inst.start), dij.stats.cost));    // exact on an empty grid
    CHECK(euc(inst.start) <= dij.stats.cost + 1e-9);  // admissible
    CHECK(!is_admissible(Heuristic::Manhattan, Connectivity::Eight));
    CHECK(is_admissible(Heuristic::Octile, Connectivity::Eight));

    for (std::uint64_t seed = 0; seed < 10; ++seed) {
        auto g = make_random_grid(60, 60, 0.25, Connectivity::Eight, seed);
        auto d = dijkstra<BinaryHeap>(g.grid, g.start, g.goal);
        for (Heuristic h : {Heuristic::Zero, Heuristic::Euclidean, Heuristic::Octile}) {
            auto a = astar_grid<BinaryHeap>(g.grid, g.start, g.goal, h);
            CHECK(close(a.stats.cost, d.stats.cost));
        }
    }
}

static void test_unreachable_and_trivial() {
    // wall splits the grid in two
    std::vector<std::uint8_t> blocked(5 * 5, 0);
    for (int y = 0; y < 5; ++y) blocked[y * 5 + 2] = 1;
    GridGraph g(5, 5, Connectivity::Four, blocked);
    auto r = dijkstra<BinaryHeap>(g, g.id(0, 0), g.id(4, 4));
    CHECK(!r.stats.found && r.stats.cost == kInf);
    CHECK(!bfs(g, g.id(0, 0), g.id(4, 4)).stats.found);
    CHECK(!astar_grid<BinaryHeap>(g, g.id(0, 0), g.id(4, 4), Heuristic::Manhattan).stats.found);
    // source == target
    auto t = dijkstra<BinaryHeap>(g, g.id(0, 0), g.id(0, 0));
    CHECK(t.stats.found && t.stats.cost == 0 && t.stats.expanded == 1);
}

static void test_dimacs() {
    std::istringstream in(
        "c tiny road network\n"
        "p sp 4 5\n"
        "a 1 2 7\n"
        "a 2 3 2\n"
        "a 1 3 20\n"
        "a 3 4 1\n"
        "a 4 1 5\n");
    CSRGraph g = load_dimacs_gr(in);
    CHECK(g.num_nodes() == 4 && g.num_edges() == 5);
    for (HeapKind k : kAllHeaps) {
        auto r = run_dijkstra(k, g, 0);
        CHECK(close(r.dist[1], 7) && close(r.dist[2], 9) && close(r.dist[3], 10));
    }
    std::istringstream bad("p sp 2 1\na 1 9 3\n");
    bool threw = false;
    try { load_dimacs_gr(bad); } catch (const std::exception&) { threw = true; }
    CHECK(threw);
}

int main() {
    test_dynamic_array();
    test_heaps();
    test_generators();
    test_dijkstra_matches_reference();
    test_heaps_expand_identically();
    test_bfs_and_astar_on_grids();
    test_8connected_heuristics();
    test_unreachable_and_trivial();
    test_dimacs();
    std::cout << g_checks << " checks, " << g_failed << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
