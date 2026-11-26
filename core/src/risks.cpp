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

// Escalation (S-010): the first criminal of an unhappy house is disgruntled, then a mugger, a thief, a vandal.
std::string criminal_for(const RiskParams& p, std::uint8_t level) {
    if (p.crime_escalation.empty()) {
        return "disgruntled_citizen";
    }
    return p.crime_escalation[std::min<std::size_t>(level, p.crime_escalation.size() - 1)];
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
    Building& b = city.buildings.items.at(house);
    const std::string kind = criminal_for(catalog.risks, b.house->crime_level);
    b.house->crime_level = static_cast<std::uint8_t>(std::min(b.house->crime_level + 1, 255));
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

void strike_plague(WorldState& st, const RiskParams& p, BuildingId id, RiskEvents& ev) {
    Building& b = st.city.buildings.items.at(id);
    HouseState& h = *b.house;
    b.disease_counter = 0;
    h.population -= h.population / static_cast<std::uint32_t>(std::max(1, p.plague_death_one_in));  // S-010
    h.quarantine_days = static_cast<std::uint16_t>(p.quarantine_days);
    ev.disasters.emplace_back(DisasterKind::Plague, id);
}

// City hygiene (family health model, S-006): the share of residents with a healer's visit and food (full),
// with only one of them (1/4), with neither (0); moves hygiene_step a month toward it. Small cities stay at
// the start value.
void update_hygiene(WorldState& st, const DataCatalog& catalog) {
    const RiskParams& p = catalog.risks;
    std::uint64_t people = 0;
    std::uint64_t healthy4 = 0;  // in quarters of a person
    for (const auto& [id, b] : st.city.buildings.items) {
        if (!b.house || b.house->population == 0) {
            continue;
        }
        const HouseState& h = *b.house;
        const auto healer = b.coverage.find("healer");
        const bool visited = healer != b.coverage.end() && healer->second > 0;
        const bool eats = levels_of(catalog, h)[h.level].consumption[0] > 0;
        const bool fed = !eats || h.goods[static_cast<std::size_t>(HouseGood::Food)] > 0;
        people += h.population;
        healthy4 += h.population * (visited && fed ? 4U : visited || fed ? 1U : 0U);
    }
    if (people < static_cast<std::uint64_t>(catalog.migration.small_city_population)) {
        st.city.hygiene = static_cast<std::uint8_t>(p.hygiene_start);
        return;
    }
    const auto target = static_cast<std::int32_t>(healthy4 * 100 / (people * 4));
    const std::int32_t step = std::clamp(target - st.city.hygiene, -p.hygiene_step, p.hygiene_step);
    st.city.hygiene = static_cast<std::uint8_t>(st.city.hygiene + step);
}

// Z:90: a superintendent rushes to a fire: the nearest staffed office within reach sends one (S-007: 25 tiles).
void dispatch_firefighter(WorldState& st, const DataCatalog& catalog, BuildingId burning, RiskEvents& ev) {
    CityState& city = st.city;
    const Building& b = city.buildings.items.at(burning);
    if (!b.road_access) {
        return;
    }
    std::optional<std::pair<std::int32_t, std::pair<BuildingId, std::string>>> best;
    for (const auto& [wid, wt] : catalog.walkers) {
        if ((wt.resets & 1U) == 0) {
            continue;
        }
        for (const auto& [oid, o] : city.buildings.items) {
            if (std::find(wt.from.begin(), wt.from.end(), o.type) == wt.from.end() || !o.road_access ||
                o.staffing_percent == 0) {
                continue;
            }
            const std::int32_t d = std::max(std::abs(o.origin.x - b.origin.x), std::abs(o.origin.y - b.origin.y));
            if (d <= catalog.risks.firefighter_reach && (!best || d < best->first)) {
                best = std::make_pair(d, std::make_pair(oid, wid));
            }
        }
    }
    if (!best) {
        return;
    }
    const WalkerId w = spawn_destination_walker(city, st.walkers, catalog, best->second.second, best->second.first,
                                                *b.road_access);
    if (w != kNoWalker) {
        st.walkers.items.at(w).job = WalkerJob::Extinguish;
        st.walkers.items.at(w).target = burning;
        ev.spawned.push_back(w);
    }
}

}  // namespace

void extinguish_at_target(WorldState& st, const DataCatalog& catalog, Walker& w) {
    const auto it = st.city.buildings.items.find(w.target);
    if (it != st.city.buildings.items.end() && it->second.burning_days > 0 &&
        it->second.burning_days <= catalog.risks.extinguish_days) {
        it->second.burning_days = 0;  // saved before the flames took the whole building (Z:90)
        it->second.fire_counter = 0;
    }
    w.job = WalkerJob::None;
    send_home(st.city, w);
}

std::vector<WalkerId> muggings(WorldState& st, const DataCatalog& catalog) {
    std::vector<WalkerId> victims;
    for (auto& [mid, m] : st.walkers.items) {
        if (m.variant != "mugger" || m.phase == WalkerPhase::Done || m.work_ticks >= kMuggerVictims) {
            continue;
        }
        for (auto& [vid, v] : st.walkers.items) {
            const auto vt = catalog.walkers.find(v.type);
            const bool guard = vt != catalog.walkers.end() && vt->second.stops_outlaws;
            if (vid == mid || v.type == "outlaw" || v.type == "disgruntled_citizen" || guard ||
                v.phase == WalkerPhase::Done || v.tile != m.tile) {
                continue;
            }
            v.phase = WalkerPhase::Done;  // attacked in the street (Z:87)
            victims.push_back(vid);
            ++m.work_ticks;
            // The dead come from the nearest home: residents of the house closest to the attack [eZeus: 5].
            std::optional<std::pair<std::int32_t, BuildingId>> home;
            for (const auto& [hid, h] : st.city.buildings.items) {
                if (h.house && h.house->population > 0) {
                    const std::int32_t d = std::max(std::abs(h.origin.x - m.tile.x), std::abs(h.origin.y - m.tile.y));
                    if (!home || d < home->first) {
                        home = std::make_pair(d, hid);
                    }
                }
            }
            if (home) {
                auto& pop = st.city.buildings.items.at(home->second).house->population;
                pop -= std::min(pop, catalog.risks.mugger_victims);
            }
            break;
        }
    }
    return victims;
}

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
    if (st.time.day_in_month() == 0) {
        update_hygiene(st, catalog);
    }
    auto& rng = st.rng.stream(RngStream::Hazards);
    const std::uint32_t day_mask = rng.next_u32() & 7U;  // fire grows on 1 day in 8 per building (S-007 OG)
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
        // Collapse: + damage_risk every day, 60 / damage_risk months to the threshold (S-004 legend).
        b.damage_counter = bump(b.damage_counter, hazard_rate(catalog, b, st.difficulty, false));
        // Fire: on the building's day (1 in 8) + fire_risk x random 1-5 [RODZINA, S-007 OG structure].
        if (((b.id + st.city.map.at(b.origin).random_byte) & 7U) == day_mask) {
            const auto span = static_cast<std::uint32_t>(p.fire_factor_max - p.fire_factor_min + 1);
            const auto factor = p.fire_factor_min + static_cast<std::int32_t>(rng.next_u32() % span);
            b.fire_counter = bump(b.fire_counter, hazard_rate(catalog, b, st.difficulty, true) * factor);
        }
        if (b.damage_counter >= p.hazard_threshold) {
            ev.disasters.emplace_back(DisasterKind::Collapsed, id);  // instant, no rescue (Z:90)
            ruin_building(st, catalog, id, ev);
            continue;
        }
        if (b.fire_counter >= p.hazard_threshold) {
            b.fire_counter = 0;
            b.burning_days = 1;
            ev.disasters.emplace_back(DisasterKind::FireStarted, id);
            dispatch_firefighter(st, catalog, id, ev);
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
        if (b.disease_counter >= p.disease_threshold && h.quarantine_days == 0 && st.city.hygiene < p.hygiene_gate) {
            const TilePos at = b.origin;
            strike_plague(st, p, id, ev);
            // Other houses around are likely to catch it too, not only neighbours (S-010; radius 5, 50 %, S-008).
            for (const BuildingId other : buildings_in_square(st.city, at, p.plague_spread_radius)) {
                Building& o = st.city.buildings.items.at(other);
                if (other != id && o.house && o.house->population > 0 && o.house->quarantine_days == 0 &&
                    static_cast<std::int32_t>(rng.next_u32() % 100U) < p.plague_spread_percent) {
                    strike_plague(st, p, other, ev);
                }
            }
        }
        if (h.happiness >= p.crime_happiness_below) {
            h.crime_level = 0;  // content houses breed no criminals [RODZINA]; escalation starts over
        }
        b.crime_counter = bump(b.crime_counter, m.crime_risk);
        if (b.crime_counter >= p.crime_threshold && b.road_access && h.happiness < p.crime_happiness_below) {
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
