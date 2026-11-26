// Modules B5-B6 and the daily walker systems: roaming walker, destination walker, spawn delay, coverage decay.
// Numbers: docs/gameplay/24-algorytmy-silnika.md (96 days, 5x5, budgets from data/walkers.json).
#include "bo/core/simulation.h"
#include "bo/core/walker.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// A ring road 20x12 with a fountain inside, touching the north road.
std::vector<std::string> ring_map() {
    std::vector<std::string> rows(14, std::string(22, '.'));
    for (int x = 1; x <= 20; ++x) {
        rows[1][static_cast<std::size_t>(x)] = '=';
        rows[12][static_cast<std::size_t>(x)] = '=';
    }
    for (int y = 1; y <= 12; ++y) {
        rows[static_cast<std::size_t>(y)][1] = '=';
        rows[static_cast<std::size_t>(y)][20] = '=';
    }
    return rows;
}

Simulation ring_sim() {
    SimulationConfig cfg{42, Difficulty::Hero, 1000, ring_map(), true};  // full staffing: no houses here
    Simulation sim(bo::test::catalog(), cfg);
    REQUIRE(sim.setup_error().empty());
    sim.issue(PlaceBuilding{"fountain", 5, 2});
    sim.issue(PlaceBuilding{"fountain", 12, 8});  // far from roads: no access, never spawns
    sim.step(1);
    return sim;
}

const Building& building_at(const Simulation& sim, TilePos p) {
    return sim.state().city.buildings.items.at(sim.state().city.map.at(p).building);
}

}  // namespace

TEST_CASE("roaming walker spawns at the walker_spawn tick and serves a 5x5 square for 96 days") {
    auto sim = ring_sim();
    const Building& f = building_at(sim, {5, 2});
    REQUIRE(f.road_access);
    CHECK_FALSE(building_at(sim, {12, 8}).road_access);
    sim.step(31);  // walker_spawn runs at tick 31 of the day
    CHECK(sim.state().walkers.items.size() == 1);
    CHECK(f.roaming_walker != kNoWalker);
    CHECK(f.coverage.at("water_bearer") == kServiceCoverageDays);
    const auto& w = sim.state().walkers.items.begin()->second;
    CHECK(w.max_roam_ticks == 384);  // water bearer budget (doc 24)
}

TEST_CASE("only one roaming walker per building; the next leaves after the first is back") {
    auto sim = ring_sim();
    sim.run_days(3);
    CHECK(sim.state().walkers.items.size() == 1);
    const Dir first_dir = rotate(building_at(sim, {5, 2}).next_roam_dir, -1);
    sim.run_days(40);  // 384 roam ticks + return is far below 40 days of 51 ticks
    std::size_t spawned = 0;
    for (const auto& e : sim.drain_events()) {
        spawned += std::holds_alternative<WalkerSpawned>(e.event) ? 1 : 0;
    }
    CHECK(spawned >= 2);
    CHECK(building_at(sim, {5, 2}).next_roam_dir != first_dir);
}

TEST_CASE("roaming walker stays on roads, ends its budget and walks back home") {
    auto sim = ring_sim();
    sim.step(32);
    REQUIRE(sim.state().walkers.items.size() == 1);
    const WalkerId id = sim.state().walkers.items.begin()->first;
    bool returned = false;
    for (int t = 0; t < 4000 && sim.state().walkers.items.count(id); ++t) {
        const auto& w = sim.state().walkers.items.at(id);
        CHECK(sim.state().city.map.is_road(w.tile));
        returned = returned || w.phase == WalkerPhase::Returning;
        sim.step(1);
    }
    CHECK(returned);
    CHECK_FALSE(sim.state().walkers.items.count(id));
}

TEST_CASE("coverage decays by one per day") {
    auto sim = ring_sim();
    sim.step(32);
    auto& b = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({5, 2}).building);
    sim.mutable_state().walkers.items.clear();  // isolate decay from new visits
    b.roaming_walker = kNoWalker;
    sim.mutable_state().labour.disabled_types.insert("fountain");  // industry off: no new walker (Z:62-63)
    const auto before = b.coverage.at("water_bearer");
    sim.run_days(10);
    CHECK(b.coverage.at("water_bearer") == before - 10);
}

TEST_CASE("spawn delay follows staffing: 100% -> 0 days, 50% -> 3 days (S-007)") {
    // Real labour: one house with 5 people gives floor(5 x 42%) = 2 workers on Hero at normal wages; the
    // fountain needs 4 (S-004), so it runs at 50%. The second fountain has no road and gets nobody.
    SimulationConfig cfg{42, Difficulty::Hero, 1000, ring_map()};
    Simulation sim(bo::test::catalog_no_migration(), cfg);
    sim.issue(PlaceBuilding{"fountain", 5, 2});
    sim.issue(PlaceBuilding{"common_housing", 10, 2});
    sim.step(1);
    sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({10, 2}).building).house->population = 5;
    sim.run_days(3);
    CHECK(sim.state().labour.workforce == 2);
    CHECK(sim.state().city.buildings.items.at(sim.state().city.map.at({5, 2}).building).staffing_percent == 50);
    CHECK(sim.state().walkers.items.empty());
    sim.run_days(1);
    CHECK(sim.state().walkers.items.size() == 1);
}

TEST_CASE("a roadblock stops roaming walkers; a destination walker passes it (Z:19-20)") {
    const auto& cat = bo::test::catalog();
    std::string err;
    CityState c;
    c.map = *parse_text_map({"..........", "====x=====", "..........", ".........."}, err);
    update_roads(c);
    const auto r = place_building(c, cat, Difficulty::Hero, "fountain", {0, 2});
    REQUIRE(r.ok());
    WalkerStore ws;
    const WalkerId roam = spawn_roaming_walker(c, ws, cat, "water_bearer", r.building, Dir::East);
    REQUIRE(roam != kNoWalker);
    for (int t = 0; t < 2000 && ws.items.at(roam).phase != WalkerPhase::Done; ++t) {
        CHECK(ws.items.at(roam).tile.x < 4);
        step_walker(c, ws.items.at(roam));
    }
    const WalkerId dest = spawn_destination_walker(c, ws, cat, "water_bearer", r.building, {9, 1});
    REQUIRE(dest != kNoWalker);
    for (int t = 0; t < 2000 && ws.items.at(dest).phase != WalkerPhase::Done; ++t) {
        step_walker(c, ws.items.at(dest));
    }
    CHECK(ws.items.at(dest).tile == TilePos{9, 1});
}

TEST_CASE("walker speed: a standard walker covers one tile per 15 ticks (doc 17)") {
    const auto& cat = bo::test::catalog();
    std::string err;
    CityState c;
    c.map = *parse_text_map({"...........", "...........", "==========="}, err);
    update_roads(c);
    const auto r = place_building(c, cat, Difficulty::Hero, "fountain", {0, 0});
    REQUIRE(r.ok());
    WalkerStore ws;
    const WalkerId id = spawn_destination_walker(c, ws, cat, "water_bearer", r.building, {10, 2});
    REQUIRE(id != kNoWalker);
    auto& w = ws.items.at(id);
    const TilePos start = w.tile;
    for (int t = 0; t < 15 * 4; ++t) {
        step_walker(c, w);
    }
    CHECK(w.tile.x - start.x == 4);
}

TEST_CASE("wide road: on a double-wide road a roaming walker keeps its lane (S-006 movement.c:386-411)") {
    std::string err;
    CityState c;
    c.map = *parse_text_map({"....................", "====================", "====================",
                             "...................."},
                            err);
    update_roads(c);
    Walker w;
    w.kind = WalkerKind::Roaming;
    w.phase = WalkerPhase::Roaming;
    w.tile = {2, 1};
    w.dir = Dir::East;
    w.came_from = Dir::West;
    w.speed_raw = 65536;
    w.max_roam_ticks = 100000;
    w.ticks_until_next_turn = 100;
    for (int t = 0; t < 15 * 12; ++t) {
        step_walker(c, w);
        CHECK(w.tile.y == 1);
    }
    CHECK(w.tile.x == 14);
}

TEST_CASE("fountain walkers return to the corner tile north of the building (doc 17, S-004)") {
    const auto& cat = bo::test::catalog();
    std::string err;
    CityState c;
    // Fountain at (3,2)-(4,3); exit road east of it, a loop reaching the north-west corner (2,1).
    c.map = *parse_text_map({"..........", "..=====...", "..=..=....", "..=..=....", "..====....", ".........."},
                            err);
    update_roads(c);
    const auto r = place_building(c, cat, Difficulty::Hero, "fountain", {3, 2});
    REQUIRE(r.ok());
    WalkerStore ws;
    const WalkerId id = spawn_roaming_walker(c, ws, cat, "water_bearer", r.building, Dir::North);
    REQUIRE(id != kNoWalker);
    Walker& w = ws.items.at(id);
    CHECK(w.return_north);
    TilePos last = w.tile;
    for (int t = 0; t < 20000 && w.phase != WalkerPhase::Done; ++t) {
        last = w.tile;
        step_walker(c, w);
    }
    CHECK(w.tile == TilePos{2, 1});
    (void)last;
}
