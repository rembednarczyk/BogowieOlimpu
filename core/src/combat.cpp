// Module D3 stage 2: invasions, combat, morale (docs/gameplay/11, 22, 24; Z:137-144; assumptions L-062, L-110).
#include "combat_internal.h"
#include <algorithm>
#include <cstdlib>
#include <optional>

#include "bo/core/military.h"
#include "bo/core/path.h"
#include "bo/core/state.h"
#include "bo/core/walker.h"

namespace bo::core {
namespace combat_detail {

std::int32_t chebyshev(TilePos a, TilePos b) { return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y)); }

const UnitStats* stats_of(const DataCatalog& catalog, const Walker& w) {
    std::string nation = "player_greek";
    std::string unit = w.type;
    if (w.job == WalkerJob::Invade || (w.job == WalkerJob::Soldier && !w.variant.empty())) {
        nation = w.variant;  // invaders, and the gods' soldiers fighting for the player (Z:140)
    } else if (w.job == WalkerJob::Monster || w.job == WalkerJob::Hero) {
        nation = w.job == WalkerJob::Monster ? "monster" : "hero";  // L-057b
        unit = nation;
    }
    const auto n = catalog.combat.nations.find(nation);
    if (n == catalog.combat.nations.end()) {
        return nullptr;
    }
    const auto u = n->second.units.find(unit);
    return u == n->second.units.end() ? nullptr : &u->second;
}

bool fighter(const Walker& w) {
    const bool armed = w.job == WalkerJob::Soldier || w.job == WalkerJob::Invade || w.job == WalkerJob::Monster ||
                       w.job == WalkerJob::Hero;
    return armed && w.phase != WalkerPhase::Done && w.hp > 0;
}

// Sides: the city (soldiers, heroes) against invaders and monsters.
bool defends(const Walker& w) { return w.job == WalkerJob::Soldier || w.job == WalkerJob::Hero; }

// The invaders' goal: the Palace (Z:137); without one, the building nearest to where they came in [ZAŁOŻENIE].
std::optional<BuildingId> goal_of(const CityState& city, TilePos from) {
    std::optional<std::pair<std::int32_t, BuildingId>> best;
    for (const auto& [id, b] : city.buildings.items) {
        if (b.type == "palace") {
            return id;
        }
        const std::int32_t d = chebyshev(b.origin, from);
        if (!b.tiles.empty() && (!best || d < best->first)) {
            best = std::make_pair(d, id);
        }
    }
    if (!best) {
        return std::nullopt;
    }
    return best->second;
}

std::int32_t distance_to(const Building& b, TilePos p) {
    std::int32_t d = chebyshev(b.origin, p);
    for (const TilePos t : b.tiles) {
        d = std::min(d, chebyshev(t, p));
    }
    return d;
}

bool route(CityState& city, Walker& w, TilePos to, BuildingId goal) {
    PathRequest req{w.tile, to, defends(w) ? PathMode::Walkable : PathMode::Hostile};
    req.goal_building = goal;
    auto path = find_path(city, req);
    if (!path) {
        return false;
    }
    w.path = std::move(*path);
    w.path_pos = 0;
    w.phase = WalkerPhase::Routed;
    return true;
}

bool fortification(const Building& b) { return b.type == "wall" || b.type == "gatehouse" || b.type == "tower"; }

// An invader with no way to its goal attacks the nearest fortification [ZAŁOŻENIE, L-117].
void breach(WorldState& st, const UnitStats& s, Walker& w, CombatEvents& ev) {
    std::optional<std::pair<std::int32_t, BuildingId>> best;
    for (const auto& [id, b] : st.city.buildings.items) {
        if (fortification(b)) {
            std::int32_t d = 1 << 30;
            for (const TilePos t : b.tiles) {
                d = std::min(d, chebyshev(t, w.tile));
            }
            if (!best || d < best->first) {
                best = std::make_pair(d, id);
            }
        }
    }
    if (!best) {
        return;
    }
    Building& f = st.city.buildings.items.at(best->second);
    if (best->first > 1) {
        PathRequest req{w.tile, f.origin, PathMode::Hostile};
        req.goal_building = best->second;
        auto path = find_path(st.city, req);
        if (path && path->size() > 1) {
            path->pop_back();  // stop next to it
            w.path = std::move(*path);
            w.path_pos = 0;
            w.phase = WalkerPhase::Routed;
        }
        return;
    }
    if (w.cooldown > 0) {
        return;
    }
    f.hits -= std::max(1, s.attack);
    w.cooldown = static_cast<std::uint16_t>(std::max(1, s.melee_rate));
    if (f.hits <= 0) {
        remove_building(st.city, best->second, ev.removed);
    }
}

// Manned towers shoot at enemies in range (Z:146): rabble missiles, tower_shots at a time [L-117].
void towers_shoot(WorldState& st, const DataCatalog& catalog, const std::vector<WalkerId>& fighters) {
    const auto rabble = catalog.combat.nations.find("player_greek");
    if (rabble == catalog.combat.nations.end() || rabble->second.units.count("rabble") == 0) {
        return;
    }
    const UnitStats& shot = rabble->second.units.at("rabble");
    for (auto& [id, b] : st.city.buildings.items) {
        if (b.type != "tower" || !st.towers_manned || b.staffing_percent == 0 || b.cursed_days > 0) {
            continue;
        }
        if (b.cooldown > 0) {
            --b.cooldown;
            continue;
        }
        const TilePos at{b.origin.x + 1, b.origin.y + 1};
        std::optional<std::pair<std::int32_t, WalkerId>> foe;
        for (const WalkerId wid : fighters) {
            const Walker& o = st.walkers.items.at(wid);
            const std::int32_t d = chebyshev(o.tile, at);
            if (fighter(o) && !defends(o) && d <= shot.missile_range && (!foe || d < foe->first)) {
                foe = std::make_pair(d, wid);
            }
        }
        if (!foe) {
            continue;
        }
        Walker& o = st.walkers.items.at(foe->second);
        const UnitStats* os = stats_of(catalog, o);
        const std::int32_t hit = std::max(0, shot.missile_attack - (os ? os->armor_vs_missiles : 0));
        o.hp -= catalog.combat.tower_shots * hit;
        b.cooldown = static_cast<std::uint16_t>(shot.missile_rate);
    }
}

void stand(Walker& w) {
    w.path.clear();
    w.path_pos = 0;
    w.phase = WalkerPhase::AtTarget;
}

// Ends the invasion: the remaining invaders walk out to where they came from.
void end_invasion(WorldState& st, std::uint8_t result, CombatEvents& ev) {
    Invasion& inv = st.military.invasion;
    for (const WalkerId id : inv.invaders) {
        const auto it = st.walkers.items.find(id);
        if (it == st.walkers.items.end()) {
            continue;
        }
        Walker& w = it->second;
        w.job = WalkerJob::None;  // removed when they reach the edge
        if (!route(st.city, w, inv.origin)) {
            w.phase = WalkerPhase::Done;
        }
    }
    ev.city = inv.city;
    inv = Invasion{};
    for (Company& c : st.military.companies) {
        c.fled = false;
        if (c.status == CompanyStatus::Fighting) {
            c.status = CompanyStatus::Marching;
        }
    }
    ev.ended = result;
}

Company* company_of(WorldState& st, WalkerId id) {
    for (Company& c : st.military.companies) {
        if (std::find(c.soldiers.begin(), c.soldiers.end(), id) != c.soldiers.end()) {
            return &c;
        }
    }
    return nullptr;
}

// A soldier dies (Z:144): the house loses its people (rabble: 6 each, doc 22) or its armor and horse.
void soldier_killed(WorldState& st, const DataCatalog& catalog, const Walker& w) {
    const auto house = st.city.buildings.items.find(w.home);
    if (house != st.city.buildings.items.end() && house->second.house) {
        HouseState& h = *house->second.house;
        const auto loss = static_cast<std::uint32_t>(w.type == "rabble" ? catalog.combat.rabble_population_loss_per_death : 1);
        h.population -= std::min(h.population, loss);
        auto& armor = h.goods[static_cast<std::size_t>(HouseGood::Armor)];
        auto& horses = h.goods[static_cast<std::size_t>(HouseGood::Horses)];
        armor -= w.type != "rabble" && armor > 0 ? 1 : 0;
        horses -= w.type == "horseman" && horses > 0 ? 1 : 0;
    }
    Company* c = company_of(st, w.id);
    if (!c || !c->nation.empty()) {
        return;  // the gods' companies never lose heart (military.json: morale_never_drops)
    }
    c->morale = static_cast<std::uint8_t>(c->morale > kMoraleLossPerDeath ? c->morale - kMoraleLossPerDeath : 0);
    if (c->morale > kFleeMorale || c->fled) {
        return;
    }
    c->fled = true;  // breaks ranks and runs to the Palace (Z:140)
    c->status = CompanyStatus::Fleeing;
    const auto palace = goal_of(st.city, w.tile);
    for (const WalkerId id : c->soldiers) {
        Walker& s = st.walkers.items.at(id);
        s.job = WalkerJob::None;
        const Building* p = palace ? &st.city.buildings.items.at(*palace) : nullptr;
        if (s.id == w.id || !p || !p->road_access || !route(st.city, s, *p->road_access)) {
            s.phase = WalkerPhase::Done;
        }
    }
}

}  // namespace combat_detail

using namespace combat_detail;

std::string start_invasion(WorldState& st, const DataCatalog& catalog, const std::string& nation,
                           std::uint32_t soldiers, CombatEvents& ev, std::int32_t city) {
    const auto n = catalog.combat.nations.find(nation);
    if (st.military.invasion.active || n == catalog.combat.nations.end() || n->second.composition.empty()) {
        return "military.bad_invasion";
    }
    std::optional<TilePos> origin;
    for (const TileFeature f : {TileFeature::InvasionPoint, TileFeature::LandEntry}) {
        for (std::size_t i = 0; i < st.city.map.size() && !origin; ++i) {
            if (st.city.map.tiles()[i].feature == static_cast<std::uint8_t>(f)) {
                origin = st.city.map.pos(i);
            }
        }
    }
    const auto goal = goal_of(st.city, origin.value_or(TilePos{}));
    if (!origin || !goal) {
        return "military.no_invasion_point";
    }
    soldiers = std::min(soldiers, static_cast<std::uint32_t>(catalog.combat.max_invasion));
    Invasion& inv = st.military.invasion;
    inv.active = true;
    inv.nation = nation;
    inv.origin = *origin;
    inv.city = city;
    const Building& target = st.city.buildings.items.at(*goal);
    for (std::size_t k = 0; k < n->second.composition.size() && k < n->second.composition_units.size(); ++k) {
        const std::string& unit = n->second.composition_units[k];
        const UnitStats& s = n->second.units.at(unit);
        const auto count = soldiers * static_cast<std::uint32_t>(n->second.composition[k]) / 100;  // rounded down
        for (std::uint32_t i = 0; i < count; ++i) {
            Walker w;
            w.id = st.walkers.next_id++;
            w.type = unit;
            w.variant = nation;
            w.job = WalkerJob::Invade;
            w.tile = *origin;
            w.speed_raw = s.speed_raw;
            w.hp = s.hp;
            w.target = *goal;
            if (!route(st.city, w, target.origin, *goal)) {
                stand(w);
            }
            inv.invaders.push_back(w.id);
            ev.spawned.push_back(w.id);
            st.walkers.items.emplace(w.id, std::move(w));
        }
    }
    return {};
}

std::int64_t bribe_cost(const WorldState& st, const DataCatalog& catalog) {
    const CombatParams& p = catalog.combat;
    std::int64_t cost = 0;
    for (const WalkerId id : st.military.invasion.invaders) {
        const auto it = st.walkers.items.find(id);
        if (it == st.walkers.items.end()) {
            continue;
        }
        const UnitStats* s = stats_of(catalog, it->second);
        cost += s && s->mounted ? p.bribe_mounted : p.bribe_foot;
    }
    return cost * p.bribe_percent[static_cast<std::size_t>(st.difficulty)] / 100;
}

std::string bribe_invaders(WorldState& st, const DataCatalog& catalog, CombatEvents& ev) {
    if (!st.military.invasion.active) {
        return "military.no_invasion";
    }
    const std::int64_t cost = bribe_cost(st, catalog);
    if (st.finance.treasury < cost) {
        return "military.cannot_afford_bribe";
    }
    st.finance.treasury -= cost;
    end_invasion(st, 1, ev);
    return {};
}

std::string surrender(WorldState& st, CombatEvents& ev) {
    if (!st.military.invasion.active) {
        return "military.no_invasion";
    }
    end_invasion(st, 2, ev);
    return {};
}

void combat_tick(WorldState& st, const DataCatalog& catalog, CombatEvents& ev) {
    Invasion& inv = st.military.invasion;
    auto& items = st.walkers.items;
    const bool monsters = std::any_of(items.begin(), items.end(),
                                      [](const auto& kv) { return kv.second.job == WalkerJob::Monster; });
    if (!inv.active && !monsters) {
        return;
    }
    std::vector<WalkerId> fighters;
    for (const auto& [id, w] : items) {
        if (fighter(w)) {
            fighters.push_back(id);
        }
    }
    for (Company& c : st.military.companies) {
        if (c.status == CompanyStatus::Fighting) {
            c.status = CompanyStatus::Still;  // set again below while someone fights
        }
    }
    const bool rechase = st.time.tick() % 15 == 0;  // re-aim chases once a tile [ZAŁOŻENIE, L-110]
    divine_help(st);
    towers_shoot(st, catalog, fighters);
    for (const WalkerId id : fighters) {
        Walker& w = items.at(id);
        if (!fighter(w)) {
            continue;
        }
        const UnitStats* s = stats_of(catalog, w);
        if (!s) {
            continue;
        }
        w.cooldown = static_cast<std::uint16_t>(w.cooldown > 0 ? w.cooldown - 1 : 0);
        std::optional<std::pair<std::int32_t, WalkerId>> foe;
        for (const WalkerId oid : fighters) {
            const Walker& o = items.at(oid);
            if (fighter(o) && defends(o) != defends(w)) {
                const std::int32_t d = chebyshev(w.tile, o.tile);
                if (!foe || d < foe->first) {
                    foe = std::make_pair(d, oid);
                }
            }
        }
        Company* c = w.job == WalkerJob::Soldier ? company_of(st, id) : nullptr;
        if (foe && foe->first <= engage_tiles(catalog, c, *s)) {
            fight(st, catalog, w, *s, c, *foe, rechase);
            continue;
        }
        if (w.job == WalkerJob::Invade) {
            const auto goal = st.city.buildings.items.find(w.target);
            if (goal == st.city.buildings.items.end()) {
                const auto next = goal_of(st.city, w.tile);
                if (!next) {
                    end_invasion(st, 3, ev);
                    return;
                }
                w.target = *next;
                stand(w);
                continue;
            }
            if (distance_to(goal->second, w.tile) <= 1) {
                if (invaders_stronger(st, catalog)) {
                    end_invasion(st, 3, ev);  // an army stronger than the player's reached the Palace (Z:144)
                    return;
                }
                stand(w);  // B-015: the player's army is stronger; they fight on at the gates
                continue;
            }
            if (w.phase == WalkerPhase::AtTarget && !route(st.city, w, goal->second.origin, goal->first)) {
                breach(st, *s, w, ev);  // walled off: break through the nearest wall or gate (Z:146-147)
            }
        }
    }
    for (const WalkerId id : fighters) {
        Walker& w = items.at(id);
        if (w.hp > 0 || w.phase == WalkerPhase::Done) {
            continue;
        }
        w.phase = WalkerPhase::Done;
        if (w.job == WalkerJob::Soldier) {
            soldier_killed(st, catalog, w);
        }
    }
    if (!inv.active) {
        return;  // only monsters about: their fate is the monster module's (monsters.cpp)
    }
    auto& inv_list = inv.invaders;
    inv_list.erase(std::remove_if(inv_list.begin(), inv_list.end(),
                                  [&items](WalkerId i) {
                                      const auto it = items.find(i);
                                      return it == items.end() || it->second.phase == WalkerPhase::Done;
                                  }),
                   inv_list.end());
    if (inv_list.empty()) {
        end_invasion(st, 0, ev);
    }
}

void recover_morale(WorldState& st) {
    const bool monster = std::any_of(st.monsters.begin(), st.monsters.end(), [](const MonsterState& m) { return !m.slain; });
    if (st.military.invasion.active || monster) {
        return;  // a monster in the city is a battle too (owner, B-004)
    }
    for (Company& c : st.military.companies) {
        c.morale = static_cast<std::uint8_t>(std::min(100, c.morale + kMoraleRecoveryPerDay));
    }
}

}  // namespace bo::core
