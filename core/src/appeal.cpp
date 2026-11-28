#include "bo/core/blessings.h"
#include "bo/core/appeal.h"

#include <algorithm>

namespace bo::core {
namespace {

void emit(std::vector<std::int32_t>& field, const TileMap& map, const AppealParams& a, TilePos o, std::int32_t w,
          std::int32_t h) {
    for (std::int32_t y = o.y - a.range; y < o.y + h + a.range; ++y) {
        for (std::int32_t x = o.x - a.range; x < o.x + w + a.range; ++x) {
            if (!map.in_bounds({x, y})) {
                continue;
            }
            const std::int32_t dx = std::max({o.x - x, 0, x - (o.x + w - 1)});
            const std::int32_t dy = std::max({o.y - y, 0, y - (o.y + h - 1)});
            const std::int32_t d = std::max(dx, dy);
            if (d >= 1) {
                field[map.index({x, y})] += appeal_at_distance(a, d);
            }
        }
    }
}

}  // namespace

std::int32_t appeal_at_distance(const AppealParams& a, std::int32_t d) {
    if (d < 1 || d > a.range || a.step_tiles <= 0) {
        return 0;
    }
    return a.initial + ((d - 1) / a.step_tiles) * a.step_change;
}

std::vector<std::int32_t> compute_appeal(const CityState& city, const DataCatalog& catalog) {
    std::vector<std::int32_t> field(city.map.size(), 0);
    for (const auto& [id, b] : city.buildings.items) {
        const auto type = catalog.buildings.find(b.type);
        const auto stats = catalog.building_stats.find(type == catalog.buildings.end() ? b.type : type->second.stats_id);
        const bool site = b.sanctuary && !b.sanctuary->complete && catalog.sanctuaries.count(b.type) != 0 &&
                          catalog.sanctuaries.at(b.type).monument;  // a monument counts once finished (S-008)
        if (stats == catalog.building_stats.end() || !stats->second.appeal || b.tiles.empty() || site) {
            continue;  // stalls have no tiles of their own
        }
        const AppealParams& a = *stats->second.appeal;
        if (b.road_tile) {
            emit(field, city.map, a, *b.road_tile, 1, 1);
            for (const TilePos p : b.tiles) {
                emit(field, city.map, a, p, 1, 1);
            }
        } else {
            emit(field, city.map, a, b.origin, b.width, b.height);
        }
    }
    if (const std::int32_t aphrodite = blessed_appeal(city, catalog); aphrodite != 0) {
        for (std::int32_t& v : field) {
            v += aphrodite;  // Aphrodite: the whole city is lovelier (Z:122, L-131)
        }
    }
    return field;
}

std::int32_t building_appeal(const CityState& city, const Building& b) {
    std::int32_t best = 0;
    bool first = true;
    for (const TilePos p : b.tiles) {
        const std::int32_t v = city.appeal.empty() ? 0 : city.appeal[city.map.index(p)];
        best = first ? v : std::max(best, v);
        first = false;
    }
    return best;
}

}  // namespace bo::core
