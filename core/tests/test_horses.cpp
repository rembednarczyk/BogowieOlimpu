// Horse ranch and chariot factory (docs/gameplay/05, 06, 19; Z:147-148, P:18-19; assumptions L-106).
#include "bo/core/logistics.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;
using bo::test::at;

namespace {

// Open land, main street on row 6.
Simulation town() {
    std::vector<std::string> rows(14, std::string(40, '.'));
    rows[6] = std::string(40, '=');
    return Simulation(bo::test::catalog_isolated(), SimulationConfig{6, Difficulty::Hero, 20000, rows, true});
}


}  // namespace

TEST_CASE("horse ranch: 4x4 with a 3x3 enclosure beside it by rotation (S-004 size 4x4 + 3x3, L-106)") {
    auto sim = town();
    sim.issue(PlaceBuilding{"horse_ranch", 14, 2, 0});
    sim.step(1);
    const auto id = sim.state().city.map.at({14, 2}).building;
    REQUIRE(id != kNoBuilding);
    CHECK(sim.state().city.map.at({20, 4}).building == id);  // enclosure east of the ranch
    CHECK(sim.state().city.map.at({20, 5}).building == kNoBuilding);
    CHECK(sim.state().city.buildings.items.at(id).tiles.size() == 25);
    sim.issue(PlaceBuilding{"horse_ranch", 30, 2, 1});  // enclosure south would cover the main street
    sim.step(1);
    CHECK(sim.state().city.map.at({30, 2}).building == kNoBuilding);
}

TEST_CASE("horse ranch: 50 wheat a horse, 6 a year at full staffing, a waiting horse eats 10 a month (doc 19)") {
    auto sim = town();
    sim.issue(PlaceBuilding{"horse_ranch", 14, 2, 0});
    sim.step(1);
    Building& ranch = at(sim, {14, 2});
    REQUIRE(ranch.producer);
    ranch.producer->units["wheat"] = 50;
    sim.step(kTicksPerYear);
    CHECK(at(sim, {14, 2}).producer->output_loads == 1);
    CHECK(at(sim, {14, 2}).producer->units["wheat"] == 0);
    at(sim, {14, 2}).producer->units["wheat"] = 100;
    sim.step(kTicksPerMonth - kTicksPerDay);  // the daily systems of the 1st have not run yet: one month start
    // One month of upkeep for the waiting horse; not yet enough time for a second horse.
    CHECK(at(sim, {14, 2}).producer->units["wheat"] == 90);
    CHECK(at(sim, {14, 2}).producer->output_loads == 1);
}

TEST_CASE("horse ranch output stays at the ranch; at most 6 horses wait (doc 19)") {
    auto sim = town();
    sim.issue(PlaceBuilding{"horse_ranch", 14, 2, 0});
    sim.issue(PlaceBuilding{"storehouse", 24, 3});
    sim.step(1);
    at(sim, {14, 2}).producer->units["wheat"] = 400;
    for (int m = 0; m < 24; ++m) {
        at(sim, {14, 2}).producer->units["wheat"] = 400;
        sim.step(kTicksPerMonth);
    }
    CHECK(at(sim, {14, 2}).producer->output_loads == 6);
    CHECK(at(sim, {24, 3}).storage->loads.count("horses") == 0);
}

TEST_CASE("a storehouse supplies wheat to the ranch, a granary never does (Z:148)") {
    auto sim = town();
    sim.issue(PlaceBuilding{"horse_ranch", 14, 2, 0});
    sim.issue(PlaceBuilding{"granary", 24, 2});
    sim.step(1);
    at(sim, {24, 2}).storage->loads["wheat"] = 8;
    sim.step(kTicksPerMonth);
    CHECK(at(sim, {14, 2}).producer->units["wheat"] == 0);
    sim.issue(PlaceBuilding{"storehouse", 4, 3});
    sim.step(1);
    at(sim, {4, 3}).storage->loads["wheat"] = 8;
    sim.step(kTicksPerMonth);
    CHECK(at(sim, {14, 2}).producer->units["wheat"] > 0);
}

TEST_CASE("the hostler brings horses to the chariot factory; wood and a horse make a chariot (P:18)") {
    auto sim = town();
    sim.mutable_state().city.atlantean = true;  // the chariot factory is Atlantean (P:16-17)
    sim.issue(PlaceBuilding{"horse_ranch", 14, 2, 0});
    sim.issue(PlaceBuilding{"chariot_factory", 26, 2});
    sim.step(1);
    at(sim, {14, 2}).producer->output_loads = 2;
    at(sim, {26, 2}).producer->units["wood"] = 3;
    sim.step(kTicksPerMonth);
    CHECK(at(sim, {14, 2}).producer->output_loads < 2);
    CHECK(at(sim, {26, 2}).producer->units["horses"] > 0);
    sim.step(kTicksPerYear / 2);
    CHECK(at(sim, {26, 2}).producer->output_loads >= 1);  // the chariot waits for its vendor
}

TEST_CASE("the horse trainer takes horses straight from the ranch; one load is one horse") {
    auto sim = town();
    sim.issue(PlaceBuilding{"horse_ranch", 24, 2, 0});
    sim.issue(PlaceBuilding{"common_agora", 10, 4, 1});
    sim.step(1);
    const Building& agora = at(sim, {10, 4});
    REQUIRE(agora.slot_origins.size() == 3);
    sim.issue(PlaceBuilding{"horse_trainer_stall", agora.slot_origins[0].x, agora.slot_origins[0].y});
    sim.step(1);
    at(sim, {24, 2}).producer->output_loads = 3;
    sim.step(kTicksPerMonth);
    const Building& stall = sim.state().city.buildings.items.at(at(sim, {10, 4}).slots[0]);
    REQUIRE(stall.stall);
    CHECK(stall.stall->units.at("horses") == 3);
    CHECK(at(sim, {24, 2}).producer->output_loads == 0);
}
