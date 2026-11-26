// Modules C4 (production) and C5 (distribution), stage 1: farms, Granary and Storehouse deliveries, agora
// vendors and peddlers. Rules: docs/gameplay/04, 06, 19, 24. Assumptions: L-100 (vendor load, stall refill
// threshold, how much a peddler leaves in a house).
#pragma once

#include <cstdint>
#include <vector>

#include "bo/core/data.h"
#include "bo/core/state.h"

namespace bo::core {

// Vendor fetches up to this many loads [ZAŁOŻENIE, L-100: same as a storage deliveryman, doc 19].
inline constexpr std::uint32_t kVendorLoads = 4;
// A stall sends its vendor when it holds less than this many units of its goods [ZAŁOŻENIE, L-100].
inline constexpr std::uint32_t kStallRefillUnits = 200;
// Default vendor reach when the data gives none (doc 24: 40 tiles [RODZINA]).
inline constexpr std::int32_t kVendorReach = 40;

// Daily production. Farms gain loads_per_year x staffing percent per day and harvest once a year at the start
// of their harvest month: loads = progress / (100 x 192); a full year at full staffing gives loads_per_year
// (S-004, doc 19). Other producers arrive in stage 2.
void advance_production(WorldState& state, const DataCatalog& catalog);

struct LogisticsEvents {
    std::vector<WalkerId> spawned;
};

// Daily: producers with finished loads send one deliveryman (1 load, doc 19) to the nearest receiver by road,
// in the order of the building's delivery_priority; stalls below the refill level send their vendor to the
// nearest storage of their source types within reach.
LogisticsEvents dispatch_workers(WorldState& state, const DataCatalog& catalog);

// Every tick: working walkers at their target hand over or take goods and turn home; walkers back home unload.
void handle_arrivals(WorldState& state, const DataCatalog& catalog);
void unload_at_home(WorldState& state, const DataCatalog& catalog, const Walker& w);

// Peddler visit (doc 06): every house in the 5x5 square around `w` gets the goods its current or next level
// needs from the stalls of the peddler's agora, up to the house target (L-100).
void peddler_sells(WorldState& state, const DataCatalog& catalog, const Walker& w);

// Units a house wants to hold of each house good [ZAŁOŻENIE, L-100: two months of the food of its current or
// next level, whichever is more; 4 units (two months) of fleece, oil, wine; 1 armor, 1 horse when needed].
std::uint32_t house_target(const DataCatalog& catalog, const HouseState& h, HouseGood g, Difficulty d);

}  // namespace bo::core
