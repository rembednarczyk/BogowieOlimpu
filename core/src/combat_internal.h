// Combat internals shared by combat.cpp and tactics.cpp (not part of the core's API).
#pragma once

#include <cstdint>
#include <utility>

#include "bo/core/data.h"
#include "bo/core/military.h"
#include "bo/core/state.h"
#include "bo/core/walker.h"

namespace bo::core::combat_detail {

const UnitStats* stats_of(const DataCatalog& catalog, const Walker& w);
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

}  // namespace bo::core::combat_detail
