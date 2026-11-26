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
