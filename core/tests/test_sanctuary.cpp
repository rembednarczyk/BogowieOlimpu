// Module D1 stage 1: sanctuary construction and sacrifices (docs/gameplay/10, 21; Z:116-118, Z:124; L-111).
#include "bo/core/sanctuary.h"
#include "bo/core/serialize.h"
#include "bo/core/simulation.h"
#include "bo/core/storage.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// 60x40: a main street on row 20, a meadow south-east of it.
Simulation town() {
    std::vector<std::string> rows(40, std::string(60, '.'));
    rows[20] = std::string(60, '=');
    for (std::size_t y = 22; y < 28; ++y) {
        for (std::size_t x = 30; x < 40; ++x) {
            rows[y][x] = ',';
        }
    }
    return Simulation(bo::test::catalog_isolated(), SimulationConfig{9, Difficulty::Hero, 30000, rows, true});
}

Building& at(Simulation& sim, TilePos p) {
    return sim.mutable_state().city.buildings.items.at(sim.state().city.map.at(p).building);
}

std::string rejection(Simulation& sim) {
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
            return r->reason_key;
        }
    }
    return {};
}

// A storehouse at (30, 17) holding the given loads (set directly; capacity is not the point here).
void stock(Simulation& sim, std::uint32_t marble, std::uint32_t wood = 0, std::uint32_t sculptures = 0) {
    if (sim.state().city.map.at({30, 17}).building == kNoBuilding) {
        sim.issue(PlaceBuilding{"storehouse", 30, 17});
        sim.step(1);
    }
    auto& loads = at(sim, {30, 17}).storage->loads;
    loads["marble"] = marble;
    loads["wood"] = wood;
    loads["sculpture"] = sculptures;
}

}  // namespace

TEST_CASE("a sanctuary needs its initial marble in storage, which the foundation uses up (Z:116, doc 21)") {
    auto sim = town();
    sim.issue(PlaceBuilding{"sanctuary_artemis", 2, 10});
    sim.step(1);
    CHECK(rejection(sim) == "build.sanctuary_needs_marble");
    stock(sim, 20);  // Artemis: 20 marble to start
    sim.issue(PlaceBuilding{"sanctuary_artemis", 2, 10});
    sim.step(1);
    REQUIRE(sim.state().city.map.at({2, 10}).building != kNoBuilding);
    const auto& s = *at(sim, {2, 10}).sanctuary;
    CHECK_FALSE(s.complete);
    CHECK(s.needed.at("marble") == 63);
    CHECK(s.needed.at("wood") == 20);
    CHECK(s.needed.at("sculpture") == 6);
    CHECK(city_stock(sim.state().city, bo::test::catalog(), "marble") == 0);
}

TEST_CASE("at most 4 sanctuaries in a city (S-004 trivia)") {
    auto sim = town();
    stock(sim, 100);
    int x = 0;
    for (const char* god : {"sanctuary_dionysus", "sanctuary_hermes", "sanctuary_aphrodite", "sanctuary_ares"}) {
        sim.issue(PlaceBuilding{god, x, 10});
        x += 15;
    }
    sim.step(1);
    CHECK(rejection(sim).empty());
    sim.issue(PlaceBuilding{"sanctuary_dionysus", 2, 30});
    sim.step(1);
    CHECK(rejection(sim) == "build.too_many_sanctuaries");
}

TEST_CASE("artisans build in the loads that carts bring, marble and wood 4 at a time (Z:117, L-111)") {
    auto sim = town();
    stock(sim, 30, 20, 6);  // Dionysus: 8 to start, then 35 marble, 14 wood, 2 sculptures
    sim.issue(PlaceBuilding{"sanctuary_dionysus", 2, 14});
    sim.issue(PlaceBuilding{"artisans_guild", 40, 18});
    sim.step(1);
    REQUIRE(sim.state().city.map.at({2, 14}).building != kNoBuilding);
    at(sim, {30, 17}).storage->loads["marble"] = 40;
    bool bulk = false;
    bool completed = false;
    for (int d = 0; d < 192 && !completed; ++d) {
        sim.run_days(1);
        for (const auto& [id, w] : sim.state().walkers.items) {
            const auto m = w.cargo.find("marble");
            bulk = bulk || (w.job == WalkerJob::Supply && m != w.cargo.end() && m->second == 4);
        }
        for (const auto& e : sim.drain_events()) {
            completed = completed || std::holds_alternative<SanctuaryCompleted>(e.event);
        }
    }
    CHECK(bulk);
    CHECK(completed);
    CHECK(at(sim, {2, 14}).sanctuary->complete);
    CHECK(at(sim, {30, 17}).storage->loads["marble"] == 40 - 35);
}

TEST_CASE("without artisans nothing is built (Z:117)") {
    auto sim = town();
    stock(sim, 30, 20, 6);
    sim.issue(PlaceBuilding{"sanctuary_dionysus", 2, 14});
    sim.step(1);
    at(sim, {30, 17}).storage->loads["marble"] = 40;
    sim.run_days(60);
    CHECK(at(sim, {2, 14}).sanctuary->needed.at("marble") == 35);
}

TEST_CASE("priests sacrifice a sheep, else food from a Granary; a year without one costs the favor (Z:124, L-052)") {
    auto sim = town();
    stock(sim, 30);
    sim.issue(PlaceBuilding{"sanctuary_dionysus", 2, 14});
    sim.issue(PlaceBuilding{"carding_shed", 42, 21});
    sim.step(1);
    for (auto& [good, n] : at(sim, {2, 14}).sanctuary->needed) {
        n = 0;  // built
    }
    for (int i = 0; i < 2; ++i) {
        sim.issue(PlaceBuilding{"sheep", 32, 24});
    }
    bool sacrificed = false;
    int removed = 0;
    for (int d = 0; d < 64 && !sacrificed; ++d) {
        const std::uint16_t before = at(sim, {2, 14}).sanctuary->days_since_sacrifice;
        sim.run_days(1);
        sacrificed = at(sim, {2, 14}).sanctuary->days_since_sacrifice < before;
        for (const auto& e : sim.drain_events()) {
            removed += std::holds_alternative<BuildingRemoved>(e.event) ? 1 : 0;
        }
    }
    REQUIRE(at(sim, {2, 14}).sanctuary->complete);
    CHECK(sacrificed);
    CHECK(removed == 1);  // one sheep went to the altar

    auto alone = town();
    stock(alone, 30);
    alone.issue(PlaceBuilding{"sanctuary_dionysus", 2, 14});
    alone.issue(PlaceBuilding{"granary", 45, 16});
    alone.step(1);
    for (auto& [good, n] : at(alone, {2, 14}).sanctuary->needed) {
        n = 0;
    }
    at(alone, {45, 16}).storage->loads["fish"] = 3;
    alone.run_days(64);
    CHECK(at(alone, {45, 16}).storage->loads["fish"] == 2);
    at(alone, {45, 16}).storage->loads["fish"] = 0;
    alone.run_days(kFavorLostAfterDays + 10);
    CHECK(at(alone, {2, 14}).sanctuary->favor_lost);
}

TEST_CASE("save v19: a sanctuary under construction survives a round trip with the same future") {
    auto a = town();
    stock(a, 30, 20, 6);
    a.issue(PlaceBuilding{"sanctuary_dionysus", 2, 14});
    a.issue(PlaceBuilding{"artisans_guild", 40, 18});
    a.step(1);
    at(a, {30, 17}).storage->loads["marble"] = 40;
    a.run_days(20);
    std::string error;
    auto b = Simulation::load(bo::test::catalog_isolated(), a.save(), error);
    REQUIRE(b.has_value());
    CHECK(b->state().city.buildings.items == a.state().city.buildings.items);
    a.run_days(10);
    b->run_days(10);
    CHECK(a.state_hash() == b->state_hash());
}

namespace {

// Days to finish a sanctuary with three guilds and full stock, or 0 if not within two years.
int days_to_build(const char* type, int y) {
    auto sim = town();
    stock(sim, 200, 60, 30);
    sim.issue(PlaceBuilding{type, 2, y});  // touching the main street
    for (int x = 34; x < 46; x += 4) {
        sim.issue(PlaceBuilding{"artisans_guild", x, 18});
    }
    sim.step(1);
    REQUIRE(sim.state().city.map.at({2, y}).building != kNoBuilding);
    for (int d = 1; d <= 2 * 192; ++d) {
        stock(sim, 200, 60, 30);
        sim.run_days(1);
        if (at(sim, {2, y}).sanctuary->complete) {
            return d;
        }
    }
    return 0;
}

}  // namespace

TEST_CASE("well organized building: Artemis in about 5 months, Zeus in about a year (S-005 Vriesea, L-111)") {
    const int artemis = days_to_build("sanctuary_artemis", 10);
    const int zeus = days_to_build("sanctuary_zeus", 4);
    MESSAGE("days: artemis " << artemis << ", zeus " << zeus);
    CHECK(artemis >= 4 * 16);  // S-005: 5 months; within 4-7
    CHECK(artemis <= 7 * 16);
    CHECK(zeus >= 9 * 16);  // S-005: 12 months; within 9-15
    CHECK(zeus <= 15 * 16);
}
