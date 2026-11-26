#include "bo/core/path.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <functional>
#include <limits>
#include <queue>
#include <tuple>

namespace bo::core {
namespace {

bool passable(const CityState& city, TilePos p, const PathRequest& req) {
    const TileMap& map = city.map;
    if (!map.in_bounds(p)) {
        return false;
    }
    const Tile& t = map.at(p);
    switch (req.mode) {
        case PathMode::Road:
            return t.road;
        case PathMode::RoadNoRoadblock:
            return t.road && !t.roadblock;
        case PathMode::Walkable:
            if (t.road) {
                return true;  // includes agora roads and crossings
            }
            if (t.building != kNoBuilding) {
                return t.building == req.goal_building;
            }
            return t.road || terrain_walkable(t.terrain);
        case PathMode::Water:
            return t.terrain == Terrain::Water && (t.building == kNoBuilding || t.road);
    }
    return false;
}

std::uint32_t manhattan(TilePos a, TilePos b) {
    return static_cast<std::uint32_t>(std::abs(a.x - b.x) + std::abs(a.y - b.y));
}

}  // namespace

std::optional<std::vector<TilePos>> find_path(const CityState& city, const PathRequest& req) {
    const TileMap& map = city.map;
    if (!map.in_bounds(req.from) || !map.in_bounds(req.to)) {
        return std::nullopt;
    }
    if (req.from == req.to) {
        return std::vector<TilePos>{};
    }
    if (!passable(city, req.to, req)) {
        return std::nullopt;
    }
    constexpr std::uint32_t kInf = std::numeric_limits<std::uint32_t>::max();
    std::vector<std::uint32_t> g(map.size(), kInf);
    std::vector<std::uint32_t> parent(map.size(), kInf);
    // (f, h, tile index): a total order, so the expansion order is deterministic.
    using Node = std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>;
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
    const auto start = static_cast<std::uint32_t>(map.index(req.from));
    const auto goal = static_cast<std::uint32_t>(map.index(req.to));
    g[start] = 0;
    open.emplace(manhattan(req.from, req.to), manhattan(req.from, req.to), start);
    std::uint32_t expanded = 0;
    while (!open.empty() && expanded < req.max_nodes) {
        const auto [f, h, cur] = open.top();
        open.pop();
        if (cur == goal) {
            break;
        }
        const TilePos cp = map.pos(cur);
        if (f > g[cur] + manhattan(cp, req.to)) {
            continue;  // stale entry
        }
        ++expanded;
        for (const Dir d : kDirs) {
            const TilePos np = step(cp, d);
            if (!passable(city, np, req) || !map.can_step(cp, d)) {
                continue;
            }
            const auto ni = static_cast<std::uint32_t>(map.index(np));
            const std::uint32_t ng = g[cur] + 1;
            if (ng < g[ni]) {
                g[ni] = ng;
                parent[ni] = cur;
                const std::uint32_t nh = manhattan(np, req.to);
                open.emplace(ng + nh, nh, ni);
            }
        }
    }
    if (g[goal] == kInf) {
        return std::nullopt;
    }
    std::vector<TilePos> path;
    for (std::uint32_t i = goal; i != start; i = parent[i]) {
        path.push_back(map.pos(i));
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::optional<TilePos> closest_road_within(const TileMap& map, TilePos c, std::int32_t radius, bool roam_only) {
    for (std::int32_t r = 0; r <= radius; ++r) {
        std::optional<TilePos> best;
        for (std::int32_t y = c.y - r; y <= c.y + r; ++y) {
            for (std::int32_t x = c.x - r; x <= c.x + r; ++x) {
                if (std::max(std::abs(x - c.x), std::abs(y - c.y)) != r) {
                    continue;  // only the ring at distance r
                }
                const TilePos p{x, y};
                if (roam_only ? map.is_roam_road(p) : map.is_road(p)) {
                    if (!best || map.index(p) < map.index(*best)) {
                        best = p;
                    }
                }
            }
        }
        if (best) {
            return best;
        }
    }
    return std::nullopt;
}

std::vector<std::uint32_t> road_distances(const CityState& city, const std::vector<TilePos>& sources, PathMode mode) {
    const TileMap& map = city.map;
    std::vector<std::uint32_t> dist(map.size(), kUnreachable);
    std::deque<std::size_t> queue;
    PathRequest req;
    req.mode = mode;
    for (const TilePos s : sources) {
        if (passable(city, s, req) && dist[map.index(s)] == kUnreachable) {
            dist[map.index(s)] = 0;
            queue.push_back(map.index(s));
        }
    }
    while (!queue.empty()) {
        const std::size_t cur = queue.front();
        queue.pop_front();
        for (const Dir d : kDirs) {
            const TilePos n = step(map.pos(cur), d);
            if (passable(city, n, req) && map.can_step(map.pos(cur), d) && dist[map.index(n)] == kUnreachable) {
                dist[map.index(n)] = dist[cur] + 1;
                queue.push_back(map.index(n));
            }
        }
    }
    return dist;
}

}  // namespace bo::core
