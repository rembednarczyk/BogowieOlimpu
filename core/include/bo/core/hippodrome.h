// The hippodrome of Atlantis (docs/gameplay/15-atlantyda.md, P:13-15; doc 18; assumptions L-127): a track of
// 4x4 segments that must close into a loop; its handler brings horses from the horse ranches; with enough
// horses the races earn money every month.
#pragma once

#include <cstdint>
#include <vector>

#include "bo/core/data.h"
#include "bo/core/logistics.h"
#include "bo/core/state.h"

namespace bo::core {

struct Track {
    std::vector<BuildingId> segments;  // in id order; the first one keeps the horses and sends the handler
    bool closed = false;               // every segment joins exactly two others, all in one loop (P:13)
};

Track hippodrome_track(const CityState& city);

// Stades of a working track: closed, with the horses its length needs (doc 18); 0 when it is not racing.
std::uint32_t working_hippodrome(const WorldState& state, const DataCatalog& catalog);

// The track's effect on every house's mood at each update (P:14): a bonus from mood_from stades of a racing
// track, unrest above unrest_above (doc 18) [ZAŁOŻENIE, L-127]; 0 otherwise.
std::int32_t hippodrome_mood(const WorldState& state, const DataCatalog& catalog);

// Daily: the handler fetches horses (one load a trip) while the track has fewer than it needs; on the first
// day of a month a racing track pays its income into the treasury.
void advance_hippodrome(WorldState& state, const DataCatalog& catalog, LogisticsEvents& ev);

}  // namespace bo::core
