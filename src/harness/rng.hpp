// Deterministic, fast PRNG (xoshiro256**) seeded via splitmix64.
// Every benchmark input is reproducible from (seed, n, distribution).
#pragma once
#include <cstdint>

namespace harness {

class Rng {
public:
    explicit Rng(uint64_t seed) { seed_with(seed); }

    void seed_with(uint64_t seed) {
        // splitmix64 to expand the seed into four state words
        for (auto& w : s_) {
            seed += 0x9E3779B97F4A7C15ULL;
            uint64_t z = seed;
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
            w = z ^ (z >> 31);
        }
    }

    uint64_t next() {
        const uint64_t result = rotl(s_[1] * 5, 7) * 9;
        const uint64_t t = s_[1] << 17;
        s_[2] ^= s_[0];
        s_[3] ^= s_[1];
        s_[1] ^= s_[2];
        s_[0] ^= s_[3];
        s_[2] ^= t;
        s_[3] = rotl(s_[3], 45);
        return result;
    }

    // Uniform integer in [0, bound)
    uint64_t below(uint64_t bound) {
        if (bound == 0) return 0;
        // Lemire's nearly-divisionless method
        __uint128_t m = (__uint128_t)next() * bound;
        uint64_t l = (uint64_t)m;
        if (l < bound) {
            uint64_t t = (0 - bound) % bound;
            while (l < t) {
                m = (__uint128_t)next() * bound;
                l = (uint64_t)m;
            }
        }
        return (uint64_t)(m >> 64);
    }

    // Uniform double in [0,1)
    double uniform01() { return (next() >> 11) * 0x1.0p-53; }

private:
    static uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
    uint64_t s_[4];
};

}  // namespace harness
