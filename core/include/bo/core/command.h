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

// Granary or Storehouse order for one good (Z:66-69). kind: 0 Accept, 1 Get, 2 Empty, 3 Don't Accept.
struct SetStorageOrder {
    std::uint32_t building = 0;
    std::string good;
    std::uint8_t kind = 0;
    std::uint32_t limit = 32;  // loads
};

// Trade (Z:76-79): the partner city of a Trading Post or Pier, and its import and export orders per good.
struct SetTradePartner {
    std::uint32_t building = 0;
    std::int32_t partner = -1;
};

struct SetTradeOrder {
    std::uint32_t building = 0;
    std::string good;
    bool import_on = false;
    std::uint32_t import_limit = 0;
    bool export_on = false;
    std::uint32_t export_keep = 0;
};

using Command = std::variant<SetTaxRate, SetWageRate, PlaceBuilding, PlaceRoad, PlaceRoadblock, ClearLand, Demolish,
                             Undo, SetSectorPriority, SetIndustryEnabled, SetStorageOrder, SetTradePartner,
                             SetTradeOrder>;

// A command scheduled for execution at the start of `tick`, in issue order (`sequence`).
struct ScheduledCommand {
    std::uint64_t tick = 0;
    std::uint64_t sequence = 0;
    Command command;
};

using CommandLog = std::vector<ScheduledCommand>;

}  // namespace bo::core
