// Atlantis stage 1 (docs/gameplay/15-atlantyda.md, P:10-13, P:16-17): the city's culture decides what may be
// built; science points work in houses as culture points.
#include <optional>

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

namespace {

// A racing 4-stade track in a catalog whose bonus thresholds are lowered to it.
struct Racing {
    DataCatalog catalog;
    std::optional<Simulation> sim;
};

void race(Racing& r, std::uint32_t mood_from, std::uint32_t opinion_from, std::uint32_t unrest_above) {
    r.catalog = bo::test::catalog_isolated();
    r.catalog.hippodrome_bonus.mood_from = mood_from;
    r.catalog.hippodrome_bonus.opinion_from = opinion_from;
    r.catalog.hippodrome_bonus.unrest_above = unrest_above;
    std::vector<std::string> rows(30, std::string(40, '.'));
    rows[10] = std::string(40, '=');
    SimulationConfig cfg{8, Difficulty::Hero, 30000, rows, true};
    cfg.atlantean = true;
    TradePartner p;
    p.id = "athens";
    p.relation = "ally";
    p.diplomacy.opinion = 50;
    cfg.trade_partners = {p};
    r.sim.emplace(r.catalog, cfg);
    for (const auto& [x, y] : {std::pair{2, 6}, {6, 6}, {6, 2}, {2, 2}}) {
        REQUIRE(place(*r.sim, "hippodrome", x, y) == "ok");
    }
    r.sim->mutable_state().hippodrome_horses = 4;
}

}  // namespace

TEST_CASE("hippodrome: a big racing track cheers the city and raises opinions; too big a one brings unrest (P:14, doc 18)") {
    Racing small;
    race(small, 60, 100, 200);  // the data: four stades are not big
    CHECK(hippodrome_mood(small.sim->state(), small.catalog) == 0);
    Racing big;
    race(big, 4, 4, 200);
    CHECK(hippodrome_mood(big.sim->state(), big.catalog) == big.catalog.hippodrome_bonus.mood);
    big.sim->run_days(16);  // a month: the visitors (P:14)
    CHECK(big.sim->state().trade.partners[0].diplomacy.opinion > 50);
    Racing huge;
    race(huge, 4, 100, 3);
    CHECK(hippodrome_mood(huge.sim->state(), huge.catalog) == huge.catalog.hippodrome_bonus.unrest_mood);  // doc 18
    huge.sim->mutable_state().hippodrome_horses = 0;  // not racing: no effect
    CHECK(hippodrome_mood(huge.sim->state(), huge.catalog) == 0);
}

TEST_CASE("crosswalk: across a straight stade, ends open, never next to another; the tiles become road (P:14)") {
    std::vector<std::string> rows(20, std::string(24, '.'));
    SimulationConfig cfg{8, Difficulty::Hero, 30000, rows, true};
    cfg.atlantean = true;
    Simulation sim(bo::test::catalog_isolated(), cfg);
    // A ring of 4 x 3 stades around an empty middle (x 6-13, y 6-9).
    for (const auto& [x, y] : {std::pair{2, 2}, {6, 2}, {10, 2}, {14, 2}, {14, 6}, {14, 10}, {10, 10}, {6, 10},
                               {2, 10}, {2, 6}}) {
        REQUIRE(place(sim, "hippodrome", x, y) == "ok");
    }
    REQUIRE(hippodrome_track(sim.state().city).closed);
    CHECK(place(sim, "crosswalk", 3, 3) == "build.crosswalk_not_straight");  // a corner
    CHECK(place(sim, "crosswalk", 20, 3) == "build.needs_hippodrome");
    CHECK(place(sim, "crosswalk", 7, 2) == "ok");  // north to south across the top stade (6, 2)
    for (std::int32_t y = 2; y < 6; ++y) {
        CHECK(sim.state().city.map.at({7, y}).road);
    }
    CHECK(place(sim, "crosswalk", 11, 2) == "build.crosswalk_adjacent");  // the next stade
    REQUIRE(place(sim, "column", 1, 7) == "ok");
    CHECK(place(sim, "crosswalk", 2, 7) == "build.crosswalk_blocked");  // the west end is built over
    CHECK(place(sim, "crosswalk", 2, 6) == "ok");  // west to east across the side stade (2, 6), row 6
    CHECK(hippodrome_track(sim.state().city).closed);  // still a loop
}
