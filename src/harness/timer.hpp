// Wall-clock timing. steady_clock is monotonic and, on Linux/x86, backed by
// clock_gettime(CLOCK_MONOTONIC) via the vDSO (~20 ns overhead). The TSC is
// also exposed for cycle-level comparisons when hardware counters are
// unavailable; note that modern x86 TSCs tick at a constant nominal rate,
// so they measure elapsed time rather than actual core cycles.
#pragma once
#include <chrono>
#include <cstdint>

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#endif

namespace harness {

inline uint64_t now_ns() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

inline uint64_t rdtsc() {
#if defined(__x86_64__) || defined(__i386__)
    unsigned aux;
    return __rdtscp(&aux);  // serializing w.r.t. prior instructions
#else
    return now_ns();
#endif
}

// Prevents the optimizer from eliding a computation whose result is unused.
template <class T>
inline void do_not_optimize(T const& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}

inline void clobber_memory() { asm volatile("" : : : "memory"); }

}  // namespace harness
