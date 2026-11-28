// Module D3 stage 3: walls, towers, gatehouses (docs/gameplay/11; Z:145-147; assumptions L-117).
#include <algorithm>

#include "bo/core/city.h"

namespace bo::core {
namespace {

bool is_wall(const CityState& city, TilePos p) {
    if (!city.map.in_bounds(p) || city.map.at(p).building == kNoBuilding) {
        return false;
    }
    const auto it = city.buildings.items.find(city.map.at(p).building);
    return it != city.buildings.items.end() && it->second.type == "wall";
}

}  // namespace

// A wall is one tile of land; it needs no road (Z:145) and does not stand on one.
BuildResult place_wall(CityState& city, Building b) {
    BuildResult r;
    r.reason = land_check(city, b.origin);
    if (!r.ok()) {
        return r;
    }
    b.width = b.height = 1;
    b.tiles = {b.origin};
    r.building = add_building(city, std::move(b));
    return r;
}

// A tower stands on a 2x2 block of wall, i.e. a wall at least two thick (Z:146) [ZAŁOŻENIE, L-117]; the walls
// under it make way. It needs the Palace and a road linked to the Palace's (Z:146).
BuildResult place_tower(CityState& city, Building b) {
    BuildResult r;
    std::vector<TilePos> tiles;
    for (std::int32_t dy = 0; dy < 2; ++dy) {
        for (std::int32_t dx = 0; dx < 2; ++dx) {
            const TilePos p{b.origin.x + dx, b.origin.y + dy};
            if (!is_wall(city, p)) {
                r.reason = "build.tower_needs_thick_wall";
                return r;
            }
            tiles.push_back(p);
        }
    }
    const auto palace = std::find_if(city.buildings.items.begin(), city.buildings.items.end(),
                                     [](const auto& kv) { return kv.second.type == "palace"; });
    if (palace == city.buildings.items.end() || !palace->second.road_access) {
        r.reason = "build.needs_building";
        return r;
    }
    std::optional<TilePos> road;
    for (const TilePos t : tiles) {
        for (const Dir d : kDirs) {
            const TilePos n = step(t, d);
            if (city.map.in_bounds(n) && city.map.at(n).road && !road) {
                road = n;
            }
        }
    }
    if (!road || city.map.at(*road).road_network != city.map.at(*palace->second.road_access).road_network) {
        r.reason = "build.tower_needs_road_to_palace";  // the generals send the sentries by road (Z:146)
        return r;
    }
    std::vector<BuildingId> walls;
    for (const TilePos t : tiles) {
        remove_building(city, city.map.at(t).building, walls);
    }
    b.width = b.height = 2;
    b.tiles = tiles;
    r.building = add_building(city, std::move(b));
    r.removed = walls;
    return r;
}

// A gatehouse (5x3, S-004) straddles a road: the middle line of its short side is road, which stays road but
// is closed to enemies; walls must touch both ends, or the enemy walks around (Z:146-147).
BuildResult place_gatehouse(CityState& city, Building b) {
    BuildResult r;
    const bool across_x = b.rotation % 2 == 0;  // rotation 0/2: 5 wide, the road runs north-south
    b.width = across_x ? 5 : 3;
    b.height = across_x ? 3 : 5;
    std::vector<TilePos> road;
    for (std::int32_t dy = 0; dy < b.height; ++dy) {
        for (std::int32_t dx = 0; dx < b.width; ++dx) {
            const TilePos p{b.origin.x + dx, b.origin.y + dy};
            const bool middle = across_x ? dx == 2 : dy == 2;
            if (middle) {
                if (!city.map.in_bounds(p) || !city.map.at(p).road || city.map.at(p).building != kNoBuilding) {
                    r.reason = "build.gate_needs_road";
                    return r;
                }
                road.push_back(p);
            } else {
                r.reason = land_check(city, p);
                if (!r.ok()) {
                    return r;
                }
            }
            b.tiles.push_back(p);
        }
    }
    bool before = false;
    bool after = false;
    for (std::int32_t i = 0; i < (across_x ? b.height : b.width); ++i) {
        const TilePos lo = across_x ? TilePos{b.origin.x - 1, b.origin.y + i} : TilePos{b.origin.x + i, b.origin.y - 1};
        const TilePos hi = across_x ? TilePos{b.origin.x + b.width, b.origin.y + i} : TilePos{b.origin.x + i, b.origin.y + b.height};
        before = before || is_wall(city, lo);
        after = after || is_wall(city, hi);
    }
    if (!before || !after) {
        r.reason = "build.gate_needs_walls";
        return r;
    }
    for (const TilePos p : road) {
        city.map.at(p).roadblock = true;  // not a road for roaming walkers (doc 24); closed to enemies
    }
    r.building = add_building(city, std::move(b));
    return r;
}

// A gatehouse taken down opens its road again.
void open_gate(CityState& city, const Building& b) {
    if (b.type != "gatehouse") {
        return;
    }
    for (const TilePos p : b.tiles) {
        if (city.map.at(p).road) {
            city.map.at(p).roadblock = false;
        }
    }
}

}  // namespace bo::core
