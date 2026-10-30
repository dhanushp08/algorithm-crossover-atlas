# Literature Review: When Does a Theoretically Faster Algorithm Actually Win?

Annotated bibliography for the crossover-atlas study. Every entry was verified
against a publisher, DOI, arXiv, author or project page (see
`paper/references.bib` for full citation data and bib keys). Annotations are
paraphrased; quantitative thresholds reported by the sources are called out in
bold. Where a source gives only qualitative results, that is stated rather than
inferred.

Thirty-one references were verified. One target (Jiménez-González, Navarro and
Larriba-Pey, "CC-Radix", 2003) could not be confirmed on a publisher page
within the search budget and was dropped.

---

## A. Methodology of experimental algorithmics and benchmarking

**McGeoch 2012** (`mcgeoch2012guide`). A book-length treatment of how to design,
run and report algorithm experiments, covering instance generation, factor
selection, variance reduction and curve fitting for empirical running-time
functions. It is the standard reference for the design-of-experiments
vocabulary (factors, levels, trials) that this study uses when sweeping input
size and reporting crossover points, and it argues that a crossover is itself
an experimental quantity with an uncertainty, not a constant.

**Johnson 2002** (`johnson2002theoretician`). A set of ten principles and a list
of pitfalls for theoreticians who run experiments, including warnings against
spending effort on already-dominated algorithms, testing only on random
instances, and drawing conclusions the data do not support. Its insistence on
reproducibility, comparability with prior work, and reporting the full
experimental history motivates the fully scripted pipeline and the inclusion of
negative results (pairs where the asymptotically better algorithm never wins
at tested sizes).

**Sanders 2009** (`sanders2009algorithm`). Defines algorithm engineering as a
cycle of design, analysis, implementation and experiment, with realistic
machine models and real inputs feeding back into theory. The present study is a
single pass through that cycle for four classic pairs, and Sanders' point that
asymptotic analysis under the RAM model can mislead on cached, pipelined
hardware is the thesis under test.

**Mytkowicz, Diwan, Hauswirth and Sweeney 2009** (`mytkowicz2009producing`).
Shows that innocuous setup details such as the size of the UNIX environment or
the link order of object files can change measured performance enough to flip
conclusions, across several processors, compilers and all SPEC CPU2006 C
programs tested; in a survey of **133 papers** none accounted for this bias. The
authors recommend setup randomization and causal analysis. Because a crossover
point is a difference between two nearly equal running times, it is especially
vulnerable to this kind of bias, so the study randomizes run order and repeats
across several binaries and machines.

**Curtsinger and Berger 2013** (`curtsinger2013stabilizer`). STABILIZER is a
compiler and runtime that randomizes code, stack and heap layout repeatedly
during execution, so that layout effects become normally distributed noise and
ordinary statistical tests (t-tests, ANOVA) become valid for comparing two
binaries. It provides the justification for treating repeated-trial
distributions, rather than single best times, as the unit of comparison when
locating a crossover.

**Hoefler and Belli 2015** (`hoefler2015scientific`). From a sample of **120
HPC papers** the authors find that reporting practice rarely allows a reader to
tell whether an improvement is distinguishable from noise, and they distil
twelve rules covering the choice of summary statistic (median rather than mean
for skewed timings), confidence intervals, and reporting of the full
measurement setup. The study follows these rules in reporting medians with
nonparametric confidence intervals and in giving a complete hardware and
compiler description.

**LaMarca and Ladner 1999** (`lamarca1999influence`). An early and influential
demonstration that cache misses, not instruction counts, dominate the running
time of sorting on then-current machines: cache-aware restructurings of
heapsort, mergesort and quicksort gave large wall-clock gains while an
instruction-count model predicted none. It is the origin of the idea that
"theoretically faster" and "actually faster" diverge because of the memory
hierarchy, which is the framing of this paper.

## B. Sorting: introsort and radix sort

**Musser 1997** (`musser1997introspective`). Introduces introsort, which runs
quicksort but switches to heapsort when recursion depth exceeds a logarithmic
bound, guaranteeing O(n log n) worst case with quicksort's average constants.
This is the algorithm behind `std::sort` in libstdc++ and libc++, so it is the
comparison-sort baseline the LSD radix sort must beat, and the paper also
documents the small-partition insertion-sort cutoff (around 16 elements) that
makes the baseline strong at small n.

**Rahman and Raman 2001** (`rahman2001adapting`). Analyses LSD radix sort under
cache and TLB models and shows that naive radix passes with many buckets thrash
the TLB; restructuring the passes to respect cache and TLB capacity gives
substantial speedups and makes radix sort competitive with tuned comparison
sorts for 32-bit integers. The TLB-aware digit-width choice (keeping the number
of buckets below the TLB reach) is adopted directly in the study's radix-sort
implementation.

**Satish et al. 2010** (`satish2010fast`). Compares a SIMD-optimised merge sort
with an optimised radix sort on multicore CPUs and GPUs. On the CPUs measured,
radix sort was faster for 32-bit keys, but the authors report that merge sort
wins for wide keys and, in simulation of a 64-core wide-SIMD machine under fixed
per-core bandwidth, becomes up to **3x** faster than radix sort. The result
establishes that the radix-versus-comparison crossover depends on key width and
on the bandwidth-to-compute ratio, which the study varies explicitly.

**Polychroniou and Ross 2014** (`polychroniou2014comprehensive`). A systematic
study of in-memory partitioning on NUMA machines that builds a stable LSB radix
sort, an in-place MSB radix sort and a range-partitioning comparison sort, and
shows that with a cache-resident index comparison-based partitioning can run at
nearly the same speed as radix partitioning. All three sorts are reported as the
fastest known at billion-key scale, so at large n the gap between the O(n) and
O(n log n) approaches is narrower than the asymptotics suggest; this frames the
study's expectation that radix sort's advantage is bounded rather than growing
without limit.

## C. Strassen and fast matrix multiplication in practice

**Strassen 1969** (`strassen1969gaussian`). The original three-page note showing
that 2x2 block multiplication can be done with seven rather than eight
sub-products, yielding an O(n^{log2 7}) ≈ O(n^{2.807}) algorithm and, as a
corollary, faster Gaussian elimination. It is the primary citation for the
algorithm and for the 7/8 operation-count ratio that must overcome the extra
additions in practice.

**Huss-Lederman, Jacobson, Johnson, Tsao and Turnbull 1996**
(`husslederman1996implementation`). Describes a portable Strassen implementation
(DGEFMM) with careful handling of odd dimensions and memory, evaluated on an
IBM RS/6000, a CRAY C90 and a CRAY T3D, and concludes that Strassen is
practical for realistically sized matrices as a drop-in replacement for DGEMM.
It is the classic reference for the recursion-cutoff design decision that the
study sweeps as a parameter.

**Huang, Smith, Henry and van de Geijn 2016** (`huang2016strassen`). Fuses
Strassen's additions into the packing and micro-kernel stages of a BLIS-style
GEMM so that no extra workspace or passes over memory are needed. The authors
note that earlier implementations typically crossed over with vendor DGEMM at
around **n ≈ 2000** for double precision on one x86 core, and that their
implementation moves the crossover down to **as low as n ≈ 500** on an Ivy
Bridge Xeon, with speedups also on Xeon Phi. This is the most direct prior
measurement of a Strassen crossover and the benchmark against which the study's
naive-versus-Strassen crossover is interpreted.

**Benson and Ballard 2015** (`benson2015framework`). A code-generation framework
for Strassen-like algorithms with many different base cases, evaluated
sequentially and in parallel against Intel MKL; it reports that several fast
algorithms beat the vendor library on both square and rectangular shapes once
the matrices are large enough, with algorithm choice depending on matrix shape.
It supports the study's decision to report crossover as a function of shape and
to treat the base-case cutoff as a tuned parameter.

**D'Alberto and Nicolau 2007** (`dalberto2007adaptive`). Observes that
Strassen's extra matrix additions reuse cache poorly, so the operation-count
advantage can be erased on machines with deep memory hierarchies, and proposes
installation-time adaptive selection of the recursion cutoff on top of ATLAS or
vendor GEMM. The study's cutoff sweep is a direct descendant of this
adaptive-threshold idea.

## D. Karatsuba and integer multiplication

**Karatsuba and Ofman 1962** (`karatsuba1962multiplication`). The two-page
Doklady note (English translation 1963) giving the first sub-quadratic integer
multiplication, O(n^{log2 3}) ≈ O(n^{1.585}), by replacing four half-size
products with three. It is the primary source for the algorithm whose
schoolbook crossover the study measures.

**GMP manual, Section 15.1** (`gmp2023manual`). Documents GMP's cascade of
multiplication algorithms (basecase, Karatsuba/Toom-2, Toom-3, Toom-4,
Toom-6.5, Toom-8.5, FFT) each gated by a tuned threshold. The Karatsuba page
states that `MUL_TOOM22_THRESHOLD` **can be as little as 10 limbs** (640 bits on
a 64-bit machine) and that the squaring threshold is usually about twice the
multiplication threshold; exact values are machine-tuned by `tuneup`. This is
the industrial reference point for the study's Karatsuba crossover, and the
large gap between a 10-limb tuned threshold and typical textbook
implementations' crossovers is itself a finding to explain.

**Harvey and van der Hoeven 2021** (`harvey2021integer`). Proves that n-bit
integers can be multiplied in O(n log n) bit operations, settling the
Schönhage–Strassen conjecture. The authors are explicit that the algorithm is
not expected to be practical, which makes it the canonical modern example of a
galactic algorithm and situates Karatsuba as the first and most practical rung
of the integer-multiplication ladder.

**Bernstein 2001** (`bernstein2001multidigit`). A survey of multidigit
multiplication methods (Karatsuba, Toom, FFT-based) written to make the
algebraic structure of each method clear. It is cited for the unified view of
these algorithms as polynomial evaluation and interpolation, which explains why
Karatsuba's three recursive products come with additions whose cost dominates
at small sizes.

## E. Priority queues and Dijkstra's algorithm

**Fredman and Tarjan 1987** (`fredman1987fibonacci`). Introduces Fibonacci
heaps with O(1) amortized insert and decrease-key and O(log n) delete-min,
improving Dijkstra to O(m + n log n). The paper is the source of the
asymptotic claim under test and itself notes that the structures are
complicated, which is the practical caveat the study quantifies.

**Cherkassky, Goldberg and Radzik 1996** (`cherkassky1996shortest`). An
extensive experimental comparison of shortest-path codes on several problem
families, finding that no single algorithm dominates and that simple
bucket-based and heap-based implementations are competitive with
asymptotically superior ones on typical inputs. It is the earliest large-scale
evidence that the Fibonacci-heap advantage does not translate to wall clock,
and the study's graph families follow its generator-based design.

**Larkin, Sen and Tarjan 2014** (`larkin2014backtobasics`). Implements a wide
set of heaps with uniform optimisations and measures them on synthetic
workloads, Dijkstra on real road maps and discrete-event simulation. On the
full USA road network the implicit 4-ary heap was fastest, the **pairing heap
about 1.7x slower**, and the **Fibonacci heap about 3x slower**; decrease-keys
were rare (about **one per 14** inserts and deletes), and L1 miss rate tracked
wall-clock time closely. The authors conclude the best heap is strongly
input-dependent. This is the closest prior measurement to the study's
Fibonacci-versus-binary-heap pair.

**Chen, Chowdhury, Ramachandran, Roche and Tong 2007** (`chen2007priority`).
Compares Dijkstra variants with and without decrease-key using binary, pairing,
sequence and buffer heaps on random graphs and US road networks. On sparse
in-core graphs every no-decrease-key variant was at least **1.4x** faster than
every decrease-key variant; on New York City the binary heap took **104 ms with
decrease-key versus 90 ms without**; the authors report Fibonacci heaps were
slower than pairing and binary heaps in preliminary runs and excluded them. It
supports measuring the lazy-deletion binary-heap variant as the practical
baseline.

**Brodal, Lagogiannis and Tarjan 2012** (`brodal2012strict`). Gives the first
pointer-based heap matching Fibonacci-heap bounds in the worst case rather than
amortized. It is cited to show that the theoretical line of work has continued
to add structure (and constant factors) even as empirical studies find simpler
heaps faster, sharpening the question the study asks.

## F. Hardware and measurement infrastructure

**Drepper 2007** (`drepper2007memory`). A long technical report on commodity
memory systems: DRAM organisation, cache hierarchy, TLBs, prefetching and NUMA,
with measured latency and bandwidth curves and advice on cache-friendly data
layout. It supplies the memory-hierarchy model used to explain why radix
passes, Strassen additions and pointer-based heaps pay costs invisible to the
RAM model.

**Fog 2026** (`fog2026optimizing`). A continuously updated optimisation manual
covering compiler behaviour, branch prediction, SIMD, cache effects and
measurement pitfalls on x86. It is used for guidance on isolating the effect
under test (fixed CPU frequency, warm caches, avoiding denormals and alignment
accidents) and for its treatment of branch misprediction, which is central to
comparison sorts and heap operations.

**perf_event_open(2)** (`linux2026perfeventopen`). The Linux system-call manual
page for hardware performance counters, including `PERF_COUNT_HW_INSTRUCTIONS`
and `PERF_COUNT_HW_CACHE_MISSES`. The study reads these counters alongside wall
clock to separate instruction-count effects from memory effects at each
crossover.

## G. Galactic algorithms and the matrix-multiplication exponent

**Lipton 2010** (`lipton2010galactic`). The blog post that coined "galactic
algorithm" for methods whose asymptotic advantage appears only on inputs
larger than anything that will ever be computed, with Strassen's successors
and fast sorting networks among the examples. The term gives this paper its
vocabulary: the four pairs are chosen to span the range from clearly practical
to borderline galactic.

**Alman and Vassilevska Williams 2024 / SODA 2021** (`alman2024refined`). Refines
the laser method to prove **ω < 2.37286**. It represents the state of the
theoretical frontier that Strassen started and makes the point that exponent
improvements below 2.807 have come with enormous constants and no
implementation.

**Alman, Duan, Vassilevska Williams, Xu, Xu and Zhou 2024**
(`alman2024asymmetry`). Pushes the bound further to **ω < 2.371177**. Cited
together with the previous entry to show that the exponent is still moving,
while the only fast matrix-multiplication algorithm with a measured practical
crossover remains Strassen's.

---

## Gap this study fills

The literature contains strong, separate empirical results for each pair:
tuned Strassen crossovers near n ≈ 500–2000 (`huang2016strassen`), a GMP
Karatsuba threshold that can be as low as 10 limbs (`gmp2023manual`),
radix-versus-comparison results that depend on key width and bandwidth
(`satish2010fast`, `polychroniou2014comprehensive`), and Fibonacci heaps that
run roughly three times slower than implicit heaps on road networks
(`larkin2014backtobasics`). What is missing is a single study that measures all
four crossovers with one methodology, on the same machines, with the same
statistical treatment (`hoefler2015scientific`, `curtsinger2013stabilizer`),
the same hardware-counter instrumentation (`linux2026perfeventopen`), and
with the implementation effort held at the level of a careful but
non-heroic programmer rather than a vendor library team. Prior crossovers are
also reported against heavily tuned baselines (MKL, GMP, BLIS), which answers
"can experts make the fast algorithm win?" rather than the question most
practitioners face, "will the fast algorithm win if I implement it myself
against the standard library?" This study answers the latter, reports each
crossover as a distribution with confidence intervals rather than a point, and
decomposes each crossover into instruction-count and cache-miss components so
that the reasons differ by pair can be seen directly.

## Justification of the four pairs

**LSD radix sort versus introsort.** This pair has the largest asymptotic gap
(O(n·w) against O(n log n)) and the longest empirical history, from the
cache-centred analysis of `lamarca1999influence` and the TLB-aware passes of
`rahman2001adapting` to the SIMD-era studies of `satish2010fast` and
`polychroniou2014comprehensive`. The baseline, introsort
(`musser1997introspective`), is what `std::sort` actually runs, so the
crossover measured here is the one a practitioner would see. It is included as
the case where the theoretically faster algorithm is expected to win early and
by a wide margin, subject to key width and memory bandwidth.

**Strassen versus naive matrix multiplication.** Strassen's algorithm
(`strassen1969gaussian`) is the archetype of an asymptotic improvement that is
practical only after careful engineering: `husslederman1996implementation`
and `dalberto2007adaptive` show the recursion cutoff must be tuned to the
memory hierarchy, `benson2015framework` shows the answer depends on shape, and
`huang2016strassen` reports crossovers from about 2000 down to about 500
depending on how tightly the additions are fused into the kernel. Measuring
Strassen against a naive triple loop and against a blocked kernel isolates how
much of the crossover is due to the algorithm and how much to the quality of
the base case, and connects the pair to the galactic end of the spectrum via
`lipton2010galactic`, `alman2024refined` and `alman2024asymmetry`.

**Karatsuba versus schoolbook multiplication.** Karatsuba
(`karatsuba1962multiplication`) is the simplest sub-quadratic algorithm and the
one with the best-documented industrial threshold: GMP's tuned Toom-2 cutoff
can be as low as 10 limbs (`gmp2023manual`). Comparing a straightforward
implementation's crossover to that figure measures the cost of not being GMP,
and the algebraic framing in `bernstein2001multidigit` plus the O(n log n)
endpoint of `harvey2021integer` place it as the practical first step on a ladder
whose top is galactic.

**Dijkstra with a Fibonacci heap versus a binary heap.** This is the pair where
the asymptotic improvement (`fredman1987fibonacci`) is most often cited and
least often realised: `cherkassky1996shortest` found simple heaps competitive
on realistic inputs, `chen2007priority` found that dropping decrease-key
altogether is often faster, and `larkin2014backtobasics` measured Fibonacci
heaps at roughly three times the running time of an implicit 4-ary heap on a
continental road network, with decrease-keys occurring only about once per 14
other operations. Continued theoretical refinement (`brodal2012strict`) has not
changed that picture. The study includes this pair as the candidate for a
crossover that does not exist at any tested size, and varies graph density to
find whether a decrease-key-heavy regime can produce one.
