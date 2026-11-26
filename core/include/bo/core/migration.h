// Module C8: house mood, popularity, migration, homeless (docs/gameplay/13, 24; parameters: data/migration.json).
// Simplification until the map entry and immigrant walkers exist: arriving and leaving people appear in and
// disappear from houses at once (deviation O-006).
#pragma once

#include <cstdint>

#include "bo/core/data.h"
#include "bo/core/state.h"

namespace bo::core {

std::uint32_t city_population(const CityState& city);

// Mood update (days 0 and 8): every inhabited house adds tax, wage, unemployment, debt and food terms; cities
// below the small-city size stay at the base value. Then popularity = mean happiness.
void update_moods(WorldState& state, const DataCatalog& catalog);

// Percent of the daily batch from the popularity table (doc 24).
std::int32_t migration_percent(const MigrationParams& p, std::int32_t popularity);

struct MigrationResult {
    std::uint32_t arrived = 0;
    std::uint32_t left = 0;
    std::uint32_t rehoused = 0;
};

// Daily: houses cut off from roads for > 4 days empty out; homeless people are rehoused first; then the pool
// grows by batch x percent and, once it reaches the minimum batch, people arrive (houses with >= 8 free places
// first) or leave (lowest house levels first, only above the emigration minimum).
MigrationResult migrate(WorldState& state, const DataCatalog& catalog);

}  // namespace bo::core
