// Module C7: fire, collapse, disease, crime. Rules: docs/gameplay/07 (Z:84-90), 20 (S-004 risk values),
// 24 (family structure). Parameters: data/risks.json. Assumptions: L-104.
// Counters grow each day by the building's risk (fire_risk, damage_risk from building_stats.json; DRI and CRI
// from housing.json); a visiting superintendent, healer or watchman clears them (provide_coverage).
#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "bo/core/data.h"
#include "bo/core/event.h"
#include "bo/core/state.h"

namespace bo::core {

struct RiskEvents {
    std::vector<std::pair<DisasterKind, BuildingId>> disasters;
    std::vector<BuildingId> removed;
    std::vector<BuildingId> placed;
    std::vector<WalkerId> spawned;
};

// Daily: counters, fires (burning 32 days, may spread every 8 days with chance 1/4 downwind), collapses, plague
// with quarantine, criminals leaving houses. Buildings lost to fire or collapse leave ruins.
RiskEvents advance_risks(WorldState& state, const DataCatalog& catalog);

// Every tick: outlaws within 2 tiles of a watchman or nobleman are stopped (Z:88).
std::vector<WalkerId> catch_outlaws(WorldState& state, const DataCatalog& catalog);

// A thief empties an elite house's stores; a vandal destroys the house (Z:87-88). The criminal then vanishes.
void crime_at_target(WorldState& state, const DataCatalog& catalog, Walker& w, RiskEvents& ev);

// A superintendent sent to a fire puts it out if it burns no longer than extinguish_days, then goes home.
void extinguish_at_target(WorldState& state, const DataCatalog& catalog, Walker& w);

// Every tick: a mugger on the same tile as a passer-by kills them; the nearest household loses
// mugger_victims residents [eZeus, Z:87]. A mugger stops after kMuggerVictims attacks [ZAŁOŻENIE, L-104].
inline constexpr std::uint32_t kMuggerVictims = 3;
std::vector<WalkerId> muggings(WorldState& state, const DataCatalog& catalog);

// Replaces a building with ruins on its own tiles (roads under it stay); residents become homeless.
void ruin_building(WorldState& state, const DataCatalog& catalog, BuildingId id, RiskEvents& ev);

}  // namespace bo::core
