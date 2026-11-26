#include "bo/core/city.h"

#include <algorithm>
#include <deque>
#include <iterator>
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

// Ordinary rectangle on free buildable land (Z:19, Z:24-25).
BuildResult place_ordinary(CityState& city, Building b, bool needs_meadow) {
    BuildResult r;
    bool has_meadow = false;
    for (std::int32_t dy = 0; dy < b.height; ++dy) {
        for (std::int32_t dx = 0; dx < b.width; ++dx) {
            const TilePos p{b.origin.x + dx, b.origin.y + dy};
            r.reason = land_check(city, p);
            if (!r.ok()) {
                return r;
            }
            has_meadow = has_meadow || city.map.at(p).terrain == Terrain::Meadow;
            b.tiles.push_back(p);
        }
    }
    if (needs_meadow && !has_meadow) {
        r.reason = "build.needs_meadow";  // farms: at least one meadow tile (S-004 size legend "m")
        return r;
    }
    r.building = add_building(city, std::move(b));
    return r;
}

}  // namespace

bool operator==(const Building& a, const Building& b) {
    return a.id == b.id && a.type == b.type && a.origin == b.origin && a.width == b.width && a.height == b.height &&
           a.rotation == b.rotation && a.tiles == b.tiles && a.road_mode == b.road_mode && a.road_tile == b.road_tile &&
           a.parent == b.parent && a.slot_origins == b.slot_origins && a.slots == b.slots &&
           a.road_access == b.road_access && a.staffing_percent == b.staffing_percent &&
           a.days_since_spawn == b.days_since_spawn && a.next_roam_dir == b.next_roam_dir &&
           a.roaming_walker == b.roaming_walker && a.coverage == b.coverage;
}

std::string land_check(const CityState& city, TilePos p) {
    if (!city.map.in_bounds(p)) {
        return "build.out_of_bounds";
    }
    const Tile& t = city.map.at(p);
    if (t.building != kNoBuilding || t.road) {
        return "build.occupied";
    }
    if (!terrain_buildable(t.terrain)) {
        return "build.bad_terrain";
    }
    return {};
}

BuildResult place_building(CityState& city, const DataCatalog& catalog, Difficulty difficulty, const std::string& type,
                           TilePos origin, std::uint8_t rotation) {
    BuildResult r;
    const auto type_it = catalog.buildings.find(type);
    const auto stats_it = catalog.building_stats.find(type);
    if (type_it == catalog.buildings.end() || stats_it == catalog.building_stats.end()) {
        r.reason = "build.unknown_type";
        return r;
    }
    if (rotation > 3) {
        r.reason = "build.bad_rotation";
        return r;
    }
    const BuildingType& bt = type_it->second;
    const BuildingStats& stats = stats_it->second;
    const std::string& rule = bt.placement;
    Building b;
    b.type = type;
    b.origin = origin;
    b.rotation = rotation;
    if (rule == "over_road_both_sides_clear" || rule == "over_road_one_side") {
        r = place_over_road(city, std::move(b), rule == "over_road_both_sides_clear");
    } else if (rule == "straight_coast_both_sides") {
        r = place_crossing(city, std::move(b), bt.no_adjacent_same);
    } else if (!stats.footprint) {
        r.reason = "build.unsupported_footprint";
    } else {
        b.width = stats.footprint->width;
        b.height = stats.footprint->height;
        if (rotation % 2 == 1) {
            std::swap(b.width, b.height);
        }
        if (rule == "straight_coast") {
            r = place_on_coast(city, std::move(b));  // faces the water by itself; rotation is ignored
        } else if (rule == "long_side_over_road" || rule == "road_through_center") {
            r = place_agora(city, std::move(b), rule == "road_through_center", bt.stall_slots);
        } else if (rule == "on_agora") {
            if (rotation == 0) {
                r = place_stall(city, std::move(b));
            } else {
                r.reason = "build.not_rotatable";
            }
        } else if (!rule.empty()) {
            r.reason = "build.placement_not_implemented";  // urchins, walls, towers, hippodrome: later modules
        } else if (rotation != 0 && !bt.rotatable) {
            r.reason = "build.not_rotatable";
        } else {
            r = place_ordinary(city, std::move(b), stats.size_text.find('m') != std::string::npos);
        }
    }
    if (r.ok()) {
        r.cost = cost_of(catalog, type, difficulty);
    }
    return r;
}

BuildingId add_building(CityState& city, Building b) {
    b.id = city.buildings.next_id++;
    b.next_roam_dir = static_cast<Dir>(city.map.at(b.origin).random_byte & 3U);  // random per building (family)
    for (const TilePos p : b.tiles) {
        Tile& t = city.map.at(p);
        t.building = b.id;
        if (b.road_mode == RoadMode::OwnRoad) {
            t.road = true;
        }
    }
    const BuildingId id = b.id;
    city.buildings.items.emplace(id, std::move(b));
    update_roads(city);
    return id;
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

void remove_building(CityState& city, BuildingId id, std::vector<BuildingId>& removed) {
    const auto it = city.buildings.items.find(id);
    if (it == city.buildings.items.end()) {
        return;
    }
    const Building b = it->second;
    for (const BuildingId stall : b.slots) {
        if (stall != kNoBuilding) {
            city.buildings.items.erase(stall);
            removed.push_back(stall);
        }
    }
    if (b.parent != kNoBuilding) {
        auto& slots = city.buildings.items.at(b.parent).slots;
        std::replace(slots.begin(), slots.end(), id, kNoBuilding);
    }
    for (const TilePos p : b.tiles) {
        Tile& t = city.map.at(p);
        t.building = kNoBuilding;
        if (b.road_mode == RoadMode::OwnRoad) {
            t.road = false;
            t.roadblock = false;
        }
    }
    city.buildings.items.erase(id);
    removed.push_back(id);
}

BuildResult demolish(CityState& city, TilePos p) {
    BuildResult r;
    if (!city.map.in_bounds(p)) {
        r.reason = "build.out_of_bounds";
        return r;
    }
    Tile& t = city.map.at(p);
    if (t.building != kNoBuilding) {
        const Building& b = city.buildings.items.at(t.building);
        BuildingId target = b.id;
        for (std::size_t i = 0; i < b.slots.size(); ++i) {
            const TilePos s = b.slot_origins[i];
            if (b.slots[i] != kNoBuilding && p.x >= s.x && p.x < s.x + 2 && p.y >= s.y && p.y < s.y + 2) {
                target = b.slots[i];  // an occupied slot: the stall goes first
            }
        }
        remove_building(city, target, r.removed);
    } else if (t.road) {
        t.road = false;
        t.roadblock = false;
        std::vector<BuildingId> ornaments;
        for (const auto& [id, b] : city.buildings.items) {
            if (b.road_tile == p) {
                ornaments.push_back(id);  // boulevards and avenues go with their road [ZAŁOŻENIE]
            }
        }
        for (const BuildingId id : ornaments) {
            remove_building(city, id, r.removed);
        }
    } else {
        r.reason = "build.nothing_to_demolish";
        return r;
    }
    update_roads(city);
    return r;
}

std::optional<TilePos> find_road_access(const CityState& city, TilePos o, std::int32_t w, std::int32_t h) {
    const TileMap& map = city.map;
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
        if (map.in_bounds(p) && map.at(p).building != kNoBuilding) {
            const auto& ornament = city.buildings.items.at(map.at(p).building);
            if (ornament.road_tile && map.is_road(*ornament.road_tile)) {
                return ornament.road_tile;  // access through a boulevard or avenue (Z:154)
            }
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
        if (b.road_mode == RoadMode::OverRoad) {
            // Agoras send walkers from the middle of their road [eZeus, S-008: "agoras spawn from their centre"].
            std::vector<TilePos> roads;
            std::copy_if(b.tiles.begin(), b.tiles.end(), std::back_inserter(roads),
                         [&map](TilePos p) { return map.is_road(p); });
            b.road_access = roads.empty() ? std::nullopt : std::optional<TilePos>(roads[roads.size() / 2]);
        } else if (b.parent == kNoBuilding) {
            b.road_access = find_road_access(city, b.origin, b.width, b.height);
        }
    }
    for (auto& [id, b] : city.buildings.items) {
        if (b.parent != kNoBuilding) {
            b.road_access = city.buildings.items.at(b.parent).road_access;  // stalls use their agora
        }
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
