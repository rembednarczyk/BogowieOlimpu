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
    Simulation sim(bo::test::catalog(), SimulationConfig{7, Difficulty::Hero, 2000, rows, true});
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

// Tail of a save with an empty city, in order: map size 8, buildings 8, undo record 29 (v3), homeless 4 (v4),
// labour and finance 149 (v5), walkers 8. Older versions lack the blocks added later.
namespace {

constexpr std::size_t kWalkersBlock = 8;
constexpr std::size_t kEconomy = 149;
constexpr std::size_t kHomeless = 4;
constexpr std::size_t kUndo = 29;
constexpr std::size_t kCityBase = 16;

void check_migration(std::uint8_t version, std::vector<std::uint8_t> bytes, std::size_t erase_before_walkers,
                     bool drop_city_and_walkers, std::uint64_t expected_hash) {
    if (drop_city_and_walkers) {
        bytes.resize(bytes.size() - (kCityBase + kUndo + kHomeless + kEconomy + kWalkersBlock));
    } else {
        const auto end = bytes.end() - static_cast<std::ptrdiff_t>(kWalkersBlock);
        bytes.erase(end - static_cast<std::ptrdiff_t>(erase_before_walkers), end);
    }
    bytes[4] = version;
    std::string error;
    auto b = Simulation::load(bo::test::catalog(), bytes, error);
    REQUIRE(b.has_value());
    CHECK(error.empty());
    CHECK(b->state_hash() == expected_hash);
}

}  // namespace

TEST_CASE("older saves (v1-v4) migrate to the current format") {
    Simulation a(bo::test::catalog(), SimulationConfig{3, Difficulty::Hero, 100});
    a.step(10);
    const auto current = a.save();
    check_migration(4, current, kEconomy, false, a.state_hash());
    check_migration(3, current, kEconomy + kHomeless, false, a.state_hash());
    check_migration(2, current, kEconomy + kHomeless + kUndo, false, a.state_hash());
    check_migration(1, current, 0, true, a.state_hash());
}
