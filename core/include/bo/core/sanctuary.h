// Module D1 stage 1 (docs/gameplay/10, 21; Z:116-118, Z:124; S-005 Vriesea; assumptions L-111): building a
// sanctuary with artisans and cart deliveries, then sacrifices.
#pragma once

#include <string>
#include <vector>

#include "bo/core/city.h"
#include "bo/core/data.h"

namespace bo::core {

struct WorldState;

// Site rules (doc 21): at most 4 sanctuaries; the foundation needs the initial marble in storage.
std::string sanctuary_site_check(const CityState& city, const DataCatalog& catalog, const std::string& type);
// Lays the foundation: takes the initial marble from storage and sets the materials still to build in.
void lay_foundation(CityState& city, const DataCatalog& catalog, Building& b);

struct MythEvents {
    std::vector<WalkerId> spawned;
    std::vector<BuildingId> removed;    // animals sacrificed
    std::vector<BuildingId> completed;  // sanctuaries finished today
};

// Daily: guilds send an artisan to the nearest unfinished sanctuary; storehouses send carts with 4 loads of
// marble or wood (Z:117) or 1 sculpture; each artisan on site builds in one delivered load a day
// [ZAŁOŻENIE, L-111] in three phases (tiles, construction with one artisan, sculptures; S-005); finished sanctuaries send a priest for a sheep or goat, else food from a Granary
// (Z:124), and lose the god's favor after a year without a sacrifice [ZAŁOŻENIE, L-052].
MythEvents advance_sanctuaries(WorldState& state, const DataCatalog& catalog);

inline constexpr std::uint32_t kBulkLoads = 4;
inline constexpr std::uint32_t kAltarMarble = 4;     // doc 21
inline constexpr std::uint32_t kSlabsPerShift = 4;   // 8 tiles of half a slab, then back to the guild (S-005)             // marble and wood only in large deliveries (Z:117)
inline constexpr std::uint16_t kSacrificeEveryDays = 32;   // [ZAŁOŻENIE, L-052]
inline constexpr std::uint16_t kFavorLostAfterDays = 192;  // [ZAŁOŻENIE, L-052]

}  // namespace bo::core
