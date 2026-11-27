// Build order (docs/PRAKTYKI.md ch. 4, "autopilot"): a scripted mayor for the demo and the whole-game gate test.
// A JSON file of dated steps; each day the caller asks for the commands due and issues them like the player's.
// Stateless: a step runs on its day, or with `retry` on every later day until its building stands, so a loaded
// game carries on. Steps that name a building do so by a tile of it ("at": [x, y]), resolved when due.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "bo/core/command.h"
#include "bo/core/simulation.h"

namespace bo::core {

struct BuildStep {
    std::uint64_t day = 0;
    bool retry = false;                        // place steps: try daily until the building stands
    std::uint64_t every = 0;                   // other steps: again every `every` days from `day` (0: once)
    std::uint64_t until = 0;                   // with `every`: the last day (0: no end)
    bool fulfill_all = false;                  // send what every open city request asks for (Z:161)
    bool all_companies = false;                // SetCompanyTactic for every company there is when due (Z:138)
    bool all_ships = false;                    // SetShipOrder for every trireme there is when due (Z:141-142)
    bool dispatch_ready = false;               // attack or raid with every company at home but rabble (Z:177)
    std::map<std::string, std::uint32_t> when;  // loads in the city's storage needed first
    Command command;
    std::optional<TilePos> at;  // building commands: a tile of the building, resolved when due
    bool later = false;  // issued a tick into the day, after the day's other steps took effect (e.g. Undo, Z:20)
};

struct BuildOrder {
    std::vector<BuildStep> steps;
};

// Parses the build order format (scenarios/README.md). Empty optional and `error` set on bad input.
std::optional<BuildOrder> parse_build_order(const std::string& json_text, std::string& error);

// Commands due on day `day` (0-based from the start of the game): `later` false, those for the start of the day;
// true, those for a tick later (BuildStep::later).
std::vector<Command> due_commands(const BuildOrder& order, const Simulation& sim, std::uint64_t day,
                                  bool later = false);

}  // namespace bo::core
