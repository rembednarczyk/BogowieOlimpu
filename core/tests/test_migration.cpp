// Module C8: moods, popularity, migration, homeless (doc 24, data/migration.json).
#include "bo/core/migration.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

std::vector<std::string> street(std::size_t width) {
    std::vector<std::string> r(8, std::string(width, '.'));
    r[3] = std::string(width, '=');
    return r;
}

}  // namespace

TEST_CASE("migration table (doc 24)") {
    const auto& p = bo::test::catalog().migration;
    CHECK(migration_percent(p, 71) == 100);
    CHECK(migration_percent(p, 70) == 75);
    CHECK(migration_percent(p, 50) == 50);
    CHECK(migration_percent(p, 49) == 0);
    CHECK(migration_percent(p, 41) == 0);
    CHECK(migration_percent(p, 40) == -10);
    CHECK(migration_percent(p, 21) == -25);
    CHECK(migration_percent(p, 20) == -50);
}

TEST_CASE("migration: a new town on Hero starts at popularity 60 and gains 6 people a day, 12 x 50% (doc 24, Z:159-160)") {
    Simulation sim(bo::test::catalog(), SimulationConfig{1, Difficulty::Hero, 1000, street(20)});
    for (int x = 0; x < 20; x += 2) {
        sim.issue(PlaceBuilding{"common_housing", x, 1});
    }
    sim.step(1);
    CHECK(sim.state().migration.popularity == 60);
    sim.run_days(1);
    CHECK(city_population(sim.state().city) == 6);
    sim.run_days(9);
    CHECK(city_population(sim.state().city) == 60);
}

TEST_CASE("migration: immigrants fill houses with at least 8 free places first, then the rest (doc 24)") {
    Simulation sim(bo::test::catalog(), SimulationConfig{1, Difficulty::Hero, 1000, street(10)});
    sim.issue(PlaceBuilding{"common_housing", 0, 1});
    sim.issue(PlaceBuilding{"common_housing", 2, 1});
    sim.step(1);
    auto& items = sim.mutable_state().city.buildings.items;
    items.at(1).house->population = 3;  // 5 free
    sim.run_days(1);
    CHECK(items.at(2).house->population == 6);  // empty plot (8 free) first
    CHECK(items.at(1).house->population == 3);
}

TEST_CASE("migration: with no room the batch does not wait (doc 24)") {
    Simulation sim(bo::test::catalog(), SimulationConfig{1, Difficulty::Hero, 1000, street(10)});
    sim.run_days(3);
    CHECK(sim.state().migration.pool == 0);
    CHECK(city_population(sim.state().city) == 0);
}

TEST_CASE("moods: high taxes and unemployment lower happiness in a city of 300+; popularity is the mean (doc 24, S-004, S-006, Z:159)") {
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{1, Difficulty::Hero, 1000, street(30)});
    for (int x = 0; x < 30; x += 2) {
        sim.issue(PlaceBuilding{"common_housing", x, 1});
    }
    sim.issue(SetTaxRate{TaxRate::Outrageous});
    sim.step(1);
    WorldState& st = sim.mutable_state();
    for (auto& [id, b] : st.city.buildings.items) {
        b.house->population = 24;  // 15 x 24 = 360 people, no jobs: unemployment 100%
        b.house->level = 2;
        b.house->happiness = 60;
    }
    st.labour.workforce = 151;  // 42% of 360, nobody employed
    st.labour.employed = 0;
    update_moods(st, bo::test::catalog());
    // tax -5 (Hero, outrageous), wages 0, unemployment -3, food: hovel eats, none in stock -1.
    CHECK(st.city.buildings.items.at(1).house->happiness == 60 - 5 - 3 - 1);
    CHECK(st.migration.popularity == 51);
    update_moods(st, bo::test::catalog());
    CHECK(st.city.buildings.items.at(1).house->happiness == 51 - 5 - 3 - 2);  // hunger grows to -3
}

TEST_CASE("moods: a city below 300 people does not complain, base happiness (doc 24)") {
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{1, Difficulty::Titan, 1000, street(10)});
    sim.issue(PlaceBuilding{"common_housing", 0, 1});
    sim.issue(SetTaxRate{TaxRate::Outrageous});
    sim.step(1);
    WorldState& st = sim.mutable_state();
    st.city.buildings.items.at(1).house->population = 8;
    update_moods(st, bo::test::catalog());
    CHECK(st.migration.popularity == 50);  // Titan base
}

TEST_CASE("migration: emigration takes people from the lowest levels, only above 100 residents (doc 24, Z:159-160)") {
    Simulation sim(bo::test::catalog(), SimulationConfig{1, Difficulty::Hero, 1000, street(20)});
    for (int x = 0; x < 10; x += 2) {
        sim.issue(PlaceBuilding{"common_housing", x, 1});
    }
    sim.step(1);
    WorldState& st = sim.mutable_state();
    int i = 0;
    for (auto& [id, b] : st.city.buildings.items) {
        b.house->level = static_cast<std::uint8_t>(i < 2 ? 2 : 6);
        b.house->population = i < 2 ? 24 : 40;
        ++i;
    }
    st.migration.popularity = 10;  // -50%: 6 leave per day
    const auto r = migrate(st, bo::test::catalog());
    CHECK(r.left == 6);
    CHECK(st.city.buildings.items.at(1).house->population == 18);
}

TEST_CASE("homeless: a house cut off from roads empties after more than 4 days; the people are rehoused first (doc 24)") {
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{1, Difficulty::Hero, 1000, street(10)});
    sim.issue(PlaceBuilding{"common_housing", 0, 1});
    sim.issue(PlaceBuilding{"common_housing", 4, 1});
    sim.step(1);
    WorldState& st = sim.mutable_state();
    st.city.buildings.items.at(1).house->population = 8;
    st.city.buildings.items.at(1).road_access.reset();  // as if its road were gone
    for (int d = 0; d < 4; ++d) {
        st.city.buildings.items.at(1).road_access.reset();
        migrate(st, bo::test::catalog_isolated());
    }
    CHECK(st.city.buildings.items.at(1).house->population == 8);
    st.city.buildings.items.at(1).road_access.reset();
    const auto r = migrate(st, bo::test::catalog_isolated());
    CHECK(st.city.buildings.items.at(1).house->population == 0);
    CHECK(r.rehoused == 8);
    CHECK(st.city.buildings.items.at(2).house->population == 8);
}

namespace {

std::vector<std::string> entry_street() {
    std::vector<std::string> r(10, std::string(30, '.'));
    r[3] = "E" + std::string(29, '=');
    r[8] = std::string(30, '=');  // a second street not connected to the entry road
    return r;
}

}  // namespace

TEST_CASE("immigrants walk from the land entry and move in on arrival (doc 24, O-006 closed for maps with entry)") {
    Simulation sim(bo::test::catalog(), SimulationConfig{1, Difficulty::Hero, 1000, entry_street()});
    sim.issue(PlaceBuilding{"common_housing", 20, 1});
    sim.step(1);
    const auto& h = *sim.state().city.buildings.items.begin()->second.house;
    sim.run_days(1);
    CHECK(h.population == 0);
    CHECK(h.incoming == 6);  // reserved for the group on its way
    bool walking = false;
    for (const auto& [id, w] : sim.state().walkers.items) {
        walking = walking || (w.type == "immigrant" && w.job == WalkerJob::Settle);
    }
    CHECK(walking);
    sim.run_days(8);  // 20 tiles at 1 tile per 15 ticks
    CHECK(h.population >= 6);
}

TEST_CASE("a house not reachable from the entry gets no immigrants (doc 24)") {
    Simulation sim(bo::test::catalog(), SimulationConfig{1, Difficulty::Hero, 1000, entry_street()});
    sim.issue(PlaceBuilding{"common_housing", 20, 6});  // on the unconnected street
    sim.step(1);
    sim.run_days(10);
    CHECK(sim.state().city.buildings.items.begin()->second.house->population == 0);
}

TEST_CASE("workers come only to buildings reachable on foot from the entry (S-005)") {
    auto rows = entry_street();
    rows[5] = std::string(30, '~');  // a river: the south street cannot be reached on foot
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{1, Difficulty::Hero, 1000, rows});
    sim.issue(PlaceBuilding{"common_housing", 2, 1});
    sim.issue(PlaceBuilding{"fountain", 6, 1});
    sim.issue(PlaceBuilding{"fountain", 6, 6});
    sim.step(1);
    sim.mutable_state().city.buildings.items.begin()->second.house->population = 40;
    sim.run_days(1);
    CHECK(sim.state().city.buildings.items.at(sim.state().city.map.at({6, 1}).building).workers > 0);
    CHECK(sim.state().city.buildings.items.at(sim.state().city.map.at({6, 6}).building).workers == 0);
}
