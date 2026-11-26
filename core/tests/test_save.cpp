#include "bo/core/serialize.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

TEST_CASE("save and load give an identical state hash and identical future") {
    Simulation a(bo::test::catalog(), SimulationConfig{99, Difficulty::Titan, 5000});
    a.issue(SetTaxRate{TaxRate::VeryHigh});
    a.issue_at(500, SetWageRate{WageRate::Low});  // still pending at save time
    a.step(300);
    const auto bytes = a.save();
    std::string error;
    auto b = Simulation::load(bo::test::catalog(), bytes, error);
    REQUIRE(b.has_value());
    CHECK(error.empty());
    CHECK(a.state_hash() == b->state_hash());
    a.step(1000);
    b->step(1000);
    CHECK(a.state_hash() == b->state_hash());
    CHECK(b->state().finance.wage_rate == WageRate::Low);
}

TEST_CASE("corrupted saves are rejected") {
    Simulation a(bo::test::catalog(), SimulationConfig{1, std::nullopt, 0});
    auto bytes = a.save();
    std::string error;
    auto bad = bytes;
    bad[0] ^= 0xFF;
    CHECK_FALSE(Simulation::load(bo::test::catalog(), bad, error).has_value());
    CHECK(error == "save.bad_magic");
    bytes.push_back(0);
    CHECK_FALSE(Simulation::load(bo::test::catalog(), bytes, error).has_value());
    CHECK(error == "save.trailing_bytes");
}

namespace {

Simulation city_sim() {
    std::vector<std::string> rows(12, std::string(16, '.'));
    rows[5] = std::string(16, '=');
    rows[5][9] = 'x';
    rows[2][3] = 'T';
    Simulation sim(bo::test::catalog(), SimulationConfig{7, Difficulty::Hero, 2000, rows});
    sim.issue(PlaceBuilding{"fountain", 2, 3});
    sim.issue(PlaceBuilding{"maintenance_office", 6, 6});
    sim.issue(ClearLand{3, 2});
    sim.issue_at(400, PlaceBuilding{"watchpost", 11, 6});  // pending at save time, string payload
    return sim;
}

}  // namespace

TEST_CASE("save v2: map, buildings, coverage and walkers mid-route survive a round trip") {
    auto a = city_sim();
    a.step(51 * 2 + 40);  // walkers are out
    REQUIRE_FALSE(a.state().walkers.items.empty());
    std::string error;
    auto b = Simulation::load(bo::test::catalog(), a.save(), error);
    REQUIRE(b.has_value());
    CHECK(a.state_hash() == b->state_hash());
    a.run_days(30);
    b->run_days(30);
    CHECK(a.state_hash() == b->state_hash());
    CHECK(b->state().city.buildings.items.size() == 3);
}

TEST_CASE("save v1 migrates to v2 with an empty city") {
    Simulation a(bo::test::catalog(), SimulationConfig{3, Difficulty::Hero, 100});
    a.step(10);
    auto bytes = a.save();
    // A v1 save is the current layout without the city and walker blocks: for an empty city 16 bytes of map
    // and buildings, 29 bytes of the undo record (v3), 4 bytes of homeless (v4) and 8 bytes of walkers.
    bytes.resize(bytes.size() - 57);
    bytes[4] = 1;
    std::string error;
    auto b = Simulation::load(bo::test::catalog(), bytes, error);
    REQUIRE(b.has_value());
    CHECK(error.empty());
    CHECK(a.state_hash() == b->state_hash());
}

TEST_CASE("save v2 and v3 migrate to the current format") {
    Simulation a(bo::test::catalog(), SimulationConfig{3, Difficulty::Hero, 100});
    a.step(10);
    const auto current = a.save();
    // Empty city: v3 lacks the 4-byte homeless count, v2 also the 29-byte undo record; both precede the
    // 8-byte walker block.
    auto v3 = current;
    v3.erase(v3.end() - 8 - 4, v3.end() - 8);
    v3[4] = 3;
    std::string error3;
    auto c = Simulation::load(bo::test::catalog(), v3, error3);
    REQUIRE(c.has_value());
    CHECK(a.state_hash() == c->state_hash());
    auto bytes = current;
    bytes.erase(bytes.end() - 8 - 33, bytes.end() - 8);
    bytes[4] = 2;
    std::string error;
    auto b = Simulation::load(bo::test::catalog(), bytes, error);
    REQUIRE(b.has_value());
    CHECK(a.state_hash() == b->state_hash());
}
