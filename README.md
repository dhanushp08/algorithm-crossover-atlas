# When Does a Theoretically Faster Algorithm Actually Win?

An empirical crossover atlas: for four classic algorithm pairs, the input
size at which the algorithm with the better asymptotic bound starts beating
the simpler one in wall-clock time, how that point moves with input
structure and tuning parameters, and (where hardware counters are
available) why.

| Pair | Faster in theory | Simpler in practice | Swept variables |
|---|---|---|---|
| Sorting | LSD radix sort, O(nk) | introsort (`std::sort`), O(n log n) | n, key width, distribution, digit width |
| Matrix multiply | Strassen, O(n^2.81) | naive triple loop, O(n^3) | n, base-case cutoff, loop order |
| Integer multiply | Karatsuba, O(n^1.58) | schoolbook, O(n^2) | operand length, cutoff |
| Shortest paths | Dijkstra + Fibonacci heap, O(m + n log n) | Dijkstra + binary heap, O((n+m) log n) | n, density, graph structure, lazy vs decrease-key |

Companion to the preprint of the same name (see `paper/`).

## Reproduce

```bash
git clone https://github.com/<you>/algorithm-crossover-atlas
cd algorithm-crossover-atlas
make            # build + 6,000 randomized correctness checks (~1 min)
make smoke      # end-to-end pipeline check (~2 min): results/ + figures/
make full PIN=2 # the paper's sweep (hours); see docs/BENCHMARKING.md first
```

`make full` produces `results/<run>/analysis/summary.md` (crossover table
with bootstrap CIs), `figures/*.pdf`, and `figures/table_crossovers.tex`.

The paper's numbers come from three runs on one machine, combined as

```bash
.venv/bin/python analysis/atlas.py results/dhanu-full results/dhanu-refine results/dhanu-patch \
    --supersede results/dhanu-patch --out results/final/analysis
.venv/bin/python analysis/figures.py results/final/analysis --out figures --sysinfo results/dhanu-full/sysinfo.txt
```

(`refine` adds a fine grid around each crossing and is pooled; `patch`
re-measures the large-input cells after a harness fix and replaces them —
see `docs/BENCHMARKING.md`.)

## Layout

```
src/harness/     timing (steady_clock, batched for tiny inputs), perf_event_open
                 counters with graceful fallback, core pinning, CSV, CLI
src/sorting/     radix_sort (configurable digit width, pass skipping), std::sort
src/matmul/      strassen (cutoff, odd-size padding), naive i-k-j and i-j-k
src/bigint/      karatsuba (cutoff, scratch buffer), schoolbook, 32-bit limbs
src/dijkstra/    Fibonacci heap, indexed binary heap, lazy std::priority_queue;
                 random / grid / complete graph generators
tests/           randomized cross-checks against independent references
scripts/         run_all.sh (smoke|quick|full), setup_machine.sh, collect_sysinfo.sh
analysis/        atlas.py (medians, crossovers, bootstrap CIs, exponents),
                 figures.py (log-log runtimes, ratio plots, heatmaps, counters)
paper/           LaTeX source and references
docs/            BENCHMARKING.md (machine setup incl. WSL2), METHODOLOGY.md
```

## Design notes

- **Fixed compiler, fixed flags.** GCC `-O2`, no `-march=native` by default
  (`cmake -DATLAS_MARCH_NATIVE=ON` to test the effect of SIMD).
- **Interleaved repetitions.** For each input, A and B alternate inside the
  repetition loop so slow drift (thermal, background load) affects both.
- **Batched tiny inputs.** Runs shorter than ~2 ms are repeated K times and
  divided, with the per-iteration input-copy cost measured and subtracted.
- **Time budget.** An algorithm that exceeds `--max-seconds` on one run is
  dropped for larger n in that cell, so O(n^3) baselines do not hold up the
  sweep.
- **Same base case.** Strassen's and Karatsuba's leaves call exactly the
  naive kernel they are compared against, isolating the recursive overhead.
- **Crossover = first n where median(fast)/median(simple) < 1**,
  log-log interpolated, with a 95 % bootstrap CI over repetitions.

## License

MIT for code; CC BY 4.0 for the paper and figures.
