// Lasting gifts of the gods (Z:119-122). The manual names them without numbers; the numbers are assumed
// (L-131, data/gods_rules.json blessings).
#include "bo/core/blessings.h"

#include "bo/core/city.h"
#include "bo/core/gods.h"

namespace bo::core {

std::int32_t blessed_attack(const WorldState& st, const DataCatalog& catalog, const Walker& w, std::int32_t attack) {
    if (w.job != WalkerJob::Soldier || !god_blesses(st.city, catalog, "athena")) {
        return attack;
    }
    return attack * catalog.blessings.athena_attack_percent / 100;
}

std::uint32_t blessed_trader_interval(const CityState& city, const DataCatalog& catalog, std::uint32_t days) {
    if (!god_blesses(city, catalog, "hermes")) {
        return days;
    }
    return std::max<std::uint32_t>(1, days * static_cast<std::uint32_t>(catalog.blessings.hermes_interval_percent) / 100);
}

std::int64_t blessed_speed(const CityState& city, const DataCatalog& catalog, std::int64_t speed_raw) {
    return god_blesses(city, catalog, "hermes") ? speed_raw * catalog.blessings.hermes_speed_percent / 100 : speed_raw;
}

std::vector<BuildingId> plant_grove(CityState& city, const DataCatalog& catalog, Difficulty d, BuildingId sanctuary) {
    std::vector<BuildingId> planted;
    const Building& s = city.buildings.items.at(sanctuary);
    const auto p = catalog.sanctuaries.find(s.type);
    if (p == catalog.sanctuaries.end() || p->second.grove.empty()) {
        return planted;
    }
    const std::int32_t reach = catalog.blessings.grove_reach;
    const TilePos lo{s.origin.x - reach, s.origin.y - reach};
    const TilePos hi{s.origin.x + s.width - 1 + reach, s.origin.y + s.height - 1 + reach};
    for (std::int32_t y = lo.y; y <= hi.y; ++y) {
        for (std::int32_t x = lo.x; x <= hi.x; ++x) {
            if (static_cast<std::int32_t>(planted.size()) >= catalog.blessings.grove_plants) {
                return planted;
            }
            const TilePos t{x, y};
            if (!city.map.in_bounds(t) || city.map.at(t).building != kNoBuilding || city.map.at(t).road ||
                (city.map.at(t).terrain != Terrain::Grassland && city.map.at(t).terrain != Terrain::Meadow)) {
                continue;
            }
            const Terrain before = city.map.at(t).terrain;
            city.map.at(t).terrain = Terrain::Meadow;  // the god's own grove [ZAŁOŻENIE, L-131]
            const BuildResult r = place_building(city, catalog, d, p->second.grove, t, 0);
            if (r.ok()) {
                planted.push_back(r.building);
            } else {
                city.map.at(t).terrain = before;
            }
        }
    }
    return planted;
}

bool feed_dragon(WorldState& st, const DataCatalog& catalog) {
    const auto every = static_cast<std::uint64_t>(std::max(1, catalog.blessings.dragon_every_days));
    if (st.time.total_days() % every != 0 || !god_blesses(st.city, catalog, "ares")) {
        return false;
    }
    for (const WalkerId id : st.military.invasion.invaders) {  // the first invader on the land
        auto it = st.walkers.items.find(id);
        if (it != st.walkers.items.end() && it->second.hp > 0 && it->second.job == WalkerJob::Invade &&
            it->second.type != "transport" && it->second.type != "warship") {
            it->second.hp = 0;
            it->second.phase = WalkerPhase::Done;  // eaten: gone at once (combat only buries those it hit itself)
            return true;
        }
    }
    for (auto& [id, w] : st.walkers.items) {
        if (w.type == "wolf" && w.phase != WalkerPhase::Done) {
            w.phase = WalkerPhase::Done;  // removed with the day's finished walkers
            return true;
        }
    }
    return false;
}

}  // namespace bo::core
