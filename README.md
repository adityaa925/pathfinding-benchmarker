# pathfinding-benchmarker

C++20, header-only study of **BFS, Dijkstra and A\*** and of the **open-list data structure** behind them.
It measures *nodes expanded*, *runtime* and *memory* on grids with 0–40 % obstacles, on sparse random
graphs and (optionally) on a DIMACS road network, and writes everything to CSV for analysis.

What is compared

| Question | Variants |
|---|---|
| How much does a heuristic save? | BFS, Dijkstra (= A\* with zero heuristic), A\* with Manhattan, Euclidean (and Octile on 8-connected grids) |
| Does the heap matter? | `std::priority_queue`, own binary heap on a dynamic array, 4-ary heap, indexed binary heap with decrease-key |
| Where does each approach break down? | obstacle density 0–40 %, grid side 64–1024, random graph size 10⁴–10⁶, average degree 2–16, real road network |

Full plan, theory, hypotheses and methodology: **[docs/WORKFLOW.md](docs/WORKFLOW.md)**.

## Layout

```
include/pf/
  common.hpp         Item, strict total order `better()` shared by all heaps
  dynamic_array.hpp  minimal dynamic array (swap in your own DS-library array)
  heaps.hpp          StdPriorityQueue, DAryHeap<2|4>, IndexedBinaryHeap
  graph.hpp          GridGraph (implicit, 4/8-connected), CSRGraph
  generators.hpp     reproducible RNG, random grids, sparse random graphs
  dimacs.hpp         DIMACS .gr loader
  algorithms.hpp     bfs(), best_first() engine, dijkstra(), astar_grid(), heuristics
  dispatch.hpp       runtime selection of the heap type
bench/               pfbench driver + allocation tracker
tests/               dependency-free test runner (CTest)
scripts/analyze.py   CSV -> Markdown tables + PNG plots
docs/WORKFLOW.md     implementation workflow, experiment design, how to read results
```

## Build and test

Windows (PowerShell, MinGW + Ninja) or Linux/macOS:

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Without CMake: `g++ -std=c++20 -O2 -DNDEBUG -Iinclude -Ibench bench/main.cpp bench/mem_tracker.cpp -o pfbench`

Always benchmark a **Release** build (the tool warns if `NDEBUG` is not defined).

## Run

```
pfbench --suite all --out results              # everything except DIMACS
pfbench --suite density --quick                # smoke test, a few seconds
pfbench --suite dimacs --dimacs USA-road-d.NY.gr --sources 10
python scripts/analyze.py results              # pip install pandas matplotlib tabulate
```

Options: `--reps N` (timed repetitions, default 5, plus one untimed warm-up), `--instances N` (random
instances per setting, default 8), `--heap std_pq|binary|quaternary|indexed_binary`, `--seed S`, `--quick`.

Road networks: 9th DIMACS Implementation Challenge, e.g. `USA-road-d.NY.gr`
(<http://www.diag.uniroma1.it/challenge9/download.shtml>). Put the file under `data/` (git-ignored).

## CSV columns

`suite, family, param_name, param_value, nodes, edges_or_free, connectivity, instance, seed, algo, heuristic,
heap, admissible, found, cost, expanded, generated, stale_pops, peak_open, container_bytes, peak_alloc_bytes,
time_mean_s, time_min_s, time_std_s, reps`

* `expanded / generated / stale_pops / peak_open / container_bytes` are deterministic (identical on every repetition).
* `peak_alloc_bytes` = peak extra heap memory during one run, measured by replacing global `operator new/delete`
  (includes the `dist` + `parent` arrays and the open list; excludes the input graph).
* `time_*` are over the timed repetitions of one instance; `analyze.py` then averages across instances.
* `admissible = 0` marks A\* runs whose heuristic can overestimate (Manhattan on 8-connected grids), so `cost` may be above optimal.

## Using your own data-structures library

`heaps.hpp` builds the binary / 4-ary / indexed heaps on `DefaultContainer<T>`. Point that alias at your
dynamic array (needs `push_back, pop_back, back, operator[], size, empty, capacity`) and rebuild.
