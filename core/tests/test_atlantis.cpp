// Atlantis stage 1 (docs/gameplay/15-atlantyda.md, P:10-13, P:16-17): the city's culture decides what may be
// built; science points work in houses as culture points.
#include <optional>
#include <tuple>

#include "bo/core/culture.h"
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
    CHECK(sim.state().hippodrome_this_year >= 10);  // bets and snacks, apart from taxes (P:14)
    const auto takings = sim.state().hippodrome_this_year;
    sim.run_days(16 * 12);  // a new year: last year's takings kept for the report
    CHECK(sim.state().hippodrome_last_year >= takings);
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
    sim.drain_events();
    sim.issue(PlaceBuilding{"crosswalk", 7, 2});
    sim.step(1);
    int changed = 0;
    for (const auto& e : sim.drain_events()) {
        changed += std::holds_alternative<TileChanged>(e.event) ? 1 : 0;  // the renderer draws the walkway
    }
    CHECK(changed == 4);  // north to south across the top stade (6, 2)
    for (std::int32_t y = 2; y < 6; ++y) {
        CHECK(sim.state().city.map.at({7, y}).road);
    }
    CHECK(place(sim, "crosswalk", 11, 2) == "build.crosswalk_adjacent");  // the next stade
    REQUIRE(place(sim, "column", 1, 7) == "ok");
    CHECK(place(sim, "crosswalk", 2, 7) == "build.crosswalk_blocked");  // the west end is built over
    CHECK(place(sim, "crosswalk", 2, 6) == "ok");  // west to east across the side stade (2, 6), row 6
    CHECK(hippodrome_track(sim.state().city).closed);  // still a loop
}

TEST_CASE("monuments: a pyramid is built like a sanctuary of marble and orichalc, then it adorns the city (P:26-28, L-128)") {
    std::vector<std::string> rows(30, std::string(40, '.'));
    rows[20] = std::string(40, '=');
    SimulationConfig cfg{8, Difficulty::Hero, 30000, rows, true};
    cfg.atlantean = true;
    ScenarioGoal goal;
    goal.type = "monument";
    goal.target = "pyramid_modest";
    cfg.scenario.goals = {goal};
    Simulation sim(bo::test::catalog_isolated(), cfg);
    REQUIRE(place(sim, "storehouse", 30, 17) == "ok");
    REQUIRE(place(sim, "artisans_guild", 24, 18) == "ok");
    auto& store = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({30, 17}).building);
    store.storage->loads["marble"] = 30;
    store.storage->loads["orichalc"] = 10;
    REQUIRE(place(sim, "pyramid_modest", 4, 17) == "ok");  // no marble taken first, no drachmas (S-008)
    const BuildingId id = sim.state().city.map.at({4, 17}).building;
    const auto& site = *sim.state().city.buildings.items.at(id).sanctuary;
    CHECK(site.needed.at("marble") == 20);
    CHECK(site.needed.at("orichalc") == 8);
    sim.run_days(1);
    CHECK(sim.state().city.appeal[sim.state().city.map.index({7, 17})] <= 0);  // a building site: no appeal yet
    for (int day = 0; day < 400 && !sim.state().city.buildings.items.at(id).sanctuary->complete; ++day) {
        sim.run_days(1);
    }
    REQUIRE(sim.state().city.buildings.items.at(id).sanctuary->complete);
    CHECK(store.storage->loads["marble"] == 10);
    CHECK(store.storage->loads["orichalc"] == 2);
    sim.run_days(1);
    CHECK(sim.state().city.appeal[sim.state().city.map.index({7, 17})] > 0);  // 15 next to it (building_stats.json)
    sim.drain_events();
    sim.issue(Pray{id});
    sim.step(1);
    bool refused = false;
    for (const auto& e : sim.drain_events()) {
        refused = refused || std::holds_alternative<CommandRejected>(e.event);
    }
    CHECK(refused);  // no god to pray to
    sim.run_days(16);
    CHECK(sim.state().scenario.goals[0].met);
}

namespace {

// A road along y = 10 of an Atlantean city; buildings above it touch it (P:10-13).
Simulation science_street() {
    std::vector<std::string> rows(16, std::string(70, '.'));
    rows[10] = std::string(70, '=');
    SimulationConfig cfg{7, Difficulty::Hero, 100000, rows, true};
    cfg.atlantean = true;
    return Simulation(bo::test::catalog(), cfg);
}

std::uint8_t visits(const Simulation& sim, TilePos p, const std::string& walker) {
    const auto& b = sim.state().city.buildings.items.at(sim.state().city.map.at(p).building);
    const auto it = b.coverage.find(walker);
    return it == b.coverage.end() ? 0 : it->second;
}

}  // namespace

TEST_CASE("science: the university sends astronomers to the observatory and curators to the museum, the workshop "
          "inventors to the laboratory; they roam from there (P:10-12)") {
    auto sim = science_street();
    for (const auto& [type, x, y] : {std::tuple{"university", 2, 7}, {"inventors_workshop", 6, 7}, {"observatory", 14, 5},
                                     {"laboratory", 30, 6}, {"museum", 46, 4}}) {
        REQUIRE(place(sim, type, x, y) == "ok");
    }
    REQUIRE(place(sim, "maintenance_office", 26, 8) == "ok");  // or they burn and fall (Z:89-90)
    REQUIRE(place(sim, "watchpost", 40, 8) == "ok");
    REQUIRE(place(sim, "maintenance_office", 60, 8) == "ok");
    for (const std::int32_t x : {20, 36, 54}) {  // neighbours to receive the visits
        REQUIRE(place(sim, "fountain", x, 8) == "ok");
    }
    sim.run_days(90);
    const auto& items = sim.state().city.buildings.items;
    CHECK(items.at(sim.state().city.map.at({14, 5}).building).performing_days > 0);
    CHECK(items.at(sim.state().city.map.at({30, 6}).building).performing_days > 0);
    CHECK(items.at(sim.state().city.map.at({46, 4}).building).performing_days > 0);
    CHECK(visits(sim, {20, 8}, "astronomer") > 0);
    CHECK(visits(sim, {36, 8}, "inventor") > 0);
    CHECK(visits(sim, {54, 8}, "curator") > 0);
    CHECK(city_culture_bonus(sim.state().city, sim.catalog()) == 10);  // a working museum: 10 for every house (doc 18)
    CHECK(place(sim, "museum", 56, 2) == "build.only_one");  // one a city (P:12)
}

namespace {

// A modest pyramid on a road with a guild and its stone in store; `black`: black marble in place of white.
struct Site {
    Simulation sim;
    BuildingId pyramid = kNoBuilding;
    BuildingId store = kNoBuilding;
};

Site pyramid_site(bool black, bool road = true) {
    std::vector<std::string> rows(30, std::string(40, '.'));
    rows[20] = std::string(40, '=');
    SimulationConfig cfg{8, Difficulty::Hero, 30000, rows, true};
    cfg.atlantean = true;
    Site s{Simulation(bo::test::catalog_isolated(), cfg)};
    REQUIRE(place(s.sim, "storehouse", 30, 17) == "ok");
    REQUIRE(place(s.sim, "artisans_guild", 24, 18) == "ok");
    s.store = s.sim.state().city.map.at({30, 17}).building;
    auto& loads = s.sim.mutable_state().city.buildings.items.at(s.store).storage->loads;
    loads[black ? "black_marble" : "marble"] = 30;
    loads["orichalc"] = 10;
    REQUIRE(place(s.sim, "pyramid_modest", 4, road ? 17 : 10) == "ok");  // off the road: its plank reaches none
    s.pyramid = s.sim.state().city.map.at({4, road ? 17 : 10}).building;
    return s;
}

bool finished_within(Site& s, int days) {
    for (int d = 0; d < days && !s.sim.state().city.buildings.items.at(s.pyramid).sanctuary->complete; ++d) {
        s.sim.run_days(1);
    }
    return s.sim.state().city.buildings.items.at(s.pyramid).sanctuary->complete;
}

}  // namespace

TEST_CASE("monuments: black marble builds a pyramid as white marble does (P:26-28, L-128)") {
    auto s = pyramid_site(true);
    CHECK(finished_within(s, 400));
    CHECK(s.sim.state().city.buildings.items.at(s.store).storage->loads.at("black_marble") == 10);
}

TEST_CASE("monuments: the building starts only when the plank reaches a road (P:26-28, L-128)") {
    auto s = pyramid_site(false, false);
    s.sim.run_days(100);
    const auto& site = *s.sim.state().city.buildings.items.at(s.pyramid).sanctuary;
    CHECK(site.needed.at("marble") == 20);  // nothing came
    for (int y = 13; y < 20; ++y) {
        s.sim.issue(PlaceRoad{5, y});  // a road up to the plank
    }
    s.sim.step(1);
    CHECK(finished_within(s, 400));
}

TEST_CASE("monuments: held, the building stops; resumed, it goes on; a god's sanctuary cannot be held (P:26-28)") {
    auto s = pyramid_site(false);
    s.sim.run_days(30);
    const std::uint32_t left = s.sim.state().city.buildings.items.at(s.pyramid).sanctuary->needed.at("marble") +
                               s.sim.state().city.buildings.items.at(s.pyramid).sanctuary->needed.at("orichalc");
    REQUIRE(left > 0);
    s.sim.issue(SetConstructionPaused{s.pyramid, true});
    s.sim.step(1);
    s.sim.run_days(5);  // the walkers on their way finish their trips
    const auto& site = *s.sim.state().city.buildings.items.at(s.pyramid).sanctuary;
    const std::uint32_t held = site.needed.at("marble") + site.needed.at("orichalc");
    s.sim.run_days(100);
    CHECK(site.paused);
    CHECK(site.needed.at("marble") + site.needed.at("orichalc") == held);
    s.sim.issue(SetConstructionPaused{s.pyramid, false});
    s.sim.step(1);
    CHECK(finished_within(s, 400));

    auto god = bo::test::with_god("");
    god.issue(PlaceBuilding{"storehouse", 50, 17});
    god.step(1);
    god.mutable_state().city.buildings.items.at(god.state().city.map.at({50, 17}).building).storage->loads["marble"] = 40;
    god.issue(PlaceBuilding{"sanctuary_hermes", 2, 14});
    god.step(1);
    god.drain_events();
    god.issue(SetConstructionPaused{god.state().city.map.at({2, 14}).building, true});
    god.step(1);
    std::string why;
    for (const auto& e : god.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
            why = r->reason_key;
        }
    }
    CHECK(why == "myth.not_a_monument");
}

TEST_CASE("save v49: a held monument stays held after a round trip") {
    auto s = pyramid_site(false);
    s.sim.issue(SetConstructionPaused{s.pyramid, true});
    s.sim.step(1);
    std::string error;
    auto b = Simulation::load(bo::test::catalog_isolated(), s.sim.save(), error);
    REQUIRE_MESSAGE(b.has_value(), error);
    CHECK(b->state().city.buildings.items.at(s.pyramid).sanctuary->paused);
    CHECK(b->state_hash() == s.sim.state_hash());
}

TEST_CASE("aesthetics of the expansion: each adorns its neighbourhood (P:16-17, building_stats.json)") {
    for (const char* t : {"water_park", "bird_bath", "short_obelisk", "tall_obelisk", "shell_garden", "sundial",
                          "dolphin_sculpture", "orrery", "spring", "topiary", "baths", "stone_circle"}) {
        INFO(t);
        auto sim = town(true);
        REQUIRE(place(sim, t, 10, 2) == "ok");
        sim.run_days(1);
        const Building& b = sim.state().city.buildings.items.at(sim.state().city.map.at({10, 2}).building);
        CHECK(sim.state().city.appeal[sim.state().city.map.index({b.origin.x + b.width, b.origin.y})] > 0);
    }
}
