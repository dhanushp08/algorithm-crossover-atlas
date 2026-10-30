# Running the benchmarks

## 1. Build and test (any Linux, including WSL2)

```bash
make            # builds build/bench_* and build/test_*, runs the tests
```

Requirements: g++ ≥ 11, cmake ≥ 3.16, python3 ≥ 3.10 with `venv`. Python
packages are installed into `.venv/` automatically on first `make smoke`.

## 2. Three profiles

| profile | time        | purpose                                   |
|---------|-------------|-------------------------------------------|
| `smoke` | ~1–2 min    | checks the whole pipeline end to end      |
| `quick` | ~15–30 min  | coarse sweeps, enough to see every crossover |
| `full`  | ~5 hours    | the sweeps used in the paper, 5 repetitions |
| `patch` | ~1 hour     | re-measures large-input cells with the fixed copy-timing (v3+ harness); analyse with `--supersede` |

```bash
make smoke
make quick
make full PIN=2          # PIN pins the benchmark thread to one CPU
```

Each run writes `results/<timestamp>-<profile>/` containing one CSV per pair,
the stderr log (with `[cell]` lines giving edge counts, decrease-key counts,
etc.), `sysinfo.txt`, `build_flags.txt`, and an `analysis/` directory with
`cells.csv`, `crossovers.csv`, `exponents.csv`, `summary.md`. Figures go to
`figures/`. Re-analyse any run with `make analyze RUN=results/<dir>`.

## 3. Making the machine quiet

### Bare-metal Linux (best case)

```bash
sudo scripts/setup_machine.sh steady     # performance governor, turbo off, perf enabled
make full PIN=2
sudo scripts/setup_machine.sh restore
```

With `perf_event_paranoid ≤ 1`, the harness collects cycles, instructions,
L1d read misses, last-level-cache misses and branch mispredictions per run
through `perf_event_open`. Check the first line each benchmark prints:
`counters: cycles:ok instructions:ok ...`.

### WSL2 on Windows (timing only)

WSL2 is a Hyper-V virtual machine. It cannot see hardware counters or the
frequency governor, so the harness runs in timing-only mode (the counter
columns are empty) and the steadying knobs are set on the Windows side:

1. **Plug in the laptop** and leave it plugged in. On battery the CPU is
   throttled aggressively and unpredictably.
2. **Power plan**: Control Panel → Power Options → choose *High performance*
   (click *Show additional plans* if hidden) → *Change plan settings* →
   *Change advanced power settings* → *Processor power management* →
   set **Maximum processor state** to **99 %**. On Intel this disables Turbo
   Boost, so the core runs at a steady frequency. Set it back to 100 %
   afterwards.
3. **Hybrid CPUs (Intel 12th gen and later: P-cores + E-cores)**: WSL2 cannot
   tell which physical core it is running on, and a run on an E-core is
   1.5–2× slower than on a P-core for reasons unrelated to the algorithm.
   Restrict the WSL VM to P-cores: Task Manager → *Details* tab → right-click
   **vmmem** (or **vmmemWSL**) → *Set affinity* → tick only the P-core
   threads (on an i7-14650HX these are CPU 0–15; E-cores are 16–23). Do this
   once after WSL has started; it does not persist across reboots.
4. **Close other applications**, particularly browsers and anything using
   the GPU, and disable Windows Update for the duration.
5. Inside WSL, pin the benchmark thread: `make full PIN=2` (any vCPU is
   fine; pinning stops it migrating between vCPUs).
6. Memory: WSL2 defaults to half the physical RAM. The full sweep needs
   ≈ 3 GB; if `free -h` in WSL shows less than 6 GB total, add to
   `C:\Users\<you>\.wslconfig`:
   ```
   [wsl2]
   memory=12GB
   ```
   then `wsl --shutdown` from PowerShell and reopen Ubuntu.

### Google Colab, shared cloud VMs

Not recommended for the paper's numbers: no counters, no frequency control,
noisy neighbours sharing the cache, and the hardware can change between
sessions. Fine for `make smoke`.

## 4. Reading the output

`results/<run>/analysis/summary.md` is the quick view. `crossovers.csv` has
one row per (pair, distribution, parameter, comparison) with:

- `n_star` — the interpolated input size where the faster-in-theory
  algorithm's median runtime first drops below the simpler one's;
- `ci_lo`, `ci_hi` — 95 % bootstrap interval from resampling repetitions;
- `status` — `crossed`, `always_faster` (fast wins even at the smallest n
  measured) or `never_faster` (does not win up to the largest n measured);
- `ratio_at_nmax` — how much faster or slower it is at the top of the sweep.

The harness batches tiny inputs (it runs them K times and divides) so that
sub-microsecond algorithms are measured accurately; the `iters` column is K
and `copy_ns` is the measured cost of the input copy that is inside each
iteration, which the analysis subtracts. For K = 1 (runs over ~2 ms) the copy
is made outside the timed region and `copy_ns` is 0.

**History.** Harness versions before v3 measured the copy baseline into a
freshly allocated buffer, which charged page faults to the baseline but not
to the timed loop; for inputs of hundreds of MB this over-subtracted by up to
the whole runtime. The `patch` profile re-measures exactly the cells where the
copy exceeded 5 % of the raw time, and `atlas.py --supersede <patch-run>`
makes those cells replace the originals over the range of n they cover.
