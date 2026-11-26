// Elite households and noblemen (docs/gameplay/03; Z:25, Z:32-34, Z:88; assumptions L-108).
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// A main street on row 6; with `entry`, the street starts at the land entry.
Simulation town(bool entry) {
    std::vector<std::string> rows(14, std::string(40, '.'));
    rows[6] = std::string(40, '=');
    if (entry) {
        rows[6][0] = 'E';
    }
    // The real migration (the elite come from the common pool, L-108); risks are far off in these few weeks.
    return Simulation(bo::test::catalog(), SimulationConfig{2, Difficulty::Hero, 9000, rows, true});
}

Building& at(Simulation& sim, TilePos p) {
    return sim.mutable_state().city.buildings.items.at(sim.state().city.map.at(p).building);
}

// Stock for an elite plot (doc 18: 2 food, 1 fleece, 1 olive oil), then the plot.
void elite_plot(Simulation& sim) {
    sim.issue(PlaceBuilding{"granary", 10, 2});
    sim.issue(PlaceBuilding{"storehouse", 16, 3});
    sim.step(1);
    at(sim, {10, 2}).storage->loads["fish"] = 2;
    at(sim, {16, 3}).storage->loads["fleece"] = 1;
    at(sim, {16, 3}).storage->loads["olive_oil"] = 1;
    sim.issue(PlaceBuilding{"elite_housing", 22, 2});
    sim.step(1);
    REQUIRE(sim.state().city.map.at({22, 2}).building != kNoBuilding);
}

bool walker_of_type(const Simulation& sim, const std::string& type) {
    for (const auto& [id, w] : sim.state().walkers.items) {
        if (w.type == type) {
            return true;
        }
    }
    return false;
}

}  // namespace

TEST_CASE("elite immigrants of the common pool walk in from the land entry, 8 at most a group (Z:34, L-108)") {
    auto sim = town(true);
    elite_plot(sim);
    bool walking = false;
    for (int d = 0; d < 20 && at(sim, {22, 2}).house->population == 0; ++d) {
        sim.run_days(1);
        walking = walking || walker_of_type(sim, "immigrant");
    }
    CHECK(walking);
    CHECK(at(sim, {22, 2}).house->population > 0);
    CHECK(at(sim, {22, 2}).house->population <= 8);
    CHECK(sim.state().city.homeless == 0);
}

TEST_CASE("without an entry the elite move in at once; an inhabited elite house sends a nobleman (Z:25, Z:88)") {
    auto sim = town(false);
    elite_plot(sim);
    sim.run_days(1);
    CHECK(at(sim, {22, 2}).house->population > 0);
    bool noble = false;
    for (int d = 0; d < 10 && !noble; ++d) {
        sim.run_days(1);
        noble = walker_of_type(sim, "nobleman");
    }
    CHECK(noble);
}

TEST_CASE("an elite plot sends no nobleman while its family is still on the way") {
    auto sim = town(true);
    elite_plot(sim);
    int empty_days = 0;
    for (int d = 0; d < 20 && at(sim, {22, 2}).house->population == 0; ++d) {
        sim.run_days(1);
        empty_days += at(sim, {22, 2}).house->population == 0 ? 1 : 0;
        CHECK((at(sim, {22, 2}).house->population > 0 || !walker_of_type(sim, "nobleman")));
    }
    CHECK(empty_days > 0);
}

TEST_CASE("no popularity, no immigrants: the elite do not come either (L-108, eZeus)") {
    auto sim = town(false);
    elite_plot(sim);
    sim.mutable_state().migration.popularity = 0;
    for (int d = 0; d < 10; ++d) {
        sim.mutable_state().migration.popularity = 0;
        sim.run_days(1);
    }
    CHECK(at(sim, {22, 2}).house->population == 0);
}
