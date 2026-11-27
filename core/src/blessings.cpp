// Lasting gifts of the gods (Z:119-122). The manual names them without numbers; the numbers are assumed
// (L-131, data/gods_rules.json blessings).
#include "bo/core/blessings.h"

#include "bo/core/city.h"
#include "bo/core/gods.h"
#include "bo/core/naval.h"

#include <algorithm>

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

namespace {

bool due(const WorldState& st, std::int32_t every) {
    return st.time.total_days() % static_cast<std::uint64_t>(std::max(1, every)) == 0;
}

// The first invader that matches `ship` (on the water or on the land): gone at once.
bool strike_invader(WorldState& st, bool ship) {
    for (const WalkerId id : st.military.invasion.invaders) {
        const auto it = st.walkers.items.find(id);
        if (it != st.walkers.items.end() && it->second.hp > 0 && it->second.phase != WalkerPhase::Done &&
            is_ship(it->second) == ship) {
            it->second.hp = 0;
            it->second.aboard = 0;  // a sunk transport takes its soldiers down (S-008)
            it->second.phase = WalkerPhase::Done;
            return true;
        }
    }
    return false;
}

bool strike_wolf(WorldState& st) {
    for (auto& [id, w] : st.walkers.items) {
        if (w.type == "wolf" && w.phase != WalkerPhase::Done) {
            w.phase = WalkerPhase::Done;
            return true;
        }
    }
    return false;
}

}  // namespace

std::vector<TilePos> bless_land(CityState& city, const DataCatalog& catalog, BuildingId sanctuary) {
    std::vector<TilePos> changed;
    const Building& s = city.buildings.items.at(sanctuary);
    const auto p = catalog.sanctuaries.find(s.type);
    if (p == catalog.sanctuaries.end() || p->second.land.empty()) {
        return changed;
    }
    const std::string& land = p->second.land;
    const Terrain to = land == "silver" ? Terrain::SilverOre : land == "copper" ? Terrain::CopperOre : Terrain::Meadow;
    const std::int32_t limit = land == "meadow" ? 1 << 20 : catalog.blessings.ore_tiles;  // meadow: all around
    const std::int32_t reach = catalog.blessings.grove_reach;
    for (std::int32_t y = s.origin.y - reach; y < s.origin.y + s.height + reach; ++y) {
        for (std::int32_t x = s.origin.x - reach; x < s.origin.x + s.width + reach; ++x) {
            const TilePos t{x, y};
            if (static_cast<std::int32_t>(changed.size()) >= limit || !city.map.in_bounds(t) ||
                city.map.at(t).building != kNoBuilding || city.map.at(t).road ||
                city.map.at(t).terrain != Terrain::Grassland) {
                continue;
            }
            city.map.at(t).terrain = to;  // the god's gift to the land [ZAŁOŻENIE, L-131]
            changed.push_back(t);
        }
    }
    return changed;
}

std::vector<std::pair<std::string, std::string>> advance_blessings(WorldState& st, const DataCatalog& catalog) {
    std::vector<std::pair<std::string, std::string>> out;
    const BlessingParams& b = catalog.blessings;
    if (st.time.day_in_month() == 0 && god_blesses(st.city, catalog, "zeus")) {
        for (TradePartner& p : st.trade.partners) {
            p.diplomacy.opinion = std::clamp(p.diplomacy.opinion + b.zeus_opinion, 0, 100);
        }
        out.emplace_back("zeus", "opinion");  // Z:119: the other leaders think more of the player
    }
    if (due(st, b.kraken_every_days) && god_blesses(st.city, catalog, "poseidon") && strike_invader(st, true)) {
        out.emplace_back("poseidon", "kraken");
    }
    if (due(st, b.cerberus_every_days) && god_blesses(st.city, catalog, "hades") && strike_invader(st, false)) {
        out.emplace_back("hades", "cerberus");
    }
    if (due(st, b.hunt_every_days) && god_blesses(st.city, catalog, "artemis") && strike_wolf(st)) {
        out.emplace_back("artemis", "hunt");
    }
    if (feed_dragon(st, catalog)) {
        out.emplace_back("ares", "dragon");
    }
    return out;
}

std::uint32_t blessed_fishing_ticks(const CityState& city, const DataCatalog& catalog, std::uint32_t ticks) {
    if (!god_blesses(city, catalog, "poseidon")) {
        return ticks;
    }
    return std::max<std::uint32_t>(1, ticks * static_cast<std::uint32_t>(catalog.blessings.poseidon_fishing_percent) / 100);
}

std::int32_t blessed_appeal(const CityState& city, const DataCatalog& catalog) {
    return god_blesses(city, catalog, "aphrodite") ? catalog.blessings.aphrodite_appeal : 0;
}

}  // namespace bo::core
