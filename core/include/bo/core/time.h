// Module A1: game time.
// Pattern: fixed timestep (G. Fiedler, "Fix Your Timestep!"). One tick = one reference animation frame.
// Values: 51 ticks per day, 16 days per month, 12 months per year (docs/gameplay/24-algorytmy-silnika.md;
// S-005 Zeus forum measurement, S-007 Akhenaten "9792" = 51*16*12).
#pragma once

#include <cstdint>

namespace bo::core {

inline constexpr std::uint32_t kTicksPerDay = 51;
inline constexpr std::uint32_t kDaysPerMonth = 16;
inline constexpr std::uint32_t kMonthsPerYear = 12;
inline constexpr std::uint32_t kTicksPerMonth = kTicksPerDay * kDaysPerMonth;
inline constexpr std::uint32_t kTicksPerYear = kTicksPerMonth * kMonthsPerYear;
// One tile is 15 movement steps; a standard walker makes 1 step per tick (54.4 tiles per month).
inline constexpr std::uint32_t kStepsPerTile = 15;

// Absolute game time counted in ticks since the start of the scenario.
class GameTime {
public:
    constexpr GameTime() = default;
    constexpr explicit GameTime(std::uint64_t tick) : tick_(tick) {}

    [[nodiscard]] constexpr std::uint64_t tick() const { return tick_; }
    [[nodiscard]] constexpr std::uint32_t tick_in_day() const { return static_cast<std::uint32_t>(tick_ % kTicksPerDay); }
    [[nodiscard]] constexpr std::uint64_t total_days() const { return tick_ / kTicksPerDay; }
    [[nodiscard]] constexpr std::uint32_t day_in_month() const {
        return static_cast<std::uint32_t>(total_days() % kDaysPerMonth);
    }
    [[nodiscard]] constexpr std::uint64_t total_months() const { return total_days() / kDaysPerMonth; }
    // Month of the year, 0 = January.
    [[nodiscard]] constexpr std::uint32_t month() const { return static_cast<std::uint32_t>(total_months() % kMonthsPerYear); }
    // Years elapsed since scenario start (the scenario adds its own start year for display).
    [[nodiscard]] constexpr std::uint64_t year() const { return total_months() / kMonthsPerYear; }

    [[nodiscard]] constexpr bool is_day_start() const { return tick_in_day() == 0; }
    [[nodiscard]] constexpr bool is_month_start() const { return is_day_start() && day_in_month() == 0; }
    [[nodiscard]] constexpr bool is_year_start() const { return is_month_start() && month() == 0; }

    constexpr void advance() { ++tick_; }

    friend constexpr bool operator==(GameTime a, GameTime b) { return a.tick_ == b.tick_; }

private:
    std::uint64_t tick_ = 0;
};

}  // namespace bo::core
