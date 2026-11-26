#include "bo/core/walker.h"

#include <algorithm>
#include <array>
#include <cstdlib>

#include "bo/core/path.h"

namespace bo::core {
namespace {

constexpr std::int64_t kStepsPerTileRaw = std::int64_t{15} << 16;

Dir general_direction(TilePos from, TilePos to) {
    const std::int32_t dx = to.x - from.x;
    const std::int32_t dy = to.y - from.y;
    if (dx == 0 && dy == 0) {
        return Dir::North;
    }
    if (std::abs(dx) > std::abs(dy)) {
        return dx > 0 ? Dir::East : Dir::West;
    }
    return dy > 0 ? Dir::South : Dir::North;
}

// Roads a roaming walker may take from its tile this step (doc 24 table plus the wide-road rule).
using RoadMask = std::array<bool, 4>;

bool roam_ok(const RoadMask& mask, Dir d) { return mask[static_cast<std::size_t>(d)]; }

// Wide roads and paved areas (S-006 movement.c:386-411): with 3 roads and a run of >= 5 road tiles among the
// 8 neighbours, or 4 roads and all 8 neighbours road, keep only the axis of travel when it is fully road.
void apply_wide_road(const CityState& city, const Walker& w, RoadMask& mask, int roads) {
    if (roads < 3) {
        return;
    }
    constexpr std::array<std::array<std::int32_t, 2>, 8> kRing = {
        {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}}};
    std::array<bool, 8> ring{};
    int total = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        ring[i] = city.map.is_roam_road({w.tile.x + kRing[i][0], w.tile.y + kRing[i][1]});
        total += ring[i] ? 1 : 0;
    }
    int longest = 0;
    for (std::size_t start = 0; start < 8; ++start) {
        int run = 0;
        while (run < 8 && ring[(start + static_cast<std::size_t>(run)) % 8]) {
            ++run;
        }
        longest = std::max(longest, run);
    }
    const bool wide = (roads == 3 && longest >= 5) || (roads == 4 && total == 8);
    if (wide && roam_ok(mask, w.dir) && roam_ok(mask, opposite(w.dir))) {
        mask[static_cast<std::size_t>(rotate(w.dir, 1))] = false;
        mask[static_cast<std::size_t>(rotate(w.dir, -1))] = false;
    }
}

// Re-aim toward the original target (family roam_set_direction): nearest road clockwise or counter-clockwise
// from the target direction; the nearer side wins, ties go clockwise. U-turns allowed.
void roam_set_direction(const RoadMask& mask, Walker& w) {
    const Dir d0 = general_direction(w.tile, w.roam_target);
    int off_cw = 4;
    int off_ccw = 4;
    for (int k = 0; k < 4; ++k) {
        if (off_cw == 4 && roam_ok(mask, rotate(d0, k))) {
            off_cw = k;
        }
        if (off_ccw == 4 && roam_ok(mask, rotate(d0, -k))) {
            off_ccw = k;
        }
    }
    if (off_cw <= off_ccw) {
        w.dir = rotate(d0, off_cw);
        w.turn = 1;
    } else {
        w.dir = rotate(d0, -off_ccw);
        w.turn = -1;
    }
    w.ticks_until_next_turn = 5;
}

void rotate_until_valid(const RoadMask& mask, Walker& w) {
    for (int k = 0; k < 4; ++k) {
        const Dir d = rotate(w.dir, w.turn * k);
        if (roam_ok(mask, d) && (!w.came_from || d != *w.came_from)) {
            w.dir = d;
            return;
        }
    }
    if (w.came_from && roam_ok(mask, *w.came_from)) {
        w.dir = *w.came_from;
    }
}

// Decision at a tile centre while roaming (doc 24 table). Returns false when the roam ends.
bool roam_decide(const CityState& city, Walker& w) {
    ++w.random_counter;
    RoadMask mask{};
    int roads = 0;
    for (const Dir d : kDirs) {
        mask[static_cast<std::size_t>(d)] = city.map.is_roam_road(step(w.tile, d)) && city.map.can_step(w.tile, d);
        roads += roam_ok(mask, d) ? 1 : 0;
    }
    apply_wide_road(city, w, mask, roads);
    roads = 0;
    for (const Dir d : kDirs) {
        roads += roam_ok(mask, d) ? 1 : 0;
    }
    if (roads == 0) {
        return false;
    }
    if (roads == 1) {
        for (const Dir d : kDirs) {
            if (roam_ok(mask, d)) {
                w.dir = d;
                break;
            }
        }
        return true;
    }
    if (roads == 2) {
        if (w.ticks_until_next_turn == -1) {
            roam_set_direction(mask, w);
            return true;
        }
        if (roam_ok(mask, w.dir) && (!w.came_from || w.dir != *w.came_from)) {
            return true;
        }
        rotate_until_valid(mask, w);
        return true;
    }
    const auto pick = static_cast<Dir>((w.random_counter + city.map.at(w.tile).random_byte) & 3U);
    if (roam_ok(mask, pick) && (!w.came_from || pick != *w.came_from)) {
        w.dir = pick;
        return true;
    }
    if (--w.ticks_until_next_turn <= 0) {
        roam_set_direction(mask, w);
    }
    rotate_until_valid(mask, w);
    return true;
}

// Return target: the home's road access. Fountain and Infirmary walkers return to the road tile "directly
// north of the building that does not touch it" (doc 17, S-004): the north-west corner diagonal in our grid
// (L-098, diagramy S-004), falling back to the road access when that tile is not a road.
std::optional<TilePos> home_access(const CityState& city, const Walker& w) {
    const auto it = city.buildings.items.find(w.home);
    if (it == city.buildings.items.end()) {
        return std::nullopt;
    }
    const Building& b = it->second;
    const TilePos corner{b.origin.x - 1, b.origin.y - 1};
    if (w.return_north && city.map.is_road(corner)) {
        return corner;
    }
    return b.road_access;
}

void start_return(CityState& city, Walker& w) {
    const auto target = home_access(city, w);
    if (!target) {
        w.phase = WalkerPhase::Done;
        return;
    }
    const bool fields = w.job == WalkerJob::Shear || w.job == WalkerJob::Tend || w.job == WalkerJob::Harvest;
    auto path = find_path(city, {w.tile, *target, fields ? PathMode::Walkable : PathMode::RoadNoRoadblock});
    if (!path) {
        path = find_path(city, {w.tile, *target, PathMode::Road});
    }
    if (!path) {
        w.phase = WalkerPhase::Done;
        return;
    }
    w.path = std::move(*path);
    w.path_pos = 0;
    w.phase = WalkerPhase::Returning;
}

void enter_tile(CityState& city, Walker& w, TilePos next) {
    w.came_from = opposite(general_direction(w.tile, next));
    w.dir = general_direction(w.tile, next);
    w.tile = next;
    if (w.kind == WalkerKind::Roaming) {
        provide_coverage(city, w);
    }
}

void follow_path(CityState& city, Walker& w) {
    if (w.path_pos >= w.path.size()) {
        return;
    }
    enter_tile(city, w, w.path[w.path_pos++]);
}

}  // namespace

void provide_coverage(CityState& city, const Walker& w) {
    for (const BuildingId id : buildings_in_square(city, w.tile, kServiceRadius)) {
        city.buildings.items.at(id).coverage[w.type] = w.coverage_days;
    }
}

WalkerId spawn_roaming_walker(CityState& city, WalkerStore& walkers, const DataCatalog& catalog,
                              const std::string& type, BuildingId home, Dir start_dir) {
    const auto b = city.buildings.items.find(home);
    const auto t = catalog.walkers.find(type);
    if (b == city.buildings.items.end() || t == catalog.walkers.end() || !b->second.road_access ||
        !t->second.roam_ticks) {
        return kNoWalker;
    }
    Walker w;
    w.id = walkers.next_id++;
    w.type = type;
    w.kind = WalkerKind::Roaming;
    w.home = home;
    w.tile = *b->second.road_access;
    w.dir = start_dir;
    w.speed_raw = t->second.speed_raw;
    w.max_roam_ticks = *t->second.roam_ticks;
    w.coverage_days = t->second.coverage_days;
    const auto bt = catalog.buildings.find(b->second.type);
    w.return_north = bt != catalog.buildings.end() && bt->second.return_north;
    constexpr std::int32_t kDx[] = {0, 1, 0, -1};
    constexpr std::int32_t kDy[] = {-1, 0, 1, 0};
    const TilePos aim{b->second.origin.x + kDx[static_cast<int>(start_dir)] * kRoamTargetDistance,
                      b->second.origin.y + kDy[static_cast<int>(start_dir)] * kRoamTargetDistance};
    w.roam_target = aim;
    const auto road = closest_road_within(city.map, aim, kRoamTargetRoadRadius, true);
    std::optional<std::vector<TilePos>> path;
    if (road) {
        path = find_path(city, {w.tile, *road, PathMode::RoadNoRoadblock});
    }
    if (path && !path->empty()) {
        w.roam_target = *road;
        w.path = std::move(*path);
        w.phase = WalkerPhase::ToTarget;
    } else {
        w.phase = WalkerPhase::Roaming;  // no target: free roaming at once, never aimed (-1)
    }
    provide_coverage(city, w);
    const WalkerId id = w.id;
    walkers.items.emplace(id, std::move(w));
    return id;
}

WalkerId spawn_destination_walker(CityState& city, WalkerStore& walkers, const DataCatalog& catalog,
                                  const std::string& type, BuildingId home, TilePos to, PathMode mode,
                                  BuildingId goal) {
    const auto b = city.buildings.items.find(home);
    const auto t = catalog.walkers.find(type);
    if (b == city.buildings.items.end() || t == catalog.walkers.end() || !b->second.road_access) {
        return kNoWalker;
    }
    PathRequest req{*b->second.road_access, to, mode};
    req.goal_building = goal;
    auto path = find_path(city, req);
    if (!path) {
        return kNoWalker;
    }
    Walker w;
    w.id = walkers.next_id++;
    w.type = type;
    w.kind = WalkerKind::Destination;
    w.home = home;
    w.tile = *b->second.road_access;
    w.speed_raw = t->second.speed_raw;
    w.path = std::move(*path);
    w.phase = WalkerPhase::Routed;
    const WalkerId id = w.id;
    walkers.items.emplace(id, std::move(w));
    return id;
}

void send_home(CityState& city, Walker& w) {
    w.step_accum_raw = 0;
    start_return(city, w);
}

void step_walker(CityState& city, Walker& w) {
    if (w.phase == WalkerPhase::Done || w.phase == WalkerPhase::AtTarget) {
        return;
    }
    if (w.phase == WalkerPhase::Roaming) {
        if (++w.roam_ticks >= w.max_roam_ticks) {
            start_return(city, w);
            return;
        }
    }
    w.step_accum_raw += w.speed_raw;
    while (w.step_accum_raw >= kStepsPerTileRaw && w.phase != WalkerPhase::Done) {
        w.step_accum_raw -= kStepsPerTileRaw;
        switch (w.phase) {
            case WalkerPhase::ToTarget:
                follow_path(city, w);
                if (w.path_pos >= w.path.size()) {
                    w.phase = WalkerPhase::Roaming;
                    w.roam_ticks = 0;  // the budget restarts on arrival (family)
                    w.ticks_until_next_turn = 100;
                    w.path.clear();
                    w.path_pos = 0;
                }
                break;
            case WalkerPhase::Roaming:
                if (!roam_decide(city, w)) {
                    start_return(city, w);
                    break;
                }
                enter_tile(city, w, step(w.tile, w.dir));
                break;
            case WalkerPhase::Returning:
                follow_path(city, w);
                if (w.path_pos >= w.path.size()) {
                    w.phase = WalkerPhase::Done;
                }
                break;
            case WalkerPhase::Routed:
                follow_path(city, w);
                if (w.path_pos >= w.path.size()) {
                    w.phase = w.job == WalkerJob::None ? WalkerPhase::Done : WalkerPhase::AtTarget;
                }
                break;
            case WalkerPhase::Done:
            case WalkerPhase::AtTarget:
                w.step_accum_raw = 0;
                return;
        }
    }
}

}  // namespace bo::core
