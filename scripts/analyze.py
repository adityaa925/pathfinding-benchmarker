#!/usr/bin/env python3
"""Aggregate pfbench CSV files into summary tables (Markdown) and plots (PNG).

    python scripts/analyze.py results            # reads results/*.csv, writes results/summary.md + results/*.png

Averaging scheme: every CSV row is already the mean/min/std over --reps timed
repetitions of ONE instance. Here we average rows over the random instances of a
setting (mean +- std across instances), so the error bars show instance-to-instance
variation, not just timer noise.
"""
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd

out = Path(sys.argv[1] if len(sys.argv) > 1 else "results")
md = []


def load(name):
    p = out / f"{name}.csv"
    return pd.read_csv(p) if p.exists() else None


def label(r):
    parts = [r["algo"]]
    if r["heuristic"] != "none":
        parts.append(r["heuristic"])
    if r["heap"] not in ("queue",) and r["algo"] != "bfs":
        parts.append(f"[{r['heap']}]")
    return " ".join(parts)


def prep(df):
    df = df.copy()
    df["variant"] = df.apply(label, axis=1)
    df["time_ms"] = df["time_mean_s"] * 1e3
    df["alloc_MiB"] = df["peak_alloc_bytes"] / 2**20
    df["open_MiB"] = df["container_bytes"] / 2**20
    return df


def table(df, index, cols, title, fmt="{:.3f}"):
    g = df.groupby([index, "variant"])[cols].mean().reset_index()
    md.append(f"\n### {title}\n")
    for c in cols:
        pv = g.pivot(index=index, columns="variant", values=c)
        md.append(f"**{c}** (mean over instances)\n")
        md.append(pv.to_markdown(floatfmt=".4g"))
        md.append("")


def lineplot(df, x, y, title, fname, ylabel, logx=False, logy=False):
    fig, ax = plt.subplots(figsize=(7, 4.2))
    for v, d in df.groupby("variant"):
        g = d.groupby(x)[y].agg(["mean", "std"]).reset_index().fillna(0)
        ax.errorbar(g[x], g["mean"], yerr=g["std"], marker="o", ms=3, capsize=2, label=v)
    ax.set_xlabel(x)
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    if logx: ax.set_xscale("log")
    if logy: ax.set_yscale("log")
    ax.grid(alpha=.3)
    ax.legend(fontsize=7)
    fig.tight_layout()
    fig.savefig(out / fname, dpi=140)
    plt.close(fig)


density = load("density")
if density is not None:
    d = prep(density)
    d["param_value"] = d["param_value"].astype(float)
    table(d, "param_value", ["expanded", "time_ms", "alloc_MiB"], "Grid obstacle density (4-connected)")
    for y, yl, f in [("expanded", "nodes expanded", "density_expanded.png"),
                     ("time_ms", "runtime [ms]", "density_time.png"),
                     ("alloc_MiB", "peak extra heap memory [MiB]", "density_memory.png")]:
        lineplot(d, "param_value", y, f"Obstacle density vs {yl}", f, yl)

size = load("size")
if size is not None:
    s = prep(size)
    s["param_value"] = s["param_value"].astype(float)
    s["nodes_k"] = s["nodes"]
    table(s, "param_value", ["expanded", "time_ms", "alloc_MiB"], "Grid side length (20% obstacles)")
    lineplot(s, "param_value", "time_ms", "Grid size vs runtime", "size_time.png", "runtime [ms]", True, True)
    lineplot(s, "param_value", "expanded", "Grid size vs nodes expanded", "size_expanded.png", "nodes expanded", True, True)

graph = load("graph")
if graph is not None:
    g = prep(graph)
    g["param_value"] = g["param_value"].astype(float)
    table(g, "param_value", ["time_ms", "alloc_MiB", "expanded"], "Sparse random graphs, SSSP")
    lineplot(g, "param_value", "time_ms", "Random graph size vs runtime", "graph_time.png", "runtime [ms]", True, True)

heaps = load("heaps")
if heaps is not None:
    h = prep(heaps)
    h["param_value"] = h["param_value"].astype(float)
    a = h[h["family"] == "random_graph"]
    if len(a):
        table(a, "param_value", ["time_ms", "generated", "stale_pops", "peak_open", "open_MiB"],
              "Heap comparison: Dijkstra SSSP on random graphs (param = average degree)")
        base = a[a["heap"] == "std_pq"].groupby("param_value")["time_ms"].mean()
        a2 = a.copy()
        a2["speedup_vs_std_pq"] = a2.apply(lambda r: base[r["param_value"]] / r["time_ms"], axis=1)
        fig, ax = plt.subplots(figsize=(7, 4.2))
        for v, dd in a2.groupby("heap"):
            gg = dd.groupby("param_value")["speedup_vs_std_pq"].mean()
            ax.plot(gg.index, gg.values, marker="o", label=v)
        ax.axhline(1, color="k", lw=.7)
        ax.set_xlabel("average degree"); ax.set_ylabel("speed-up vs std::priority_queue (>1 = faster)")
        ax.set_title("Dijkstra heaps on sparse random graphs"); ax.grid(alpha=.3); ax.legend()
        fig.tight_layout(); fig.savefig(out / "heaps_graph_speedup.png", dpi=140); plt.close(fig)
    b = h[h["family"] == "grid"]
    if len(b):
        table(b, "param_value", ["time_ms", "expanded", "generated", "stale_pops", "peak_open", "open_MiB"],
              "Heap comparison: 8-connected grid (param = side length)")

dimacs = load("dimacs")
if dimacs is not None:
    x = prep(dimacs)
    table(x, "param_value", ["time_ms", "alloc_MiB", "generated", "stale_pops"], "DIMACS road network, SSSP")

(out / "summary.md").write_text("\n".join(md))
print(f"wrote {out/'summary.md'} and PNG plots")
