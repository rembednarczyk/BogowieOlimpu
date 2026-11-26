// Module B2/B3: placement rules beyond the ordinary rectangle.
// Sources: Z:46-47 and Z:141 (coast), Z:69-70 and doc 06 (agoras and stalls), Z:154 and doc 12 (boulevard,
// avenue), Z:101-103 and P:19 (water crossings, bridges). Assumptions are marked and listed in 99-luki.md.
#include <algorithm>
#include <cstdlib>

#include "bo/core/city.h"

namespace bo::core {
namespace {

std::string rect_check(const CityState& city, const Building& b, std::vector<TilePos>& tiles) {
    for (std::int32_t dy = 0; dy < b.height; ++dy) {
        for (std::int32_t dx = 0; dx < b.width; ++dx) {
            const TilePos p{b.origin.x + dx, b.origin.y + dy};
            auto reason = land_check(city, p);
            if (!reason.empty()) {
                return reason;
            }
            tiles.push_back(p);
        }
    }
    return level_check(city, tiles);
}

bool is_water(const TileMap& map, TilePos p) { return map.in_bounds(p) && map.at(p).terrain == Terrain::Water; }
bool is_land(const TileMap& map, TilePos p) { return map.in_bounds(p) && map.at(p).terrain != Terrain::Water; }

// The row of tiles just outside side `d` of the rectangle, extended by one tile at each end.
std::vector<TilePos> outer_edge(const Building& b, Dir d) {
    std::vector<TilePos> out;
    const bool horizontal = d == Dir::North || d == Dir::South;
    const std::int32_t len = horizontal ? b.width : b.height;
    for (std::int32_t i = -1; i <= len; ++i) {
        switch (d) {
            case Dir::North: out.push_back({b.origin.x + i, b.origin.y - 1}); break;
            case Dir::South: out.push_back({b.origin.x + i, b.origin.y + b.height}); break;
            case Dir::West: out.push_back({b.origin.x - 1, b.origin.y + i}); break;
            case Dir::East: out.push_back({b.origin.x + b.width, b.origin.y + i}); break;
        }
    }
    return out;
}

// Converts (lane, along) agora coordinates to a tile. Lanes cross the long axis; `along` runs with the road.
TilePos agora_tile(const Building& b, std::int32_t lane, std::int32_t along) {
    const bool along_y = b.height >= b.width;
    return along_y ? TilePos{b.origin.x + lane, b.origin.y + along} : TilePos{b.origin.x + along, b.origin.y + lane};
}

}  // namespace

// Coast buildings: the whole footprint on free land, one whole side facing water, and that shore straight:
// the water row runs one tile past both corners [ZAŁOŻENIE, L-096]. The side found first (N, E, S, W) wins
// and is stored as the rotation (the water side, for boats).
BuildResult place_on_coast(CityState& city, Building b) {
    BuildResult r;
    r.reason = rect_check(city, b, b.tiles);
    if (!r.ok()) {
        return r;
    }
    for (const Dir d : kDirs) {
        const auto edge = outer_edge(b, d);
        if (std::all_of(edge.begin(), edge.end(), [&city](TilePos p) { return is_water(city.map, p); })) {
            b.rotation = static_cast<std::uint8_t>(d);
            r.building = add_building(city, std::move(b));
            return r;
        }
    }
    r.reason = "build.needs_straight_coast";
    return r;
}

// Agoras (Z:69-70, doc 06): common 3x6 has an existing road along one long side, grand 5x6 a road through the
// middle; the remaining lanes hold 2x2 stall slots, three per two-lane band.
BuildResult place_agora(CityState& city, Building b, bool road_in_center, std::int32_t slots) {
    BuildResult r;
    const bool along_y = b.height >= b.width;
    const std::int32_t lanes = along_y ? b.width : b.height;
    const std::int32_t length = along_y ? b.height : b.width;
    std::vector<std::int32_t> candidates;
    if (road_in_center) {
        candidates = {lanes / 2};
    } else {
        candidates = {0, lanes - 1};
    }
    r.reason = "build.needs_road_under";
    for (const std::int32_t road_lane : candidates) {
        std::vector<TilePos> tiles;
        std::string reason;
        for (std::int32_t lane = 0; lane < lanes && reason.empty(); ++lane) {
            for (std::int32_t a = 0; a < length && reason.empty(); ++a) {
                const TilePos p = agora_tile(b, lane, a);
                if (lane == road_lane) {
                    const bool ok = city.map.is_road(p) && city.map.at(p).building == kNoBuilding;
                    reason = ok ? "" : "build.needs_road_under";
                } else {
                    reason = land_check(city, p);
                }
                tiles.push_back(p);
            }
        }
        if (reason.empty()) {
            reason = level_check(city, tiles);
        }
        if (!reason.empty()) {
            r.reason = reason == "build.needs_road_under" ? r.reason : reason;
            continue;
        }
        b.tiles = std::move(tiles);
        b.road_mode = RoadMode::OverRoad;
        for (std::int32_t band = 0; band + 1 < lanes; ++band) {
            if (band == road_lane || band + 1 == road_lane) {
                continue;
            }
            for (std::int32_t a = 0; a + 1 < length; a += 2) {
                b.slot_origins.push_back(agora_tile(b, band, a));
            }
            ++band;
        }
        if (static_cast<std::int32_t>(b.slot_origins.size()) != slots) {
            r.reason = "build.bad_agora_data";
            return r;
        }
        b.slots.assign(b.slot_origins.size(), kNoBuilding);
        r.reason.clear();
        r.building = add_building(city, std::move(b));
        return r;
    }
    return r;
}

// A stall goes into a free slot of an agora; its origin must be the slot's north-west corner (Z:70-71).
BuildResult place_stall(CityState& city, Building b) {
    BuildResult r;
    if (!city.map.in_bounds(b.origin) || city.map.at(b.origin).building == kNoBuilding) {
        r.reason = "build.needs_agora_slot";
        return r;
    }
    const BuildingId agora_id = city.map.at(b.origin).building;
    const Building& agora = city.buildings.items.at(agora_id);
    const auto slot = std::find(agora.slot_origins.begin(), agora.slot_origins.end(), b.origin);
    if (slot == agora.slot_origins.end()) {
        r.reason = "build.needs_agora_slot";
        return r;
    }
    const auto index = static_cast<std::size_t>(slot - agora.slot_origins.begin());
    if (agora.slots[index] != kNoBuilding) {
        r.reason = "build.occupied";
        return r;
    }
    b.parent = agora_id;
    r.building = add_building(city, std::move(b));
    city.buildings.items.at(agora_id).slots[index] = r.building;
    return r;
}

// Boulevard: ornaments on both sides of a road tile (rotation even: road runs north-south). Avenue: one
// ornament on side `rotation`. The ornament tiles must be free land (Z:154, doc 12).
BuildResult place_over_road(CityState& city, Building b, bool both_sides) {
    BuildResult r;
    const TilePos road = b.origin;
    if (!city.map.is_road(road)) {
        r.reason = "build.needs_road";
        return r;
    }
    std::vector<Dir> sides;
    if (both_sides) {
        sides = b.rotation % 2 == 0 ? std::vector<Dir>{Dir::East, Dir::West} : std::vector<Dir>{Dir::North, Dir::South};
    } else {
        sides = {static_cast<Dir>(b.rotation)};
    }
    TilePos lo = road;
    TilePos hi = road;
    for (const Dir d : sides) {
        const TilePos p = step(road, d);
        r.reason = land_check(city, p);
        if (!r.ok()) {
            return r;
        }
        if (!city.map.can_step(road, d)) {
            r.reason = "build.uneven_ground";
            return r;
        }
        b.tiles.push_back(p);
        lo = {std::min(lo.x, p.x), std::min(lo.y, p.y)};
        hi = {std::max(hi.x, p.x), std::max(hi.y, p.y)};
    }
    b.origin = lo;
    b.width = hi.x - lo.x + 1;
    b.height = hi.y - lo.y + 1;
    b.road_tile = road;
    r.building = add_building(city, std::move(b));
    return r;
}

// Water crossing and bridge: from the first water tile `origin`, straight in direction `rotation` across
// water to land. Both landings are road terrain; both shores straight: the landing's side neighbours are land
// and the end water tiles' side neighbours are water [ZAŁOŻENIE, L-097]. Width 1 [ZAŁOŻENIE, L-097].
// Two crossings may not touch side by side (Z:102). Small or large (ships) is decided with ships (module D).
BuildResult place_crossing(CityState& city, Building b, bool no_adjacent_same) {
    BuildResult r;
    const TileMap& map = city.map;
    const Dir d = static_cast<Dir>(b.rotation);
    const TilePos back = step(b.origin, opposite(d));
    auto landing_ok = [&map](TilePos p) {
        if (!is_land(map, p) || map.at(p).building != kNoBuilding) {
            return false;
        }
        const Terrain t = map.at(p).terrain;
        return t == Terrain::Grassland || t == Terrain::Meadow || t == Terrain::Beach;
    };
    if (!is_water(map, b.origin) || !landing_ok(back)) {
        r.reason = "build.needs_coast";
        return r;
    }
    TilePos cur = b.origin;
    std::int32_t span = 0;
    while (is_water(map, cur)) {
        if (map.at(cur).building != kNoBuilding || map.at(cur).road) {
            r.reason = "build.occupied";
            return r;
        }
        for (const int side : {-1, 1}) {
            const TilePos n = step(cur, rotate(d, side));
            const bool other_crossing =
                map.in_bounds(n) && map.at(n).building != kNoBuilding &&
                city.buildings.items.at(map.at(n).building).road_mode == RoadMode::OwnRoad;
            if (no_adjacent_same && other_crossing) {
                r.reason = "build.crossing_adjacent";
                return r;
            }
        }
        b.tiles.push_back(cur);
        if (++span > kMaxCrossingSpan) {
            r.reason = "build.crossing_too_long";
            return r;
        }
        cur = step(cur, d);
    }
    if (!landing_ok(cur)) {
        r.reason = "build.needs_coast";
        return r;
    }
    for (const int side : {-1, 1}) {
        const Dir s = rotate(d, side);
        const bool straight = is_land(map, step(back, s)) && is_land(map, step(cur, s)) &&
                              is_water(map, step(b.tiles.front(), s)) && is_water(map, step(b.tiles.back(), s));
        if (!straight) {
            r.reason = "build.needs_straight_coast";
            return r;
        }
    }
    const TilePos first = b.tiles.front();
    const TilePos last = b.tiles.back();
    b.origin = {std::min(first.x, last.x), std::min(first.y, last.y)};
    b.width = std::abs(last.x - first.x) + 1;
    b.height = std::abs(last.y - first.y) + 1;
    b.road_mode = RoadMode::OwnRoad;
    r.building = add_building(city, std::move(b));
    return r;
}

}  // namespace bo::core
