// Wolves (docs/gameplay/01, 04; Z:45, P:7; assumptions L-107 after S-008 defaults).
#include "bo/core/logistics.h"
#include "bo/core/simulation.h"
#include "bo/core/walker.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// Meadow on the east, a road on row 6, a wolf den at (den_x, 2).
Simulation pasture(int den_x, std::uint64_t seed = 8) {
    std::vector<std::string> rows(14, std::string(90, '.'));
    for (std::size_t y = 0; y < 6; ++y) {
        for (std::size_t x = 10; x < 20; ++x) {
            rows[y][x] = ',';
        }
    }
    rows[6] = std::string(90, '=');
    rows[2][static_cast<std::size_t>(den_x)] = 'w';
    return Simulation(bo::test::catalog_isolated(), SimulationConfig{seed, Difficulty::Hero, 9000, rows});
}

int animals(const Simulation& sim) {
    int n = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        n += b.animal ? 1 : 0;
    }
    return n;
}

void herd(Simulation& sim) {
    sim.issue(PlaceBuilding{"carding_shed", 21, 3});  // no workers: no shepherd watches the flock
    sim.step(1);
    for (int i = 0; i < 4; ++i) {
        sim.issue(PlaceBuilding{"sheep", 12, 2});
    }
    sim.step(1);
}

}  // namespace

TEST_CASE("wolves occasionally take an unguarded sheep (Z:45); the pack starts full (L-107)") {
    auto sim = pasture(2);
    herd(sim);
    REQUIRE(animals(sim) == 4);
    bool saw_wolf = false;
    bool taken = false;
    for (int d = 0; d < 192 && !taken; ++d) {
        const int before = animals(sim);
        sim.run_days(1);
        taken = animals(sim) < before;  // herd births only add animals (L-101)
        for (const auto& [id, w] : sim.state().walkers.items) {
            saw_wolf = saw_wolf || w.type == "wolf";
        }
    }
    CHECK(saw_wolf);
    CHECK(taken);
    CHECK(sim.state().city.spawns.begin()->second.animals == 8);
}

TEST_CASE("prey farther than 60 tiles is out of the wolves' reach (S-008 wolf_hunt_distance)") {
    auto sim = pasture(85);
    herd(sim);
    bool saw_wolf = false;
    for (int d = 0; d < 2 * 192; ++d) {
        sim.run_days(1);
        for (const auto& [id, w] : sim.state().walkers.items) {
            saw_wolf = saw_wolf || w.type == "wolf";
        }
    }
    CHECK_FALSE(saw_wolf);
}

TEST_CASE("a shepherd of the same shed near the flock scares the wolf off (Z:45)") {
    auto sim = pasture(2);
    herd(sim);
    WorldState& st = sim.mutable_state();
    BuildingId sheep = kNoBuilding;
    BuildingId shed = kNoBuilding;
    for (const auto& [id, b] : st.city.buildings.items) {
        sheep = b.animal ? id : sheep;
        shed = b.animal ? b.animal->shed : shed;
    }
    // A wolf standing at the sheep, and the shepherd two tiles away.
    const auto den = static_cast<std::uint32_t>(st.city.map.index({2, 2}));
    st.city.spawns[den] = SpawnState{8, 0, 900};
    Walker wolf;
    wolf.id = 900;
    wolf.type = "wolf";
    wolf.tile = st.city.buildings.items.at(sheep).origin;
    wolf.job = WalkerJob::Prowl;
    wolf.phase = WalkerPhase::AtTarget;
    wolf.target = sheep;
    st.walkers.items.emplace(900, wolf);
    Walker shepherd;
    shepherd.id = 901;
    shepherd.type = "shepherd";
    shepherd.home = shed;
    shepherd.job = WalkerJob::Shear;
    shepherd.tile = {wolf.tile.x + 2, wolf.tile.y};
    st.walkers.items.emplace(901, shepherd);
    advance_wolves(st, bo::test::catalog());
    CHECK(animals(sim) == 4);
    CHECK(st.walkers.items.at(900).phase == WalkerPhase::Routed);  // back to the den
    st.walkers.items.at(900).phase = WalkerPhase::AtTarget;
    st.walkers.items.at(900).job = WalkerJob::Prowl;
    st.walkers.items.erase(901);
    advance_wolves(st, bo::test::catalog());
    CHECK(animals(sim) == 3);
}
