#include "bo/core/risks.h"

#include <algorithm>
#include <cstdlib>
#include <optional>

#include "bo/core/housing.h"
#include "bo/core/walker.h"

namespace bo::core {
namespace {

// Daily risk of fire or collapse from building_stats.json; houses use the stats of their level.
std::uint16_t hazard_rate(const DataCatalog& catalog, const Building& b, Difficulty d, bool fire) {
    std::string id = catalog.buildings.count(b.type) ? catalog.buildings.at(b.type).stats_id : b.type;
    if (b.house) {
        id = levels_of(catalog, *b.house)[b.house->level].id;
    }
    const auto s = catalog.building_stats.find(id);
    if (s == catalog.building_stats.end()) {
        return 0;
    }
    const auto& v = (fire ? s->second.fire_risk : s->second.damage_risk)[static_cast<std::size_t>(d)];
    return static_cast<std::uint16_t>(std::max(0, v.value_or(0)));
}

std::uint16_t bump(std::uint16_t counter, std::int32_t add) {
    return static_cast<std::uint16_t>(std::clamp(static_cast<std::int32_t>(counter) + add, 0, 0xFFFF));
}

std::string criminal_for(const RiskParams& p, std::int32_t happiness) {
    for (const auto& [below, walker] : p.criminal_by_happiness) {
        if (happiness < below) {
            return walker;
        }
    }
    return "disgruntled_citizen";
}

std::optional<BuildingId> nearest_elite(const CityState& city, TilePos from) {
    std::optional<std::pair<std::int32_t, BuildingId>> best;
    for (const auto& [id, b] : city.buildings.items) {
        if (b.house && b.house->elite && b.house->population > 0 && !b.house->abandoned && b.road_access) {
            const std::int32_t d = std::max(std::abs(b.origin.x - from.x), std::abs(b.origin.y - from.y));
            if (!best || d < best->first) {
                best = std::make_pair(d, id);
            }
        }
    }
    return best ? std::optional<BuildingId>(best->second) : std::nullopt;
}

void spawn_criminal(WorldState& st, const DataCatalog& catalog, BuildingId house, RiskEvents& ev) {
    CityState& city = st.city;
    const Building& b = city.buildings.items.at(house);
    const std::string kind = criminal_for(catalog.risks, b.house->happiness);
    const std::string type = kind == "disgruntled_citizen" ? kind : "outlaw";
    WalkerId w = kNoWalker;
    if (kind == "thief" || kind == "vandal") {
        if (const auto elite = nearest_elite(city, b.origin)) {  // thieves and vandals go for elite houses (Z:88)
            w = spawn_destination_walker(city, st.walkers, catalog, type, house,
                                         *city.buildings.items.at(*elite).road_access);
            if (w != kNoWalker) {
                st.walkers.items.at(w).job = WalkerJob::Crime;
                st.walkers.items.at(w).target = *elite;
            }
        }
    }
    if (w == kNoWalker) {
        w = spawn_roaming_walker(city, st.walkers, catalog, type, house, b.next_roam_dir);
    }
    if (w != kNoWalker) {
        st.walkers.items.at(w).variant = kind;
        ev.spawned.push_back(w);
        ev.disasters.emplace_back(DisasterKind::Crime, house);
    }
}

// Sets the building downwind of `b` on fire, if it can burn and is not burning yet.
void spread_fire(WorldState& st, const DataCatalog& catalog, const Building& b) {
    const auto wind = static_cast<Dir>(st.city.wind);
    const TilePos mid{b.origin.x + b.width / 2, b.origin.y + b.height / 2};
    TilePos p = mid;
    for (std::int32_t i = 0; i <= std::max(b.width, b.height); ++i) {
        p = step(p, wind);
        if (!st.city.map.in_bounds(p)) {
            return;
        }
        const BuildingId id = st.city.map.at(p).building;
        if (id != kNoBuilding && id != b.id) {
            Building& n = st.city.buildings.items.at(id);
            if (n.burning_days == 0 && hazard_rate(catalog, n, st.difficulty, true) > 0) {
                n.burning_days = 1;
            }
            return;
        }
    }
}

}  // namespace

void ruin_building(WorldState& st, const DataCatalog&, BuildingId id, RiskEvents& ev) {
    CityState& city = st.city;
    const auto it = city.buildings.items.find(id);
    if (it == city.buildings.items.end()) {
        return;
    }
    const Building old = it->second;
    if (old.house) {
        city.homeless += old.house->population;  // the residents look for another home (Z:88)
    }
    remove_building(city, id, ev.removed);
    Building ruins;
    ruins.type = "ruins";
    ruins.origin = old.origin;
    ruins.width = old.width;
    ruins.height = old.height;
    for (const TilePos p : old.tiles) {
        if (!city.map.at(p).road) {
            ruins.tiles.push_back(p);  // roads under an agora stay roads
        }
    }
    if (!ruins.tiles.empty()) {
        ev.placed.push_back(add_building(city, std::move(ruins)));
    } else {
        update_roads(city);
    }
}

RiskEvents advance_risks(WorldState& st, const DataCatalog& catalog) {
    RiskEvents ev;
    const RiskParams& p = catalog.risks;
    if (st.time.month() == 0 && st.time.day_in_month() == 0) {
        st.city.wind = static_cast<std::uint8_t>(st.rng.stream(RngStream::Hazards).next_u32() & 3U);  // yearly wind
    }
    std::vector<BuildingId> ids;
    for (const auto& [id, b] : st.city.buildings.items) {
        if (b.type != "ruins" && !b.tiles.empty()) {
            ids.push_back(id);
        }
    }
    const auto di = static_cast<std::size_t>(st.difficulty);
    for (const BuildingId id : ids) {
        auto it = st.city.buildings.items.find(id);
        if (it == st.city.buildings.items.end()) {
            continue;  // already ruined by a vandal or a spreading fire this day
        }
        Building& b = it->second;
        if (b.burning_days > 0) {
            if (b.house && b.house->population > 0) {
                st.city.homeless += b.house->population;  // residents flee a burning house
                b.house->population = 0;
            }
            if (b.burning_days % p.spread_every_days == 0 &&
                static_cast<std::int32_t>(st.rng.stream(RngStream::Hazards).next_u32() & 3U) < p.spread_chance_per_4) {
                spread_fire(st, catalog, b);
            }
            if (++b.burning_days > p.burn_days) {
                ev.disasters.emplace_back(DisasterKind::BurnedDown, id);
                ruin_building(st, catalog, id, ev);
            }
            continue;
        }
        // A random multiplier 0, 1 or 2 (mean 1) spreads the moment of fire and collapse [RODZINA, doc 24].
        auto& rng = st.rng.stream(RngStream::Hazards);
        const auto fire_mult = static_cast<std::int32_t>(rng.next_u32() % 3U);
        const auto damage_mult = static_cast<std::int32_t>(rng.next_u32() % 3U);
        b.fire_counter = bump(b.fire_counter, hazard_rate(catalog, b, st.difficulty, true) * fire_mult);
        b.damage_counter = bump(b.damage_counter, hazard_rate(catalog, b, st.difficulty, false) * damage_mult);
        if (b.damage_counter >= p.hazard_threshold) {
            ev.disasters.emplace_back(DisasterKind::Collapsed, id);  // instant, no rescue (Z:90)
            ruin_building(st, catalog, id, ev);
            continue;
        }
        if (b.fire_counter >= p.hazard_threshold) {
            b.fire_counter = 0;
            b.burning_days = 1;
            ev.disasters.emplace_back(DisasterKind::FireStarted, id);
            continue;
        }
        if (!b.house || b.house->population == 0) {
            continue;
        }
        HouseState& h = *b.house;
        if (h.quarantine_days > 0) {
            --h.quarantine_days;
        }
        const HouseModel& m = levels_of(catalog, h)[h.level].model[di];
        const bool eats = levels_of(catalog, h)[h.level].consumption[0] > 0;
        const bool fed = !eats || h.goods[static_cast<std::size_t>(HouseGood::Food)] > 0;
        b.disease_counter = bump(b.disease_counter, m.disease_risk * (fed ? 1 : p.disease_hungry_multiplier));
        if (b.disease_counter >= p.disease_threshold && h.quarantine_days == 0) {
            b.disease_counter = 0;
            h.population -= h.population * static_cast<std::uint32_t>(p.plague_death_percent) / 100;  // Z:85
            h.quarantine_days = static_cast<std::uint16_t>(p.quarantine_days);
            ev.disasters.emplace_back(DisasterKind::Plague, id);
        }
        b.crime_counter = bump(b.crime_counter, m.crime_risk);
        if (b.crime_counter >= p.crime_threshold && b.road_access) {
            b.crime_counter = 0;
            spawn_criminal(st, catalog, id, ev);
        }
    }
    return ev;
}

std::vector<WalkerId> catch_outlaws(WorldState& st, const DataCatalog& catalog) {
    std::vector<TilePos> guards;
    for (const auto& [id, w] : st.walkers.items) {
        const auto t = catalog.walkers.find(w.type);
        if (t != catalog.walkers.end() && t->second.stops_outlaws && w.phase != WalkerPhase::Done) {
            guards.push_back(w.tile);
        }
    }
    std::vector<WalkerId> caught;
    for (auto& [id, w] : st.walkers.items) {
        if (w.type != "outlaw" || w.phase == WalkerPhase::Done) {
            continue;
        }
        for (const TilePos g : guards) {
            if (std::max(std::abs(g.x - w.tile.x), std::abs(g.y - w.tile.y)) <= kServiceRadius) {
                w.phase = WalkerPhase::Done;  // stopped on the street (Z:88)
                caught.push_back(id);
                break;
            }
        }
    }
    return caught;
}

void crime_at_target(WorldState& st, const DataCatalog& catalog, Walker& w, RiskEvents& ev) {
    const auto it = st.city.buildings.items.find(w.target);
    if (it != st.city.buildings.items.end() && it->second.house && it->second.house->population > 0) {
        if (w.variant == "thief") {
            it->second.house->goods.fill(0);  // food and goods stolen; the peddler must refill (Z:88)
            ev.disasters.emplace_back(DisasterKind::Crime, w.target);
        } else if (w.variant == "vandal") {
            ev.disasters.emplace_back(DisasterKind::Crime, w.target);
            ruin_building(st, catalog, w.target, ev);  // the house is destroyed (Z:88)
        }
    }
    w.phase = WalkerPhase::Done;
}

}  // namespace bo::core
