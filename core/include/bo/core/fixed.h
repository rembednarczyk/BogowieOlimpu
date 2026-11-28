// Fixed-point number for deterministic arithmetic across platforms (no float in simulation state).
// Pattern: Q-format fixed point (Q32.16 in a 64-bit integer). Floats are allowed only in presentation code.
#pragma once

#include <cstdint>

namespace bo::core {

class Fixed {
public:
    static constexpr int kFracBits = 16;
    static constexpr std::int64_t kOne = std::int64_t{1} << kFracBits;

    constexpr Fixed() = default;
    static constexpr Fixed from_raw(std::int64_t raw) {
        Fixed f;
        f.raw_ = raw;
        return f;
    }
    static constexpr Fixed from_int(std::int64_t v) { return from_raw(v * kOne); }
    // Exact ratio num/den rounded toward zero, e.g. Fixed::ratio(272, 5) == 54.4.
    static constexpr Fixed ratio(std::int64_t num, std::int64_t den) { return from_raw((num * kOne) / den); }

    [[nodiscard]] constexpr std::int64_t raw() const { return raw_; }
    [[nodiscard]] constexpr std::int64_t to_int() const { return raw_ / kOne; }  // toward zero

    friend constexpr Fixed operator+(Fixed a, Fixed b) { return from_raw(a.raw_ + b.raw_); }
    friend constexpr Fixed operator-(Fixed a, Fixed b) { return from_raw(a.raw_ - b.raw_); }
    friend constexpr Fixed operator*(Fixed a, Fixed b) { return from_raw((a.raw_ * b.raw_) / kOne); }
    friend constexpr Fixed operator/(Fixed a, Fixed b) { return from_raw((a.raw_ * kOne) / b.raw_); }
    friend constexpr bool operator==(Fixed a, Fixed b) { return a.raw_ == b.raw_; }
    friend constexpr bool operator<(Fixed a, Fixed b) { return a.raw_ < b.raw_; }

private:
    std::int64_t raw_ = 0;
};

}  // namespace bo::core
