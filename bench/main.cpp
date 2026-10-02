// pfbench -- benchmark driver for BFS / Dijkstra / A* and the open-list heaps.
//
//   pfbench --suite all --out results
//   pfbench --suite density --quick
//   pfbench --suite dimacs --dimacs USA-road-d.NY.gr --sources 10
//
// One CSV row = one (instance, algorithm, heuristic, heap) combination, with
// time statistics over --reps repetitions and the deterministic search counters.
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "mem_tracker.hpp"
#include "pf/dimacs.hpp"
#include "pf/dispatch.hpp"
#include "pf/generators.hpp"

using namespace pf;
using Clock = std::chrono::steady_clock;

// ------------------------------------------------------------------ options
struct Options {
    std::string suite = "all";
    std::string out_dir = "results";
    std::string dimacs_path;
    int reps = 5;          // timed repetitions per (instance, algorithm)
    int instances = 8;     // random instances per parameter setting
    int sources = 5;       // DIMACS: random source nodes
    int grid_side = 512;   // density suite grid size
    bool quick = false;
    HeapKind heap = HeapKind::Binary;  // heap used by the density/size/graph suites
    std::uint64_t seed = 20260101;
};

static void usage() {
    std::cout <<
R"(usage: pfbench [options]
  --suite  density|size|graph|heaps|dimacs|all   (default all; dimacs needs --dimacs)
  --out    DIR        output directory for CSV files (default results)
  --reps   N          timed repetitions per run, after 1 warm-up (default 5)
  --instances N       random instances per setting (default 8)
  --heap   std_pq|binary|quaternary|indexed_binary   heap for density/size/graph suites (default binary)
  --dimacs FILE.gr    road network in DIMACS format
  --sources N         random sources on the DIMACS graph (default 5)
  --seed   S          base seed (default 20260101)
  --quick             small sizes, 2 instances, 3 reps (smoke test)
)";
}

static Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) { std::cerr << "missing value for " << a << "\n"; std::exit(2); }
            return argv[++i];
        };
        if (a == "--suite") o.suite = next();
        else if (a == "--out") o.out_dir = next();
        else if (a == "--reps") o.reps = std::atoi(next().c_str());
        else if (a == "--instances") o.instances = std::atoi(next().c_str());
        else if (a == "--heap") o.heap = parse_heap(next());
        else if (a == "--dimacs") o.dimacs_path = next();
        else if (a == "--sources") o.sources = std::atoi(next().c_str());
        else if (a == "--seed") o.seed = std::strtoull(next().c_str(), nullptr, 10);
        else if (a == "--quick") { o.quick = true; }
        else if (a == "--help" || a == "-h") { usage(); std::exit(0); }
        else { std::cerr << "unknown option " << a << "\n"; usage(); std::exit(2); }
    }
    if (o.quick) { o.reps = 3; o.instances = 2; o.grid_side = 128; o.sources = 2; }
    if (o.reps < 1) o.reps = 1;
    return o;
}

// ------------------------------------------------------------------ measurement
struct Measured {
    SearchStats stats;
    std::size_t peak_alloc = 0;  // peak extra heap bytes during one run (includes dist/parent arrays)
    double mean = 0, min = 0, stddev = 0;
    int reps = 0;
};

// Run 0 (untimed warm-up) also measures peak memory. Runs 1..reps are timed.
static Measured measure(const std::function<SearchResult()>& run, int reps) {
    Measured m;
    m.reps = reps;
    {
        const std::size_t base = mem::current();
        mem::reset_peak();
        SearchResult r = run();
        m.peak_alloc = mem::peak() - base;
        m.stats = r.stats;
    }
    std::vector<double> t;
    for (int i = 0; i < reps; ++i) {
        const auto t0 = Clock::now();
        SearchResult r = run();
        const auto t1 = Clock::now();
        if (r.stats.expanded != m.stats.expanded) { std::cerr << "non-deterministic run!\n"; std::exit(1); }
        t.push_back(std::chrono::duration<double>(t1 - t0).count());
    }
    double sum = 0;
    m.min = t[0];
    for (double x : t) { sum += x; m.min = std::min(m.min, x); }
    m.mean = sum / t.size();
    if (t.size() > 1) {
        double ss = 0;
        for (double x : t) ss += (x - m.mean) * (x - m.mean);
        m.stddev = std::sqrt(ss / (t.size() - 1));
    }
    return m;
}

// ------------------------------------------------------------------ CSV output
struct Context {  // per-row metadata
    std::string suite, family, param_name, param_value, connectivity;
    std::size_t nodes = 0, edges_or_free = 0;
    int instance = 0;
    std::uint64_t seed = 0;
};

class Csv {
public:
    Csv(const std::string& path) : f_(path) {
        if (!f_) { std::cerr << "cannot write " << path << "\n"; std::exit(1); }
        f_ << "suite,family,param_name,param_value,nodes,edges_or_free,connectivity,instance,seed,"
              "algo,heuristic,heap,admissible,found,cost,expanded,generated,stale_pops,peak_open,"
              "container_bytes,peak_alloc_bytes,time_mean_s,time_min_s,time_std_s,reps\n";
        f_ << std::setprecision(10);
    }
    void row(const Context& c, const std::string& algo, const std::string& heur, const std::string& heap,
             bool admissible, const Measured& m) {
        f_ << c.suite << ',' << c.family << ',' << c.param_name << ',' << c.param_value << ',' << c.nodes
           << ',' << c.edges_or_free << ',' << c.connectivity << ',' << c.instance << ',' << c.seed << ','
           << algo << ',' << heur << ',' << heap << ',' << (admissible ? 1 : 0) << ','
           << (m.stats.found ? 1 : 0) << ',' << (m.stats.found ? m.stats.cost : -1.0) << ','
           << m.stats.expanded << ',' << m.stats.generated << ',' << m.stats.stale_pops << ','
           << m.stats.peak_open << ',' << m.stats.container_bytes << ',' << m.peak_alloc << ',' << m.mean
           << ',' << m.min << ',' << m.stddev << ',' << m.reps << '\n';
    }

private:
    std::ofstream f_;
};

static std::string fmt(double v) {
    char b[32];
    std::snprintf(b, sizeof b, "%g", v);
    return b;
}

// ------------------------------------------------------------------ grid experiments
static void run_grid_instance(Csv& csv, Context ctx, const GridInstance& inst, const Options& o,
                              bool all_heaps_for_dijkstra, bool include_bfs) {
    const GridGraph& g = inst.grid;
    const bool eight = g.connectivity() == Connectivity::Eight;
    ctx.family = "grid";
    ctx.nodes = g.num_nodes();
    ctx.edges_or_free = inst.free_cells;
    ctx.connectivity = eight ? "8" : "4";
    const NodeId s = inst.start, t = inst.goal;
    if (s == kNoNode || t == kNoNode || s == t) return;

    if (include_bfs)
        csv.row(ctx, "bfs", "none", "queue", true, measure([&] { return bfs(g, s, t); }, o.reps));

    std::vector<HeapKind> heaps = {o.heap};
    if (all_heaps_for_dijkstra) heaps.assign(kAllHeaps.begin(), kAllHeaps.end());
    for (HeapKind hk : heaps)
        csv.row(ctx, "dijkstra", "none", to_string(hk), true,
                measure([&] { return run_dijkstra(hk, g, s, t); }, o.reps));

    std::vector<Heuristic> hs = {Heuristic::Zero, Heuristic::Manhattan, Heuristic::Euclidean};
    if (eight) hs.push_back(Heuristic::Octile);
    for (HeapKind hk : heaps)
        for (Heuristic h : hs) {
            if (all_heaps_for_dijkstra && h != Heuristic::Octile && h != Heuristic::Euclidean) continue;  // keep heap study focused
            csv.row(ctx, "astar", to_string(h), to_string(hk), is_admissible(h, g.connectivity()),
                    measure([&] { return run_astar_grid(hk, g, s, t, h); }, o.reps));
        }
}

static void suite_density(const Options& o) {
    Csv csv(o.out_dir + "/density.csv");
    const double densities[] = {0, 0.05, 0.10, 0.15, 0.20, 0.25, 0.30, 0.35, 0.40};
    std::cerr << "[density] " << o.grid_side << "x" << o.grid_side << " grids, 4-connected\n";
    int di = 0;
    for (double d : densities) {
        for (int k = 0; k < o.instances; ++k) {
            Context c;
            c.suite = "density"; c.param_name = "obstacle_density"; c.param_value = fmt(d);
            c.instance = k; c.seed = derive_seed(o.seed, 1, di, k);
            auto inst = make_random_grid(o.grid_side, o.grid_side, d, Connectivity::Four, c.seed);
            run_grid_instance(csv, c, inst, o, false, true);
        }
        std::cerr << "  density " << d << " done\n";
        ++di;
    }
}

static void suite_size(const Options& o) {
    Csv csv(o.out_dir + "/size.csv");
    std::vector<int> sides = o.quick ? std::vector<int>{32, 64, 128} : std::vector<int>{64, 128, 256, 512, 1024};
    std::cerr << "[size] 20% obstacles, 4-connected\n";
    for (int side : sides) {
        for (int k = 0; k < o.instances; ++k) {
            Context c;
            c.suite = "size"; c.param_name = "grid_side"; c.param_value = std::to_string(side);
            c.instance = k; c.seed = derive_seed(o.seed, 2, side, k);
            auto inst = make_random_grid(side, side, 0.20, Connectivity::Four, c.seed);
            run_grid_instance(csv, c, inst, o, false, true);
        }
        std::cerr << "  side " << side << " done\n";
    }
}

// ------------------------------------------------------------------ sparse random graphs
static void suite_graph(const Options& o) {
    Csv csv(o.out_dir + "/graph.csv");
    std::vector<std::size_t> ns = o.quick ? std::vector<std::size_t>{10000, 30000}
                                          : std::vector<std::size_t>{10000, 30000, 100000, 300000, 1000000};
    std::cerr << "[graph] sparse random graphs, avg degree 4, weights 1..100, SSSP from a random source\n";
    for (std::size_t n : ns) {
        for (int k = 0; k < o.instances; ++k) {
            Context c;
            c.suite = "graph"; c.family = "random_graph"; c.param_name = "n"; c.param_value = std::to_string(n);
            c.connectivity = "undirected"; c.instance = k; c.seed = derive_seed(o.seed, 3, n, k);
            CSRGraph g = make_random_sparse_graph(n, 4.0, 100, c.seed);
            c.nodes = g.num_nodes(); c.edges_or_free = g.num_edges();
            const NodeId src = static_cast<NodeId>(Rng(c.seed ^ 0xABCDEF).below(n));
            csv.row(c, "bfs", "none", "queue", true, measure([&] { return bfs(g, src); }, o.reps));
            csv.row(c, "dijkstra", "none", to_string(o.heap), true,
                    measure([&] { return run_dijkstra(o.heap, g, src); }, o.reps));
        }
        std::cerr << "  n=" << n << " done\n";
    }
}

// ------------------------------------------------------------------ heap comparison
static void suite_heaps(const Options& o) {
    Csv csv(o.out_dir + "/heaps.csv");
    // (a) Dijkstra SSSP on sparse graphs, varying density of edges
    const std::size_t n = o.quick ? 30000 : 200000;
    std::cerr << "[heaps] (a) Dijkstra SSSP, n=" << n << ", varying average degree\n";
    for (double deg : {2.0, 4.0, 8.0, 16.0}) {
        for (int k = 0; k < o.instances; ++k) {
            Context c;
            c.suite = "heaps"; c.family = "random_graph"; c.param_name = "avg_degree"; c.param_value = fmt(deg);
            c.connectivity = "undirected"; c.instance = k; c.seed = derive_seed(o.seed, 4, static_cast<std::uint64_t>(deg), k);
            CSRGraph g = make_random_sparse_graph(n, deg, 100, c.seed);
            c.nodes = g.num_nodes(); c.edges_or_free = g.num_edges();
            const NodeId src = static_cast<NodeId>(Rng(c.seed ^ 0xABCDEF).below(n));
            for (HeapKind hk : kAllHeaps)
                csv.row(c, "dijkstra", "none", to_string(hk), true, measure([&] { return run_dijkstra(hk, g, src); }, o.reps));
        }
        std::cerr << "  degree " << deg << " done\n";
    }
    // (b) 8-connected grid, point-to-point: Dijkstra and A* (octile / euclidean) with every heap
    const int side = o.quick ? 128 : 768;
    std::cerr << "[heaps] (b) " << side << "x" << side << " 8-connected grid, 20% obstacles\n";
    for (int k = 0; k < o.instances; ++k) {
        Context c;
        c.suite = "heaps"; c.param_name = "grid_side"; c.param_value = std::to_string(side);
        c.instance = k; c.seed = derive_seed(o.seed, 5, side, k);
        auto inst = make_random_grid(side, side, 0.20, Connectivity::Eight, c.seed);
        run_grid_instance(csv, c, inst, o, true, false);
    }
}

// ------------------------------------------------------------------ DIMACS road network
static void suite_dimacs(const Options& o) {
    if (o.dimacs_path.empty()) { std::cerr << "[dimacs] skipped: pass --dimacs FILE.gr\n"; return; }
    const auto t0 = Clock::now();
    CSRGraph g = load_dimacs_gr_file(o.dimacs_path);
    std::cerr << "[dimacs] loaded " << g.num_nodes() << " nodes, " << g.num_edges() << " arcs in "
              << std::chrono::duration<double>(Clock::now() - t0).count() << " s\n";
    Csv csv(o.out_dir + "/dimacs.csv");
    for (int k = 0; k < o.sources; ++k) {
        Context c;
        c.suite = "dimacs"; c.family = "dimacs"; c.param_name = "graph";
        c.param_value = std::filesystem::path(o.dimacs_path).stem().string();
        c.connectivity = "directed"; c.instance = k; c.seed = derive_seed(o.seed, 6, k);
        c.nodes = g.num_nodes(); c.edges_or_free = g.num_edges();
        const NodeId src = static_cast<NodeId>(Rng(c.seed).below(g.num_nodes()));
        csv.row(c, "bfs", "none", "queue", true, measure([&] { return bfs(g, src); }, o.reps));
        for (HeapKind hk : kAllHeaps)
            csv.row(c, "dijkstra", "none", to_string(hk), true, measure([&] { return run_dijkstra(hk, g, src); }, o.reps));
        std::cerr << "  source " << k << " done\n";
    }
}

int main(int argc, char** argv) {
    Options o = parse(argc, argv);
    std::filesystem::create_directories(o.out_dir);
#ifndef NDEBUG
    std::cerr << "WARNING: this is not an optimized build (NDEBUG undefined) -- timings are meaningless.\n"
                 "         Build with -DCMAKE_BUILD_TYPE=Release.\n";
#endif
    const bool all = o.suite == "all";
    if (all || o.suite == "density") suite_density(o);
    if (all || o.suite == "size") suite_size(o);
    if (all || o.suite == "graph") suite_graph(o);
    if (all || o.suite == "heaps") suite_heaps(o);
    if (o.suite == "dimacs" || (all && !o.dimacs_path.empty())) suite_dimacs(o);
    std::cerr << "CSV files written to " << o.out_dir << "/\n";
    return 0;
}
