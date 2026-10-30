# Crossover summary

| pair | distribution | param | fast vs simple | crossover n* | 95% CI | status | ratio at n_max |
|---|---|---|---|---|---|---|---|
| sorting | few_unique | 11 | radix vs introsort | 2.78k | [2.55k, 3.02k] | crossed | 0.60 |
| sorting | few_unique | 8 | radix vs introsort | 3.26k | [3.02k, 7.12M] | crossed | 0.63 |
| sorting | nearly_sorted | 11 | radix vs introsort | - | - | never_faster | 3.00 |
| sorting | nearly_sorted | 8 | radix vs introsort | - | - | never_faster | 3.32 |
| sorting | uniform16 | 11 | radix vs introsort | 1.5k | [1.46k, 1.64k] | crossed | 0.22 |
| sorting | uniform16 | 8 | radix vs introsort | 1.7k | [1.62k, 1.81k] | crossed | 0.25 |
| sorting | uniform32 | 11 | radix vs introsort | 1.9k | [1.84k, 2.04k] | crossed | 0.26 |
| sorting | uniform32 | 8 | radix vs introsort | 1.92k | [1.87k, 2.05k] | crossed | 0.22 |
| sorting | uniform64 | 11 | radix vs introsort | 2.31k | [2.01k, 2.7k] | crossed | 0.28 |
| sorting | uniform64 | 8 | radix vs introsort | 2.39k | [2.16k, 2.66k] | crossed | 0.30 |
| matmul | uniform | 128 | strassen vs naive_ikj | 149 | [131, 199] | crossed | 0.49 |
| matmul | uniform | 16 | strassen vs naive_ikj | 73 | [56, 144] | crossed | 0.44 |
| matmul | uniform | 32 | strassen vs naive_ikj | 26 | [16, 33] | crossed | 0.39 |
| matmul | uniform | 64 | strassen vs naive_ikj | 75 | [23, 102] | crossed | 0.46 |
| matmul | uniform | 128 | strassen vs naive_ijk | 78 | [66, 88] | crossed | 0.10 |
| matmul | uniform | 16 | strassen vs naive_ijk | 73 | [69, 75] | crossed | 0.09 |
| matmul | uniform | 32 | strassen vs naive_ijk | 16 | - | always_faster | 0.08 |
| matmul | uniform | 64 | strassen vs naive_ijk | 73 | [69, 78] | crossed | 0.09 |
| bigint | uniform | 16 | karatsuba vs schoolbook | 39 | [34, 45] | crossed | 0.09 |
| bigint | uniform | 32 | karatsuba vs schoolbook | 33 | [23, 46] | crossed | 0.07 |
| bigint | uniform | 64 | karatsuba vs schoolbook | 77 | [26, 115] | crossed | 0.06 |
| bigint | uniform | 8 | karatsuba vs schoolbook | 1.08k | [101, 1.46k] | crossed | 0.13 |
| dijkstra | dense | - | fib_heap vs binary_heap | 4.42k | [468, 5.1k] | crossed | 0.92 |
| dijkstra | grid | - | fib_heap vs binary_heap | - | - | never_faster | 2.77 |
| dijkstra | random | 16 | fib_heap vs binary_heap | - | - | never_faster | 1.39 |
| dijkstra | random | 4 | fib_heap vs binary_heap | - | - | never_faster | 1.74 |
| dijkstra | random | 64 | fib_heap vs binary_heap | - | - | never_faster | 1.36 |
| dijkstra | dense | - | fib_heap vs lazy_heap | - | - | never_faster | 1.35 |
| dijkstra | grid | - | fib_heap vs lazy_heap | - | - | never_faster | 2.23 |
| dijkstra | random | 16 | fib_heap vs lazy_heap | - | - | never_faster | 1.51 |
| dijkstra | random | 4 | fib_heap vs lazy_heap | - | - | never_faster | 2.40 |
| dijkstra | random | 64 | fib_heap vs lazy_heap | - | - | never_faster | 1.28 |

## Empirical scaling exponents (fit over largest decade)

| pair | distribution | param | algorithm | exponent | fit range |
|---|---|---|---|---|---|
| bigint | uniform | - | schoolbook | 2.03 | 16.4k–131k |
| bigint | uniform | 16 | karatsuba | 1.60 | 65.5k–524k |
| bigint | uniform | 32 | karatsuba | 1.58 | 65.5k–524k |
| bigint | uniform | 64 | karatsuba | 1.59 | 65.5k–524k |
| bigint | uniform | 8 | karatsuba | 1.59 | 65.5k–524k |
| dijkstra | dense | - | binary_heap | 1.98 | 400–6.4k |
| dijkstra | dense | - | fib_heap | 1.81 | 400–6.4k |
| dijkstra | dense | - | lazy_heap | 1.82 | 400–6.4k |
| dijkstra | grid | - | binary_heap | 1.21 | 410k–6.55M |
| dijkstra | grid | - | fib_heap | 1.15 | 410k–6.55M |
| dijkstra | grid | - | lazy_heap | 1.14 | 410k–6.55M |
| dijkstra | random | 16 | binary_heap | 1.35 | 410k–6.55M |
| dijkstra | random | 16 | fib_heap | 1.23 | 410k–6.55M |
| dijkstra | random | 16 | lazy_heap | 1.20 | 410k–6.55M |
| dijkstra | random | 4 | binary_heap | 1.41 | 410k–6.55M |
| dijkstra | random | 4 | fib_heap | 1.28 | 410k–6.55M |
| dijkstra | random | 4 | lazy_heap | 1.24 | 410k–6.55M |
| dijkstra | random | 64 | binary_heap | 1.33 | 102k–1.64M |
| dijkstra | random | 64 | fib_heap | 1.28 | 102k–1.64M |
| dijkstra | random | 64 | lazy_heap | 1.23 | 102k–1.64M |
| matmul | uniform | - | naive_ijk | 3.82 | 128–1.02k |
| matmul | uniform | - | naive_ikj | 3.09 | 256–2.05k |
| matmul | uniform | 128 | strassen | 2.85 | 256–2.05k |
| matmul | uniform | 16 | strassen | 2.80 | 256–2.05k |
| matmul | uniform | 32 | strassen | 2.87 | 256–2.05k |
| matmul | uniform | 64 | strassen | 2.83 | 256–2.05k |
| sorting | few_unique | - | introsort | 1.03 | 4.19M–67.1M |
| sorting | few_unique | 11 | radix | 1.11 | 4.19M–67.1M |
| sorting | few_unique | 8 | radix | 0.98 | 4.19M–67.1M |
| sorting | nearly_sorted | - | introsort | 1.12 | 4.19M–67.1M |
| sorting | nearly_sorted | 11 | radix | 1.05 | 4.19M–67.1M |
| sorting | nearly_sorted | 8 | radix | 0.93 | 4.19M–67.1M |
| sorting | uniform16 | - | introsort | 1.00 | 4.19M–67.1M |
| sorting | uniform16 | 11 | radix | 1.03 | 4.19M–67.1M |
| sorting | uniform16 | 8 | radix | 1.06 | 4.19M–67.1M |
| sorting | uniform32 | - | introsort | 1.14 | 4.19M–67.1M |
| sorting | uniform32 | 11 | radix | 1.03 | 4.19M–67.1M |
| sorting | uniform32 | 8 | radix | 1.08 | 4.19M–67.1M |
| sorting | uniform64 | - | introsort | 1.13 | 4.19M–67.1M |
| sorting | uniform64 | 11 | radix | 1.01 | 4.19M–67.1M |
| sorting | uniform64 | 8 | radix | 1.06 | 4.19M–67.1M |

_Hardware counters were not available on this run (timing only)._
