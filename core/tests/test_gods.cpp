// Module D1 stage 2: blessings, sanctification, prayers (docs/gameplay/10, 21; Z:119-124; L-053, L-112).
#include "bo/core/gods.h"
#include "bo/core/migration.h"
#include "bo/core/sanctuary.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// 60x40: a main street on row 20, a meadow at x 30-40, y 22-27; a finished sanctuary of `god` at (2, 14).
Simulation with_god(const std::string& god) {
    std::vector<std::string> rows(40, std::string(60, '.'));
    rows[20] = std::string(60, '=');
    for (std::size_t y = 22; y < 28; ++y) {
        for (std::size_t x = 30; x < 40; ++x) {
            rows[y][x] = ',';
        }
    }
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{11, Difficulty::Hero, 30000, rows, true});
    if (god.empty()) {
        return sim;
    }
    sim.issue(PlaceBuilding{"storehouse", 50, 17});
    sim.step(1);
    auto& store = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({50, 17}).building);
    store.storage->loads["marble"] = 60;
    sim.issue(PlaceBuilding{"sanctuary_" + god, 2, 20 - bo::test::catalog().building_stats.at("sanctuary_" + god).footprint->height});
    sim.step(1);
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        if (b.sanctuary) {
            for (auto& [g, n] : b.sanctuary->needed) {
                n = 0;
            }
        }
    }
    sim.run_days(1);
    return sim;
}

BuildingId sanctuary_id(const Simulation& sim) {
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        if (b.sanctuary) {
            return id;
        }
    }
    return kNoBuilding;
}

std::string result(Simulation& sim) {
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
            return r->reason_key;
        }
        if (std::holds_alternative<PrayerAnswered>(e.event)) {
            return "answered";
        }
    }
    return {};
}

}  // namespace

TEST_CASE("Artemis answers a prayer with 24 loads of pork, then not again too soon (S-004 trivia, Z:124)") {
    auto sim = with_god("artemis");
    const BuildingId s = sanctuary_id(sim);
    REQUIRE(sim.state().city.buildings.items.at(s).sanctuary->complete);
    sim.issue(PlaceBuilding{"granary", 44, 16});
    sim.step(1);
    sim.drain_events();
    sim.issue(Pray{s});
    sim.step(1);
    CHECK(result(sim) == "answered");
    CHECK(sim.state().city.buildings.items.at(sim.state().city.map.at({44, 16}).building).storage->loads.at("pork") == 24);
    sim.issue(Pray{s});
    sim.step(1);
    CHECK(result(sim) == "myth.prayer_too_soon");
    sim.run_days(97);
    sim.mutable_state().city.buildings.items.at(s).sanctuary->days_since_sacrifice = 0;
    sim.drain_events();
    sim.issue(Pray{s});
    sim.step(1);
    CHECK(result(sim) == "answered");
}

TEST_CASE("prayers: Hades gives money, a god who turned away does not listen, Ares needs the world (L-112)") {
    auto hades = with_god("hades");
    const std::int64_t before = hades.state().finance.treasury;
    hades.issue(Pray{sanctuary_id(hades)});
    hades.step(1);
    CHECK(result(hades) == "answered");
    CHECK(hades.state().finance.treasury >= before + 1000 - 200);  // minus a day of wages at most

    auto ares = with_god("ares");
    ares.issue(Pray{sanctuary_id(ares)});
    ares.step(1);
    CHECK(result(ares) == "myth.prayer_not_available");

    auto angry = with_god("dionysus");
    angry.mutable_state().city.buildings.items.at(sanctuary_id(angry)).sanctuary->favor_lost = true;
    angry.issue(Pray{sanctuary_id(angry)});
    angry.step(1);
    CHECK(result(angry) == "myth.god_turned_away");
}

TEST_CASE("Demeter sanctifies a farm; the next production gives 4x (doc 21)") {
    auto sim = with_god("demeter");
    sim.issue(PlaceBuilding{"carrot_farm", 32, 22});
    sim.step(1);
    Building& farm = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({32, 22}).building);
    REQUIRE(farm.producer);
    sim.run_days(17);
    CHECK(farm.sanctified);
    farm.producer->output_loads += 2;  // a harvest of 2 loads
    apply_sanctification(sim.mutable_state().city);
    CHECK(farm.producer->output_loads == farm.sanctified_base);
    CHECK(farm.producer->output_loads == 8);
    CHECK_FALSE(farm.sanctified);
}

TEST_CASE("Hephaestus prevents fires, Dionysus unrest (doc 10, Z:121-122)") {
    auto blessed = with_god("hephaestus");
    auto plain = with_god("");
    for (auto* sim : {&blessed, &plain}) {
        sim->issue(PlaceBuilding{"common_housing", 30, 18});
        sim->step(1);
        sim->run_days(40);
    }
    const auto fire = [](const Simulation& sim) {
        return sim.state().city.buildings.items.at(sim.state().city.map.at({30, 18}).building).fire_counter;
    };
    CHECK(fire(blessed) == 0);
    CHECK(fire(plain) > 0);

    auto calm = with_god("dionysus");
    WorldState& st = calm.mutable_state();
    st.city.buildings.items.at(sanctuary_id(calm)).sanctuary->days_since_sacrifice = 0;
    calm.issue(PlaceBuilding{"common_housing", 30, 18});
    calm.step(1);
    auto& house = calm.mutable_state().city.buildings.items.at(calm.state().city.map.at({30, 18}).building);
    house.house->population = 8;
    calm.run_days(20);
    CHECK(house.crime_counter == 0);
}

TEST_CASE("Aphrodite: nobody emigrates (doc 10, Z:122)") {
    auto love = with_god("aphrodite");
    auto plain = with_god("");
    for (auto* sim : {&love, &plain}) {
        for (int x = 2; x < 60; x += 2) {
            sim->issue(PlaceBuilding{"common_housing", x, 21});
        }
        sim->step(1);
        WorldState& st = sim->mutable_state();
        for (auto& [id, b] : st.city.buildings.items) {
            if (b.house) {
                b.house->population = 8;
            }
        }
        st.migration.popularity = 0;
        st.migration.pool = -40;
    }
    CHECK(migrate(love.mutable_state(), bo::test::catalog_isolated()).left == 0);
    CHECK(migrate(plain.mutable_state(), bo::test::catalog_isolated()).left > 0);
}

TEST_CASE("B-011: the sanctuary goal counts a finished sanctuary that stands, without the god's favour") {
    auto sim = with_god("dionysus");
    sim.mutable_state().scenario.goals.push_back(ScenarioGoal{"sanctuary", "dionysus", 0, false});
    sim.mutable_state().scenario.goals.push_back(ScenarioGoal{"population", "", 1000000, false});  // never won
    sim.mutable_state().city.buildings.items.at(sanctuary_id(sim)).sanctuary->days_since_sacrifice = 10000;
    sim.run_days(17);
    REQUIRE(sim.state().city.buildings.items.at(sanctuary_id(sim)).sanctuary->favor_lost);  // no sacrifices
    CHECK(sim.state().scenario.goals[0].met);
    const TilePos origin = sim.state().city.buildings.items.at(sanctuary_id(sim)).origin;
    sim.issue(Demolish{origin.x, origin.y});
    sim.step(1);
    sim.run_days(17);
    CHECK_FALSE(sim.state().scenario.goals[0].met);  // destroyed: the goal is lost again (eZeus, S-008)
}
