// The hippodrome's crosswalk (P:14; docs/gameplay/15-atlantyda.md; assumptions L-127): across a straight stade
// with free land or a road at both ends; anyone may cross; never on a stade next to one that has a crosswalk.
// It turns the four tiles across the stade into road, so every walker crosses there.
#include <algorithm>
#include <cstdlib>
#include <optional>
#include <vector>

#include "bo/core/city.h"

namespace bo::core {
namespace {

bool joined(const Building& a, const Building& b) {
    const std::int32_t dx = std::abs(a.origin.x - b.origin.x);
    const std::int32_t dy = std::abs(a.origin.y - b.origin.y);
    return (dx == 0 && dy == 4) || (dy == 0 && dx == 4);
}

bool has_crosswalk(const CityState& city, const Building& s) {
    return std::any_of(s.tiles.begin(), s.tiles.end(), [&](TilePos p) { return city.map.at(p).road; });
}

bool open_end(const CityState& city, TilePos p) {
    if (!city.map.in_bounds(p)) {
        return false;
    }
    const Tile& t = city.map.at(p);
    return t.road || (t.building == kNoBuilding && terrain_walkable(t.terrain));
}

}  // namespace

BuildResult place_crosswalk(CityState& city, const DataCatalog& catalog, Difficulty difficulty, TilePos at) {
    BuildResult r;
    const BuildingId id = city.map.in_bounds(at) ? city.map.at(at).building : kNoBuilding;
    const auto it = city.buildings.items.find(id);
    if (it == city.buildings.items.end() || it->second.type != "hippodrome") {
        r.reason = "build.needs_hippodrome";
        return r;
    }
    const Building& s = it->second;
    std::vector<const Building*> next;
    for (const auto& [oid, o] : city.buildings.items) {
        if (o.type == "hippodrome" && oid != id && joined(s, o)) {
            next.push_back(&o);
        }
    }
    const bool east_west = next.size() == 2 && next[0]->origin.y == s.origin.y && next[1]->origin.y == s.origin.y;
    const bool north_south = next.size() == 2 && next[0]->origin.x == s.origin.x && next[1]->origin.x == s.origin.x;
    if (!east_west && !north_south) {
        r.reason = "build.crosswalk_not_straight";  // a straight stade only (P:14)
        return r;
    }
    if (has_crosswalk(city, s) || std::any_of(next.begin(), next.end(), [&](const Building* o) { return has_crosswalk(city, *o); })) {
        r.reason = "build.crosswalk_adjacent";  // two crosswalks never side by side (P:14)
        return r;
    }
    std::vector<TilePos> across;  // across the track: north to south on an east-west stade, else west to east
    for (std::int32_t k = 0; k < 4; ++k) {
        across.push_back(east_west ? TilePos{at.x, s.origin.y + k} : TilePos{s.origin.x + k, at.y});
    }
    const TilePos before = east_west ? TilePos{at.x, s.origin.y - 1} : TilePos{s.origin.x - 1, at.y};
    const TilePos after = east_west ? TilePos{at.x, s.origin.y + 4} : TilePos{s.origin.x + 4, at.y};
    if (!open_end(city, before) || !open_end(city, after)) {
        r.reason = "build.crosswalk_blocked";  // free land or a road on both sides (P:14)
        return r;
    }
    for (const TilePos p : across) {
        city.map.at(p).road = true;
    }
    r.tiles = across;  // the renderer draws the crosswalk over the track
    update_roads(city);
    const auto cost = catalog.building_stats.find("crosswalk");
    r.cost = cost != catalog.building_stats.end() ? cost->second.cost[static_cast<std::size_t>(difficulty)].value_or(0) : 0;
    return r;
}

}  // namespace bo::core
