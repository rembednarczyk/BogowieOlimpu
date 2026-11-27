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
// wind 1 (v12), hygiene 1 (v13), spawn herds 4 (v11), labour 85 (v5), migration 13 (v6), trade 16 (v14),
// finance 64 (v5), walkers 8. Older versions lack the
// blocks added later.
namespace {

constexpr std::size_t kWalkersBlock = 8;
constexpr std::size_t kFinance = 64;
constexpr std::size_t kMigration = 13;
constexpr std::size_t kLabour = 85;
constexpr std::size_t kHomeless = 4;
constexpr std::size_t kSpawns = 4;
constexpr std::size_t kWind = 1;
constexpr std::size_t kHygiene = 1;
constexpr std::size_t kTrade = 16;
constexpr std::size_t kUndo = 29;
constexpr std::size_t kCityBase = 16;
constexpr std::size_t kInvasion = 17;  // v18: no invasion (flag, empty nation, no invaders, origin)
constexpr std::size_t kWorldTail = 4 + 9;  // v21: invasion city, then the world block (no partners, no armies)
constexpr std::size_t kMilitary = 4 + kInvasion;  // v17: company count, after the walkers

constexpr std::size_t kScenario = 17;  // v22: empty id, no goals, events or requests, not won

constexpr std::size_t kMonsters = 8;  // v24: no monsters, no heroes

constexpr std::size_t kQuests = 8;  // v26: no quests, no monuments earned

constexpr std::size_t kAwarded = 4;  // v33: no milestones rewarded (the end of the scenario block)

constexpr std::size_t kDeliveries = 4;  // v27: no goods on their way (end of the world block)

constexpr std::size_t kDebt = 4;  // v29: days of debt (v30 adds per-partner fields only: none here)

constexpr std::size_t kGodHelp = 5;  // v31: Zeus and Ares flags, Talos days

constexpr std::size_t kTradeCurses = 8;  // v32: caravan and ship curse days

constexpr std::size_t kStockpiled = 4;  // v35: no goods kept

constexpr std::size_t kTowersManned = 1;  // v36: Man All flag

constexpr std::size_t kHostYear = 4;  // v37: the year to host the games (the end of the world block)

constexpr std::size_t kSea = 13;  // v39: invasion by sea flag, landing tile, no ships (end of the army block)
constexpr std::size_t kWorldBlock = 9 + kDeliveries + kDebt + kGodHelp + kTradeCurses + kStockpiled + kTowersManned +
                                    kHostYear;  // the world block of an empty city, after the army block

// The save without the v39 fleet fields: the layout of v37 and v38 (v38 adds only per-company fields).
std::vector<std::uint8_t> as_v38(std::vector<std::uint8_t> bytes) {
    const auto end = bytes.end() - static_cast<std::ptrdiff_t>(kWorldBlock + kScenario + kQuests + kAwarded + kMonsters);
    bytes.erase(end - static_cast<std::ptrdiff_t>(kSea), end);
    return bytes;
}

// The save without the v37 hosting year: the layout of v36.
std::vector<std::uint8_t> as_v36(std::vector<std::uint8_t> bytes) {
    bytes = as_v38(std::move(bytes));
    const auto end = bytes.end() - static_cast<std::ptrdiff_t>(kScenario + kQuests + kAwarded + kMonsters);
    bytes.erase(end - static_cast<std::ptrdiff_t>(kHostYear), end);
    return bytes;
}

// The save without the v36 Man All flag: the layout of v35.
std::vector<std::uint8_t> as_v35(std::vector<std::uint8_t> bytes) {
    bytes = as_v36(std::move(bytes));
    const auto end = bytes.end() - static_cast<std::ptrdiff_t>(kScenario + kQuests + kAwarded + kMonsters);
    bytes.erase(end - static_cast<std::ptrdiff_t>(kTowersManned), end);
    return bytes;
}

// The save without the v35 stockpile: the layout of v33 and v34 (v34 adds only per-company fields).
std::vector<std::uint8_t> as_v34(std::vector<std::uint8_t> bytes) {
    bytes = as_v35(std::move(bytes));
    const auto end = bytes.end() - static_cast<std::ptrdiff_t>(kScenario + kQuests + kAwarded + kMonsters);
    bytes.erase(end - static_cast<std::ptrdiff_t>(kStockpiled), end);
    return bytes;
}

// The save without the v33 milestone record: the layout of v32.
std::vector<std::uint8_t> as_v32(std::vector<std::uint8_t> bytes) {
    bytes = as_v34(std::move(bytes));
    const auto end = bytes.end() - static_cast<std::ptrdiff_t>(kMonsters);
    bytes.erase(end - static_cast<std::ptrdiff_t>(kAwarded), end);
    return bytes;
}

// The save without the v32 trade curses: the layout of v31.
std::vector<std::uint8_t> as_v31(std::vector<std::uint8_t> bytes) {
    bytes = as_v32(std::move(bytes));
    const auto end = bytes.end() - static_cast<std::ptrdiff_t>(kScenario + kQuests + kMonsters);
    bytes.erase(end - static_cast<std::ptrdiff_t>(kTradeCurses), end);
    return bytes;
}

// The save without the v31 help of the gods: the layout of v29 and v30.
std::vector<std::uint8_t> as_v30(std::vector<std::uint8_t> bytes) {
    bytes = as_v31(std::move(bytes));
    const auto end = bytes.end() - static_cast<std::ptrdiff_t>(kScenario + kQuests + kMonsters);
    bytes.erase(end - static_cast<std::ptrdiff_t>(kGodHelp), end);
    return bytes;
}

// The save without the v29 debt days: the layout of v27 and v28 (v28 adds only per-building fields).
std::vector<std::uint8_t> as_v28(std::vector<std::uint8_t> bytes) {
    bytes = as_v30(std::move(bytes));
    const auto end = bytes.end() - static_cast<std::ptrdiff_t>(kScenario + kQuests + kMonsters);
    bytes.erase(end - static_cast<std::ptrdiff_t>(kDebt), end);
    return bytes;
}

// The save without the v27 deliveries: the layout of v26.
std::vector<std::uint8_t> as_v26(std::vector<std::uint8_t> bytes) {
    bytes = as_v28(std::move(bytes));
    const auto end = bytes.end() - static_cast<std::ptrdiff_t>(kScenario + kQuests + kMonsters);
    bytes.erase(end - static_cast<std::ptrdiff_t>(kDeliveries), end);
    return bytes;
}

// The save without the v26 quest fields: the layout of v25.
std::vector<std::uint8_t> as_v25(std::vector<std::uint8_t> bytes) {
    bytes = as_v26(std::move(bytes));
    const auto end = bytes.end() - static_cast<std::ptrdiff_t>(kMonsters);
    bytes.erase(end - static_cast<std::ptrdiff_t>(kQuests), end);
    return bytes;
}

// The save without the v24 monster block: the layout of v23.
std::vector<std::uint8_t> as_v23(std::vector<std::uint8_t> bytes) {
    bytes = as_v25(std::move(bytes));
    bytes.resize(bytes.size() - kMonsters);
    return bytes;
}

// The save without the v22 scenario block and later: the layout of v21.
std::vector<std::uint8_t> as_v21(std::vector<std::uint8_t> bytes) {
    bytes = as_v23(std::move(bytes));
    bytes.resize(bytes.size() - kScenario);
    return bytes;
}

// The save without the v21 and v22 tails: the layout of v20.
std::vector<std::uint8_t> as_v20(std::vector<std::uint8_t> bytes) {
    bytes = as_v21(std::move(bytes));
    bytes.resize(bytes.size() - kWorldTail);
    return bytes;
}

// The save without the v17 army block (no companies): the layout of v16 and older.
std::vector<std::uint8_t> without_military(std::vector<std::uint8_t> bytes) {
    bytes = as_v20(std::move(bytes));
    bytes.resize(bytes.size() - kMilitary);
    return bytes;
}

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
    const std::uint64_t h = a.state_hash();
    check_load(a.save(), 40, h);  // v41 adds only a per-sanctuary field
    check_load(a.save(), 39, h);  // v40 adds only a per-company field
    check_load(as_v38(a.save()), 38, h);
    check_load(as_v38(a.save()), 37, h);
    check_load(as_v36(a.save()), 36, h);
    check_load(as_v35(a.save()), 35, h);
    check_load(as_v34(a.save()), 34, h);
    check_load(as_v34(a.save()), 33, h);
    check_load(as_v32(a.save()), 32, h);
    check_load(as_v31(a.save()), 31, h);
    check_load(as_v30(a.save()), 30, h);
    check_load(as_v30(a.save()), 29, h);
    check_load(as_v28(a.save()), 28, h);
    check_load(as_v28(a.save()), 27, h);
    check_load(as_v26(a.save()), 26, h);
    check_load(as_v25(a.save()), 25, h);
    check_load(as_v23(a.save()), 23, h);
    check_load(as_v21(a.save()), 21, h);
    check_load(as_v20(a.save()), 20, h);
    auto v17 = as_v20(a.save());
    v17.resize(v17.size() - kInvasion);
    check_load(v17, 17, h);
    const auto current = without_military(a.save());
    check_load(current, 16, h);
    check_load(current, 14, h);  // v15 adds only a house field, v16 only a spawn field
    auto v13 = current;
    cut(v13, kWalkersBlock + kFinance, kTrade);
    check_load(v13, 13, h);
    auto v12 = v13;
    cut(v12, kWalkersBlock + kFinance + kMigration + kLabour + kSpawns, kHygiene);
    check_load(v12, 12, h);
    auto v11 = v12;
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

TEST_CASE("save v15: spawn points load without the hunting wolf field (v16)") {
    std::vector<std::string> rows(6, std::string(6, '.'));
    rows[2][2] = 'w';
    Simulation a(bo::test::catalog(), SimulationConfig{3, Difficulty::Hero, 100, rows});
    a.run_days(2);  // the den gets its pack; no prey, so no wolf is out
    REQUIRE(a.state().city.spawns.size() == 1);
    REQUIRE(a.state().walkers.items.empty());
    auto v15 = without_military(a.save());
    cut(v15, kWalkersBlock + kFinance + kTrade + kMigration + kLabour, 4);  // the hunter id of the one den
    check_load(v15, 15, a.state_hash());
}

TEST_CASE("B-001: burning days and quarantine survive a save (read in the order they are written)") {
    std::vector<std::string> rows(10, std::string(20, '.'));
    rows[5] = std::string(20, '=');
    Simulation a(bo::test::catalog_isolated(), SimulationConfig{4, Difficulty::Hero, 5000, rows});
    a.issue(PlaceBuilding{"common_housing", 2, 3});
    a.issue(PlaceBuilding{"granary", 10, 1});
    a.step(1);
    for (auto& [id, b] : a.mutable_state().city.buildings.items) {
        if (b.house) {
            b.house->quarantine_days = 1;  // a value that the misread turned into 256
        } else {
            b.burning_days = 3;
        }
    }
    std::string error;
    auto b = Simulation::load(bo::test::catalog_isolated(), a.save(), error);
    REQUIRE(b.has_value());
    CHECK(b->state().city.buildings.items == a.state().city.buildings.items);
    CHECK(b->state_hash() == a.state_hash());
}
