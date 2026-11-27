// Hero requirements (Z:130, P:24-25; data/heroes.json; docs/gameplay/21-mitologia-liczby.md). What "excellent"
// and "good" mean, how near the Palace is near and what walls must keep out are not in the manual: assumed in
// data/gods_rules.json monster_numbers [ZAŁOŻENIE, L-116].
#include <algorithm>
#include <cstdlib>
#include <optional>
#include <vector>

#include "bo/core/military.h"
#include "bo/core/appeal.h"
#include "bo/core/culture.h"
#include "bo/core/games.h"
#include "bo/core/gods.h"
#include "bo/core/hippodrome.h"
#include "bo/core/housing.h"
#include "bo/core/migration.h"
#include "bo/core/monsters.h"
#include "bo/core/state.h"
#include "bo/core/storage.h"

namespace bo::core {

namespace {

std::int32_t number(const DataCatalog& catalog, const char* key, std::int32_t fallback) {
    const auto it = catalog.monster_numbers.find(key);
    return it == catalog.monster_numbers.end() ? fallback : it->second;
}

// Percent of the residents whose houses `walker` has visited lately.
std::int64_t covered_percent(const WorldState& st, const std::string& walker) {
    std::int64_t people = 0;
    std::int64_t covered = 0;
    for (const auto& [id, b] : st.city.buildings.items) {
        if (!b.house || b.house->population == 0) {
            continue;
        }
        people += b.house->population;
        const auto it = b.coverage.find(walker);
        covered += it != b.coverage.end() && it->second > 0 ? b.house->population : 0;
    }
    return people == 0 ? 0 : 100 * covered / people;
}

bool is_fortification(const CityState& city, const Tile& t) {
    if (t.building == kNoBuilding) {
        return false;
    }
    const std::string& type = city.buildings.items.at(t.building).type;
    return type == "wall" || type == "tower" || type == "gatehouse";
}

// Whether no army can walk from where armies come in (invasion points; else the land entry; else any edge) to
// the hall without breaking through a wall, tower or gate. Other buildings do not stop it: invaders smash them.
bool walled_in(const CityState& city, const Building& hall) {
    const TileMap& map = city.map;
    std::vector<std::size_t> open;
    for (const auto feature : {TileFeature::InvasionPoint, TileFeature::LandEntry}) {
        for (std::size_t i = 0; i < map.size(); ++i) {
            if (map.tiles()[i].feature == static_cast<std::uint8_t>(feature)) {
                open.push_back(i);
            }
        }
        if (!open.empty()) {
            break;  // invasion points when the map has any
        }
    }
    if (open.empty()) {
        for (std::size_t i = 0; i < map.size(); ++i) {
            const TilePos p = map.pos(i);
            if (p.x == 0 || p.y == 0 || p.x == map.width() - 1 || p.y == map.height() - 1) {
                open.push_back(i);
            }
        }
    }
    std::vector<bool> seen(map.size(), false);
    std::vector<std::size_t> next;
    for (const std::size_t i : open) {
        const Tile& t = map.tiles()[i];
        if (!seen[i] && terrain_walkable(t.terrain) && !is_fortification(city, t)) {
            seen[i] = true;
            next.push_back(i);
        }
    }
    while (!next.empty()) {
        const TilePos p = map.pos(next.back());
        next.pop_back();
        for (const TilePos h : hall.tiles) {
            if (std::max(std::abs(h.x - p.x), std::abs(h.y - p.y)) <= 1) {
                return false;  // an army reaches the hall
            }
        }
        for (const Dir d : {Dir::North, Dir::East, Dir::South, Dir::West}) {
            const TilePos q = step(p, d);
            if (!map.in_bounds(q) || seen[map.index(q)]) {
                continue;
            }
            const Tile& t = map.at(q);
            if (terrain_walkable(t.terrain) && !is_fortification(city, t)) {
                seen[map.index(q)] = true;
                next.push_back(map.index(q));
            }
        }
    }
    return true;
}

// The requirements about the Hero's Hall itself; nullopt for any other.
std::optional<bool> hall_requirement(const WorldState& st, const DataCatalog& catalog, const std::string& req,
                                     BuildingId hall_id) {
    if (req != "hall_culture_access:excellent" && req != "hall_near_palace" && req != "hall_appeal:good" &&
        req != "hall_protected_by_walls") {
        return std::nullopt;
    }
    const auto it = st.city.buildings.items.find(hall_id);
    if (it == st.city.buildings.items.end()) {
        return false;
    }
    const Building& hall = it->second;
    if (req == "hall_culture_access:excellent") {  // every kind of performer visits it (4 kinds give 80 points)
        return house_culture(hall, catalog, city_culture_bonus(st.city, catalog)) >=
               number(catalog, "hall_excellent_culture", 80);
    }
    if (req == "hall_appeal:good") {
        return building_appeal(st.city, hall) >= number(catalog, "hall_good_appeal", 10);
    }
    if (req == "hall_protected_by_walls") {
        return walled_in(st.city, hall);
    }
    const std::int32_t reach = number(catalog, "hall_near_palace_tiles", 20);
    return std::any_of(st.city.buildings.items.begin(), st.city.buildings.items.end(), [&](const auto& kv) {
        if (kv.second.type != "palace") {
            return false;
        }
        for (const TilePos a : kv.second.tiles) {
            for (const TilePos b : hall.tiles) {
                if (std::max(std::abs(a.x - b.x), std::abs(a.y - b.y)) <= reach) {
                    return true;
                }
            }
        }
        return false;
    });
}

std::size_t count_houses(const WorldState& st, bool elite) {
    return static_cast<std::size_t>(std::count_if(st.city.buildings.items.begin(), st.city.buildings.items.end(), [elite](const auto& kv) {
        return kv.second.house && kv.second.house->elite == elite && kv.second.house->population > 0;
    }));
}

// Companies of a role: hoplites count spearmen in Atlantis, horsemen charioteers (Z:130 "(Atl: ...)", P:15).
std::int64_t companies_of(const WorldState& st, const std::string& unit) {
    return std::count_if(st.military.companies.begin(), st.military.companies.end(),
                         [&unit](const Company& c) { return unit.empty() || unit_role(c.unit) == unit; });
}

std::int64_t horses_in_city(const WorldState& st) {
    std::int64_t n = 0;
    for (const auto& [id, b] : st.city.buildings.items) {
        if (b.house) {
            n += b.house->goods[static_cast<std::size_t>(HouseGood::Horses)];
        }
        if (b.producer && b.type == "horse_ranch") {
            n += b.producer->output_loads;
        }
    }
    return n;
}

// Triremes the city has launched (Z:150): every ship of the navy, crewed or not.
std::int64_t triremes(const WorldState& st) { return static_cast<std::int64_t>(st.military.ships.size()); }

// One requirement of heroes.json (doc 21), without its note in brackets.
// A Museum staffed, on a road and in a performance (L-120): Atalanta's demand in Atlantis (P:24).
bool working_museum(const CityState& city) {
    return std::any_of(city.buildings.items.begin(), city.buildings.items.end(), [](const auto& kv) {
        const Building& b = kv.second;
        return b.type == "museum" && b.staffing_percent > 0 && b.road_access && b.performing_days > 0;
    });
}

// In Atlantis some demands differ (heroes.json "(Atl: ...)", P:24-25): the hippodrome for the games, the
// observatories for the gymnasiums, the museum for the stadium, orichalc for bronze. Science, spearmen,
// charioteers and frigates count already by role. nullopt when the demand is the same.
std::optional<bool> atlantean_requirement(const WorldState& st, const DataCatalog& catalog, const std::string& req) {
    if (!st.city.atlantean || req.find("(Atl: ") == std::string::npos) {
        return std::nullopt;
    }
    if (req.rfind("win_any_panhellenic_game", 0) == 0) {
        return working_hippodrome(st, catalog) > 0;
    }
    if (req.rfind("citywide_gymnasium_access:excellent", 0) == 0) {
        return covered_percent(st, "astronomer") >= number(catalog, "excellent_percent", 80);
    }
    if (req.rfind("working_stadium", 0) == 0) {
        return working_museum(st.city);
    }
    if (req.rfind("bronze:", 0) == 0) {
        return city_stock(st.city, catalog, "orichalc") >= std::stoll(req.substr(7, req.find(' ') - 7));
    }
    return std::nullopt;
}

bool requirement_met(const WorldState& st, const DataCatalog& catalog, std::string req, BuildingId hall) {
    if (const auto atl = atlantean_requirement(st, catalog, req)) {
        return *atl;
    }
    req = req.substr(0, req.find(" ("));
    const auto at_least = [&req](const std::string& key, std::int64_t have) {
        return req.rfind(key + ">=", 0) == 0 && have >= std::stoll(req.substr(key.size() + 2));
    };
    if (at_least("population", city_population(st.city)) || at_least("elite_houses", count_houses(st, true)) ||
        at_least("horses_in_city", horses_in_city(st)) || at_least("hoplite_companies", companies_of(st, "hoplite")) ||
        at_least("horsemen_companies", companies_of(st, "horseman")) || at_least("military_companies", companies_of(st, "")) ||
        at_least("treasury", st.finance.treasury) || at_least("triremes", triremes(st))) {
        return true;
    }
    if (req.rfind("sanctuary:", 0) == 0 || req.rfind("sanctuaries>=", 0) == 0) {
        const bool many = req.rfind("sanctuaries>=", 0) == 0;
        const std::int64_t need = many ? std::stoll(req.substr(13)) : 1;
        const std::string list = req.substr(req.find(many ? " of " : ":") + (many ? 4 : 1));
        std::int64_t have = 0;
        std::size_t start = 0;
        while (start <= list.size()) {
            const std::size_t bar = list.find('|', start);
            const std::string god = list.substr(start, bar == std::string::npos ? std::string::npos : bar - start);
            have += god_blesses(st.city, catalog, god) ? 1 : 0;
            start = bar == std::string::npos ? list.size() + 1 : bar + 1;
        }
        return have >= need;
    }
    if (const auto met = hall_requirement(st, catalog, req, hall)) {
        return *met;
    }
    if (req == "win_any_panhellenic_game") {  // a Scholar or Athlete monument earned (Z:110-111)
        return std::any_of(catalog.games.games.begin(), catalog.games.games.end(), [&st](const GameParams& g) {
            const auto it = st.scenario.monuments.find(g.monument);
            return it != st.scenario.monuments.end() && it->second > 0;
        });
    }
    if (req == "citywide_gymnasium_access:excellent") {
        return covered_percent(st, "athlete") >= number(catalog, "excellent_percent", 80);
    }
    if (req == "citywide_tax_coverage:excellent") {
        return covered_percent(st, "clerk") >= number(catalog, "excellent_percent", 80);
    }
    if (req == "working_stadium") {
        return working_stadium(st.city);
    }
    if (req == "popularity:excellent") {
        return st.migration.popularity >= 80;  // [ZAŁOŻENIE, L-116]
    }
    if (req == "city_health:excellent") {
        return st.city.hygiene >= 80 && std::none_of(st.city.buildings.items.begin(), st.city.buildings.items.end(),
                                                      [](const auto& kv) { return kv.second.house && kv.second.house->quarantine_days > 0; });
    }
    if (req == "no_unrest") {
        return god_blesses(st.city, catalog, "dionysus") ||
               std::none_of(st.city.buildings.items.begin(), st.city.buildings.items.end(), [&catalog](const auto& kv) {
                   return kv.second.house && kv.second.house->population > 0 &&
                          kv.second.house->happiness < catalog.risks.crime_happiness_below;
               });
    }
    const auto colon = req.find(':');
    const std::string count = colon == std::string::npos ? std::string() : req.substr(colon + 1);
    if (!count.empty() && std::all_of(count.begin(), count.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) {
        const std::string good = req.substr(0, colon);
        const auto amount = std::stoll(count);  // B-024: only digits reach stoll
        if (good == "drachmas") {
            return st.finance.treasury >= amount;
        }
        if (good == "horses") {
            return horses_in_city(st) >= amount;
        }
        if (catalog.goods.count(good) != 0 || good == "food") {
            return city_stock(st.city, catalog, good) >= amount;  // in city storage [ZAŁOŻENIE, L-116]
        }
    }
    return false;  // an unknown requirement is never met (test_heroes.cpp meets every one in heroes.json)
}
}  // namespace

std::vector<std::string> unmet_requirements(const WorldState& st, const DataCatalog& catalog, const std::string& hero,
                                            BuildingId hall) {
    std::vector<std::string> out;
    const auto it = catalog.hero_requirements.find(hero);
    if (it == catalog.hero_requirements.end()) {
        return {"unknown_hero"};
    }
    for (const std::string& r : it->second) {
        if (!requirement_met(st, catalog, r, hall)) {
            out.push_back(r);
        }
    }
    return out;
}

}  // namespace bo::core
