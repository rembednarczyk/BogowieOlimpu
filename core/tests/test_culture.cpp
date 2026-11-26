// Culture (B-008, B-009, B-010): two-phase performers and one-per-city buildings.
// Rules: docs/gameplay/02-spacerujacy.md, 09-kultura.md (Z:26, Z:95, Z:105-109); numbers doc 18 and L-120.
#include "bo/core/culture.h"
#include "bo/core/housing.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// One straight road along y = 5; buildings north of it touch it.
Simulation road_sim() {
    std::vector<std::string> rows(16, std::string(60, '.'));
    for (int x = 0; x < 60; ++x) {
        rows[5][static_cast<std::size_t>(x)] = '=';
    }
    SimulationConfig cfg{7, Difficulty::Hero, 100000, rows, true};  // full staffing: no houses needed
    Simulation sim(bo::test::catalog(), cfg);
    REQUIRE(sim.setup_error().empty());
    return sim;
}

const Building& at(const Simulation& sim, TilePos p) {
    return sim.state().city.buildings.items.at(sim.state().city.map.at(p).building);
}

std::uint8_t coverage(const Building& b, const std::string& walker) {
    const auto it = b.coverage.find(walker);
    return it == b.coverage.end() ? 0 : it->second;
}

}  // namespace

TEST_CASE("B-008: a College sends a philosopher to the Podium, then philosophers roam from the Podium") {
    auto sim = road_sim();
    sim.issue(PlaceBuilding{"college", 2, 2});
    sim.issue(PlaceBuilding{"podium", 30, 3});
    sim.issue(PlaceBuilding{"fountain", 34, 3});  // a neighbour of the Podium to receive the visits
    sim.step(1);
    sim.run_days(60);
    CHECK(at(sim, {30, 3}).performing_days > 0);
    CHECK(coverage(at(sim, {34, 3}), "philosopher") > 0);
}

TEST_CASE("B-008: a Podium without a College never performs") {
    auto sim = road_sim();
    sim.issue(PlaceBuilding{"podium", 30, 3});
    sim.issue(PlaceBuilding{"fountain", 34, 3});
    sim.step(1);
    sim.run_days(120);
    CHECK(at(sim, {30, 3}).performing_days == 0);
    CHECK(coverage(at(sim, {34, 3}), "philosopher") == 0);
}

TEST_CASE("B-010: a Gymnasium trains competitors only when there is a Stadium (Z:108)") {
    auto sim = road_sim();
    sim.issue(PlaceBuilding{"gymnasium", 2, 2});
    sim.step(1);
    sim.run_days(60);
    for (const auto& [id, w] : sim.state().walkers.items) {
        CHECK(w.type != "competitor");
    }
    CHECK(at(sim, {2, 2}).schools.at("competitor").second == kNoBuilding);
}

TEST_CASE("B-008: a working Stadium gives every house 10 culture points (doc 18)") {
    auto sim = road_sim();
    sim.issue(PlaceBuilding{"gymnasium", 2, 2});
    sim.issue(PlaceBuilding{"stadium", 30, 0});
    sim.step(1);
    REQUIRE(sim.state().city.map.at({30, 0}).building != kNoBuilding);
    CHECK(city_culture_bonus(sim.state().city, sim.catalog()) == 0);
    sim.run_days(60);
    CHECK(at(sim, {30, 0}).performing_days > 0);
    CHECK(city_culture_bonus(sim.state().city, sim.catalog()) == 10);
}

TEST_CASE("B-009: one Palace and one Stadium per city (Z:95, Z:109)") {
    auto sim = road_sim();
    sim.issue(PlaceBuilding{"palace", 2, 7});
    sim.issue(PlaceBuilding{"palace", 20, 7});
    sim.step(1);
    std::size_t palaces = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        palaces += b.type == "palace" ? 1 : 0;
    }
    CHECK(palaces == 1);
    bool refused = false;
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
            refused = refused || r->reason_key == "build.only_one";
        }
    }
    CHECK(refused);
}
