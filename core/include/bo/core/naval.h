// Module D3 stage 3: fleets (docs/gameplay/11-wojsko.md, Z:141-144; assumptions L-124). Players' triremes sail
// from their wharves and fight on the water; invaders may come by sea in transports with warships.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "bo/core/data.h"
#include "bo/core/military.h"
#include "bo/core/state.h"

namespace bo::core {

// Ships of either side: the player's triremes, the invaders' transports and warships.
bool is_ship(const Walker& w);
bool is_transport(const Walker& w);
// A frigate armed with Atlantean Fire (its wharf took orichalc for it) strikes harder (P:15-18, L-133).
std::int32_t with_atlantean_fire(const WorldState& state, const DataCatalog& catalog, const Walker& w, std::int32_t attack);

// Daily: a wharf that has built its trireme launches it at its mooring (one ship a wharf, Z:141; a sunk one is
// built again); a ship beaten below repair_below_percent goes back to the wharf (Carry Out Repairs, Z:142),
// mends repair_hp_per_day there and returns to its post; an idle ship sails back to its post.
void advance_fleet(WorldState& state, const DataCatalog& catalog, CombatEvents& ev);

// Orders (Z:142): 0 Defend Location (at x, y: a water tile; -1: where it is), 1 Attack All Enemies, 2 Carry Out
// Repairs. Returns an error key or "".
std::string set_ship_order(WorldState& state, const DataCatalog& catalog, std::uint32_t ship, std::uint8_t order,
                           TilePos at);

// Invasion by sea (Z:144): transports of transport_soldiers each and a warship for every
// transports_per_warship, from the sea entry to the water by the shore nearest the goal. "" or an error key.
std::string start_sea_invasion(WorldState& state, const DataCatalog& catalog, std::uint32_t soldiers,
                               BuildingId goal, CombatEvents& ev);

// Triremes with an army abroad (Z:123, Z:177): every crewed one leaves the city's waters; returns their wharves.
std::vector<BuildingId> sail_abroad(WorldState& state);
// Back from abroad: the trireme of `wharf` is at its mooring again, crewed and whole [ZAŁOŻENIE, L-124].
void sail_home(WorldState& state, const DataCatalog& catalog, BuildingId wharf);

}  // namespace bo::core
