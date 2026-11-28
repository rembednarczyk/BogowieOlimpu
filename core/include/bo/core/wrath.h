// Module D5 stage 2 (docs/gameplay/10, 21, 23; Z:125-127; assumptions L-054b, L-115): hostile gods and natural
// disasters as scenario events.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "bo/core/city.h"
#include "bo/core/data.h"

namespace bo::core {

struct WorldState;

struct WrathEvents {
    bool repelled = false;              // a god with a sanctuary drove the attacker away (Z:119, Z:122, P:21)
    std::vector<BuildingId> removed;
    std::vector<BuildingId> fires;      // fire counters set to the threshold: they burn at the next risk check
    std::vector<WalkerId> walkers_gone;
    std::vector<TilePos> tiles_changed;
    std::string invasion_nation;        // Artemis (Amazons), Ares (Sparti)
    std::uint32_t invasion_soldiers = 0;
};

// A hostile god attacks (doc 10 table, doc 21). Zeus from his sanctuary defends against any god, Hera against
// Zeus, Aphrodite against Ares, Hephaestus, Hermes and Dionysus. Numbers in gods_rules.json wrath_numbers.
WrathEvents god_attack(WorldState& state, const DataCatalog& catalog, const std::string& god);

// A natural disaster (doc 23; L-090, L-115): every building within `radius` of (x, y) is destroyed; an
// earthquake and lava also turn the land there to rock.
WrathEvents disaster(WorldState& state, const std::string& kind, TilePos centre, std::int32_t radius);

}  // namespace bo::core
