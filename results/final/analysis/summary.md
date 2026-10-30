# Crossover summary

| pair | distribution | param | fast vs simple | crossover n* | 95% CI | precision | status | ratio at n_max |
|---|---|---|---|---|---|---|---|---|
| sorting | few_unique | 11 | radix vs introsort | 2.66k | [2.36k, 2.84k] | sharp | crossed | 0.51 |
| sorting | few_unique | 16 | radix vs introsort | 7.47k | [6.21k, 8.09k] | sharp | crossed | 0.38 |
| sorting | few_unique | 8 | radix vs introsort | 3.03k | [2.7k, 3.14k] | sharp | crossed | 0.71 |
| sorting | nearly_sorted | 11 | radix vs introsort | - | - | - | never_faster | 3.69 |
| sorting | nearly_sorted | 16 | radix vs introsort | - | - | - | never_faster | 2.49 |
| sorting | nearly_sorted | 8 | radix vs introsort | - | - | - | never_faster | 4.52 |
| sorting | organ_pipe | 11 | radix vs introsort | 1.64k | [1.5k, 1.77k] | sharp | crossed | 0.72 |
| sorting | organ_pipe | 16 | radix vs introsort | 6.85k | [6.57k, 6.99k] | sharp | crossed | 0.45 |
| sorting | organ_pipe | 8 | radix vs introsort | 48.1M | [38.6M, 55.7M] | marginal | crossed | 0.96 |
| sorting | reverse | 11 | radix vs introsort | - | - | - | never_faster | 8.23 |
| sorting | reverse | 16 | radix vs introsort | - | - | - | never_faster | 5.83 |
| sorting | reverse | 8 | radix vs introsort | - | - | - | never_faster | 10.28 |
| sorting | uniform16 | 11 | radix vs introsort | 1.6k | [1.57k, 1.62k] | sharp | crossed | 0.25 |
| sorting | uniform16 | 16 | radix vs introsort | 3.1k | [3.03k, 3.2k] | sharp | crossed | 0.37 |
| sorting | uniform16 | 8 | radix vs introsort | 1.62k | [1.6k, 1.63k] | sharp | crossed | 0.27 |
| sorting | uniform32 | 11 | radix vs introsort | 1.51k | [1.37k, 1.59k] | sharp | crossed | 0.25 |
| sorting | uniform32 | 16 | radix vs introsort | 3.35k | [3.07k, 3.57k] | sharp | crossed | 0.37 |
| sorting | uniform32 | 8 | radix vs introsort | 1.58k | [1.43k, 1.66k] | sharp | crossed | 0.24 |
| sorting | uniform64 | 11 | radix vs introsort | 1.71k | [1.62k, 2.05k] | sharp | crossed | 0.37 |
| sorting | uniform64 | 16 | radix vs introsort | 7.03k | [6.28k, 8.62M] | gradual | crossed | 0.73 |
| sorting | uniform64 | 8 | radix vs introsort | 1.67k | [1.59k, 2.05k] | sharp | crossed | 0.32 |
| sorting | uniform8 | 11 | radix vs introsort | 1.43k | [1.41k, 1.48k] | sharp | crossed | 0.32 |
| sorting | uniform8 | 16 | radix vs introsort | 3.52k | [3.41k, 3.59k] | sharp | crossed | 0.29 |
| sorting | uniform8 | 8 | radix vs introsort | 1.35k | [1.33k, 1.39k] | sharp | crossed | 0.41 |
| matmul | integer (n=2^k,3*2^k) | 128 | strassen vs naive_ikj | 256 | - | - | always_faster | 0.41 |
| matmul | integer (n=2^k,3*2^k) | 16 | strassen vs naive_ikj | 114 | [99, 833] | gradual | crossed | 0.36 |
| matmul | integer (n=2^k,3*2^k) | 256 | strassen vs naive_ikj | 256 | - | - | always_faster | 0.43 |
| matmul | integer (n=2^k,3*2^k) | 32 | strassen vs naive_ikj | 56 | [32, 100] | gradual | crossed | 0.31 |
| matmul | integer (n=2^k,3*2^k) | 64 | strassen vs naive_ikj | 69 | [64, 76] | sharp | crossed | 0.37 |
| matmul | integer (n=2^k,3*2^k) | 8 | strassen vs naive_ikj | 811 | [726, 853] | sharp | crossed | 0.49 |
| matmul | uniform (n=2^k,3*2^k) | 128 | strassen vs naive_ikj | 258 | [256, 1.05k] | gradual | crossed | 0.40 |
| matmul | uniform (n=2^k,3*2^k) | 16 | strassen vs naive_ikj | 99 | [50, 905] | gradual | crossed | 0.35 |
| matmul | uniform (n=2^k,3*2^k) | 256 | strassen vs naive_ikj | 258 | [256, 1.09k] | gradual | crossed | 0.42 |
| matmul | uniform (n=2^k,3*2^k) | 32 | strassen vs naive_ikj | 32 | - | - | always_faster | 0.31 |
| matmul | uniform (n=2^k,3*2^k) | 64 | strassen vs naive_ikj | 64 | - | - | always_faster | 0.37 |
| matmul | uniform (n=2^k,3*2^k) | 8 | strassen vs naive_ikj | 1.16k | [778, 1.27k] | gradual | crossed | 0.49 |
| matmul | integer (all n) | 128 | strassen vs naive_ikj | 247 | [239, 840] | gradual | crossed | 0.41 |
| matmul | integer (all n) | 16 | strassen vs naive_ikj | 244 | [241, 834] | gradual | crossed | 0.36 |
| matmul | integer (all n) | 256 | strassen vs naive_ikj | 256 | - | - | always_faster | 0.43 |
| matmul | integer (all n) | 32 | strassen vs naive_ikj | 218 | [202, 235] | sharp | crossed | 0.31 |
| matmul | integer (all n) | 64 | strassen vs naive_ikj | 239 | [233, 244] | sharp | crossed | 0.37 |
| matmul | integer (all n) | 8 | strassen vs naive_ikj | 811 | [723, 854] | sharp | crossed | 0.49 |
| matmul | uniform (all n) | 128 | strassen vs naive_ikj | 366 | [313, 1.06k] | gradual | crossed | 0.40 |
| matmul | uniform (all n) | 16 | strassen vs naive_ikj | 456 | [443, 890] | gradual | crossed | 0.35 |
| matmul | uniform (all n) | 256 | strassen vs naive_ikj | 258 | [256, 1.09k] | gradual | crossed | 0.42 |
| matmul | uniform (all n) | 32 | strassen vs naive_ikj | 297 | [231, 308] | sharp | crossed | 0.31 |
| matmul | uniform (all n) | 64 | strassen vs naive_ikj | 241 | [235, 246] | sharp | crossed | 0.37 |
| matmul | uniform (all n) | 8 | strassen vs naive_ikj | 1.16k | [778, 1.28k] | gradual | crossed | 0.49 |
| matmul | integer (n=2^k,3*2^k) | 128 | strassen vs naive_ijk | 256 | - | - | always_faster | 0.04 |
| matmul | integer (n=2^k,3*2^k) | 16 | strassen vs naive_ijk | 106 | [98, 121] | sharp | crossed | 0.03 |
| matmul | integer (n=2^k,3*2^k) | 256 | strassen vs naive_ijk | 256 | - | - | always_faster | 0.04 |
| matmul | integer (n=2^k,3*2^k) | 32 | strassen vs naive_ijk | 67 | [32, 89] | gradual | crossed | 0.03 |
| matmul | integer (n=2^k,3*2^k) | 64 | strassen vs naive_ijk | 71 | [64, 77] | sharp | crossed | 0.03 |
| matmul | integer (n=2^k,3*2^k) | 8 | strassen vs naive_ijk | 150 | [138, 159] | sharp | crossed | 0.04 |
| matmul | uniform (n=2^k,3*2^k) | 128 | strassen vs naive_ijk | 256 | - | - | always_faster | 0.03 |
| matmul | uniform (n=2^k,3*2^k) | 16 | strassen vs naive_ijk | 103 | [99, 106] | sharp | crossed | 0.03 |
| matmul | uniform (n=2^k,3*2^k) | 256 | strassen vs naive_ijk | 256 | - | - | always_faster | 0.03 |
| matmul | uniform (n=2^k,3*2^k) | 32 | strassen vs naive_ijk | 32 | - | - | always_faster | 0.02 |
| matmul | uniform (n=2^k,3*2^k) | 64 | strassen vs naive_ijk | 72 | [71, 75] | sharp | crossed | 0.03 |
| matmul | uniform (n=2^k,3*2^k) | 8 | strassen vs naive_ijk | 169 | [158, 174] | sharp | crossed | 0.04 |
| matmul | integer (all n) | 128 | strassen vs naive_ijk | 136 | - | - | always_faster | 0.04 |
| matmul | integer (all n) | 16 | strassen vs naive_ijk | 223 | [217, 230] | sharp | crossed | 0.03 |
| matmul | integer (all n) | 256 | strassen vs naive_ijk | 256 | - | - | always_faster | 0.04 |
| matmul | integer (all n) | 32 | strassen vs naive_ijk | 67 | [32, 89] | gradual | crossed | 0.03 |
| matmul | integer (all n) | 64 | strassen vs naive_ijk | 71 | [68, 116] | gradual | crossed | 0.03 |
| matmul | integer (all n) | 8 | strassen vs naive_ijk | 243 | [241, 247] | sharp | crossed | 0.04 |
| matmul | uniform (all n) | 128 | strassen vs naive_ijk | 136 | - | - | always_faster | 0.03 |
| matmul | uniform (all n) | 16 | strassen vs naive_ijk | 294 | [286, 303] | sharp | crossed | 0.03 |
| matmul | uniform (all n) | 256 | strassen vs naive_ijk | 256 | - | - | always_faster | 0.03 |
| matmul | uniform (all n) | 32 | strassen vs naive_ijk | 59 | [48, 69] | sharp | crossed | 0.02 |
| matmul | uniform (all n) | 64 | strassen vs naive_ijk | 114 | [67, 116] | gradual | crossed | 0.03 |
| matmul | uniform (all n) | 8 | strassen vs naive_ijk | 244 | [242, 244] | sharp | crossed | 0.04 |
| bigint | ones | 128 | karatsuba vs schoolbook | 137 | - | - | always_faster | 0.04 |
| bigint | ones | 16 | karatsuba vs schoolbook | 46 | [35, 60] | gradual | crossed | 0.03 |
| bigint | ones | 32 | karatsuba vs schoolbook | 41 | - | - | always_faster | 0.03 |
| bigint | ones | 64 | karatsuba vs schoolbook | 91 | - | - | always_faster | 0.03 |
| bigint | ones | 8 | karatsuba vs schoolbook | 133 | [83, 141] | gradual | crossed | 0.04 |
| bigint | uniform | 128 | karatsuba vs schoolbook | 137 | - | - | always_faster | 0.05 |
| bigint | uniform | 16 | karatsuba vs schoolbook | 43 | [34, 63] | gradual | crossed | 0.07 |
| bigint | uniform | 32 | karatsuba vs schoolbook | 34 | [32, 42] | sharp | crossed | 0.05 |
| bigint | uniform | 64 | karatsuba vs schoolbook | 65 | [64, 70] | sharp | crossed | 0.05 |
| bigint | uniform | 8 | karatsuba vs schoolbook | 681 | [664, 1.12k] | gradual | crossed | 0.10 |
| dijkstra | dense | - | fib_heap vs binary_heap | 6.95k | [6.47k, 7.59k] | marginal | crossed | 0.98 |
| dijkstra | grid | - | fib_heap vs binary_heap | - | - | - | never_faster | 2.64 |
| dijkstra | random | 16 | fib_heap vs binary_heap | - | - | - | never_faster | 1.41 |
| dijkstra | random | 2 | fib_heap vs binary_heap | - | - | - | never_faster | 2.09 |
| dijkstra | random | 256 | fib_heap vs binary_heap | - | - | - | never_faster | 1.29 |
| dijkstra | random | 4 | fib_heap vs binary_heap | - | - | - | never_faster | 1.73 |
| dijkstra | random | 64 | fib_heap vs binary_heap | - | - | - | never_faster | 1.30 |
| dijkstra | dense | - | fib_heap vs lazy_heap | - | - | - | never_faster | 1.73 |
| dijkstra | grid | - | fib_heap vs lazy_heap | - | - | - | never_faster | 2.19 |
| dijkstra | random | 16 | fib_heap vs lazy_heap | - | - | - | never_faster | 1.52 |
| dijkstra | random | 2 | fib_heap vs lazy_heap | - | - | - | never_faster | 2.52 |
| dijkstra | random | 256 | fib_heap vs lazy_heap | - | - | - | never_faster | 1.35 |
| dijkstra | random | 4 | fib_heap vs lazy_heap | - | - | - | never_faster | 2.38 |
| dijkstra | random | 64 | fib_heap vs lazy_heap | - | - | - | never_faster | 1.22 |

precision: *sharp* = interval narrower than one grid step (or a factor 1.5); *gradual* = wider, i.e. the two algorithms are within noise of each other across the interval and the crossover is a zone rather than a point; *marginal* = crosses but is never more than 10% ahead by the largest size measured (parity, not a win).

## Empirical scaling exponents (fit over largest decade)

| pair | distribution | param | algorithm | exponent | fit range |
|---|---|---|---|---|---|
| bigint | ones | - | schoolbook | 2.01 | 39.9k–303k |
| bigint | ones | 128 | karatsuba | 1.61 | 135k–1.02M |
| bigint | ones | 16 | karatsuba | 1.57 | 135k–1.02M |
| bigint | ones | 32 | karatsuba | 1.59 | 135k–1.02M |
| bigint | ones | 64 | karatsuba | 1.66 | 135k–1.02M |
| bigint | ones | 8 | karatsuba | 1.58 | 135k–1.02M |
| bigint | uniform | - | schoolbook | 1.97 | 39.9k–303k |
| bigint | uniform | 128 | karatsuba | 1.59 | 135k–1.02M |
| bigint | uniform | 16 | karatsuba | 1.56 | 135k–1.02M |
| bigint | uniform | 32 | karatsuba | 1.55 | 135k–1.02M |
| bigint | uniform | 64 | karatsuba | 1.60 | 135k–1.02M |
| bigint | uniform | 8 | karatsuba | 1.58 | 135k–1.02M |
| dijkstra | dense | - | binary_heap | 2.03 | 806–7.74k |
| dijkstra | dense | - | fib_heap | 1.92 | 806–7.74k |
| dijkstra | dense | - | lazy_heap | 1.82 | 806–7.74k |
| dijkstra | grid | - | binary_heap | 1.25 | 1.05M–8.39M |
| dijkstra | grid | - | fib_heap | 1.17 | 1.05M–8.39M |
| dijkstra | grid | - | lazy_heap | 1.16 | 1.05M–8.39M |
| dijkstra | random | 16 | binary_heap | 1.31 | 1.05M–8.39M |
| dijkstra | random | 16 | fib_heap | 1.23 | 1.05M–8.39M |
| dijkstra | random | 16 | lazy_heap | 1.20 | 1.05M–8.39M |
| dijkstra | random | 2 | binary_heap | 1.30 | 1.05M–8.39M |
| dijkstra | random | 2 | fib_heap | 1.25 | 1.05M–8.39M |
| dijkstra | random | 2 | lazy_heap | 1.22 | 1.05M–8.39M |
| dijkstra | random | 256 | binary_heap | 1.24 | 205k–1.05M |
| dijkstra | random | 256 | fib_heap | 1.22 | 205k–1.05M |
| dijkstra | random | 256 | lazy_heap | 1.25 | 205k–1.05M |
| dijkstra | random | 4 | binary_heap | 1.34 | 1.05M–8.39M |
| dijkstra | random | 4 | fib_heap | 1.25 | 1.05M–8.39M |
| dijkstra | random | 4 | lazy_heap | 1.18 | 1.05M–8.39M |
| dijkstra | random | 64 | binary_heap | 1.35 | 819k–4.19M |
| dijkstra | random | 64 | fib_heap | 1.24 | 819k–4.19M |
| dijkstra | random | 64 | lazy_heap | 1.27 | 819k–4.19M |
| matmul | integer | - | naive_ijk | 4.16 | 229–2.05k |
| matmul | integer | - | naive_ikj | 3.22 | 512–4.1k |
| matmul | integer | 128 | strassen | 2.82 | 512–4.1k |
| matmul | integer | 16 | strassen | 2.83 | 512–4.1k |
| matmul | integer | 256 | strassen | 2.83 | 512–4.1k |
| matmul | integer | 32 | strassen | 2.82 | 512–4.1k |
| matmul | integer | 64 | strassen | 2.83 | 512–4.1k |
| matmul | integer | 8 | strassen | 2.82 | 512–4.1k |
| matmul | uniform | - | naive_ijk | 4.25 | 229–2.05k |
| matmul | uniform | - | naive_ikj | 3.21 | 436–4.1k |
| matmul | uniform | 128 | strassen | 2.83 | 436–4.1k |
| matmul | uniform | 16 | strassen | 2.77 | 436–4.1k |
| matmul | uniform | 256 | strassen | 2.84 | 512–4.1k |
| matmul | uniform | 32 | strassen | 2.78 | 436–4.1k |
| matmul | uniform | 64 | strassen | 2.83 | 436–4.1k |
| matmul | uniform | 8 | strassen | 2.82 | 512–4.1k |
| sorting | few_unique | - | introsort | 1.02 | 8.39M–67.1M |
| sorting | few_unique | 11 | radix | 1.03 | 8.39M–67.1M |
| sorting | few_unique | 16 | radix | 1.02 | 8.39M–67.1M |
| sorting | few_unique | 8 | radix | 1.04 | 8.39M–67.1M |
| sorting | nearly_sorted | - | introsort | 1.11 | 8.39M–67.1M |
| sorting | nearly_sorted | 11 | radix | 1.04 | 8.39M–67.1M |
| sorting | nearly_sorted | 16 | radix | 1.10 | 8.39M–67.1M |
| sorting | nearly_sorted | 8 | radix | 1.13 | 8.39M–67.1M |
| sorting | organ_pipe | - | introsort | 1.13 | 8.39M–67.1M |
| sorting | organ_pipe | 11 | radix | 1.07 | 8.39M–67.1M |
| sorting | organ_pipe | 16 | radix | 1.13 | 8.39M–67.1M |
| sorting | organ_pipe | 8 | radix | 1.21 | 8.39M–67.1M |
| sorting | reverse | - | introsort | 1.09 | 8.39M–67.1M |
| sorting | reverse | 11 | radix | 1.07 | 8.39M–67.1M |
| sorting | reverse | 16 | radix | 1.13 | 8.39M–67.1M |
| sorting | reverse | 8 | radix | 1.16 | 8.39M–67.1M |
| sorting | uniform16 | - | introsort | 1.01 | 8.39M–67.1M |
| sorting | uniform16 | 11 | radix | 1.05 | 8.39M–67.1M |
| sorting | uniform16 | 16 | radix | 1.17 | 8.39M–67.1M |
| sorting | uniform16 | 8 | radix | 1.02 | 8.39M–67.1M |
| sorting | uniform32 | - | introsort | 1.06 | 8.39M–67.1M |
| sorting | uniform32 | 11 | radix | 1.00 | 8.39M–67.1M |
| sorting | uniform32 | 16 | radix | 1.12 | 8.39M–67.1M |
| sorting | uniform32 | 8 | radix | 1.04 | 8.39M–67.1M |
| sorting | uniform64 | - | introsort | 1.14 | 8.39M–67.1M |
| sorting | uniform64 | 11 | radix | 1.06 | 8.39M–67.1M |
| sorting | uniform64 | 16 | radix | 1.17 | 8.39M–67.1M |
| sorting | uniform64 | 8 | radix | 1.02 | 8.39M–67.1M |
| sorting | uniform8 | - | introsort | 1.02 | 8.39M–67.1M |
| sorting | uniform8 | 11 | radix | 1.00 | 8.39M–67.1M |
| sorting | uniform8 | 16 | radix | 1.03 | 8.39M–67.1M |
| sorting | uniform8 | 8 | radix | 1.04 | 8.39M–67.1M |

## Run-to-run drift (pooled runs)

Cells: 2214; runs per cell: 1; median max/min spread of per-run medians: 1.000; 90th percentile: 1.000; worst: 1.17
- sorting/radix uniform64 param=11 n=512: spread 1.17
- sorting/radix uniform16 param=11 n=512: spread 1.14
- sorting/radix few_unique param=11 n=512: spread 1.13
- matmul/strassen uniform param=128 n=16: spread 1.12
- sorting/radix uniform64 param=8 n=512: spread 1.12

_Hardware counters were not available on this run (timing only)._
