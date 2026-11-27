// The demands of each hero (Z:130, P:24-25; data/heroes.json; docs/gameplay/21-mitologia-liczby.md; L-116):
// a city that meets them all has none unmet, and each demand that depends on the city's state shows up when
// that state is missing.
#include <algorithm>
#include <sstream>

#include "bo/core/military.h"
#include "bo/core/monsters.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// 120x60: a main street on row 30; the Hero's Hall at (30,26) by the Palace at (36,24); a storehouse and a
// granary; a house; sanctuaries of Athena, Hermes and Artemis below the street; a Stadium.
struct HeroCity {
    Simulation sim;
    BuildingId hall = kNoBuilding;
    BuildingId store = kNoBuilding;
    BuildingId granary = kNoBuilding;
    BuildingId house = kNoBuilding;
    BuildingId stadium = kNoBuilding;
};

BuildingId at(const Simulation& sim, std::int32_t x, std::int32_t y) { return sim.state().city.map.at({x, y}).building; }

// A monster in the city now (one without a walker is gone after the next day of the simulation).
void with_monster(Simulation& sim, const std::string& id, TilePos home = {0, 10}) {
    sim.mutable_state().monsters.push_back(MonsterState{id, kNoWalker, home, "food", kNoBuilding, 0, false, false});
}

HeroCity hero_city() {
    std::vector<std::string> rows(60, std::string(120, '.'));
    rows[30] = std::string(120, '=');
    rows[10][0] = 'M';
    HeroCity c{Simulation(bo::test::catalog(), SimulationConfig{14, Difficulty::Hero, 30000, rows, true})};
    Simulation& sim = c.sim;
    with_monster(sim, "medusa");
    sim.issue(PlaceBuilding{"storehouse", 50, 27});
    sim.issue(PlaceBuilding{"granary", 54, 26});
    sim.issue(PlaceBuilding{"common_housing", 96, 28});
    sim.issue(PlaceBuilding{"heros_hall", 30, 26});
    sim.issue(PlaceBuilding{"palace", 36, 24});
    sim.issue(PlaceBuilding{"stadium", 100, 31});
    sim.step(1);
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
            FAIL_CHECK("setup refused: " << r->reason_key);
        }
    }
    c.store = at(sim, 50, 27);
    auto& loads = sim.mutable_state().city.buildings.items.at(c.store).storage->loads;
    for (const char* g : {"marble", "wine", "olive_oil", "fleece", "armor", "horses", "wheat"}) {
        loads[g] = 200;  // elite houses are built only with their goods in store (build.elite_needs_goods)
    }
    for (int x = 60; x < 92; x += 4) {
        sim.issue(PlaceBuilding{"elite_housing", x, 26});
    }
    sim.issue(PlaceBuilding{"sanctuary_athena", 2, 31});
    sim.issue(PlaceBuilding{"sanctuary_hermes", 20, 31});
    sim.issue(PlaceBuilding{"sanctuary_artemis", 34, 31});
    sim.step(1);
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        if (b.sanctuary) {
            b.sanctuary->complete = true;
            b.sanctuary->needed.clear();
        }
    }
    sim.run_days(1);  // appeal and road access
    c.hall = at(sim, 30, 26);
    c.granary = at(sim, 54, 26);
    c.house = at(sim, 96, 28);
    for (int x = 60; x < 92; x += 4) {
        REQUIRE(at(sim, x, 26) != kNoBuilding);
    }
    c.stadium = at(sim, 100, 31);
    REQUIRE(c.hall != kNoBuilding);
    REQUIRE(c.granary != kNoBuilding);
    REQUIRE(c.house != kNoBuilding);
    REQUIRE(c.stadium != kNoBuilding);
    return c;
}

Building& b_of(HeroCity& c, BuildingId id) { return c.sim.mutable_state().city.buildings.items.at(id); }

void companies(HeroCity& c, const char* unit, int n) {
    for (int i = 0; i < n; ++i) {
        Company co;
        co.unit = unit;
        c.sim.mutable_state().military.companies.push_back(co);
    }
}

// Everything every hero asks for, set straight into the state.
void meet_all(HeroCity& c) {
    WorldState& st = c.sim.mutable_state();
    auto& loads = b_of(c, c.store).storage->loads;
    loads.clear();
    for (const char* g : {"wine", "armor", "olive_oil", "marble", "fleece", "meat", "wood", "bronze"}) {
        loads[g] = 32;
    }
    loads["sculpture"] = 6;
    b_of(c, c.granary).storage->loads["wheat"] = 64;
    Building& house = b_of(c, c.house);
    for (auto& [id, b] : st.city.buildings.items) {
        if (b.house && b.house->elite) {
            b.house->population = 1;
            b.house->happiness = 100;
        }
    }
    house.house->population = 1500 - 8;
    house.house->happiness = 100;
    house.house->goods[static_cast<std::size_t>(HouseGood::Horses)] = 15;
    house.coverage["athlete"] = 10;
    house.coverage["clerk"] = 10;
    for (const char* w : {"philosopher", "actor", "athlete", "competitor"}) {
        b_of(c, c.hall).coverage[w] = 10;
    }
    st.scenario.monuments["athlete"] = 1;  // the city has won a game
    st.migration.popularity = 90;
    st.city.hygiene = 90;
    for (int i = 0; i < 3; ++i) {
        st.military.ships.push_back(Ship{});
    }
    companies(c, "hoplite", 3);
    companies(c, "horseman", 2);
    while (st.military.companies.size() < 8) {  // with Artemis's two companies of Amazons
        companies(c, "rabble", 1);
    }
    st.finance.treasury = 10000;
    Building& stadium = b_of(c, c.stadium);
    stadium.staffing_percent = 100;
    stadium.performing_days = 100;
}

std::string joined(const std::vector<std::string>& v) {
    std::ostringstream s;
    for (const auto& x : v) {
        s << x << "; ";
    }
    return s.str();
}

std::vector<std::string> unmet(HeroCity& c, const char* hero) {
    return unmet_requirements(c.sim.state(), c.sim.catalog(), hero, c.hall);
}

bool lacks(HeroCity& c, const char* hero, const std::string& prefix) {
    const auto u = unmet(c, hero);
    return std::any_of(u.begin(), u.end(), [&prefix](const std::string& r) { return r.rfind(prefix, 0) == 0; });
}

}  // namespace

TEST_CASE("heroes: a city that has all they ask for leaves no demand unmet, walls aside (Z:130, P:24-25)") {
    auto c = hero_city();
    meet_all(c);
    for (const char* hero : {"hercules", "achilles", "odysseus", "jason", "perseus", "atalanta", "bellerophon"}) {
        INFO(hero << ": " << joined(unmet(c, hero)));
        CHECK(unmet(c, hero).empty());
    }
    // Theseus also wants walls round the Hall (the next test); all else is met.
    CHECK(unmet(c, "theseus") == std::vector<std::string>{"hall_protected_by_walls"});
}

TEST_CASE("heroes: each demand on the city's state is unmet while that state is missing (Z:130, L-116)") {
    auto c = hero_city();
    meet_all(c);
    WorldState& st = c.sim.mutable_state();
    Building& house = b_of(c, c.house);

    house.coverage.erase("athlete");
    CHECK(lacks(c, "hercules", "citywide_gymnasium_access"));
    house.coverage.erase("clerk");
    CHECK(lacks(c, "bellerophon", "citywide_tax_coverage"));
    b_of(c, c.hall).coverage.erase("actor");  // three kinds of performers of four
    CHECK(lacks(c, "hercules", "hall_culture_access"));
    st.scenario.monuments.erase("athlete");
    CHECK(lacks(c, "hercules", "win_any_panhellenic_game"));
    house.house->population -= 1;
    CHECK(lacks(c, "hercules", "population>=1500"));

    st.military.companies.resize(7);  // 7 companies of 8
    CHECK(lacks(c, "atalanta", "military_companies>=8"));
    b_of(c, c.stadium).performing_days = 0;  // no performance: the Stadium does not work (L-120)
    CHECK(lacks(c, "atalanta", "working_stadium"));
    st.military.ships.pop_back();
    CHECK(lacks(c, "jason", "triremes>=3"));
    house.house->goods[static_cast<std::size_t>(HouseGood::Horses)] = 14;
    CHECK(lacks(c, "bellerophon", "horses_in_city>=15"));
    st.finance.treasury = 9999;
    CHECK(lacks(c, "bellerophon", "treasury>=10000"));
    st.migration.popularity = 70;
    CHECK(lacks(c, "odysseus", "popularity:excellent"));
    st.city.hygiene = 70;
    CHECK(lacks(c, "odysseus", "city_health:excellent"));
    for (auto& [id, b] : st.city.buildings.items) {
        if (b.house && b.house->elite) {
            b.house->population = 0;  // empty: 7 lived-in elite houses of 8
            break;
        }
    }
    CHECK(lacks(c, "odysseus", "elite_houses>=8"));
}

TEST_CASE("heroes: Theseus wants his Hall near the Palace, in good appeal (Z:130, L-116)") {
    auto c = hero_city();
    meet_all(c);
    CHECK_FALSE(lacks(c, "theseus", "hall_near_palace"));
    CHECK_FALSE(lacks(c, "theseus", "hall_appeal"));
    // A second Hall far from the Palace, among granaries and storehouses (bad appeal, Z:47).
    with_monster(c.sim, "minotaur");
    c.sim.issue(PlaceBuilding{"granary", 106, 22});
    c.sim.issue(PlaceBuilding{"granary", 110, 22});
    c.sim.issue(PlaceBuilding{"heros_hall", 108, 26});
    c.sim.run_days(1);
    const BuildingId far = at(c.sim, 108, 26);
    REQUIRE(far != kNoBuilding);
    const auto u = unmet_requirements(c.sim.state(), c.sim.catalog(), "theseus", far);
    CHECK(std::count(u.begin(), u.end(), "hall_near_palace") == 1);
    CHECK(std::count(u.begin(), u.end(), "hall_appeal:good") == 1);
}

TEST_CASE("heroes: a Hall ringed by walls is protected; one gap and it is not (Z:130, Z:145, L-116)") {
    std::vector<std::string> rows(30, std::string(40, '.'));
    rows[5][0] = 'i';  // armies come in here
    for (int x = 16; x <= 23; ++x) {
        rows[16][x] = '=';
    }
    Simulation sim(bo::test::catalog(), SimulationConfig{14, Difficulty::Hero, 30000, rows, true});
    with_monster(sim, "minotaur", {0, 5});
    sim.issue(PlaceBuilding{"heros_hall", 18, 12});
    sim.step(1);
    const BuildingId hall = at(sim, 18, 12);
    REQUIRE(hall != kNoBuilding);
    const auto open = [&sim, hall] {
        const auto u = unmet_requirements(sim.state(), sim.catalog(), "theseus", hall);
        return std::count(u.begin(), u.end(), "hall_protected_by_walls") == 1;
    };
    CHECK(open());
    for (int x = 15; x <= 24; ++x) {
        sim.issue(PlaceBuilding{"wall", x, 10});
        sim.issue(PlaceBuilding{"wall", x, 18});
    }
    for (int y = 11; y <= 17; ++y) {
        sim.issue(PlaceBuilding{"wall", 15, y});
        if (y != 14) {
            sim.issue(PlaceBuilding{"wall", 24, y});
        }
    }
    sim.step(1);
    CHECK(open());  // a gap at (24,14)
    sim.issue(PlaceBuilding{"wall", 24, 14});
    sim.step(1);
    REQUIRE(at(sim, 24, 14) != kNoBuilding);
    CHECK_FALSE(open());
}

TEST_CASE("heroes: for each of the 16 monsters its own hero answers the summons (Z:130, P:23-24)") {
    for (const auto& [monster, slayer] : bo::test::catalog().monster_slayer) {
        INFO(monster << " -> " << slayer);
        auto c = hero_city();
        meet_all(c);
        c.sim.mutable_state().monsters.clear();
        with_monster(c.sim, monster);
        c.sim.drain_events();
        c.sim.issue(SummonHero{c.hall});
        c.sim.step(1);
        std::string refused;
        for (const auto& e : c.sim.drain_events()) {
            if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
                refused = r->reason_key;
            }
        }
        if (slayer == "theseus") {  // his Hall has no walls round it here
            CHECK(refused == "myth.hero_requirements");
            CHECK(c.sim.state().heroes.empty());
            continue;
        }
        INFO(refused << " " << joined(unmet(c, slayer.c_str())));
        CHECK(refused.empty());
        REQUIRE(c.sim.state().heroes.size() == 1);
        CHECK(c.sim.state().heroes.front().id == slayer);
    }
}
