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

// The land a sanctuary just completed changes (Hades: silver ore, Hephaestus: copper ore, Demeter: meadow);
// returns the tiles changed.
std::vector<TilePos> bless_land(CityState& city, const DataCatalog& catalog, BuildingId sanctuary);

// Daily, the other lasting gifts at work; the (god, kind) of each, for GodBlessing events: Zeus raises the
// opinion of every leader once a month ("opinion"), Poseidon's Kraken sinks an enemy ship ("kraken"), Hades's
// Cerberus kills an invader ("cerberus"), Artemis hunts down a wolf ("hunt"), Ares's dragon ("dragon").
std::vector<std::pair<std::string, std::string>> advance_blessings(WorldState& state, const DataCatalog& catalog);

// Poseidon (Z:119): the days a fishing boat spends at sea.
std::uint32_t blessed_fishing_ticks(const CityState& city, const DataCatalog& catalog, std::uint32_t ticks);
// Aphrodite (Z:122): appeal added to every tile of the city.
std::int32_t blessed_appeal(const CityState& city, const DataCatalog& catalog);

}  // namespace bo::core
