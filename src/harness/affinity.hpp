// Pin the benchmark thread to one core so that measurements are not
// perturbed by migration (cold caches, different frequency domains).
#pragma once
#include <string>

#if defined(__linux__)
#include <sched.h>
#include <pthread.h>
#endif

namespace harness {

// Returns an empty string on success, otherwise a diagnostic.
inline std::string pin_to_core(int core) {
    if (core < 0) return "";
#if defined(__linux__)
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
    if (rc != 0) return "pthread_setaffinity_np failed (rc=" + std::to_string(rc) + ")";
    return "";
#else
    return "affinity pinning not supported on this platform";
#endif
}

}  // namespace harness
