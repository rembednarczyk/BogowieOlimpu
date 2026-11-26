// Module D5 stage 2: hostile gods and disasters (docs/gameplay/10, 21, 23; Z:119-127; L-054b, L-115).
#include "bo/core/wrath.h"

#include <algorithm>
#include <cstdlib>

#include "bo/core/gods.h"
#include "bo/core/logistics.h"
#include "bo/core/state.h"

namespace bo::core {
namespace {

std::int32_t number(const DataCatalog& catalog, const char* key, std::int32_t fallback) {
    const auto it = catalog.wrath.find(key);
    return it == catalog.wrath.end() ? fallback : it->second;
}

bool repelled(const CityState& city, const DataCatalog& catalog, const std::string& god) {
    if (god != "zeus" && god_blesses(city, catalog, "zeus")) {
        return true;  // Zeus defends the city against any other god (Z:119)
    }
    if (god == "zeus" && god_blesses(city, catalog, "hera")) {
        return true;  // Hera drives Zeus away (P:21-22)
    }
    const bool lesser = god == "ares" || god == "hephaestus" || god == "hermes" || god == "dionysus";
    return lesser && god_blesses(city, catalog, "aphrodite");  // Z:122
}

void curse(CityState& city, const DataCatalog& catalog, const std::vector<std::string>& types) {
    const auto days = static_cast<std::uint16_t>(number(catalog, "curse_days", 45));
    for (auto& [id, b] : city.buildings.items) {
        if (std::find(types.begin(), types.end(), b.type) != types.end()) {
            b.cursed_days = days;
        }
    }
}

// Buildings in id order that pass `ok`, at most `n`; the god's choice is not random in our model [L-115].
template <class Pred>
std::vector<BuildingId> pick(const CityState& city, std::int32_t n, Pred ok) {
    std::vector<BuildingId> out;
    for (const auto& [id, b] : city.buildings.items) {
        if (static_cast<std::int32_t>(out.size()) < n && ok(b)) {
            out.push_back(id);
        }
    }
    return out;
}

void destroy(CityState& city, const std::vector<BuildingId>& ids, WrathEvents& ev) {
    for (const BuildingId id : ids) {
        if (city.buildings.items.count(id) != 0) {
            remove_building(city, id, ev.removed);
        }
    }
}

// Walkers of the city are taken away (Hades, Aphrodite, Dionysus); soldiers and invaders stay.
void take_walkers(WorldState& st, WrathEvents& ev) {
    for (auto& [id, w] : st.walkers.items) {
        if (w.job != WalkerJob::Soldier && w.job != WalkerJob::Invade && w.job != WalkerJob::Prowl) {
            w.phase = WalkerPhase::Done;
            ev.walkers_gone.push_back(id);
        }
    }
}

}  // namespace

WrathEvents god_attack(WorldState& st, const DataCatalog& catalog, const std::string& god) {
    WrathEvents ev;
    CityState& city = st.city;
    if (repelled(city, catalog, god)) {
        ev.repelled = true;
        return ev;
    }
    const auto n = [&catalog](const char* key, std::int32_t fallback) { return number(catalog, key, fallback); };
    if (god == "zeus") {  // the Palace first, then other buildings (Z:125)
        auto ids = pick(city, 1, [](const Building& b) { return b.type == "palace"; });
        const auto more = pick(city, n("buildings_destroyed", 3),
                               [](const Building& b) { return b.type != "palace" && !b.tiles.empty() && !b.animal; });
        ids.insert(ids.end(), more.begin(), more.end());
        destroy(city, ids, ev);
        st.caravan_curse_days = st.ship_curse_days = static_cast<std::uint16_t>(n("curse_days", 45));  // Z:125
    } else if (god == "poseidon") {
        curse(city, catalog, {"fishery", "urchin_quay", "trireme_wharf"});
        st.ship_curse_days = static_cast<std::uint16_t>(n("curse_days", 45));  // no trade ships (Z:126)
    } else if (god == "hades") {
        take_walkers(st, ev);
        curse(city, catalog, {"foundry", "mint"});
    } else if (god == "demeter") {  // crops and flocks (Z:126)
        for (auto& [id, b] : city.buildings.items) {
            const auto& stats = catalog.building_stats.at(catalog.buildings.at(b.type).stats_id);
            if (b.producer && stats.size_text.find('m') != std::string::npos) {  // farms: built on meadow
                b.producer->progress = 0;
            }
        }
        destroy(city, pick(city, 1 << 30, [](const Building& b) { return b.animal.has_value(); }), ev);
    } else if (god == "athena") {
        curse(city, catalog, {"olive_press", "growers_lodge"});
    } else if (god == "artemis") {
        for (auto& [id, w] : st.walkers.items) {
            if (w.job == WalkerJob::Hunt) {
                w.phase = WalkerPhase::Done;  // she kills hunters (Z:126)
                ev.walkers_gone.push_back(id);
            }
        }
        for (auto& [tile, s] : city.spawns) {
            s.animals = std::max<std::uint8_t>(s.animals, static_cast<std::uint8_t>(catalog.wolves.pack));
        }
        ev.invasion_nation = "amazon";
        ev.invasion_soldiers = static_cast<std::uint32_t>(n("amazons", 16));
    } else if (god == "apollo") {  // plague (Z:126)
        const RiskParams& r = catalog.risks;
        for (const BuildingId id : pick(city, n("plague_houses", 3),
                                        [](const Building& b) { return b.house && b.house->population > 0; })) {
            city.buildings.items.at(id).disease_counter = static_cast<std::uint16_t>(r.disease_threshold);
        }
    } else if (god == "atlas") {
        curse(city, catalog, {"masonry_shop", "black_marble_workshop", "artisans_guild"});
        for (auto& [id, b] : city.buildings.items) {
            if (b.producer && (b.type == "masonry_shop" || b.type == "black_marble_workshop")) {
                b.producer->output_loads = 0;  // the stone on site is destroyed (S-004)
            }
        }
    } else if (god == "ares") {
        ev.invasion_nation = "divine";
        ev.invasion_soldiers = static_cast<std::uint32_t>(n("sparti", 16));
    } else if (god == "hephaestus") {  // arson (Z:127)
        for (const BuildingId id : pick(city, n("buildings_set_on_fire", 3), [](const Building& b) {
                 return !b.tiles.empty() && !b.animal && b.burning_days == 0 && b.type != "ruins";
             })) {
            city.buildings.items.at(id).fire_counter = static_cast<std::uint16_t>(catalog.risks.hazard_threshold);
            ev.fires.push_back(id);
        }
        curse(city, catalog, {"armory", "sculpture_studio"});
    } else if (god == "aphrodite") {
        take_walkers(st, ev);
        const auto pct = static_cast<std::uint32_t>(n("residents_leave_percent", 10));
        for (auto& [id, b] : city.buildings.items) {
            if (b.house) {
                b.house->population -= b.house->population * pct / 100;
            }
        }
    } else if (god == "hermes") {
        if (st.finance.treasury > 0) {
            st.finance.treasury -= st.finance.treasury * n("money_stolen_percent", 25) / 100;
        }
        st.caravan_curse_days = static_cast<std::uint16_t>(n("curse_days", 45));  // roads unsafe for caravans
    } else if (god == "dionysus") {
        take_walkers(st, ev);
        curse(city, catalog, {"winery", "grapevine"});
    } else if (god == "hera") {
        destroy(city, pick(city, 1 << 30, [](const Building& b) { return b.type == "cattle"; }), ev);
        for (TradePartner& p : st.trade.partners) {
            p.diplomacy.opinion = std::max(0, p.diplomacy.opinion - n("opinion_loss", 10));  // P:22
        }
    }
    return ev;
}

WrathEvents disaster(WorldState& st, const std::string& kind, TilePos centre, std::int32_t radius) {
    WrathEvents ev;
    CityState& city = st.city;
    const auto near = [&](TilePos p) { return std::max(std::abs(p.x - centre.x), std::abs(p.y - centre.y)) <= radius; };
    destroy(city, pick(city, 1 << 30, [&](const Building& b) {
                return std::any_of(b.tiles.begin(), b.tiles.end(), near) || (b.tiles.empty() && near(b.origin));
            }),
            ev);
    if (kind == "earthquake" || kind == "lava") {  // cracks and lava leave rock (doc 01: Z:189) [L-115]
        for (std::int32_t y = centre.y - radius; y <= centre.y + radius; ++y) {
            for (std::int32_t x = centre.x - radius; x <= centre.x + radius; ++x) {
                const TilePos p{x, y};
                if (city.map.in_bounds(p) && city.map.at(p).terrain != Terrain::Water) {
                    Tile& t = city.map.at(p);
                    t.terrain = Terrain::Rock;
                    t.road = false;
                    t.roadblock = false;
                    ev.tiles_changed.push_back(p);
                }
            }
        }
        update_roads(city);
    }
    return ev;
}

}  // namespace bo::core
