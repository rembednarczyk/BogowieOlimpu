// Atlantis stage 1 (docs/gameplay/15-atlantyda.md, P:10-13, P:16-17): the city's culture decides what may be
// built; science points work in houses as culture points.
#include "bo/core/hippodrome.h"
#include "bo/core/housing.h"
#include "bo/core/scenario.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

Simulation town(bool atlantean) {
    std::vector<std::string> rows(30, std::string(40, '.'));
    rows[6] = std::string(40, '=');
    SimulationConfig cfg{8, Difficulty::Hero, 30000, rows, true};
    cfg.atlantean = atlantean;
    return Simulation(bo::test::catalog_isolated(), cfg);
}

std::string place(Simulation& sim, const std::string& type, std::int32_t x, std::int32_t y) {
    sim.drain_events();
    sim.issue(PlaceBuilding{type, x, y});
    sim.step(1);
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
            return r->reason_key;
        }
    }
    return "ok";
}

}  // namespace

TEST_CASE("Atlantis: Atlantean buildings only in an Atlantean city, Greek-only ones never there (P:16-17)") {
    auto greek = town(false);
    CHECK(place(greek, "bibliotheke", 2, 4) == "build.atlantean_only");
    CHECK(place(greek, "podium", 2, 4) == "ok");
    CHECK(place(greek, "granary", 10, 2) == "ok");  // common to both
    auto atlantis = town(true);
    CHECK(place(atlantis, "podium", 2, 4) == "build.greek_only");
    CHECK(place(atlantis, "bibliotheke", 2, 4) == "ok");
    CHECK(place(atlantis, "granary", 10, 2) == "ok");
    std::string error;
    auto copy = Simulation::load(bo::test::catalog_isolated(), atlantis.save(), error);
    REQUIRE(copy.has_value());
    CHECK(copy->state().city.atlantean);  // saved (v44)
}

TEST_CASE("Atlantis: scholars, astronomers, inventors and curators give houses their points as culture (P:10-13)") {
    auto sim = town(true);
    REQUIRE(place(sim, "common_housing", 2, 4) == "ok");
    Building* house = nullptr;
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        house = b.house ? &b : house;
    }
    REQUIRE(house != nullptr);
    CHECK(house_culture(*house, sim.catalog(), 0) == 0);
    house->coverage["scholar"] = 10;
    house->coverage["astronomer"] = 10;
    CHECK(house_culture(*house, sim.catalog(), 0) == 15 + 25);  // S-004 points, 1:1 with culture (S-008)
}

TEST_CASE("Atlantis: an adventure says its city's culture (scenarios/README.md)") {
    std::string map = R"("..........","==========")";
    std::string error;
    auto cfg = parse_scenario(R"({"id": "a", "culture": "atlantean", "map": [)" + map + "]}", error);
    REQUIRE_MESSAGE(cfg.has_value(), error);
    CHECK(cfg->atlantean);
}

namespace {

Simulation track_town() {
    std::vector<std::string> rows(30, std::string(40, '.'));
    rows[10] = std::string(40, '=');
    SimulationConfig cfg{8, Difficulty::Hero, 30000, rows, true};
    cfg.atlantean = true;
    return Simulation(bo::test::catalog_isolated(), cfg);
}

}  // namespace

TEST_CASE("hippodrome: 4x4 stades joined into a closed loop; one track a city (P:13, L-127)") {
    auto sim = track_town();
    CHECK(place(sim, "hippodrome", 2, 6) == "ok");  // by the road: it keeps the horses
    CHECK(place(sim, "hippodrome", 20, 6) == "build.segment_apart");
    CHECK(place(sim, "hippodrome", 6, 6) == "ok");
    CHECK(place(sim, "hippodrome", 6, 2) == "ok");
    CHECK_FALSE(hippodrome_track(sim.state().city).closed);  // three pieces: open
    CHECK(place(sim, "hippodrome", 2, 2) == "ok");
    const Track t = hippodrome_track(sim.state().city);
    CHECK(t.segments.size() == 4);
    CHECK(t.closed);
    CHECK(working_hippodrome(sim.state(), sim.catalog()) == 0);  // no horses yet
    auto greek = town(false);
    CHECK(place(greek, "hippodrome", 2, 2) == "build.atlantean_only");
}

TEST_CASE("hippodrome: the handler brings horses from the ranch; with 4 the races earn 10 dr a month (P:14, doc 18)") {
    auto sim = track_town();
    for (const auto& [x, y] : {std::pair{2, 6}, {6, 6}, {6, 2}, {2, 2}}) {
        REQUIRE(place(sim, "hippodrome", x, y) == "ok");
    }
    REQUIRE(place(sim, "horse_ranch", 20, 11) == "ok");
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        if (b.type == "horse_ranch") {
            b.producer->output_loads = 6;
        }
    }
    for (int day = 0; day < 60 && sim.state().hippodrome_horses < 4; ++day) {
        sim.run_days(1);
    }
    CHECK(sim.state().hippodrome_horses == 4);  // as many as 4 stades need, one a trip
    CHECK(working_hippodrome(sim.state(), sim.catalog()) == 4);
    const auto before = sim.state().finance.treasury;
    sim.mutable_state().finance.treasury = before;
    std::int64_t income = 0;
    for (int day = 0; day < 16; ++day) {
        const auto t = sim.state().finance.treasury;
        sim.run_days(1);
        if (sim.state().time.day_in_month() == 1) {
            income = sim.state().finance.treasury - t;
        }
    }
    CHECK(income >= 10);  // the month's 10 dr (with the month's wages and taxes of an empty town: none)
}
