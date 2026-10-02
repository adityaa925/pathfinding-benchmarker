# Workflow: BFS vs Dijkstra vs A\* and the heap behind them

This document is the plan, the theory, the experiment design and the way to read the results.
Phases 0–6 are already implemented in this repository; phases 7–10 are what you run and write up on your machine.

---------------------------------------------------------------------------------------------------

## 1. Research questions

1. **Heuristic value.** How many fewer nodes does A\* expand than Dijkstra, and does that saving turn into less *time*, given that the heuristic itself costs work?
2. **Obstacle density.** How do all three algorithms behave from 0 % to 40 % obstacles? (Near 40 % a 4-connected grid is close to the percolation threshold, see §4.)
3. **Scaling.** Does measured growth match the theory (BFS Θ(V+E); Dijkstra Θ((V+E) log V))?
4. **The heap.** `std::priority_queue` vs a hand-written binary heap on a dynamic array vs a 4-ary heap vs an indexed heap with true decrease-key: time, memory, wasted (stale) pops.
5. **Reality check.** Do the conclusions survive on a real road network (DIMACS) instead of synthetic graphs?

## 2. Architecture decisions (and why)

| Decision | Reason |
|---|---|
| One engine `best_first()` for Dijkstra and A\* | Dijkstra *is* A\* with h = 0. Any difference in results is then caused only by the heuristic. Tests check the engine against an independent `std::set` Dijkstra. |
| Graph "concept": `num_nodes()` + `for_each_neighbor(u, f)` | Same algorithm code runs on an implicit grid and on a CSR graph. |
| Implicit `GridGraph` (bitmap of blocked cells) | No edge storage, 1 byte per cell; neighbours computed on the fly. |
| `CSRGraph` (offset / target / weight arrays) | Cache-friendly adjacency for random graphs and DIMACS. |
| Heaps are compile-time template parameters, selected at runtime in `dispatch.hpp` | No virtual calls inside the hot loop, so the comparison is fair. |
| Strict total order `better()` (f, then larger g, then node id) | All heaps pop in the same order → **identical `expanded` for every heap** (tested). Heap benchmarks therefore measure only data-structure cost. |
| Own splitmix64 RNG, not `<random>` distributions | `std::uniform_*_distribution` is implementation-defined: same seed ≠ same instance on MSVC / MinGW / Linux. |
| Start/goal chosen inside the largest connected component, near opposite corners | At 40 % obstacles most random corner pairs are disconnected; this keeps every instance solvable without resampling bias. |
| Stop rule: stop when the target is **popped/dequeued** (all algorithms) | Makes `expanded` comparable across BFS, Dijkstra and A\*. |
| `admissible` flag in the CSV | Manhattan on an 8-connected grid overestimates (diagonal = √2 < 2), so those A\* costs may be suboptimal; they must not be mixed with optimal runs unnoticed. |

## 3. Implementation phases

Each phase ends with a commit. Suggested messages in brackets.

| Phase | Deliverable | Acceptance test | Status |
|---|---|---|---|
| 0 | Repo skeleton, CMake, `.gitignore` (build/ ignored) [`init: project skeleton`] | `cmake --build` works from a clean checkout | done |
| 1 | `common.hpp`, `graph.hpp`: `Item`, total order, `GridGraph`, `CSRGraph` [`graph abstractions`] | neighbour enumeration unit tests | done |
| 2 | `algorithms.hpp`: BFS, `best_first`, Dijkstra, A\* + heuristics zero / Manhattan / Euclidean / Octile [`bfs, dijkstra, astar`] | cost equals an independent `std::set` Dijkstra on every generated test instance | done |
| 3 | `heaps.hpp` + `dynamic_array.hpp`: std PQ, binary, 4-ary, indexed binary [`open-list implementations`] | pops equal `std::sort` order incl. ties; decrease-key semantics | done |
| 4 | `generators.hpp`, `dimacs.hpp` [`instance generators`] | same seed → same instance; density within ±3 %; DIMACS parser tests | done |
| 5 | `tests/` [`tests: reference dijkstra + invariants`] | 547 checks pass, also under ASan + UBSan | done |
| 6 | `bench/`: harness, allocation tracker, CSV [`benchmark driver`] | `pfbench --quick` finishes in seconds, CSV well-formed | done |
| 7 | **Run the experiments on your machine** (§5) | CSVs in `results/`, machine info noted | you |
| 8 | `scripts/analyze.py`: tables + plots [`analysis`] | plots reproduce from CSV alone | done (run it) |
| 9 | **DIMACS road network** (NY first, then a larger one) | `dimacs.csv` exists, results discussed | you |
| 10 | **Write-up** (§8) + curated results in `docs/` | someone else can reproduce a figure from the README | you |

A bug the tests caught during development: the indexed heap originally decided "is this a decrease-key?" with `better(new, old)`. With floating point, `g_new + h` can round to the same `f` as `g_old + h` even though `g_new < g_old`; the tie-break (larger g first) then says "old is better", the update was dropped and the cheaper path was lost. The fix compares `g` directly. (Lazy heaps were never affected because they keep both entries.)

## 4. Theory to check the measurements against

**Complexities** (V nodes, E edges)

| | time | extra space |
|---|---|---|
| BFS | Θ(V + E) | Θ(V) |
| Dijkstra, binary heap, lazy deletion | O(E log V) (heap holds up to E entries) | O(E) worst case |
| Dijkstra, indexed binary heap | O((V + E) log V) | O(V) |
| Dijkstra, Fibonacci heap (Fredman–Tarjan 1987) | O(E + V log V) in theory; large constants | O(V) |
| A\* | worst case as Dijkstra; expands a subset of Dijkstra's nodes for an admissible, consistent h | as Dijkstra |

On grids and sparse graphs E = Θ(V), so Dijkstra is Θ(V log V) and BFS is Θ(V): expect BFS to scale ~linearly and Dijkstra with a small extra log factor. Duan et al. (2025), on your reading list, report an O(m log^(2/3) n) algorithm for directed SSSP; it is a theory result, a candidate "future work" comparison rather than something to expect to win on small inputs.

**Heuristics on a 4-connected unit-cost grid**: Manhattan is exact on an empty grid and consistent; Euclidean is admissible but weaker (h_euclid ≤ h_manhattan pointwise). With consistent heuristics, a pointwise larger h never expands more nodes than a smaller one apart from ties (Hart–Nilsson–Raphael 1968). So expansions should satisfy Manhattan ≤ Euclidean ≤ Zero = Dijkstra.
**8-connected, diagonal √2**: Octile is exact on an empty grid, Euclidean admissible, **Manhattan inadmissible**.

**Percolation.** Site percolation on the square lattice has a threshold at ≈ 59.3 % *open* cells. At 40 % obstacles a 4-connected grid is only just above it: the giant component is thin and winding, optimal paths become much longer than the Manhattan distance, and the heuristic becomes much less informative. 8-connected grids percolate far earlier (≈ 40.7 % open), so there 40 % obstacles is not special.

## 5. Experiment matrix

| Suite | Fixed | Varied | Algorithms | Instances × reps |
|---|---|---|---|---|
| `density` | 512×512, 4-conn | obstacles 0, 5, …, 40 % | BFS, Dijkstra, A\* zero / Manhattan / Euclidean | 8 × (1 warm-up + 5) |
| `size` | 20 % obstacles, 4-conn | side 64, 128, 256, 512, 1024 | same | 8 × 6 |
| `graph` | avg degree 4, weights 1–100, SSSP | n = 10⁴ … 10⁶ | BFS, Dijkstra | 8 × 6 |
| `heaps` (a) | n = 200 000, SSSP | avg degree 2, 4, 8, 16 | Dijkstra × 4 heaps | 8 × 6 |
| `heaps` (b) | 768×768, 8-conn, 20 % | – | Dijkstra, A\* Euclid, A\* Octile × 4 heaps | 8 × 6 |
| `dimacs` | real road network | random sources | BFS, Dijkstra × 4 heaps | `--sources` × 6 |

Run: `pfbench --suite all --out results` then `python scripts/analyze.py results`.

## 6. Measurement methodology

* **Same instance for every algorithm**, so differences are not instance noise. Instances differ by seed; seeds are derived from `--seed`, setting and index and recorded in the CSV.
* **One untimed warm-up run** (also used for the memory measurement), then `--reps` timed runs with `std::chrono::steady_clock`. Report the **mean across instances** and its spread; keep `time_min_s` as the least noisy estimate of the machine-limited time.
* **Timed region** = the whole search including allocation of `dist` / `parent` and the open list (what a user of the function pays). Instance generation and graph construction are excluded.
* **Counters** (`expanded`, `generated`, `stale_pops`, `peak_open`) are deterministic; the harness aborts if repetitions disagree.
* **Memory** has two views: `container_bytes` (peak bytes owned by the open list / queue, exact) and `peak_alloc_bytes` (peak extra live heap bytes during a run, from a replaced global `operator new/delete`; includes `dist` + `parent`, excludes the input graph, excludes stack and OS overhead).
* **Hygiene on your machine**: Release build; plugged in with the high-performance power plan; close browsers/IDEs; run the whole suite twice and check that the trends repeat; write down CPU model, compiler + version, flags, OS in the write-up.

## 7. Hypotheses to confirm or refute

Written *before* looking at your own results, so the write-up can say which held.

* **H1** Dijkstra = A\* zero = BFS in nodes expanded on unit-cost grids (BFS differs only by tie-breaking at the end).
* **H2** Manhattan expands a tiny fraction of Dijkstra's nodes on open grids; the advantage **shrinks as density grows** because the heuristic ignores obstacles.
* **H3** Euclidean saves little on a 4-connected grid, and its `sqrt` can make it *slower* than Dijkstra in wall-clock time even when it expands fewer nodes.
* **H4** On unit-cost grids BFS is much faster than Dijkstra (no heap), and can beat A\* at high density.
* **H5** Own binary ≈ 4-ary ≈ `std::priority_queue` within a modest factor; differences come from constants (comparison count, cache behaviour), not asymptotics.
* **H6** The indexed heap wins on **memory** (O(V) instead of O(E) entries, zero stale pops) and increasingly on **time as average degree rises**, because lazy heaps accumulate many stale entries; on tree-like graphs (degree 2) it has no advantage.

## 8. Results from the development run (sandbox, treat as a preview)

Produced in a **single-vCPU Linux sandbox**, 5 instances × 5 timed reps (`--instances 5`), GCC 13 `-O2`. Absolute times will differ on your machine; use it only to see what the pipeline produces. Raw CSVs, tables and plots: `docs/sample-results/`.

*Grid, 512×512, 4-connected (nodes expanded, mean of 5 instances):*

| obstacles | Dijkstra / A\* zero | A\* Euclidean | A\* Manhattan |
|---|---|---|---|
| 0 % | 262 144 | 261 123 | 1 023 |
| 20 % | 209 295 | 201 540 | 18 128 |
| 35 % | 161 035 | 133 270 | 65 481 |
| 40 % | 91 219 | 77 039 | 71 093 |

* H1 held (BFS differed by ≤ 6 nodes on average). H2 held: Manhattan expands 0.4 % of Dijkstra's nodes on an empty grid, but at 40 % obstacles it still expands ~78 % as many, so almost all of the benefit is gone. Its instance-to-instance variance also grows sharply from 30 % up (error bars in `density_expanded.png`).
* H3 held: Euclidean expands only 0–17 % fewer nodes, and its runtime was **not** better than Dijkstra's (e.g. 35 ms vs 30 ms at 0 %).
* H4 held: BFS took 4–7 ms where Dijkstra took 22–29 ms on the 512² grid up to 35 % obstacles (≈ 3–5×), and beat A\* Manhattan at 35 % obstacles (7.4 ms vs 11.9 ms) despite expanding more nodes.
* Scaling (20 % obstacles): Dijkstra 1.2 → 6.0 → 27.6 → 119.8 ms for side 128 → 256 → 512 → 1024, i.e. ≈ 4.3–5× per doubling of the side (4× more nodes), consistent with Θ(V log V).
* Random graphs, avg degree 4, 10⁴ → 10⁶ nodes: BFS 0.3 → 90 ms, Dijkstra 2.2 → 782 ms.

*Heaps, Dijkstra SSSP, n = 200 000 (time in ms):*

| avg degree | std_pq | binary | 4-ary | indexed_binary | stale pops of the lazy heaps |
|---|---|---|---|---|---|
| 2 | 44.5 | 61.5 | 55.7 | 56.8 | 0 |
| 4 | 78.0 | 96.6 | 89.8 | 78.3 | 60 919 |
| 8 | 119.6 | 137.4 | 126.7 | 100.1 | 155 160 |
| 16 | 189.1 | 194.6 | 194.6 | 128.6 | 270 122 |

* H6 held: the indexed heap pulls ahead as degree grows (−32 % time vs `std_pq` at degree 16) and its peak extra memory is 12.1 vs 20.3 MiB; at degree 2 it uses slightly *more* memory (position array) and is not faster than `std_pq`.
* H5 only partly: the hand-written binary heap was 3–38 % *slower* than `std::priority_queue`, the gap shrinking as degree grows. A likely reason (not verified here) is that libstdc++'s `pop_heap` sifts the hole to a leaf first and then back up, which saves comparisons; testing that against a "bottom-up" variant of your heap is a natural follow-up experiment. The 4-ary heap sat between the two.
* On the 768² 8-connected grid the indexed heap was fastest for A\* (36 vs 46–50 ms with Octile) while all other heaps were within ~10 % of each other. Dijkstra there expanded 471 k nodes vs 176 k for Octile A\*.

Caveats: 5 instances only, one machine, no CPU pinning; differences below ~10 % should not be claimed without more instances.

## 9. Threats to validity (put these in the write-up)

* The grid is implicit and the random graphs are CSR: **do not compare grid times with graph times**; compare algorithms within one family.
* Tie-breaking changes `expanded` slightly for A\*; it is fixed and identical across algorithms here, but other choices (e.g. FIFO among ties) would change the numbers.
* `peak_alloc_bytes` only sees `operator new`; it does not see stack, page-table or allocator slack, and the 16-byte tracking header per allocation is not counted.
* Timings include one allocation of `dist`/`parent` per run; for tiny grids this is a noticeable share.
* Synthetic random graphs have no geometry or hierarchy; road networks do. The DIMACS suite exists to check this.
* Obstacle placement is independent Bernoulli noise; real maps (rooms, mazes) behave differently, so conclusions are about random obstacle fields.
* Floating-point costs: with √2 weights, `f` ties and near-ties depend on rounding. Costs are compared with a 1e-9 relative tolerance in tests.

## 10. Extensions, in order of value

1. DIMACS NY → a larger network; add the `.co` coordinates and an admissible Euclidean/landmark heuristic (needs weight ≥ scaled distance).
2. A bottom-up-sift binary heap and a pairing heap; measure against `std_pq` (tests H5).
3. Bidirectional Dijkstra / bidirectional A\*.
4. Maze / room-structured maps next to random noise.
5. Replace the dynamic array with your DS-library one (`DefaultContainer` alias in `heaps.hpp`) and report the difference.
6. A Fibonacci heap, to see whether the O(E + V log V) bound ever pays off in practice.

## 11. Write-up skeleton (README section / blog / CV)

1. Question and motivation (one paragraph).
2. Implementation: engine, heaps, generators (diagram of §2).
3. Verification: reference Dijkstra, ASan/UBSan, the decrease-key bug and how it was found.
4. Results: one figure per research question, 2–3 sentences each, tied to H1–H6.
5. Threats to validity (§9).
6. How to reproduce (exact commands, seed, machine).

CV bullet shape, once *your* numbers exist: *"Benchmarked BFS, Dijkstra and A\* (Manhattan/Euclidean) on grids with 0–40 % obstacles and random graphs up to 10⁶ nodes; compared four heap implementations; found A\*-Manhattan expands X % of Dijkstra's nodes on empty grids but Y % at 40 % obstacles."* Fill X and Y from your own CSVs.
