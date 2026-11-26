#include "bo/core/city.h"

#include <algorithm>
#include <deque>
#include <set>

namespace bo::core {
namespace {

std::int64_t cost_of(const DataCatalog& catalog, const std::string& id, Difficulty difficulty) {
    const auto it = catalog.building_stats.find(id);
    if (it == catalog.building_stats.end()) {
        return 0;
    }
    return it->second.cost[static_cast<std::size_t>(difficulty)].value_or(0);
}

bool road_terrain(Terrain t) { return t == Terrain::Grassland || t == Terrain::Meadow || t == Terrain::Beach; }

}  // namespace

BuildResult place_building(CityState& city, const DataCatalog& catalog, Difficulty difficulty, const std::string& type,
                           TilePos origin) {
    BuildResult r;
    const auto type_it = catalog.buildings.find(type);
    const auto stats_it = catalog.building_stats.find(type);
    if (type_it == catalog.buildings.end() || stats_it == catalog.building_stats.end()) {
        r.reason = "build.unknown_type";
        return r;
    }
    const BuildingStats& stats = stats_it->second;
    if (!stats.footprint) {
        r.reason = "build.unsupported_footprint";
        return r;
    }
    // Coast ('w'), on-agora ('a') and on-road ('r') placements arrive with their modules.
    if (stats.size_text.find_first_of("war") != std::string::npos) {
        r.reason = "build.placement_not_implemented";
        return r;
    }
    const bool needs_meadow = stats.size_text.find('m') != std::string::npos;
    bool has_meadow = false;
    for (std::int32_t dy = 0; dy < stats.footprint->height; ++dy) {
        for (std::int32_t dx = 0; dx < stats.footprint->width; ++dx) {
            const TilePos p{origin.x + dx, origin.y + dy};
            if (!city.map.in_bounds(p)) {
                r.reason = "build.out_of_bounds";
                return r;
            }
            const Tile& t = city.map.at(p);
            if (t.building != kNoBuilding || t.road) {
                r.reason = "build.occupied";
                return r;
            }
            if (!terrain_buildable(t.terrain)) {
                r.reason = "build.bad_terrain";
                return r;
            }
            has_meadow = has_meadow || t.terrain == Terrain::Meadow;
        }
    }
    if (needs_meadow && !has_meadow) {
        r.reason = "build.needs_meadow";  // farms: at least one meadow tile (S-004 size legend "m")
        return r;
    }
    Building b;
    b.id = city.buildings.next_id++;
    b.type = type;
    b.origin = origin;
    b.width = stats.footprint->width;
    b.height = stats.footprint->height;
    b.next_roam_dir = static_cast<Dir>(city.map.at(origin).random_byte & 3U);  // random per building (family)
    for (std::int32_t dy = 0; dy < b.height; ++dy) {
        for (std::int32_t dx = 0; dx < b.width; ++dx) {
            city.map.at({origin.x + dx, origin.y + dy}).building = b.id;
        }
    }
    r.building = b.id;
    r.cost = cost_of(catalog, type, difficulty);
    city.buildings.items.emplace(b.id, std::move(b));
    update_roads(city);
    return r;
}

BuildResult place_road(CityState& city, const DataCatalog& catalog, Difficulty difficulty, TilePos p) {
    BuildResult r;
    if (!city.map.in_bounds(p)) {
        r.reason = "build.out_of_bounds";
        return r;
    }
    Tile& t = city.map.at(p);
    if (t.road) {
        r.reason = "build.already_road";
        return r;
    }
    if (t.building != kNoBuilding) {
        r.reason = "build.occupied";
        return r;
    }
    if (!road_terrain(t.terrain)) {
        r.reason = "build.bad_terrain";
        return r;
    }
    t.road = true;
    r.cost = cost_of(catalog, "road", difficulty);
    update_roads(city);
    return r;
}

BuildResult place_roadblock(CityState& city, const DataCatalog& catalog, Difficulty difficulty, TilePos p) {
    BuildResult r;
    if (!city.map.is_road(p) || city.map.at(p).roadblock) {
        r.reason = "build.needs_road";
        return r;
    }
    city.map.at(p).roadblock = true;
    r.cost = cost_of(catalog, "roadblock", difficulty);
    return r;
}

BuildResult clear_land(CityState& city, const DataCatalog& catalog, Difficulty difficulty, TilePos p) {
    BuildResult r;
    if (!city.map.in_bounds(p) || city.map.at(p).terrain != Terrain::Forest) {
        r.reason = "build.nothing_to_clear";
        return r;
    }
    city.map.at(p).terrain = Terrain::Grassland;  // a cleared forest never grows back (Z:18)
    r.cost = cost_of(catalog, "clear_land", difficulty);
    return r;
}

BuildResult demolish(CityState& city, TilePos p) {
    BuildResult r;
    if (!city.map.in_bounds(p)) {
        r.reason = "build.out_of_bounds";
        return r;
    }
    Tile& t = city.map.at(p);
    if (t.building != kNoBuilding) {
        const auto it = city.buildings.items.find(t.building);
        const Building& b = it->second;
        for (std::int32_t dy = 0; dy < b.height; ++dy) {
            for (std::int32_t dx = 0; dx < b.width; ++dx) {
                city.map.at({b.origin.x + dx, b.origin.y + dy}).building = kNoBuilding;
            }
        }
        r.building = b.id;
        city.buildings.items.erase(it);
    } else if (t.road) {
        t.road = false;
        t.roadblock = false;
    } else {
        r.reason = "build.nothing_to_demolish";
        return r;
    }
    update_roads(city);
    return r;
}

std::optional<TilePos> find_road_access(const TileMap& map, TilePos o, std::int32_t w, std::int32_t h) {
    std::vector<TilePos> ring;
    for (std::int32_t x = o.x; x < o.x + w; ++x) {
        ring.push_back({x, o.y - 1});  // north edge, west to east
    }
    for (std::int32_t y = o.y; y < o.y + h; ++y) {
        ring.push_back({o.x + w, y});  // east edge, north to south
    }
    for (std::int32_t x = o.x + w - 1; x >= o.x; --x) {
        ring.push_back({x, o.y + h});  // south edge, east to west
    }
    for (std::int32_t y = o.y + h - 1; y >= o.y; --y) {
        ring.push_back({o.x - 1, y});  // west edge, south to north
    }
    for (const TilePos p : ring) {
        if (map.is_road(p)) {
            return p;
        }
    }
    return std::nullopt;
}

void update_roads(CityState& city) {
    TileMap& map = city.map;
    for (auto& t : map.tiles()) {
        t.road_network = 0;
    }
    std::uint32_t next_network = 1;
    std::deque<std::size_t> queue;
    for (std::size_t i = 0; i < map.size(); ++i) {
        if (!map.tiles()[i].road || map.tiles()[i].road_network != 0) {
            continue;
        }
        map.tiles()[i].road_network = next_network;
        queue.push_back(i);
        while (!queue.empty()) {
            const TilePos p = map.pos(queue.front());
            queue.pop_front();
            for (const Dir d : kDirs) {
                const TilePos n = step(p, d);
                if (map.is_road(n) && map.at(n).road_network == 0) {
                    map.at(n).road_network = next_network;
                    queue.push_back(map.index(n));
                }
            }
        }
        ++next_network;
    }
    for (auto& [id, b] : city.buildings.items) {
        b.road_access = find_road_access(map, b.origin, b.width, b.height);
    }
}

std::vector<BuildingId> buildings_in_square(const CityState& city, TilePos c, std::int32_t radius) {
    std::set<BuildingId> found;
    for (std::int32_t y = c.y - radius; y <= c.y + radius; ++y) {
        for (std::int32_t x = c.x - radius; x <= c.x + radius; ++x) {
            const TilePos p{x, y};
            if (city.map.in_bounds(p) && city.map.at(p).building != kNoBuilding) {
                found.insert(city.map.at(p).building);
            }
        }
    }
    return {found.begin(), found.end()};
}

}  // namespace bo::core
