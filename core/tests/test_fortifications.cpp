// Module D3 stage 3: walls, gatehouses, towers, trireme wharves (docs/gameplay/11; Z:141-147; L-117).
#include "bo/core/military.h"
#include "bo/core/monsters.h"
#include "bo/core/path.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// 40x30: a road on row 10 from the land entry, a Palace east of a wall line at x = 15 with a gatehouse on
// the road, an invasion point west of it.
Simulation walled(bool gate = true) {
    std::vector<std::string> rows(30, std::string(40, '.'));
    rows[10] = std::string(40, '=');
    rows[10][0] = 'E';
    rows[20][2] = 'i';
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{15, Difficulty::Hero, 30000, rows, true});
    sim.issue(PlaceBuilding{"palace", 25, 4});
    for (int y = 0; y < 30; ++y) {
        if (y < 8 || y > 12) {
            sim.issue(PlaceBuilding{"wall", 15, y});
        }
    }
    sim.step(1);
    if (gate) {
        sim.issue(PlaceBuilding{"gatehouse", 14, 8, 1});  // 3 wide, 5 high; the road crosses its middle row
        sim.step(1);
    }
    return sim;
}

std::string rejection(Simulation& sim) {
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
            return r->reason_key;
        }
    }
    return {};
}

}  // namespace

TEST_CASE("walls: one tile, no road needed, never on a road (Z:145)") {
    auto sim = walled(false);
    CHECK(sim.state().city.map.at({15, 3}).building != kNoBuilding);
    sim.drain_events();
    sim.issue(PlaceBuilding{"wall", 20, 10});
    sim.step(1);
    CHECK_FALSE(rejection(sim).empty());
}

TEST_CASE("a gatehouse lets road walkers through and closes the road to enemies (Z:146-147, doc 24)") {
    auto bad = walled(false);
    bad.issue(PlaceBuilding{"gatehouse", 36, 8, 1});  // no walls at its ends
    bad.step(1);
    CHECK(rejection(bad) == "build.gate_needs_walls");
    auto sim = walled();
    REQUIRE(sim.state().city.map.at({15, 10}).building != kNoBuilding);
    CHECK(sim.state().city.map.at({15, 10}).roadblock);
    CHECK(find_path(sim.state().city, {{1, 10}, {30, 10}, PathMode::Road}).has_value());
    CHECK_FALSE(find_path(sim.state().city, {{1, 10}, {30, 10}, PathMode::Hostile}).has_value());
    sim.issue(Demolish{14, 8});
    sim.step(1);
    CHECK_FALSE(sim.state().city.map.at({15, 10}).roadblock);
    CHECK(find_path(sim.state().city, {{1, 10}, {30, 10}, PathMode::Hostile}).has_value());
}

TEST_CASE("a tower needs a wall two thick, the Palace and a road to it (Z:146)") {
    auto sim = walled();
    sim.issue(PlaceBuilding{"tower", 15, 14});  // a single wall line
    sim.step(1);
    CHECK(rejection(sim) == "build.tower_needs_thick_wall");
    for (int y = 13; y < 16; ++y) {
        sim.issue(PlaceBuilding{"wall", 16, y});
    }
    sim.step(1);
    sim.issue(PlaceRoad{17, 14});
    sim.issue(PlaceRoad{17, 13});
    sim.issue(PlaceRoad{17, 12});
    sim.issue(PlaceRoad{17, 11});
    sim.step(1);
    sim.drain_events();
    sim.issue(PlaceBuilding{"tower", 15, 13});
    sim.step(1);
    CHECK(rejection(sim).empty());
    const BuildingId tower = sim.state().city.map.at({15, 13}).building;
    REQUIRE(tower != kNoBuilding);
    CHECK(sim.state().city.buildings.items.at(tower).type == "tower");
    CHECK(sim.state().city.buildings.items.at(tower).hits == 600);
}

TEST_CASE("invaders walled off break through a wall or the gate, then march on (L-117)") {
    auto sim = walled();
    sim.issue(StartInvasion{"greek", 10});
    bool breached = false;
    int result = -1;
    for (int d = 0; d < 80 && result < 0; ++d) {
        sim.run_days(1);
        for (const auto& e : sim.drain_events()) {
            if (const auto* r = std::get_if<BuildingRemoved>(&e.event)) {
                breached = breached || r->id != 0;
            }
            if (const auto* end = std::get_if<InvasionEnded>(&e.event)) {
                result = end->result;
            }
        }
    }
    CHECK(breached);
    CHECK(result == 3);  // nobody defended the Palace
}

TEST_CASE("a manned tower shoots the invaders in range (Z:146, L-117)") {
    auto sim = walled();
    for (int y = 13; y < 16; ++y) {
        sim.issue(PlaceBuilding{"wall", 16, y});
    }
    sim.issue(PlaceRoad{17, 14});
    sim.issue(PlaceRoad{17, 13});
    sim.issue(PlaceRoad{17, 12});
    sim.issue(PlaceRoad{17, 11});
    sim.step(1);
    sim.issue(PlaceBuilding{"tower", 15, 13});
    sim.issue(StartInvasion{"greek", 5});
    sim.step(1);
    std::int64_t hp_before = 0;
    for (const auto& [id, w] : sim.state().walkers.items) {
        hp_before += w.job == WalkerJob::Invade ? w.hp : 0;
    }
    sim.run_days(8);
    std::int64_t hp_after = 0;
    for (const auto& [id, w] : sim.state().walkers.items) {
        hp_after += w.job == WalkerJob::Invade ? w.hp : 0;
    }
    CHECK(hp_after < hp_before);
}

TEST_CASE("a trireme wharf builds a trireme from wood and armor, one at a time (Z:141, L-117)") {
    std::vector<std::string> rows(20, std::string(30, '.'));
    rows[8] = std::string(30, '=');
    for (std::size_t y = 12; y < 20; ++y) {
        rows[y] = std::string(30, '~');
    }
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{16, Difficulty::Hero, 30000, rows, true});
    sim.issue(PlaceBuilding{"trireme_wharf", 10, 9});
    sim.step(1);
    CHECK(rejection(sim) == "build.needs_building");  // the Palace first (Z:141)
    sim.issue(PlaceBuilding{"palace", 2, 2});
    sim.step(1);
    sim.issue(PlaceBuilding{"trireme_wharf", 10, 9});
    sim.step(1);
    BuildingId wharf = kNoBuilding;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        wharf = b.type == "trireme_wharf" ? id : wharf;
    }
    REQUIRE(wharf != kNoBuilding);
    auto& p = *sim.mutable_state().city.buildings.items.at(wharf).producer;
    p.units["wood"] = 8;
    p.units["armor"] = 8;
    sim.run_days(100);
    CHECK(sim.state().city.buildings.items.at(wharf).producer->output_loads == 1);
    sim.run_days(100);
    CHECK(sim.state().city.buildings.items.at(wharf).producer->output_loads == 1);  // one ship a wharf
    CHECK(unmet_requirements(sim.state(), sim.catalog(), "jason", wharf).size() == 5);  // nothing yet
    sim.mutable_state().city.buildings.items.at(wharf).producer->output_loads = 3;      // as if three wharves
    CHECK(unmet_requirements(sim.state(), sim.catalog(), "jason", wharf).size() == 4);  // triremes>=3 met
}
