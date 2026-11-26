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
    const std::uint64_t kGolden = 0xcf68c117db7a7f70ULL;
    if (h.value() != kGolden) {
        std::printf("golden hash is 0x%016llx\n", static_cast<unsigned long long>(h.value()));
    }
    CHECK(h.value() == kGolden);
}

namespace {

// 32x32 city: a road grid with a roadblock, forest, meadow, four service buildings, two years of play.
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
    Simulation sim(bo::test::catalog(), SimulationConfig{20261126, Difficulty::Hero, 3000, rows});
    sim.issue(PlaceBuilding{"fountain", 6, 5});
    sim.issue(PlaceBuilding{"maintenance_office", 17, 17});
    sim.issue(PlaceBuilding{"watchpost", 10, 14});
    sim.issue_at(kTicksPerMonth, PlaceBuilding{"gymnasium", 24, 12});
    sim.issue_at(kTicksPerMonth * 2, ClearLand{21, 8});
    sim.issue_at(kTicksPerMonth * 2, PlaceRoad{10, 10});
    sim.issue_at(kTicksPerMonth * 5, Demolish{16, 10});
    sim.issue_at(kTicksPerYear, PlaceRoadblock{4, 10});
    sim.step(kTicksPerYear * 2 + 17);
    return sim;
}

}  // namespace

TEST_CASE("golden hash: city with buildings and walkers") {
    const auto sim = golden_city();
    REQUIRE(sim.setup_error().empty());
    CHECK(sim.state().city.buildings.items.size() == 4);
    CHECK(sim.state().city.map.at({4, 10}).roadblock);
    CHECK_FALSE(sim.state().city.map.at({16, 10}).road);
    const auto bytes = bo::core::serialize_state(sim.state(), 0);
    bo::core::Fnv1a64 h;
    h.add_bytes(bytes.data(), bytes.size());
    // 2026-11-26: initial value (module B: map, construction, roads, A*, roaming and destination walkers).
    const std::uint64_t kGolden = 0xb7bba380019598a8ULL;
    if (h.value() != kGolden) {
        std::printf("golden city hash is 0x%016llx\n", static_cast<unsigned long long>(h.value()));
    }
    CHECK(h.value() == kGolden);
}
