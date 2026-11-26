// Module A3: commands are the only way into the simulation (docs/PRAKTYKI.md ch. 2).
// Pattern: Command (GoF) as a closed std::variant; dispatch uses std::visit with one handler per alternative,
// so a missing handler is a compile error ("typ wymuszający komplet"). Player input, autopilot, scenario
// scripts and tests all issue the same commands; replay = seed + command log.
// Append new command types at the end of the variant only (the index is stored in saves and logs).
#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "bo/core/types.h"

namespace bo::core {

struct SetTaxRate {
    TaxRate rate = TaxRate::Normal;
};

struct SetWageRate {
    WageRate rate = WageRate::Normal;
};

struct PlaceBuilding {
    std::string type;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::uint8_t rotation = 0;  // see place_building in city.h
};

struct PlaceRoad {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct PlaceRoadblock {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct ClearLand {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct Demolish {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

// Reverts the build commands of the last tick that had any (Z:20), within kUndoWindowTicks.
struct Undo {};

// Labour panel (Z:62-63).
struct SetSectorPriority {
    std::uint8_t sector = 0;    // bo::core::Sector
    std::uint8_t priority = 3;  // 0 none .. 5 very high
};

struct SetIndustryEnabled {
    std::string type;  // building type
    bool enabled = true;
};

using Command = std::variant<SetTaxRate, SetWageRate, PlaceBuilding, PlaceRoad, PlaceRoadblock, ClearLand, Demolish,
                             Undo, SetSectorPriority, SetIndustryEnabled>;

// A command scheduled for execution at the start of `tick`, in issue order (`sequence`).
struct ScheduledCommand {
    std::uint64_t tick = 0;
    std::uint64_t sequence = 0;
    Command command;
};

using CommandLog = std::vector<ScheduledCommand>;

}  // namespace bo::core
