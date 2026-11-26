// Module C2: houses. Rules: docs/gameplay/03-mieszkancy.md (Z:29-37), 18-domy-i-ekonomia-liczby.md (S-004 model),
// 24-algorytmy-silnika.md (daily check, devolution after 3 failed checks [RODZINA], consumption on days 0 and 8).
// Level L holds while appeal >= AppD(L) and the needs of L are met; it evolves to L+1 when appeal >= AppE(L) and
// the needs of L+1 are met (one level per day). Devolution drops to the highest level still satisfied; an elite
// house below Residence is abandoned (Z:34).
#pragma once

#include <cstdint>
#include <vector>

#include "bo/core/city.h"
#include "bo/core/data.h"

namespace bo::core {

const std::vector<HousingLevel>& levels_of(const DataCatalog& catalog, const HouseState& h);

// Culture points of a house: sum over performer walkers whose coverage counter is running (doc 18).
std::int32_t house_culture(const Building& b, const DataCatalog& catalog);

// Whether the needs of `level` (water, goods, culture) are met now; appeal is checked by the caller.
bool house_needs_met(const Building& b, const DataCatalog& catalog, const HousingLevel& level, Difficulty d);

// People a house can hold at its level.
std::uint32_t house_capacity(const DataCatalog& catalog, const HouseState& h, Difficulty d);

struct HouseChange {
    BuildingId id = kNoBuilding;
    std::uint8_t level = 0;
    bool abandoned = false;
};

// Daily evolution check for every inhabited house. Uses city.appeal (recomputed before). Excess people after a
// devolution become homeless (city.homeless).
std::vector<HouseChange> update_houses(CityState& city, const DataCatalog& catalog, Difficulty d);

// Consumption (twice a month): food scales with population (0.25 per person per month for common houses),
// fleece, olive oil and wine 2 per house per month (doc 18).
void consume_house_goods(CityState& city, const DataCatalog& catalog);

}  // namespace bo::core
