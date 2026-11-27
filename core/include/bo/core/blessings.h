// Lasting gifts of the gods with a sanctuary (docs/gameplay/10-mitologia.md table, Z:119-122; gods.json benefits)
// [ZAŁOŻENIE, L-131]: Athena's stronger soldiers and sacred grove, Dionysus's vines, Hermes's quicker traders and
// deliverymen, Ares's dragon.
#pragma once

#include <cstdint>
#include <vector>

#include "bo/core/data.h"
#include "bo/core/state.h"

namespace bo::core {

// Athena (Z:120): the blow of one of the city's soldiers (WalkerJob::Soldier).
std::int32_t blessed_attack(const WorldState& state, const DataCatalog& catalog, const Walker& w, std::int32_t attack);

// Hermes (Z:122): days between traders at a trade building, and the pace of traders and deliverymen.
std::uint32_t blessed_trader_interval(const CityState& city, const DataCatalog& catalog, std::uint32_t days);
std::int64_t blessed_speed(const CityState& city, const DataCatalog& catalog, std::int64_t speed_raw);

// The grove of a sanctuary just completed (Athena: olive trees, Dionysus: vines): up to grove_plants on free land
// next to it, the land turned to meadow. Returns the new buildings.
std::vector<BuildingId> plant_grove(CityState& city, const DataCatalog& catalog, Difficulty d, BuildingId sanctuary);

// Daily: every dragon_every_days Ares's dragon eats an invader on the land, else a wolf (Z:121); true when it ate.
bool feed_dragon(WorldState& state, const DataCatalog& catalog);

}  // namespace bo::core
