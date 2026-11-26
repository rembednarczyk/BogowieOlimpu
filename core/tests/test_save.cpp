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
// wind 1 (v12), spawn herds 4 (v11), labour 85 (v5), migration 13 (v6), finance 64 (v5), walkers 8. Older versions lack the
// blocks added later.
namespace {

constexpr std::size_t kWalkersBlock = 8;
constexpr std::size_t kFinance = 64;
constexpr std::size_t kMigration = 13;
constexpr std::size_t kLabour = 85;
constexpr std::size_t kHomeless = 4;
constexpr std::size_t kSpawns = 4;
constexpr std::size_t kWind = 1;
constexpr std::size_t kUndo = 29;
constexpr std::size_t kCityBase = 16;

// Removes `len` bytes that end `from_end` bytes before the end of the save.
void cut(std::vector<std::uint8_t>& bytes, std::size_t from_end, std::size_t len) {
    const auto end = bytes.end() - static_cast<std::ptrdiff_t>(from_end);
    bytes.erase(end - static_cast<std::ptrdiff_t>(len), end);
}

void check_load(std::vector<std::uint8_t> bytes, std::uint8_t version, std::uint64_t expected_hash) {
    bytes[4] = version;
    std::string error;
    auto b = Simulation::load(bo::test::catalog(), bytes, error);
    REQUIRE(b.has_value());
    CHECK(error.empty());
    CHECK(b->state_hash() == expected_hash);
}

}  // namespace

TEST_CASE("older saves (v1-v6) migrate to the current format") {
    Simulation a(bo::test::catalog(), SimulationConfig{3, Difficulty::Hero, 100});
    a.step(10);
    const auto current = a.save();
    const std::uint64_t h = a.state_hash();
    auto v11 = current;
    cut(v11, kWalkersBlock + kFinance + kMigration + kLabour + kSpawns, kWind);
    check_load(v11, 11, h);
    auto v10 = v11;
    cut(v10, kWalkersBlock + kFinance + kMigration + kLabour, kSpawns);
    check_load(v10, 10, h);
    check_load(v10, 6, h);  // v7-v10 add only per-tile, per-building and per-walker fields
    auto v5 = v10;
    cut(v5, kWalkersBlock + kFinance, kMigration);
    check_load(v5, 5, h);
    auto v4 = v5;
    cut(v4, kWalkersBlock, kLabour + kFinance);
    check_load(v4, 4, h);
    auto v3 = v4;
    cut(v3, kWalkersBlock, kHomeless);
    check_load(v3, 3, h);
    auto v2 = v3;
    cut(v2, kWalkersBlock, kUndo);
    check_load(v2, 2, h);
    auto v1 = v2;
    v1.resize(v1.size() - kCityBase - kWalkersBlock);
    check_load(v1, 1, h);
}
