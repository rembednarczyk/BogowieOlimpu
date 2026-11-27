// Module D5 stage 3: monsters, Hero's Halls, heroes (docs/gameplay/10, 21, 23; Z:127-131; L-057b, L-116).
#include "bo/core/monsters.h"

#include <algorithm>
#include <cstdlib>
#include <optional>

#include "bo/core/gods.h"
#include "bo/core/migration.h"
#include "bo/core/path.h"
#include "bo/core/state.h"
#include "bo/core/storage.h"

namespace bo::core {
namespace {

std::int32_t number(const DataCatalog& catalog, const char* key, std::int32_t fallback) {
    const auto it = catalog.monster_numbers.find(key);
    return it == catalog.monster_numbers.end() ? fallback : it->second;
}

std::int32_t chebyshev(TilePos a, TilePos b) { return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y)); }

bool route(CityState& city, Walker& w, TilePos to, BuildingId goal = kNoBuilding) {
    PathRequest req{w.tile, to, PathMode::Walkable};  // monsters go over gates [ZAŁOŻENIE, L-117]
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

// The building a monster goes for (doc 21: food by default; citizens, military, resources).
std::optional<BuildingId> pick_target(const WorldState& st, const DataCatalog& catalog, const std::string& kind,
                                      TilePos from) {
    const auto wanted = [&](const Building& b) {
        if (b.tiles.empty() || b.type == "ruins") {
            return false;
        }
        if (kind == "citizens") {
            return b.house.has_value() && b.house->population > 0;
        }
        if (kind == "military") {
            return b.type == "palace" || catalog.buildings.at(b.type).tab == "military";
        }
        if (kind == "resources") {
            return b.producer.has_value();
        }
        return b.storage.has_value() && !b.trade && std::any_of(b.storage->loads.begin(), b.storage->loads.end(), [&](const auto& kv) {
                   return kv.second > 0 && catalog.goods.count(kv.first) != 0 && catalog.goods.at(kv.first) == "food";
               });
    };
    std::optional<std::pair<std::int32_t, BuildingId>> best;
    std::optional<std::pair<std::int32_t, BuildingId>> any;
    for (const auto& [id, b] : st.city.buildings.items) {
        const std::int32_t d = chebyshev(b.origin, from);
        if (wanted(b) && (!best || d < best->first)) {
            best = std::make_pair(d, id);
        }
        if (!b.tiles.empty() && b.type != "ruins" && (!any || d < any->first)) {
            any = std::make_pair(d, id);
        }
    }
    if (best) {
        return best->second;
    }
    if (any) {
        return any->second;  // nothing of its kind left: anything will do [ZAŁOŻENIE, L-116]
    }
    return std::nullopt;
}

}  // namespace

std::string spawn_monster(WorldState& st, const DataCatalog& catalog, const std::string& id, std::uint32_t strength,
                          const std::string& target_kind, MonsterEvents& ev) {
    const auto stats = catalog.combat.nations.find("monster");
    if (catalog.monster_slayer.count(id) == 0 || stats == catalog.combat.nations.end()) {
        return "myth.unknown_monster";
    }
    std::optional<TilePos> home;
    for (const TileFeature f : {TileFeature::MonsterPoint, TileFeature::LandEntry}) {
        for (std::size_t i = 0; i < st.city.map.size() && !home; ++i) {
            if (st.city.map.tiles()[i].feature == static_cast<std::uint8_t>(f)) {
                home = st.city.map.pos(i);  // without a monster point it starts at the land entry (doc 23)
            }
        }
    }
    if (!home) {
        return "myth.no_monster_point";
    }
    const UnitStats& s = stats->second.units.at("monster");
    Walker w;
    w.id = st.walkers.next_id++;
    w.type = id;
    w.job = WalkerJob::Monster;
    w.tile = *home;
    w.phase = WalkerPhase::AtTarget;
    w.speed_raw = s.speed_raw;
    w.hp = s.hp * static_cast<std::int32_t>(std::max<std::uint32_t>(strength, 1));
    MonsterState m;
    m.id = id;
    m.walker = w.id;
    m.home = *home;
    m.target_kind = target_kind.empty() ? "food" : target_kind;
    ev.spawned.push_back(w.id);
    st.walkers.items.emplace(w.id, std::move(w));
    st.monsters.push_back(m);
    return {};
}

void advance_monsters(WorldState& st, const DataCatalog& catalog, MonsterEvents& ev) {
    const bool apollo = god_blesses(st.city, catalog, "apollo");
    for (MonsterState& m : st.monsters) {
        if (m.slain) {
            continue;
        }
        const auto it = st.walkers.items.find(m.walker);
        if (it == st.walkers.items.end() || it->second.hp <= 0 || it->second.phase == WalkerPhase::Done || apollo) {
            if (it != st.walkers.items.end()) {
                it->second.phase = WalkerPhase::Done;  // Apollo kills monsters that attack the city (Z:129)
            }
            m.slain = true;
            const auto slayer = catalog.monster_slayer.find(m.id);  // its own hero, not any hero (Z:131)
            const bool by_hero = slayer != catalog.monster_slayer.end() &&
                                 std::any_of(st.walkers.items.begin(), st.walkers.items.end(), [&slayer](const auto& kv) {
                                     return kv.second.job == WalkerJob::Hero && kv.second.type == slayer->second;
                                 });
            ev.slain.emplace_back(m.id, by_hero && !apollo);
            if (by_hero && !apollo) {
                ++st.scenario.monuments["hero"];  // a monster slain in the city earns a Hero monument (Z:131)
            }
            continue;
        }
        Walker& w = it->second;
        if (w.phase != WalkerPhase::AtTarget) {
            continue;  // on its way (or chasing a soldier)
        }
        if (m.resting) {
            if (m.days > 0) {
                --m.days;
                continue;
            }
            m.resting = false;
        }
        const auto target = st.city.buildings.items.find(m.target);
        if (target == st.city.buildings.items.end()) {
            const auto next = pick_target(st, catalog, m.target_kind, w.tile);
            if (next) {
                m.target = *next;
                m.days = 0;
                const Building& b = st.city.buildings.items.at(*next);
                route(st.city, w, b.origin, *next);
            }
            continue;
        }
        if (std::none_of(target->second.tiles.begin(), target->second.tiles.end(),
                         [&w](TilePos p) { return chebyshev(p, w.tile) <= 1; })) {
            route(st.city, w, target->second.origin, m.target);
            continue;
        }
        if (++m.days >= static_cast<std::uint16_t>(number(catalog, "destroy_days", 2))) {
            remove_building(st.city, m.target, ev.removed);  // wrecked (Z:127)
            m.target = kNoBuilding;
            m.resting = true;
            m.days = static_cast<std::uint16_t>(number(catalog, "rest_days", 16));
            route(st.city, w, m.home);  // back to where it came from (S-004 scendesign)
        }
    }
    for (HeroState& h : st.heroes) {
        const auto hero = st.walkers.items.find(h.walker);
        if (hero == st.walkers.items.end()) {
            continue;
        }
        Walker& w = hero->second;
        const auto prey = std::find_if(st.monsters.begin(), st.monsters.end(), [&](const MonsterState& m) {
            const auto s = catalog.monster_slayer.find(m.id);
            return !m.slain && s != catalog.monster_slayer.end() && s->second == h.id;
        });
        if (prey != st.monsters.end() && st.walkers.items.count(prey->walker) != 0) {
            route(st.city, w, st.walkers.items.at(prey->walker).tile);  // the hero hunts it down (Z:130)
        } else if (w.phase == WalkerPhase::AtTarget) {
            const auto hall = st.city.buildings.items.find(h.hall);
            if (hall != st.city.buildings.items.end() && hall->second.road_access && w.tile != *hall->second.road_access) {
                route(st.city, w, *hall->second.road_access);  // back to the Hall
            }
        }
    }
}

bool hero_needed(const WorldState& st) {
    return std::any_of(st.monsters.begin(), st.monsters.end(), [](const MonsterState& m) { return !m.slain; }) ||
           std::any_of(st.scenario.quests.begin(), st.scenario.quests.end(),
                       [](const Quest& q) { return q.status == QuestStatus::Open; });
}


std::string summon_hero(WorldState& st, const DataCatalog& catalog, BuildingId hall, MonsterEvents& ev) {
    const auto h = st.city.buildings.items.find(hall);
    if (h == st.city.buildings.items.end() || h->second.type != "heros_hall" || !h->second.road_access) {
        return "myth.no_hall";
    }
    std::string hero;
    const auto absent = [&st](const std::string& id) {
        return std::none_of(st.heroes.begin(), st.heroes.end(), [&id](const HeroState& x) { return x.id == id; });
    };
    for (const MonsterState& m : st.monsters) {
        const auto s = catalog.monster_slayer.find(m.id);
        if (!m.slain && s != catalog.monster_slayer.end() && absent(s->second)) {
            hero = s->second;
            break;
        }
    }
    for (const Quest& q : st.scenario.quests) {  // a quest is also a reason for a hero to come (Z:129)
        if (hero.empty() && q.status == QuestStatus::Open && !q.hero.empty() && absent(q.hero)) {
            hero = q.hero;
        }
    }
    if (hero.empty()) {
        return "myth.no_hero_needed";
    }
    if (!unmet_requirements(st, catalog, hero, hall).empty()) {
        return "myth.hero_requirements";
    }
    for (const std::string& r : catalog.hero_requirements.at(hero)) {
        if (r.rfind("drachmas:", 0) == 0 && r.find("pobierane") != std::string::npos &&
            r.find("nie pobierane") == std::string::npos) {
            st.finance.treasury -= std::stoll(r.substr(9, r.find(' ') - 9));  // Perseus takes his fee (doc 21)
        }
    }
    st.heroes.push_back(HeroState{hero, hall, kNoWalker});
    st.heroes.back().walker = hero_walker(st, catalog, st.heroes.back());
    ev.spawned.push_back(st.heroes.back().walker);
    return {};
}

WalkerId hero_walker(WorldState& st, const DataCatalog& catalog, const HeroState& hero) {
    const auto hall = st.city.buildings.items.find(hero.hall);
    if (hall == st.city.buildings.items.end() || !hall->second.road_access) {
        return kNoWalker;
    }
    const auto& stats = catalog.combat.nations.at("hero").units.at("hero");
    Walker w;
    w.id = st.walkers.next_id++;
    w.type = hero.id;
    w.job = WalkerJob::Hero;
    w.home = hero.hall;
    w.tile = *hall->second.road_access;
    w.phase = WalkerPhase::AtTarget;
    w.speed_raw = stats.speed_raw;
    w.hp = stats.hp;
    const WalkerId id = w.id;
    st.walkers.items.emplace(id, std::move(w));
    return id;
}

}  // namespace bo::core
