// Shared enums. Numeric values are part of the save format: append only, never reorder.
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace bo::core {

// Difficulty levels 1:1 with the reference (R-013: default Hero).
enum class Difficulty : std::uint8_t { Beginner = 0, Mortal, Hero, Titan, Olympian, Count };

// Tax rates (docs/gameplay/18-domy-i-ekonomia-liczby.md).
enum class TaxRate : std::uint8_t { None = 0, VeryLow, Low, Normal, High, VeryHigh, Outrageous, Count };

// Wage rates (docs/gameplay/05-przemysl-praca.md).
enum class WageRate : std::uint8_t { None = 0, VeryLow, Low, Normal, High, VeryHigh, Count };

constexpr std::string_view difficulty_code(Difficulty d) {
    constexpr std::string_view kCodes[] = {"B", "M", "H", "T", "O"};
    return kCodes[static_cast<std::uint8_t>(d)];
}

constexpr std::optional<Difficulty> difficulty_from_code(std::string_view code) {
    constexpr std::string_view kCodes[] = {"B", "M", "H", "T", "O"};
    for (std::uint8_t i = 0; i < static_cast<std::uint8_t>(Difficulty::Count); ++i) {
        if (kCodes[i] == code) {
            return static_cast<Difficulty>(i);
        }
    }
    return std::nullopt;
}

}  // namespace bo::core
