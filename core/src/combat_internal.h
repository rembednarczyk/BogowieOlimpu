// Combat internals shared by combat.cpp and tactics.cpp (not part of the core's API).
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <utility>

#include "bo/core/data.h"
#include "bo/core/military.h"
#include "bo/core/state.h"
#include "bo/core/walker.h"

namespace bo::core::combat_detail {

std::int32_t chebyshev(TilePos a, TilePos b);
const UnitStats* stats_of(const DataCatalog& catalog, const Walker& w);
bool fighter(const Walker& w);
bool defends(const Walker& w);
// An invader of the current invasion's nation at `at`, marching on its goal (the Palace, Z:137).
WalkerId spawn_invader(WorldState& st, const DataCatalog& catalog, const std::string& unit, TilePos at, CombatEvents& ev);
bool route(CityState& city, Walker& w, TilePos to, BuildingId goal = kNoBuilding);
void stand(Walker& w);
Company* company_of(WorldState& st, WalkerId id);

// tactics.cpp
std::int64_t side_strength(const WorldState& st, const DataCatalog& catalog, bool invaders);
bool invaders_stronger(const WorldState& st, const DataCatalog& catalog);
void divine_help(WorldState& st);
std::int32_t engage_tiles(const DataCatalog& catalog, const Company* c, const UnitStats& s);
void fight(WorldState& st, const DataCatalog& catalog, Walker& w, const UnitStats& s, Company* c,
           std::pair<std::int32_t, WalkerId> foe, bool rechase);

// naval.cpp
std::int32_t ship_engage(const WorldState& st, const DataCatalog& catalog, const Walker& w, const UnitStats& s);
std::optional<std::pair<std::int32_t, WalkerId>> naval_foe(const WorldState& st, const DataCatalog& catalog,
                                                           const Walker& w, const std::vector<WalkerId>& fighters,
                                                           std::int32_t limit);
void ship_idle(WorldState& st, Walker& w);
void land_transports(WorldState& st, const DataCatalog& catalog, CombatEvents& ev);

}  // namespace bo::core::combat_detail
