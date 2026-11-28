// Undo (Z:20): reverts the last tick's build commands, refunds, expires after kUndoWindowTicks (L-035).
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

Simulation make() {
    std::vector<std::string> rows(10, std::string(10, '.'));
    rows[2] = "TTTT......";
    Simulation sim(bo::test::catalog(), SimulationConfig{5, Difficulty::Hero, 1000, rows});
    return sim;
}

}  // namespace

TEST_CASE("undo reverts a whole drag (one tick) and refunds it") {
    auto sim = make();
    const auto hash0 = sim.state_hash();
    for (int x = 0; x < 6; ++x) {
        sim.issue(PlaceRoad{x, 5});
    }
    sim.step(1);
    CHECK(sim.state().finance.treasury == 1000 - 6 * 3);
    sim.issue(Undo{});
    sim.step(1);
    CHECK(sim.state().finance.treasury == 1000);
    CHECK_FALSE(sim.state().city.map.is_road({3, 5}));
    sim.issue(Undo{});  // nothing left
    sim.step(1);
    bool rejected = false;
    for (const auto& e : sim.drain_events()) {
        rejected = rejected || std::holds_alternative<CommandRejected>(e.event);
    }
    CHECK(rejected);
    (void)hash0;
}

TEST_CASE("undo restores a demolished building and cleared forest exactly") {
    auto sim = make();
    sim.issue(PlaceBuilding{"fountain", 5, 5});
    sim.step(1);
    const auto before = sim.state().city.buildings.items;
    sim.issue(Demolish{6, 6});
    sim.issue(ClearLand{0, 2});
    sim.step(1);
    CHECK(sim.state().city.buildings.items.empty());
    sim.issue(Undo{});
    sim.step(1);
    CHECK(sim.state().city.buildings.items == before);
    CHECK(sim.state().city.map.at({0, 2}).terrain == Terrain::Forest);
}

TEST_CASE("undo expires after the window") {
    auto sim = make();
    sim.issue(PlaceRoad{1, 1});
    sim.step(1);
    sim.step(kUndoWindowTicks);
    CHECK_FALSE(sim.state().city.undo.active);
    sim.issue(Undo{});
    sim.step(1);
    CHECK(sim.state().city.map.is_road({1, 1}));
}

TEST_CASE("undo record survives save and load") {
    auto sim = make();
    sim.issue(PlaceBuilding{"fountain", 5, 5});
    sim.step(1);
    std::string error;
    auto loaded = Simulation::load(bo::test::catalog(), sim.save(), error);
    REQUIRE(loaded);
    loaded->issue(Undo{});
    loaded->step(1);
    CHECK(loaded->state().city.buildings.items.empty());
    CHECK(loaded->state().finance.treasury == 1000);
}
