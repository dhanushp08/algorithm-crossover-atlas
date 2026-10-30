# Methodology

This is the draft of the paper's methodology section, kept in sync with
what the code actually does. Section numbers refer to `paper/main.tex`.

## 1. Research question

Under what input sizes, input structures and tuning parameters does an
algorithm with a better asymptotic bound begin to outperform a simpler
algorithm with a worse bound in wall-clock time, on a current commodity CPU?
We call the smallest such size the *crossover point* $n^*$ and report it
with an uncertainty interval, for four algorithm pairs, as a function of the
variables that practitioners actually control.

## 2. Algorithm pairs and what is held fixed

Each pair consists of a *faster-in-theory* algorithm $F$ and a *simpler*
algorithm $S$. Three design rules keep the comparison about the algorithmic
idea rather than about implementation effort:

1. **Same language, same compiler, same flags.** C++17, GCC, `-O2`, no
   `-march=native` (ISA baseline x86-64/SSE2), no hand-written SIMD, no
   multithreading. The effect of `-O3` and `-march=native` is reported as a
   secondary experiment, not mixed into the main sweep.
2. **Same base case.** The recursive algorithms (Strassen, Karatsuba) bottom
   out in exactly the kernel they are compared against, so the measured
   difference is attributable to the recursive structure, its extra
   additions and its memory traffic.
3. **Competent, textbook implementations on both sides.** Neither side is
   deliberately handicapped nor heroically tuned. Radix sort has a single
   histogram pass and skips trivial digit passes; Karatsuba uses a
   caller-provided scratch buffer; the Fibonacci heap pre-allocates one node
   per vertex. The simpler algorithms are what a careful programmer writes:
   `std::sort`, the i-k-j loop order, 64-bit multiply-accumulate, an
   indexed binary heap with decrease-key (and, as a second baseline, the
   lazy `std::priority_queue` variant that is far more common in practice).

| Pair | $F$ | $S$ | Swept variables | Fixed |
|---|---|---|---|---|
| Sorting | LSD radix sort, $O(n \cdot 64/d)$ for $d$-bit digits | introsort (`std::sort`), $O(n \log n)$ | $n \in [16, 10^8]$; key width 8/16/32/64 bits; nearly-sorted, reverse, few-unique, organ-pipe inputs; digit width $d \in \{8, 11, 16\}$ | 64-bit unsigned keys, no satellite data |
| Matrix multiply | Strassen, $O(n^{2.807})$ | naive triple loop, $O(n^3)$ | $n \in [16, 4096]$ incl. non-powers of two; base-case cutoff $\in \{8..256\}$; i-k-j vs i-j-k baseline | square, dense, double precision |
| Integer multiply | Karatsuba, $O(n^{1.585})$ | schoolbook, $O(n^2)$ | operand length $n \in [8, 2^{20}]$ limbs; cutoff $\in \{4..256\}$ limbs | 32-bit limbs, equal-length operands |
| Shortest paths | Dijkstra + Fibonacci heap, $O(m + n \log n)$ | Dijkstra + binary heap, $O((n+m)\log n)$ | $n \in [10^2, 10^7]$; average degree $\in \{2..256\}$; random / 2-D grid / complete graphs | integer weights uniform in $[1, 1000]$, single source |

## 3. Inputs

Inputs are generated deterministically from a 64-bit seed combined with $n$
(xoshiro256\*\* seeded through splitmix64), so every cell of the experiment
is reproducible from the command line that produced it. Sizes form a
geometric grid (factor 2 for the main sweeps, 1.5 for integer
multiplication, and a hand-picked list for matrices that includes sizes
such as 24, 48, 96 that are not powers of two and exercise Strassen's
padding path).

A cell is a (pair, distribution, parameter, $n$) tuple. Cells that make no
sense are skipped by rule rather than by budget (a complete graph on
$10^7$ vertices), and cells whose single run exceeds a wall-clock budget
cause that algorithm to be dropped for larger $n$ in the same series, so
that cubic baselines do not dominate the running time of the experiment.

## 4. Measurement

**Timer.** `std::chrono::steady_clock` (CLOCK_MONOTONIC through the vDSO,
~20 ns overhead). Runs shorter than 2 ms are batched: the harness runs
$K$ iterations of (copy input; run algorithm) and reports the per-iteration
mean, with $K$ chosen from a calibration run. Because in-place algorithms
require a fresh copy per iteration, the copy is inside the timed region;
its cost is measured separately with the same $K$ and subtracted during
analysis (never below 5 % of the raw time, to keep noise in the baseline
from producing non-positive runtimes).

**Repetitions and interleaving.** Each cell is measured $r$ times (7 in the
full profile) after $w$ untimed warm-up runs (2). Within the repetition
loop the algorithms alternate in a rotating order, so that slow drift in
machine state (thermal throttling, background activity, frequency changes)
is shared equally by $F$ and $S$. The per-cell statistic is the median;
the interquartile range is retained and plotted as a band.

**Hardware counters.** When `perf_event_open` is permitted, each measurement
also records cycles, retired instructions, L1d read misses, last-level-cache
misses and branch mispredictions for the user-space portion of the run,
scaled for multiplexing. These are reported per element ($n$, $n^3$, $n^2$
or $n$ respectively) and are explanatory, not primary: the crossover is
defined on wall-clock time. On platforms where counters are unavailable
(virtual machines, WSL2) the study is timing-only and says so.

**Correctness.** Every benchmark binary cross-checks the outputs of $F$ and
$S$ on the last repetition of each cell and aborts on mismatch. Separately,
randomized test suites compare each implementation with an independent
reference (std::sort; the i-j-k product; schoolbook plus the modular
identity $(ab) \bmod p = ((a \bmod p)(b \bmod p)) \bmod p$ with
$p = 2^{61}-1$; Bellman-Ford), and the heaps are exercised directly against
a scan-for-minimum oracle.

## 5. Machine control

Bare-metal Linux: performance governor, turbo boost disabled, minimum
frequency pinned to maximum, benchmark thread pinned to one core,
`perf_event_paranoid = 1`, full `lscpu` and compiler output logged with the
run. WSL2 (the environment of the reported numbers unless stated otherwise):
the same knobs are set from the Windows side (high-performance power plan
with maximum processor state 99 % to disable turbo; the WSL virtual machine
restricted to performance cores on hybrid CPUs; laptop on mains power),
with the thread pinned to one virtual CPU. We report the machine and the
environment exactly, and treat absolute times as specific to it; the
crossover *ordering* and *order of magnitude* are the transferable results.

## 6. Crossover estimation

For a series (pair, distribution, parameter) let $\rho(n) = \tilde t_F(n) /
\tilde t_S(n)$ be the ratio of median runtimes on the size grid. The
crossover $n^*$ is the *sustained* crossing: the grid interval after which
$\rho < 1$ at every larger measured size, interpolated linearly in
$(\log n, \log \rho)$. A transient dip below 1 followed by a return above it
is not counted (it remains visible in the ratio plots and in the reported
minimum ratio). Series in which
$\rho < 1$ already at the smallest $n$ are reported as *always faster
(from $n_{\min}$)* and series in which $\rho \ge 1$ throughout as *never
faster (up to $n_{\max}$)*, together with the ratio at $n_{\max}$ so the
reader can see how far from crossing they are.

Uncertainty: a non-parametric bootstrap resamples the $r$ repetitions of
every cell with replacement (1000 replicates), recomputes medians and
$n^*$, and reports the 2.5th and 97.5th percentiles. Because $n^*$ is
interpolated between grid points, the interval is bounded below by the grid
resolution; the full profile's factor-2 grid therefore gives at best a
factor-2 locatable crossover, which is the precision at which the question
"from what size on?" is usually asked.

Empirical scaling exponents are fitted as $\log t = b \log n + c$ over the
largest decade of $n$ for each algorithm, to show how far the measured
growth is from the asymptotic bound in the measured range (e.g. radix sort
drifting above slope 1 as the working set leaves L2).

## 7. Threats to validity

- **Single machine.** Crossovers depend on cache sizes, memory bandwidth
  and the branch predictor; we report one machine in full and treat
  cross-machine generalisation as future work, offering the harness as the
  means.
- **Implementation quality.** Both sides are deliberately plain. A SIMD
  radix sort or a BLAS kernel would move every crossover; the secondary
  `-O3`/`-march=native` experiment bounds how much the compiler alone moves
  them.
- **Virtualisation.** WSL2 adds timer and scheduling noise and hides
  counters; the interleaving and the bootstrap are designed to make the
  ratio robust to it, and the interval widths make the residual noise
  visible rather than hidden.
- **Input families.** Each family is a convenience sample of structures
  known to matter (sortedness, key width, density); the atlas is a map, not
  a census.
