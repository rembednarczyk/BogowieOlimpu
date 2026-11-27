// The expansion's husbandry and workshops (P:7-8, P:18-19; docs/gameplay/04, 05, 15; data/buildings.json,
// data/production.json): cattle in a corral, orange trees, the hunting lodge, chariots and their stall.
#include "bo/core/logistics.h"
#include "bo/core/simulation.h"
#include "bo/core/storage.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// Meadow in the north-west (x < 12, y < 6), main street on row 6.
Simulation farm(bool atlantean = false) {
    std::vector<std::string> r(14, std::string(40, '.'));
    for (std::size_t y = 0; y < 6; ++y) {
        for (std::size_t x = 0; x < 12; ++x) {
            r[y][x] = ',';
        }
    }
    r[6] = std::string(40, '=');
    r[2][30] = 'd';  // deer
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{4, Difficulty::Hero, 9000, r, true});
    sim.mutable_state().city.atlantean = atlantean;
    return sim;
}

std::uint32_t stock(const Simulation& sim, const char* good) { return city_stock(sim.state().city, sim.catalog(), good); }

std::string refusal(Simulation& sim) {
    std::string r;
    for (const auto& e : sim.drain_events()) {
        if (const auto* c = std::get_if<CommandRejected>(&e.event)) {
            r = c->reason_key;
        }
    }
    return r;
}

}  // namespace

TEST_CASE("expansion: cattle need a corral and bring meat to the granary (P:7)") {
    auto sim = farm();
    sim.issue(PlaceBuilding{"cattle", 2, 2});
    sim.step(1);
    CHECK(refusal(sim) == "build.needs_building");
    sim.issue(PlaceBuilding{"corral", 14, 2});
    sim.issue(PlaceBuilding{"granary", 22, 2});
    sim.step(1);
    for (int i = 0; i < 8; ++i) {
        sim.issue(PlaceBuilding{"cattle", 2 + i, 2});
    }
    sim.step(1);
    CHECK(refusal(sim).empty());
    sim.run_days(16 * 24);
    MESSAGE("meat in two years: " << stock(sim, "meat"));
    CHECK(stock(sim, "meat") >= 12);  // 12-14 loads a year once grown (data/production.json corral)
}

TEST_CASE("expansion: orange tenders pick the oranges for the granary (P:8)") {
    auto sim = farm();
    sim.issue(PlaceBuilding{"orange_tenders_lodge", 14, 4});
    sim.issue(PlaceBuilding{"granary", 22, 2});
    for (int i = 0; i < 8; ++i) {
        sim.issue(PlaceBuilding{"orange_tree", 2 + i, 3});
    }
    sim.step(1);
    CHECK(refusal(sim).empty());
    sim.run_days(16 * 24);
    MESSAGE("oranges in two years: " << stock(sim, "oranges"));
    CHECK(stock(sim, "oranges") >= 4);
}

TEST_CASE("expansion: the hunting lodge brings venison from the deer (P:8)") {
    auto sim = farm();
    sim.issue(PlaceBuilding{"hunting_lodge", 26, 4});
    sim.issue(PlaceBuilding{"granary", 32, 2});
    sim.step(1);
    CHECK(refusal(sim).empty());
    sim.run_days(16 * 12);
    MESSAGE("meat in a year: " << stock(sim, "meat"));
    CHECK(stock(sim, "meat") >= 6);
}

TEST_CASE("expansion: a chariot factory makes chariots of wood and horses, only in Atlantis (P:18)") {
    auto greek = farm();
    greek.issue(PlaceBuilding{"chariot_factory", 20, 2});
    greek.step(1);
    CHECK(refusal(greek) == "build.atlantean_only");
    auto sim = farm(true);
    sim.issue(PlaceBuilding{"chariot_factory", 20, 2});
    sim.step(1);
    CHECK(refusal(sim).empty());
    auto& p = *sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({20, 2}).building).producer;
    p.units["wood"] = 400;
    p.units["horses"] = 400;
    sim.run_days(16 * 6);
    const Building& f = sim.state().city.buildings.items.at(sim.state().city.map.at({20, 2}).building);
    CHECK(f.producer->output_loads >= 1);
}

TEST_CASE("expansion: the corral's bull is never slaughtered; a head taken away is not replaced (P:7, L-132)") {
    auto sim = farm();
    sim.issue(PlaceBuilding{"corral", 14, 2});
    sim.issue(PlaceBuilding{"granary", 22, 2});
    sim.step(1);
    for (int i = 0; i < 3; ++i) {
        sim.issue(PlaceBuilding{"cattle", 2 + i, 2});
    }
    sim.step(1);
    std::vector<BuildingId> herd;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        if (b.animal) {
            herd.push_back(id);
        }
    }
    REQUIRE(herd.size() == 3);
    sim.mutable_state().city.buildings.items.erase(herd.back());  // as if the priests took it (P:7)
    sim.run_days(16 * 24);
    std::size_t left = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        left += b.animal ? 1 : 0;
    }
    CHECK(left == 2);  // no calves of its own: the herd is restocked by hand
    // Two years: the one cow slaughtered twice (2 loads each), the bull never.
    CHECK(stock(sim, "meat") == 4);
    CHECK(sim.state().city.buildings.items.at(herd.front()).animal->days_to_ready == 0);  // the bull, ready, kept
}

TEST_CASE("expansion: the chariot stall takes chariots from the factory; an estate keeps them as its horses (P:19)") {
    std::vector<std::string> rows(20, std::string(40, '.'));
    rows[6] = std::string(40, '=');
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{6, Difficulty::Hero, 20000, rows, true});
    sim.mutable_state().city.atlantean = true;
    sim.issue(PlaceBuilding{"chariot_factory", 26, 2});
    sim.issue(PlaceBuilding{"common_agora", 10, 4, 1});
    sim.issue(PlaceBuilding{"storehouse", 30, 7});
    sim.step(1);
    const Building& agora = sim.state().city.buildings.items.at(sim.state().city.map.at({10, 4}).building);
    REQUIRE(agora.slot_origins.size() == 3);
    sim.issue(PlaceBuilding{"chariot_vendor_stall", agora.slot_origins[0].x, agora.slot_origins[0].y});
    auto& loads = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({30, 7}).building).storage->loads;
    for (const char* g : {"wine", "olive_oil", "fleece", "armor", "horses", "wheat"}) {
        loads[g] = 20;  // an estate is built only with its goods in store
    }
    sim.issue(PlaceBuilding{"elite_housing", 16, 7});
    sim.step(1);
    CHECK(refusal(sim).empty());
    auto& factory = *sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({26, 2}).building).producer;
    factory.output_loads = 2;
    sim.step(kTicksPerMonth);
    const BuildingId stall_id = sim.state().city.buildings.items.at(sim.state().city.map.at({10, 4}).building).slots[0];
    CHECK(sim.state().city.buildings.items.at(stall_id).stall->units.at("chariot") == 2);
    // The peddler at the estate: its horse slots fill with chariots.
    const BuildingId estate = sim.state().city.map.at({16, 7}).building;
    REQUIRE(estate != kNoBuilding);
    HouseState& h = *sim.mutable_state().city.buildings.items.at(estate).house;
    h.population = 10;
    h.level = static_cast<std::uint8_t>(sim.catalog().elite_levels.size() - 1);
    h.goods[static_cast<std::size_t>(HouseGood::Horses)] = 0;
    Walker peddler;
    peddler.home = sim.state().city.map.at({10, 4}).building;
    peddler.tile = {17, 6};
    peddler_sells(sim.mutable_state(), sim.catalog(), peddler);
    CHECK(h.goods[static_cast<std::size_t>(HouseGood::Horses)] > 0);
    CHECK(sim.state().city.buildings.items.at(stall_id).stall->units.at("chariot") < 2);
}
