// Culture: two-phase performers (B-008, B-010). Rules: docs/gameplay/02-spacerujacy.md, 09-kultura.md (Z:26,
// Z:105-109, P:10-12), numbers 18 (city-wide bonus of a working Stadium or Museum).
// Model after eZeus (S-008, read only: buildings/epatrolsourcebuilding.cpp, epatroltarget.cpp): a staffed school
// with road access sends one performer every `school_send_days` to the next venue of its kind reachable by road
// (round robin by building id); the venue performs for `performance_days` after the performer arrives and
// meanwhile sends its own performers roaming like any service building [ZAŁOŻENIE, L-120].
#pragma once

#include <cstdint>

#include "bo/core/city.h"
#include "bo/core/data.h"
#include "bo/core/walker.h"

namespace bo::core {

// Daily: performances run down; schools send performers.
void run_schools(CityState& city, WalkerStore& walkers, const DataCatalog& catalog);

// A performer reached its venue: the performance starts (or goes on) and the walker leaves.
void performer_arrived(CityState& city, const DataCatalog& catalog, Walker& w);

// Whether `b` sends performer `wt` roaming today: a venue during a performance.
bool venue_performs(const Building& b, const WalkerType& wt);

// Points every house gets from working Stadiums and Museums (doc 18: +10 each).
std::int32_t city_culture_bonus(const CityState& city, const DataCatalog& catalog);

}  // namespace bo::core
