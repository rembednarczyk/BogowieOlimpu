// Module B4: path finding. Pattern: A* (Hart, Nilsson, Raphael 1968) on the 4-connected tile grid with the
// Manhattan heuristic; ties broken by tile index so the result is identical on every platform (doc 24).
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "bo/core/city.h"
#include "bo/core/map.h"

namespace bo::core {

enum class PathMode : std::uint8_t {
    Road = 0,            // roads including roadblocks (destination walkers pass roadblocks, Z:19-20)
    RoadNoRoadblock,     // roads without roadblocks (roaming walkers)
    Walkable,            // any walkable terrain off road; buildings block except the goal building
};

struct PathRequest {
    TilePos from;
    TilePos to;
    PathMode mode = PathMode::Road;
    BuildingId goal_building = kNoBuilding;  // its tiles count as passable in Walkable mode
    std::uint32_t max_nodes = 200000;        // search budget
};

// Tiles from `from` (excluded) to `to` (included). Empty when from == to. nullopt when unreachable.
std::optional<std::vector<TilePos>> find_path(const CityState& city, const PathRequest& request);

// Closest road tile (Chebyshev distance, then tile index) within `radius` of `center`, optionally excluding
// roadblocks. Used by roaming walkers to aim 8 tiles out (doc 24).
std::optional<TilePos> closest_road_within(const TileMap& map, TilePos center, std::int32_t radius, bool roam_only);

}  // namespace bo::core
