// Module A3: events are the only way out of the simulation (docs/PRAKTYKI.md ch. 2).
// Pattern: Event Queue (R. Nystrom, "Game Programming Patterns"). Presentation (render, audio, HUD, haptics)
// drains the queue after each step; events never feed back into the simulation.
#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "bo/core/types.h"

namespace bo::core {

struct DayStarted {
    std::uint64_t day = 0;
};

struct MonthStarted {
    std::uint32_t month = 0;  // 0 = January
    std::uint64_t year = 0;
};

struct YearStarted {
    std::uint64_t year = 0;
};

struct TaxRateChanged {
    TaxRate rate = TaxRate::Normal;
};

struct WageRateChanged {
    WageRate rate = WageRate::Normal;
};

struct CommandRejected {
    std::uint32_t command_index = 0;
    std::string reason_key;  // i18n key, never player-facing text (docs/PRAKTYKI.md ch. 12)
};

using Event = std::variant<DayStarted, MonthStarted, YearStarted, TaxRateChanged, WageRateChanged, CommandRejected>;

struct TimedEvent {
    std::uint64_t tick = 0;
    Event event;
};

using EventQueue = std::vector<TimedEvent>;

}  // namespace bo::core
