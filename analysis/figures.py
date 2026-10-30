"""Publication figures for the crossover atlas.

Usage:
    python analysis/figures.py results/<run>/analysis --out figures/ [--sysinfo results/<run>/sysinfo.txt]

Reads cells.csv and crossovers.csv written by atlas.py. Every figure is
written as PDF (vector, for the paper) and PNG (for the README).
"""
from __future__ import annotations

import argparse
import os
import re

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter, LogLocator, NullFormatter
import numpy as np
import pandas as pd

# Okabe–Ito palette (colour-vision-deficiency safe). Colour follows the
# algorithm identity everywhere, never the subplot or rank.
COLORS = {
    "radix": "#D55E00", "introsort": "#0072B2",
    "strassen": "#D55E00", "naive_ikj": "#0072B2", "naive_ijk": "#999999",
    "karatsuba": "#D55E00", "schoolbook": "#0072B2",
    "fib_heap": "#D55E00", "binary_heap": "#0072B2", "lazy_heap": "#009E73",
}
LABELS = {
    "radix": "LSD radix sort", "introsort": "introsort (std::sort)",
    "strassen": "Strassen", "naive_ikj": "naive (i-k-j)", "naive_ijk": "naive (i-j-k)",
    "karatsuba": "Karatsuba", "schoolbook": "schoolbook",
    "fib_heap": "Fibonacci heap", "binary_heap": "binary heap (decrease-key)",
    "lazy_heap": "binary heap (lazy)",
}
XLABEL = {"sorting": "n (keys)", "matmul": "n (matrix dimension)",
          "bigint": "n (32-bit limbs per operand)", "dijkstra": "n (vertices)"}
TITLE = {"sorting": "Sorting: radix vs introsort",
         "matmul": "Matrix multiplication: Strassen vs naive",
         "bigint": "Integer multiplication: Karatsuba vs schoolbook",
         "dijkstra": "Dijkstra: Fibonacci heap vs binary heap"}

# Working-set size in bytes as a function of n, used to draw cache-boundary
# guides. Sorting: keys + temp buffer; matmul: three n×n doubles; bigint: two
# operands + product; dijkstra: CSR arrays + dist + heap (degree-dependent,
# approximated for the default degree 4).
def working_set_bytes(pair: str, n: np.ndarray, degree: int = 4) -> np.ndarray:
    n = np.asarray(n, float)
    if pair == "sorting":
        return 16 * n
    if pair == "matmul":
        return 24 * n * n
    if pair == "bigint":
        return 16 * n
    if pair == "dijkstra":
        return n * (8 + 8 + 8 + 1 + 16) + degree * n * 8
    return 8 * n


def parse_caches(sysinfo_path: str | None) -> dict[str, int]:
    """Extract L1d / L2 (per core) / L3 sizes in bytes from an lscpu dump."""
    caches = {"L1d": 48 * 1024, "L2": 2 * 1024 * 1024, "L3": 30 * 1024 * 1024}
    if not sysinfo_path or not os.path.exists(sysinfo_path):
        return caches
    txt = open(sysinfo_path, errors="ignore").read()
    for level in ("L1d", "L2", "L3"):
        m = re.search(rf"{level} cache:\s+([\d.]+)\s*([KMG])iB\s*\((\d+) instance", txt)
        if m:
            size = float(m.group(1)) * {"K": 1024, "M": 1024**2, "G": 1024**3}[m.group(2)]
            inst = int(m.group(3))
            caches[level] = int(size / inst) if level != "L3" else int(size)
    return caches


def cache_lines(ax, pair: str, caches: dict[str, int], n_range: tuple[float, float]):
    """Vertical guides where the working set equals each cache level."""
    ns = np.logspace(np.log10(n_range[0]), np.log10(n_range[1]), 2000)
    ws = working_set_bytes(pair, ns)
    for level, size in caches.items():
        idx = np.searchsorted(ws, size)
        if 0 < idx < len(ns):
            ax.axvline(ns[idx], color="#BBBBBB", lw=0.8, ls=":", zorder=0)
            ax.text(ns[idx], ax.get_ylim()[1], f" {level}", fontsize=7, color="#888888",
                    va="top", ha="left")


def style():
    plt.rcParams.update({
        "font.size": 9, "axes.titlesize": 9.5, "axes.labelsize": 9, "legend.fontsize": 8,
        "axes.spines.top": False, "axes.spines.right": False,
        "axes.grid": True, "grid.color": "#E6E6E6", "grid.linewidth": 0.6,
        "lines.linewidth": 1.6, "lines.markersize": 3.5,
        "figure.dpi": 150, "savefig.bbox": "tight",
    })


def save(fig, out, name):
    fig.savefig(os.path.join(out, name + ".pdf"))
    fig.savefig(os.path.join(out, name + ".png"), dpi=200)
    plt.close(fig)


def default_param(cells: pd.DataFrame, pair: str) -> str:
    sub = cells[(cells["pair"] == pair) & (cells["param"] != "-")]
    if sub.empty:
        return "-"
    # the most frequently measured parameter is the default sweep
    return sub["param"].value_counts().idxmax()


# ---------------------------------------------------------------------------
def fig_runtime_small_multiples(cells, pair, out, caches):
    sub = cells[cells["pair"] == pair]
    if sub.empty:
        return
    p = default_param(cells, pair)
    sub = sub[sub["param"].isin([p, "-"])]
    dists = list(dict.fromkeys(sub["dist"]))
    ncol = len(dists) if len(dists) <= 5 else 4
    nrow = int(np.ceil(len(dists) / ncol))
    fig, axes = plt.subplots(nrow, ncol, figsize=(3.2 * ncol, 2.6 * nrow), squeeze=False, sharey=True)
    for ax, dist in zip(axes.flat, dists):
        d = sub[sub["dist"] == dist]
        for algo, g in d.groupby("algorithm"):
            g = g.sort_values("n")
            ax.plot(g["n"], g["median"] / 1e6, "-o", color=COLORS.get(algo, "k"),
                    label=LABELS.get(algo, algo))
            ax.fill_between(g["n"], g["q25"] / 1e6, g["q75"] / 1e6, color=COLORS.get(algo, "k"),
                            alpha=0.15, lw=0)
        ax.set_xscale("log"); ax.set_yscale("log")
        ax.set_title(dist)
        ax.set_xlabel(XLABEL[pair])
        cache_lines(ax, pair, caches, (d["n"].min(), d["n"].max()))
    for ax in axes.flat[len(dists):]:
        ax.axis("off")
    axes[0, 0].set_ylabel("median time (ms)")
    handles, labels = axes[0, 0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=len(labels), bbox_to_anchor=(0.5, -0.08),
               frameon=False)
    fig.suptitle(TITLE[pair] + (f"  (param = {p})" if p != "-" else ""), y=1.02)
    fig.tight_layout()
    save(fig, out, f"fig_runtime_{pair}")


def fig_ratio(cells, cross, pair, out, by="dist"):
    """Ratio t_fast / t_simple vs n; one line per distribution (or per param)."""
    sub = cells[cells["pair"] == pair]
    if sub.empty:
        return
    fast, simple = {"sorting": ("radix", "introsort"), "matmul": ("strassen", "naive_ikj"),
                    "bigint": ("karatsuba", "schoolbook"), "dijkstra": ("fib_heap", "binary_heap")}[pair]
    p = default_param(cells, pair)
    if by == "dist":
        groups = [(d, sub[(sub["dist"] == d) & (sub["param"].isin([p, "-"]))]) for d in dict.fromkeys(sub["dist"])]
        title_extra = f" (param = {p})" if p != "-" else ""
    else:
        d0 = [d for d in dict.fromkeys(sub["dist"])][0]
        groups = [(f"param={q}", sub[(sub["dist"] == d0) & (sub["param"].isin([q, "-"]))])
                  for q in dict.fromkeys(sub[sub["param"] != "-"]["param"])]
        title_extra = f" ({d0})"
    fig, ax = plt.subplots(figsize=(5.2, 3.2))
    # Colour by group identity: input families get fixed distinct hues
    # (Okabe-Ito); uniform key widths form a single-hue blue ramp ordered by
    # width; numeric parameters a viridis ramp ordered by value.
    FAMILY = {"nearly_sorted": "#D55E00", "reverse": "#CC79A7", "organ_pipe": "#E69F00",
              "few_unique": "#009E73", "grid": "#D55E00", "dense": "#E69F00", "random": "#0072B2",
              "integer": "#009E73", "uniform": "#0072B2", "ones": "#D55E00"}
    BLUES = {"uniform8": "#9ECAE1", "uniform16": "#6BAED6", "uniform32": "#3182BD", "uniform64": "#08519C"}
    cmap = plt.get_cmap("viridis")
    def colour(i, name):
        if name in BLUES: return BLUES[name]
        if name in FAMILY: return FAMILY[name]
        return cmap(i / max(1, len(groups) - 1) * 0.85)
    for i, (name, g) in enumerate(groups):
        f = g[g["algorithm"] == fast].set_index("n")["median"]
        s = g[g["algorithm"] == simple].set_index("n")["median"]
        ns = sorted(set(f.index) & set(s.index))
        if not ns:
            continue
        ratio = np.array([f[n] / s[n] for n in ns])
        col = colour(i, name)
        ax.plot(ns, ratio, "-o", color=col, label=name.replace("_", " "))
        c = cross[(cross["pair"] == pair) & (cross["fast"] == fast) & (cross["simple"] == simple)]
        c = c[(c["dist"].str.replace(r" \(.*\)$", "", regex=True) == name) if by == "dist"
              else (c["param"] == name.split("=")[1])]
        if pair == "matmul":
            c = c[c["dist"].str.contains("all n")]
        if not c.empty and c.iloc[0]["status"] == "crossed":
            r = c.iloc[0]
            ax.plot([r["n_star"]], [1.0], marker="v", color=col, ms=7, zorder=5)
            if pd.notna(r["ci_lo"]):
                ax.plot([r["ci_lo"], r["ci_hi"]], [1.0, 1.0], color=col, lw=3, alpha=0.4, solid_capstyle="butt")
    ax.axhline(1.0, color="#444444", lw=0.8)
    ax.set_xscale("log"); ax.set_yscale("log")
    ax.yaxis.set_major_locator(LogLocator(base=10, subs=(1, 2, 3, 5)))
    ax.yaxis.set_minor_formatter(NullFormatter())
    ax.yaxis.set_major_formatter(FuncFormatter(lambda v, _: f"{v:g}"))
    ax.set_xlabel(XLABEL[pair])
    ax.set_ylabel(f"t({LABELS[fast]}) / t({LABELS[simple]})")
    ax.set_title(TITLE[pair] + title_extra)
    ax.legend(frameon=False, ncol=2)
    ax.text(0.99, 0.02, "below 1: faster-in-theory wins;  ▼ crossover (bar: 95% CI)",
            transform=ax.transAxes, fontsize=7, color="#666666", ha="right")
    save(fig, out, f"fig_ratio_{pair}_by_{by}")


def fig_heatmap(cells, pair, out):
    """log2 speed ratio over (n, cutoff) for the two recursive pairs."""
    sub = cells[cells["pair"] == pair]
    fast, simple = {"matmul": ("strassen", "naive_ikj"), "bigint": ("karatsuba", "schoolbook")}[pair]
    sub = sub[sub["dist"] == list(dict.fromkeys(sub["dist"]))[0]] if not sub.empty else sub
    params = sorted([p for p in sub["param"].unique() if p != "-"], key=lambda x: float(x))
    if len(params) < 2:
        return
    s = sub[sub["algorithm"] == simple].set_index("n")["median"]
    ns = sorted(set(s.index))
    M = np.full((len(params), len(ns)), np.nan)
    for i, p in enumerate(params):
        f = sub[(sub["algorithm"] == fast) & (sub["param"] == p)].set_index("n")["median"]
        for j, n in enumerate(ns):
            if n in f.index:
                M[i, j] = np.log2(f[n] / s[n])
    fig, ax = plt.subplots(figsize=(6.0, 0.45 * len(params) + 1.6))
    vmax = np.nanmax(np.abs(M))
    im = ax.imshow(M, aspect="auto", cmap="RdBu_r", vmin=-vmax, vmax=vmax, origin="lower")
    ax.set_xticks(range(len(ns)))
    ax.set_xticklabels([f"{n:g}" for n in ns], rotation=90, fontsize=7)
    ax.set_yticks(range(len(params)))
    ax.set_yticklabels(params)
    ax.set_xlabel(XLABEL[pair])
    ax.set_ylabel("base-case cutoff")
    ax.grid(False)
    cb = fig.colorbar(im, ax=ax, fraction=0.03, pad=0.02)
    cb.set_label(f"log2  t({LABELS[fast]}) / t({LABELS[simple]})")
    ax.set_title(f"{TITLE[pair]}: blue = {LABELS[fast]} wins")
    save(fig, out, f"fig_heatmap_{pair}")


def fig_counters(cells, pair, out):
    """Hardware-counter view: L1d / LLC misses and branch mispredictions per
    element vs n, for the default parameter and first distribution."""
    cols = [c for c in ("l1d_misses_per_elem", "llc_misses_per_elem", "branch_misses_per_elem")
            if c in cells.columns and cells[c].notna().any()]
    if not cols:
        return
    sub = cells[cells["pair"] == pair]
    if sub.empty:
        return
    p = default_param(cells, pair)
    d0 = list(dict.fromkeys(sub["dist"]))[0]
    sub = sub[(sub["dist"] == d0) & (sub["param"].isin([p, "-"]))]
    fig, axes = plt.subplots(1, len(cols), figsize=(3.3 * len(cols), 2.8), squeeze=False)
    for ax, c in zip(axes[0], cols):
        for algo, g in sub.groupby("algorithm"):
            g = g.sort_values("n")
            ax.plot(g["n"], g[c], "-o", color=COLORS.get(algo, "k"), label=LABELS.get(algo, algo))
        ax.set_xscale("log"); ax.set_yscale("log")
        ax.set_xlabel(XLABEL[pair])
        ax.set_title(c.replace("_per_elem", " / element").replace("_", " "))
    axes[0, 0].legend(frameon=False)
    fig.suptitle(f"{TITLE[pair]} — hardware counters ({d0})", y=1.03)
    fig.tight_layout()
    save(fig, out, f"fig_counters_{pair}")


DEFAULT_PARAM = {"sorting": "11", "matmul": "32", "bigint": "32", "dijkstra": None}


def _table_rows(cross: pd.DataFrame):
    def fmt(x):
        if x is None or pd.isna(x):
            return "--"
        x = float(x)
        if x >= 1e6: return f"{x/1e6:.1f}\\,M"
        if x >= 1e3: return f"{x/1e3:.1f}\\,k"
        return f"{x:.0f}"
    rows = []
    for r in cross.itertuples():
        ci = f"[{fmt(r.ci_lo)}, {fmt(r.ci_hi)}]" if pd.notna(r.ci_lo) else "--"
        prec = getattr(r, "precision", "-")
        if r.status == "crossed" and prec == "marginal":
            nstar = f"$\\approx${fmt(r.n_star)}"
            outcome = "parity only"
        elif r.status == "crossed" and prec == "gradual":
            nstar = f"{fmt(r.ci_lo)}--{fmt(r.ci_hi)}"
            outcome = "crosses (gradual)"
        elif r.status == "crossed":
            nstar = fmt(r.n_star)
            outcome = "crosses"
        elif r.status == "always_faster":
            nstar = f"$\\le {fmt(r.n_min)}$"
            outcome = "faster at every $n$"
        else:
            nstar = "--"
            outcome = f"never faster to {fmt(r.n_max)}"
        comp = f"{LABELS.get(r.fast, r.fast)} vs {LABELS.get(r.simple, r.simple)}"
        dist = r.dist.replace("_", " ").replace("(n=2^k,3*2^k)", "($2^k, 3\\cdot2^k$)").replace("(all n)", "(all $n$)")
        rows.append(f"{r.pair} & {dist} & {r.param} & {comp} & {nstar} & {ci} & {outcome} & {r.ratio_at_nmax:.2f} \\\\")
    return rows


def table_latex(cross: pd.DataFrame, out):
    header = ["\\toprule",
              "Pair & Input & Param & Comparison & $n^*$ & 95\\% CI & Outcome & $\\rho(n_{\\max})$ \\\\",
              "\\midrule"]
    # Main table: the default parameter of each pair.
    sel = cross[[(DEFAULT_PARAM.get(p) is None or str(q) == DEFAULT_PARAM[p])
                 for p, q in zip(cross["pair"], cross["param"])]]
    lines = ["\\begin{tabular}{llllrrlr}"] + header + _table_rows(sel) + ["\\bottomrule", "\\end{tabular}"]
    with open(os.path.join(out, "table_crossovers.tex"), "w") as f:
        f.write("\n".join(lines) + "\n")
    # Appendix: every row, as a longtable.
    lines = ["\\begin{longtable}{llllrrlr}"] + header + ["\\endhead"] + _table_rows(cross) + \
            ["\\bottomrule", "\\end{longtable}"]
    with open(os.path.join(out, "table_crossovers_full.tex"), "w") as f:
        f.write("\n".join(lines) + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("analysis_dir")
    ap.add_argument("--out", default="figures")
    ap.add_argument("--sysinfo", default=None)
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    style()
    cells = pd.read_csv(os.path.join(args.analysis_dir, "cells.csv"), dtype={"param": str})
    cross = pd.read_csv(os.path.join(args.analysis_dir, "crossovers.csv"), dtype={"param": str})
    cells["param"] = cells["param"].fillna("-")
    cross["param"] = cross["param"].fillna("-")
    caches = parse_caches(args.sysinfo)
    for pair in ("sorting", "matmul", "bigint", "dijkstra"):
        fig_runtime_small_multiples(cells, pair, args.out, caches)
        fig_ratio(cells, cross, pair, args.out, by="dist")
        if pair in ("matmul", "bigint", "dijkstra"):
            fig_ratio(cells, cross, pair, args.out, by="param")
        if pair in ("matmul", "bigint"):
            fig_heatmap(cells, pair, args.out)
        fig_counters(cells, pair, args.out)
    table_latex(cross, args.out)
    print(f"figures written to {args.out}/")


if __name__ == "__main__":
    main()
