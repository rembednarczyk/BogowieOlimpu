// Module A2: deterministic randomness.
// Pattern: PCG32 (M. E. O'Neill, "PCG: A Family of Simple Fast Space-Efficient Statistically Good Algorithms
// for Random Number Generation", 2014), seeded through SplitMix64 (S. Vigna). Independent streams per system
// (docs/PRAKTYKI.md, chapter 3): a new system gets a new stream so it never reshuffles existing ones.
// Domain code must never use std::rand, <random> engines or wall-clock time (checked by tools/checks/check_core.py).
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace bo::core {

class Pcg32 {
public:
    constexpr Pcg32() = default;
    constexpr Pcg32(std::uint64_t seed, std::uint64_t sequence) { reseed(seed, sequence); }

    constexpr void reseed(std::uint64_t seed, std::uint64_t sequence) {
        state_ = 0;
        inc_ = (sequence << 1U) | 1U;
        next_u32();
        state_ += seed;
        next_u32();
    }

    constexpr std::uint32_t next_u32() {
        const std::uint64_t old = state_;
        state_ = old * 6364136223846793005ULL + inc_;
        const auto xorshifted = static_cast<std::uint32_t>(((old >> 18U) ^ old) >> 27U);
        const auto rot = static_cast<std::uint32_t>(old >> 59U);
        return (xorshifted >> rot) | (xorshifted << ((32U - rot) & 31U));
    }

    // Uniform value in [0, bound) without modulo bias (Lemire-style rejection on the low threshold).
    constexpr std::uint32_t next_below(std::uint32_t bound) {
        if (bound == 0) {
            return 0;
        }
        const std::uint32_t threshold = (0U - bound) % bound;
        for (;;) {
            const std::uint32_t r = next_u32();
            if (r >= threshold) {
                return r % bound;
            }
        }
    }

    [[nodiscard]] constexpr std::uint64_t state() const { return state_; }
    [[nodiscard]] constexpr std::uint64_t increment() const { return inc_; }
    constexpr void restore(std::uint64_t state, std::uint64_t inc) {
        state_ = state;
        inc_ = inc;
    }

private:
    std::uint64_t state_ = 0x853c49e6748fea9bULL;
    std::uint64_t inc_ = 0xda3e39cb94b95bdbULL;
};

constexpr std::uint64_t splitmix64(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27U)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31U);
}

// Append new streams at the end only: the numeric value seeds the stream (stable ids, docs/PRAKTYKI.md ch. 3).
enum class RngStream : std::uint8_t {
    MapGeneration = 0,
    Migration,
    Hazards,
    Disease,
    Crime,
    Gods,
    Monsters,
    World,
    Military,
    Walkers,
    Count
};

class RngSet {
public:
    explicit RngSet(std::uint64_t seed = 0) { reseed(seed); }

    void reseed(std::uint64_t seed) {
        seed_ = seed;
        for (std::size_t i = 0; i < streams_.size(); ++i) {
            streams_[i].reseed(splitmix64(seed ^ splitmix64(i + 1)), i + 1);
        }
    }

    Pcg32& stream(RngStream s) { return streams_[static_cast<std::size_t>(s)]; }
    [[nodiscard]] const Pcg32& stream(RngStream s) const { return streams_[static_cast<std::size_t>(s)]; }
    [[nodiscard]] std::uint64_t seed() const { return seed_; }
    [[nodiscard]] static constexpr std::size_t size() { return static_cast<std::size_t>(RngStream::Count); }

private:
    std::uint64_t seed_ = 0;
    std::array<Pcg32, static_cast<std::size_t>(RngStream::Count)> streams_{};
};

}  // namespace bo::core
