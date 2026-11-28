// Module C1: appeal (desirability) field. Rule: docs/gameplay/20-atrakcyjnosc-i-budynki.md (S-004):
// appeal(d) = initial + floor((d - 1) / step_tiles) * step_change for 1 <= d <= range, where d is the
// Chebyshev distance from the footprint edge; contributions of all buildings add up on a tile.
#pragma once

#include <cstdint>
#include <vector>

#include "bo/core/city.h"
#include "bo/core/data.h"

namespace bo::core {

std::int32_t appeal_at_distance(const AppealParams& a, std::int32_t d);

// Recomputes the whole field (one value per tile, row-major). Boulevards and avenues emit from each ornament
// tile and from their road tile separately (S-004).
std::vector<std::int32_t> compute_appeal(const CityState& city, const DataCatalog& catalog);

// Appeal a building sees: the highest value over its tiles [RODZINA: Caesar III takes the maximum].
std::int32_t building_appeal(const CityState& city, const Building& b);

}  // namespace bo::core
