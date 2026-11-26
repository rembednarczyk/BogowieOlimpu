// Modules B2 (construction) and B3 (roads, road networks, building road access).
// Rules: docs/gameplay/01-podstawy.md, 20-atrakcyjnosc-i-budynki.md, 17-czas-i-ruch.md (exit point).
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "bo/core/data.h"
#include "bo/core/map.h"

namespace bo::core {

using WalkerId = std::uint32_t;  // 0 = none
inline constexpr WalkerId kNoWalker = 0;

struct Building {
    BuildingId id = kNoBuilding;
    std::string type;
    TilePos origin;  // north-west corner of the footprint
    std::int32_t width = 1;
    std::int32_t height = 1;
    std::optional<TilePos> road_access;   // exit and entry tile (recomputed on every road or building change)
    std::uint8_t staffing_percent = 100;  // set by the labour module (C3); 100 until it exists
    std::uint32_t days_since_spawn = 0;
    Dir next_roam_dir = Dir::North;       // rotates 90 degrees clockwise per roaming walker (doc 24)
    WalkerId roaming_walker = kNoWalker;  // at most one roaming walker alive per building (doc 24)
    // Service coverage counters, e.g. "water_bearer" -> 96, decaying 1 per day (doc 24).
    std::map<std::string, std::uint8_t> coverage;
};

// Buildings with stable ids in ascending order (ids are never reused).
struct BuildingStore {
    std::uint32_t next_id = 1;
    std::map<BuildingId, Building> items;
};

struct CityState {
    TileMap map;
    BuildingStore buildings;
};

// Reason keys are i18n keys (docs/PRAKTYKI.md ch. 12). Empty string = success.
struct BuildResult {
    std::string reason;
    BuildingId building = kNoBuilding;
    std::int64_t cost = 0;
    [[nodiscard]] bool ok() const { return reason.empty(); }
};

// Validates and places a building with its north-west corner at `origin`. Does not touch the treasury.
BuildResult place_building(CityState& city, const DataCatalog& catalog, Difficulty difficulty,
                           const std::string& type, TilePos origin);
BuildResult place_road(CityState& city, const DataCatalog& catalog, Difficulty difficulty, TilePos p);
BuildResult place_roadblock(CityState& city, const DataCatalog& catalog, Difficulty difficulty, TilePos p);
BuildResult clear_land(CityState& city, const DataCatalog& catalog, Difficulty difficulty, TilePos p);
// Removes the building covering `p`, or the road on `p`.
BuildResult demolish(CityState& city, TilePos p);

// B3: recomputes road network ids (connected components in tile-index order) and every building's road access.
void update_roads(CityState& city);

// Exit tile search (S-004 units; S-007 road_access): perimeter tiles starting north of the north-west corner,
// clockwise; the first road tile wins.
std::optional<TilePos> find_road_access(const TileMap& map, TilePos origin, std::int32_t width, std::int32_t height);

// Every building whose footprint intersects the square of `radius` around `center`, in ascending id order.
std::vector<BuildingId> buildings_in_square(const CityState& city, TilePos center, std::int32_t radius);

}  // namespace bo::core
