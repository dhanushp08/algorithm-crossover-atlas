"""Crossover analysis for the algorithm-crossover atlas.

Reads the CSV files produced by the C++ benchmark binaries, computes per-cell
medians, locates crossover points (the smallest n at which the asymptotically
faster algorithm's median runtime drops below the simpler algorithm's), and
attaches bootstrap confidence intervals. Also fits empirical scaling exponents
and summarises hardware counters when present.

Usage:
    python analysis/atlas.py results/<run>/   [--out results/<run>/analysis]

Produces in --out:
    cells.csv       one row per (pair, dist, param, algorithm, n): median, IQR,
                    counters per element
    crossovers.csv  one row per (pair, dist, param, fast, simple): n*, CI, status
    exponents.csv   fitted t = c * n^b over the largest decade, per algorithm
    summary.md      human-readable tables
"""
from __future__ import annotations

import argparse
import glob
import os
import sys
from dataclasses import dataclass

import numpy as np
import pandas as pd

# Which algorithm is "theoretically faster" vs "simpler" in each pair. A pair
# may declare several comparisons; each becomes a row in crossovers.csv.
COMPARISONS: dict[str, list] = {
    "sorting": [("radix", "introsort")],
    # Strassen's padding path makes its cost depend on how n factorises, so
    # the matrix pair is reported twice: on sizes with many factors of two
    # (2^k and 3*2^k, the usual benchmark grid) and on all measured sizes.
    "matmul": [("strassen", "naive_ikj", "n=2^k,3*2^k", lambda n: n % 32 == 0),
               ("strassen", "naive_ikj", "all n", None),
               ("strassen", "naive_ijk", "n=2^k,3*2^k", lambda n: n % 32 == 0),
               ("strassen", "naive_ijk", "all n", None)],
    "bigint": [("karatsuba", "schoolbook")],
    "dijkstra": [("fib_heap", "binary_heap"), ("fib_heap", "lazy_heap")],
}

# Pairs whose parameter is a base-case cutoff below which fast == simple.
CUTOFF_PAIRS = {"matmul", "bigint"}

COUNTER_COLS = ["cycles", "instructions", "l1d_misses", "llc_misses", "branch_misses"]

# Elements processed per input, for "per element" normalisation of counters.
def elements(pair: str, n: int) -> float:
    if pair == "sorting":
        return float(n)
    if pair == "matmul":
        return float(n) ** 3  # multiply-adds for the naive algorithm
    if pair == "bigint":
        return float(n) ** 2  # limb products for schoolbook
    if pair == "dijkstra":
        return float(n)  # vertices (edges = n * degree)
    return float(n)


def load_run(run_dirs, supersede=None) -> pd.DataFrame:
    """Load one run directory or pool several. Pooled runs are treated as
    additional repetitions of every cell (rep numbers are offset per run) and
    tagged with a `run` column so that run-to-run drift can be inspected.
    `supersede` names the run directories whose cells *replace* (rather than
    pool with) the same series over the range of n they cover in the other
    runs; used for re-measurements after a harness fix."""
    if isinstance(run_dirs, str):
        run_dirs = [run_dirs]
    frames = []
    for run_dir in run_dirs:
        files = sorted(glob.glob(os.path.join(run_dir, "*.csv")))
        files = [f for f in files if os.path.basename(f) != "cells.csv"]
        if not files:
            sys.exit(f"no CSV files in {run_dir}")
        for f in files:
            df = pd.read_csv(f, dtype={"param": str})
            if df.empty:
                continue
            df["run"] = os.path.basename(os.path.normpath(run_dir))
            frames.append(df)
    df = pd.concat(frames, ignore_index=True)
    sup = {os.path.basename(os.path.normpath(r)) for r in (supersede or [])}
    if sup:
        # A superseding run replaces the other runs' cells over the *range of
        # n* it covers for each (pair, dist, param, algorithm) series, so that
        # re-measured series are not interleaved with stale cells on a
        # different grid.
        key = ["pair", "dist", "param", "algorithm"]
        covered: dict[tuple, list[tuple[float, float]]] = {}
        for run in sup:
            for k, g in df[df["run"] == run].groupby(key):
                covered.setdefault(tuple(map(str, k)), []).append((g["n"].min(), g["n"].max()))
        others = df[~df["run"].isin(sup)]
        mask = np.ones(len(others), dtype=bool)
        for i, (k, n) in enumerate(zip(map(tuple, others[key].astype(str).values.tolist()), others["n"])):
            for lo, hi in covered.get(k, []):
                if lo <= n <= hi:
                    mask[i] = False
                    break
        df = pd.concat([others[mask], df[df["run"].isin(sup)]], ignore_index=True)
    if len(run_dirs) > 1:
        # offset rep numbers so pooled repetitions stay distinct
        offsets = {r: i * 1000 for i, r in enumerate(dict.fromkeys(df["run"]))}
        df["rep"] = df["rep"] + df["run"].map(offsets)
    df["param"] = df["param"].fillna("-").astype(str)
    df["n"] = df["n"].astype(np.int64)
    df["time_ns"] = df["time_ns"].astype(np.float64)
    # The harness times (copy input + run) per iteration and separately
    # reports the copy alone; subtract it, but never below 5% of the raw time
    # so that noise in the baseline cannot produce zero or negative runtimes.
    if "copy_ns" in df.columns:
        df["time_raw_ns"] = df["time_ns"]
        df["copy_ns"] = pd.to_numeric(df["copy_ns"], errors="coerce").fillna(0.0)
        df["time_ns"] = np.maximum(df["time_ns"] - df["copy_ns"], 0.05 * df["time_ns"])
    for c in COUNTER_COLS:
        if c in df.columns:
            df[c] = pd.to_numeric(df[c], errors="coerce")
    return df


def cell_stats(df: pd.DataFrame) -> pd.DataFrame:
    g = df.groupby(["pair", "dist", "param", "algorithm", "n"], sort=True)
    out = g["time_ns"].agg(median="median", q25=lambda s: s.quantile(0.25),
                           q75=lambda s: s.quantile(0.75), reps="count").reset_index()
    for c in COUNTER_COLS:
        if c in df.columns and df[c].notna().any():
            med = g[c].median().reset_index(name=c)
            out = out.merge(med, on=["pair", "dist", "param", "algorithm", "n"], how="left")
            out[c + "_per_elem"] = out[c] / out.apply(lambda r: elements(r["pair"], r["n"]), axis=1)
    out["ns_per_elem"] = out["median"] / out.apply(lambda r: elements(r["pair"], r["n"]), axis=1)
    return out


# ---------------------------------------------------------------------------
# Crossover detection
# ---------------------------------------------------------------------------
@dataclass
class Crossover:
    n_star: float | None     # interpolated crossover size, None if no crossing
    status: str              # "crossed", "always_faster", "never_faster"
    n_min: int
    n_max: int


def find_crossover(ns: np.ndarray, ratio: np.ndarray) -> Crossover:
    """ratio = t_fast / t_simple at each n (sorted ascending).

    Returns the *sustained* crossover: the size beyond which ratio < 1 at
    every larger measured n, interpolated in log-log space between the last
    grid point at or above 1 and the next one. A transient dip below 1 that
    is followed by a return above 1 is not a crossover (it shows up in the
    ratio plots, and min_ratio records it). If the series ends above 1 the
    status is never_faster; if it is below 1 everywhere, always_faster.
    """
    ns = np.asarray(ns, dtype=float)
    ratio = np.asarray(ratio, dtype=float)
    mask = np.isfinite(ratio) & (ratio > 0)
    ns, ratio = ns[mask], ratio[mask]
    if len(ns) == 0:
        return Crossover(None, "no_data", 0, 0)
    n_min, n_max = int(ns[0]), int(ns[-1])
    above = ratio >= 1.0
    if not above.any():
        return Crossover(float(ns[0]), "always_faster", n_min, n_max)
    last_above = int(np.max(np.nonzero(above)[0]))
    if last_above == len(ns) - 1:
        return Crossover(None, "never_faster", n_min, n_max)
    idx = last_above + 1
    x0, x1 = np.log(ns[idx - 1]), np.log(ns[idx])
    y0, y1 = np.log(ratio[idx - 1]), np.log(ratio[idx])
    if y1 == y0:
        return Crossover(float(ns[idx]), "crossed", n_min, n_max)
    x = x0 + (0.0 - y0) * (x1 - x0) / (y1 - y0)
    return Crossover(float(np.exp(x)), "crossed", n_min, n_max)


def bootstrap_crossover(fast: dict[int, np.ndarray], simple: dict[int, np.ndarray],
                        B: int = 1000, seed: int = 0) -> tuple[Crossover, tuple[float, float] | None,
                                                               np.ndarray, np.ndarray]:
    """fast/simple map n -> array of repetition times. Resamples repetitions
    with replacement within each cell, recomputes medians and the crossover.
    Returns (point estimate, (lo, hi) CI or None, ns, point-estimate ratio)."""
    rng = np.random.default_rng(seed)
    ns = np.array(sorted(set(fast) & set(simple)))
    if len(ns) == 0:
        return Crossover(None, "no_data", 0, 0), None, ns, np.array([])
    med_f = np.array([np.median(fast[n]) for n in ns])
    med_s = np.array([np.median(simple[n]) for n in ns])
    point = find_crossover(ns, med_f / med_s)
    if point.status != "crossed":
        return point, None, ns, med_f / med_s
    samples = []
    for _ in range(B):
        rf = np.array([np.median(rng.choice(fast[n], size=len(fast[n]))) for n in ns])
        rs = np.array([np.median(rng.choice(simple[n], size=len(simple[n]))) for n in ns])
        c = find_crossover(ns, rf / rs)
        if c.n_star is not None:
            samples.append(c.n_star)
    if len(samples) < B // 2:
        return point, None, ns, med_f / med_s
    lo, hi = shortest_interval(np.array(samples), 0.95)
    return point, (float(lo), float(hi)), ns, med_f / med_s


def shortest_interval(samples: np.ndarray, mass: float = 0.95) -> tuple[float, float]:
    """Shortest interval (in log n) containing `mass` of the bootstrap
    samples — a highest-density interval. Preferred over the central
    percentile interval because the sustained-crossover statistic can be
    bimodal: a small fraction of resamples in which one noisy mid-range cell
    pops above ratio 1 relocates the crossing to the far end of the sweep,
    and a percentile interval would then stretch over that tail even when
    95% of the mass sits tightly around the true crossing."""
    x = np.sort(np.log(samples))
    k = int(np.ceil(mass * len(x)))
    if k >= len(x):
        return float(np.exp(x[0])), float(np.exp(x[-1]))
    widths = x[k - 1:] - x[:len(x) - k + 1]
    i = int(np.argmin(widths))
    return float(np.exp(x[i])), float(np.exp(x[i + k - 1]))


def crossovers(df: pd.DataFrame, B: int = 1000) -> pd.DataFrame:
    rows = []
    for pair, comps in COMPARISONS.items():
        sub = df[df["pair"] == pair]
        if sub.empty:
            continue
        for comp in comps:
            fast_name, simple_name = comp[0], comp[1]
            view = comp[2] if len(comp) > 2 else ""
            size_filter = comp[3] if len(comp) > 3 else None
            for (dist, param), cell in sub.groupby(["dist", "param"]):
                f = cell[cell["algorithm"] == fast_name]
                # the simple algorithm may be recorded under param "-" (param-insensitive)
                s = sub[(sub["algorithm"] == simple_name) & (sub["dist"] == dist)
                        & (sub["param"].isin([param, "-"]))]
                if f.empty or s.empty:
                    continue
                fast = {int(n): g["time_ns"].to_numpy() for n, g in f.groupby("n")}
                simple = {int(n): g["time_ns"].to_numpy() for n, g in s.groupby("n")}
                if size_filter is not None:
                    fast = {n: v for n, v in fast.items() if size_filter(n)}
                    simple = {n: v for n, v in simple.items() if size_filter(n)}
                # For the recursive pairs the "fast" algorithm *is* the simple
                # kernel whenever n <= cutoff, so the two are identical there and
                # any apparent crossing below the cutoff is noise. The crossover
                # is therefore searched over n >= cutoff only.
                if pair in CUTOFF_PAIRS and param not in ("-", ""):
                    try:
                        cutoff = float(param)
                        fast = {n: v for n, v in fast.items() if n >= cutoff}
                        simple = {n: v for n, v in simple.items() if n >= cutoff}
                    except ValueError:
                        pass
                point, ci, ns, ratio = bootstrap_crossover(fast, simple, B=B)
                # Precision class: an interval narrower than one grid step
                # (the ratio of consecutive sizes) is "sharp"; one spanning
                # more than a grid step means the ratio curve approaches 1
                # slowly and the crossover is a zone, not a point.
                grid_step = float(np.median(ns[1:] / ns[:-1])) if len(ns) > 1 else 2.0
                precision = "-"
                if point.status == "crossed":
                    if ci is None:
                        precision = "unstable"
                    else:
                        # sharp = interval narrower than one grid step or than
                        # a factor 1.5, whichever is wider (a +-22% interval is
                        # sharp by any practical standard even on a fine grid)
                        precision = "sharp" if ci[1] / max(ci[0], 1) <= max(grid_step, 1.5) else "gradual"
                    # a crossing that never gets more than 10% ahead by the top
                    # of the sweep is parity, not a win
                    if len(ratio) and float(ratio[-1]) > 0.9:
                        precision = "marginal"
                rows.append({
                    "pair": pair, "dist": dist + (f" ({view})" if view else ""), "param": param,
                    "fast": fast_name, "simple": simple_name,
                    "status": point.status,
                    "precision": precision,
                    "grid_step": round(grid_step, 2),
                    "n_star": point.n_star,
                    "ci_lo": ci[0] if ci else None,
                    "ci_hi": ci[1] if ci else None,
                    "n_min": point.n_min, "n_max": point.n_max,
                    "ratio_at_nmax": float(ratio[-1]) if len(ratio) else None,
                    "min_ratio": float(np.nanmin(ratio)) if len(ratio) else None,
                })
    return pd.DataFrame(rows)


# ---------------------------------------------------------------------------
# Empirical exponents
# ---------------------------------------------------------------------------
def exponents(cells: pd.DataFrame) -> pd.DataFrame:
    """Fit log t = log c + b log n over the largest decade of n for each
    (pair, dist, param, algorithm). Reports b and the local slope between the
    two largest sizes."""
    rows = []
    for key, g in cells.groupby(["pair", "dist", "param", "algorithm"]):
        g = g.sort_values("n")
        if len(g) < 3:
            continue
        n = g["n"].to_numpy(float)
        t = g["median"].to_numpy(float)
        top = n >= n[-1] / 10.0
        if top.sum() < 3:  # coarse grid: use the three largest sizes instead
            top = np.zeros_like(n, dtype=bool)
            top[-3:] = True
        b, logc = np.polyfit(np.log(n[top]), np.log(t[top]), 1)
        local = (np.log(t[-1]) - np.log(t[-2])) / (np.log(n[-1]) - np.log(n[-2]))
        rows.append({"pair": key[0], "dist": key[1], "param": key[2], "algorithm": key[3],
                     "fit_exponent": b, "fit_n_min": int(n[top][0]), "fit_n_max": int(n[-1]),
                     "local_slope_top": local})
    return pd.DataFrame(rows)


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------
def fmt_n(x) -> str:
    if x is None or (isinstance(x, float) and not np.isfinite(x)):
        return "-"
    x = float(x)
    if x >= 1e6:
        return f"{x/1e6:.3g}M"
    if x >= 1e3:
        return f"{x/1e3:.3g}k"
    return f"{x:.0f}"


def drift_report(df: pd.DataFrame) -> pd.DataFrame:
    """Per-run medians of each cell, and the spread across runs (max/min) —
    only meaningful when several runs are pooled."""
    if "run" not in df.columns or df["run"].nunique() < 2:
        return pd.DataFrame()
    per_run = df.groupby(["pair", "dist", "param", "algorithm", "n", "run"])["time_ns"].median().reset_index()
    spread = per_run.groupby(["pair", "dist", "param", "algorithm", "n"])["time_ns"].agg(
        runs="count", run_min="min", run_max="max")
    spread["run_spread"] = spread["run_max"] / spread["run_min"]
    return spread.reset_index()


def summary_markdown(cross: pd.DataFrame, expo: pd.DataFrame, counters_present: bool,
                     drift: pd.DataFrame | None = None) -> str:
    lines = ["# Crossover summary", ""]
    lines.append("| pair | distribution | param | fast vs simple | crossover n* | 95% CI | precision | status | ratio at n_max |")
    lines.append("|---|---|---|---|---|---|---|---|---|")
    for r in cross.itertuples():
        ci = f"[{fmt_n(r.ci_lo)}, {fmt_n(r.ci_hi)}]" if r.ci_lo is not None and not pd.isna(r.ci_lo) else "-"
        lines.append(f"| {r.pair} | {r.dist} | {r.param} | {r.fast} vs {r.simple} | {fmt_n(r.n_star)} | {ci} | {r.precision} | {r.status} | {r.ratio_at_nmax:.2f} |")
    lines += ["", "precision: *sharp* = interval narrower than one grid step (or a factor 1.5); *gradual* = wider, "
              "i.e. the two algorithms are within noise of each other across the interval and the "
              "crossover is a zone rather than a point; *marginal* = crosses but is never more than 10% "
              "ahead by the largest size measured (parity, not a win)."]
    lines += ["", "## Empirical scaling exponents (fit over largest decade)", ""]
    lines.append("| pair | distribution | param | algorithm | exponent | fit range |")
    lines.append("|---|---|---|---|---|---|")
    for r in expo.itertuples():
        lines.append(f"| {r.pair} | {r.dist} | {r.param} | {r.algorithm} | {r.fit_exponent:.2f} | {fmt_n(r.fit_n_min)}–{fmt_n(r.fit_n_max)} |")
    if drift is not None and not drift.empty:
        lines += ["", "## Run-to-run drift (pooled runs)", "",
                  f"Cells: {len(drift)}; runs per cell: {int(drift['runs'].median())}; "
                  f"median max/min spread of per-run medians: {drift['run_spread'].median():.3f}; "
                  f"90th percentile: {drift['run_spread'].quantile(0.9):.3f}; "
                  f"worst: {drift['run_spread'].max():.2f}"]
        worst = drift.sort_values("run_spread", ascending=False).head(5)
        for r in worst.itertuples():
            lines.append(f"- {r.pair}/{r.algorithm} {r.dist} param={r.param} n={fmt_n(r.n)}: spread {r.run_spread:.2f}")
    if not counters_present:
        lines += ["", "_Hardware counters were not available on this run (timing only)._"]
    return "\n".join(lines) + "\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("run_dir", nargs="+", help="one or more run directories to pool")
    ap.add_argument("--out", default=None)
    ap.add_argument("--bootstrap", type=int, default=1000)
    ap.add_argument("--supersede", nargs="*", default=None, metavar="RUN_DIR",
                    help="run directories whose cells replace (not pool with) the other runs' "
                         "cells over the range of n they cover")
    args = ap.parse_args()
    out = args.out or os.path.join(args.run_dir[-1], "analysis")
    os.makedirs(out, exist_ok=True)

    df = load_run(args.run_dir, supersede=args.supersede)
    with open(os.path.join(out, "runs_pooled.txt"), "w") as f:
        f.write("\n".join(args.run_dir) + "\n")
    cells = cell_stats(df)
    cells.to_csv(os.path.join(out, "cells.csv"), index=False)
    cross = crossovers(df, B=args.bootstrap)
    cross.to_csv(os.path.join(out, "crossovers.csv"), index=False)
    expo = exponents(cells)
    expo.to_csv(os.path.join(out, "exponents.csv"), index=False)
    counters_present = any(c in df.columns and df[c].notna().any() for c in COUNTER_COLS)
    drift = drift_report(df)
    if not drift.empty:
        drift.to_csv(os.path.join(out, "drift.csv"), index=False)
    text = summary_markdown(cross, expo, counters_present, drift)
    with open(os.path.join(out, "summary.md"), "w") as f:
        f.write(text)
    print(f"wrote {out}/cells.csv, crossovers.csv, exponents.csv, summary.md")
    print(text)


if __name__ == "__main__":
    main()
