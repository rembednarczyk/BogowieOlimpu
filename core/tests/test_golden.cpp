// Golden hash (docs/PRAKTYKI.md ch. 3): a small fixed scenario, hash updated ONLY by hand, and the commit that
// changes it must name the rule that changed it. Never regenerate it automatically.
#include <cstdio>

#include "bo/core/hash.h"
#include "bo/core/serialize.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

Simulation golden_scenario() {
    Simulation sim(bo::test::catalog(), SimulationConfig{20261126, Difficulty::Hero, 10000});
    sim.issue_at(10, SetTaxRate{TaxRate::High});
    sim.issue_at(kTicksPerMonth * 3, SetWageRate{WageRate::VeryHigh});
    sim.issue_at(kTicksPerYear, SetTaxRate{TaxRate::Low});
    sim.step(kTicksPerYear * 2 + 17);
    return sim;
}

}  // namespace

TEST_CASE("replay: the same seed and command log give the same hash") {
    CHECK(golden_scenario().state_hash() == golden_scenario().state_hash());
}

TEST_CASE("golden hash") {
    // The data version is part of the hash, so this test covers the logic, not the data: it hashes a state
    // whose data_version field is fixed to 0.
    const auto sim = golden_scenario();
    const auto bytes = bo::core::serialize_state(sim.state(), 0);
    bo::core::Fnv1a64 h;
    h.add_bytes(bytes.data(), bytes.size());
    // 2026-11-26: initial value (modules A1-A5).
    // 2026-11-26: save format v2 appends the (empty) city and walker blocks (module B, serialize.cpp).
    // 2026-11-26: save format v3 appends the (empty) undo record (Z:20, serialize_city.cpp).
    // 2026-11-26: save format v4 appends the homeless count (module C2, serialize_city.cpp).
    // 2026-11-26: save format v5 adds labour and finance blocks (modules C3, C6).
    const std::uint64_t kGolden = 0x6f5e4e8f131f6c29ULL;
    if (h.value() != kGolden) {
        std::printf("golden hash is 0x%016llx\n", static_cast<unsigned long long>(h.value()));
    }
    CHECK(h.value() == kGolden);
}

namespace {

// 32x32 city: a road grid with a roadblock, forest, meadow, four service buildings, an agora with a stall,
// a boulevard, an undone road, two years of play.
Simulation golden_city() {
    std::vector<std::string> rows(32, std::string(32, '.'));
    for (std::size_t i = 2; i < 30; ++i) {
        rows[4][i] = '=';
        rows[16][i] = '=';
        rows[28][i] = '=';
        rows[i][4] = '=';
        rows[i][16] = '=';
        rows[i][28] = '=';
    }
    rows[16][22] = 'x';
    for (std::size_t i = 20; i < 26; ++i) {
        rows[8][i] = 'T';
        rows[22][i] = ',';
    }
    // Full staffing until immigration (C8) fills houses with workers.
    Simulation sim(bo::test::catalog(), SimulationConfig{20261126, Difficulty::Hero, 3000, rows, true});
    sim.issue(PlaceBuilding{"fountain", 6, 5});
    sim.issue(PlaceBuilding{"maintenance_office", 17, 17});
    sim.issue(PlaceBuilding{"watchpost", 10, 14});
    sim.issue_at(kTicksPerMonth, PlaceBuilding{"gymnasium", 24, 12});
    sim.issue_at(kTicksPerMonth * 2, ClearLand{21, 8});
    sim.issue_at(kTicksPerMonth * 2, PlaceRoad{10, 10});
    sim.issue_at(kTicksPerMonth * 5, Demolish{16, 10});
    sim.issue_at(kTicksPerYear, PlaceRoadblock{4, 10});
    sim.issue_at(kTicksPerMonth * 3, PlaceBuilding{"common_agora", 28, 6, 0});
    sim.issue_at(kTicksPerMonth * 3 + 1, PlaceBuilding{"food_vendor_stall", 29, 8, 0});
    sim.issue_at(kTicksPerMonth * 4, PlaceBuilding{"boulevard", 8, 4, 1});
    sim.issue_at(kTicksPerMonth * 6, PlaceRoad{12, 10});
    sim.issue_at(kTicksPerMonth * 6 + 5, Undo{});
    sim.issue_at(kTicksPerMonth * 7, PlaceBuilding{"common_housing", 5, 2, 0});
    sim.issue_at(kTicksPerMonth * 7, PlaceBuilding{"flower_garden", 8, 1, 0});
    sim.step(kTicksPerYear * 2 + 17);
    return sim;
}

}  // namespace

TEST_CASE("golden hash: city with buildings and walkers") {
    const auto sim = golden_city();
    REQUIRE(sim.setup_error().empty());
    CHECK(sim.state().city.buildings.items.size() == 9);
    CHECK_FALSE(sim.state().city.map.at({12, 10}).road);  // undone
    CHECK(sim.state().city.map.at({4, 10}).roadblock);
    CHECK_FALSE(sim.state().city.map.at({16, 10}).road);
    const auto bytes = bo::core::serialize_state(sim.state(), 0);
    bo::core::Fnv1a64 h;
    h.add_bytes(bytes.data(), bytes.size());
    // 2026-11-26: initial value (module B: map, construction, roads, A*, roaming and destination walkers).
    // 2026-11-26: module B completed: save format v3, scenario extended with an agora, a stall, a boulevard and
    // an undo; wide-road rule (S-006) and Fountain return tile (S-004) change walker routes.
    // 2026-11-26: modules C1-C2: save format v4 (house state), scenario extended with a house plot and a garden.
    // 2026-11-26: modules C3, C6: save format v5, full staffing switch, monthly finance, clerk counter 50 days.
    const std::uint64_t kGolden = 0x5f40be787f94d37aULL;
    if (h.value() != kGolden) {
        std::printf("golden city hash is 0x%016llx\n", static_cast<unsigned long long>(h.value()));
    }
    CHECK(h.value() == kGolden);
}
