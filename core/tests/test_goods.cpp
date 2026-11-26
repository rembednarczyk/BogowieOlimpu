// Modules C4/C5 stage 1: storage, farms, deliverymen, vendors, peddlers (docs/gameplay/04, 06, 19, 24).
#include "bo/core/housing.h"
#include "bo/core/logistics.h"
#include "bo/core/simulation.h"
#include "bo/core/storage.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// Meadow on the left for farms, a main street on row 6.
std::vector<std::string> valley() {
    std::vector<std::string> r(14, std::string(40, '.'));
    for (std::size_t y = 0; y < 6; ++y) {
        for (std::size_t x = 0; x < 8; ++x) {
            r[y][x] = ',';
        }
    }
    r[6] = std::string(40, '=');
    return r;
}

Building& at(Simulation& sim, TilePos p) {
    return sim.mutable_state().city.buildings.items.at(sim.state().city.map.at(p).building);
}

}  // namespace

TEST_CASE("storage: 8 slots of 4 loads; one good per slot; sculptures take a slot each (doc 19)") {
    Simulation sim(bo::test::catalog_no_migration(), SimulationConfig{1, Difficulty::Hero, 5000, valley(), true});
    sim.issue(PlaceBuilding{"storehouse", 10, 3});
    sim.step(1);
    Building& s = at(sim, {10, 3});
    const auto& cat = bo::test::catalog();
    CHECK(storage_room(s, cat, "fleece") == 32);
    s.storage->loads["fleece"] = 5;  // two slots
    CHECK(storage_room(s, cat, "fleece") == 3 + 24);
    CHECK(storage_room(s, cat, "wood") == 24);
    s.storage->loads["sculpture"] = 2;
    CHECK(storage_room(s, cat, "wood") == 16);
    CHECK(storage_room(s, cat, "sculpture") == 4);
}

TEST_CASE("default orders: Granary takes food only when fully staffed; Storehouse refuses wheat (Z:66-68)") {
    Simulation sim(bo::test::catalog_no_migration(), SimulationConfig{1, Difficulty::Hero, 5000, valley(), true});
    sim.issue(PlaceBuilding{"granary", 10, 2});
    sim.issue(PlaceBuilding{"storehouse", 16, 3});
    sim.step(52);
    const auto& cat = bo::test::catalog();
    Building& g = at(sim, {10, 2});
    Building& s = at(sim, {16, 3});
    CHECK(storage_accepts(g, cat, "carrots"));
    CHECK_FALSE(storage_accepts(g, cat, "fleece"));
    CHECK_FALSE(storage_accepts(s, cat, "wheat"));
    CHECK(storage_accepts(s, cat, "fleece"));
    g.staffing_percent = 90;
    CHECK_FALSE(storage_accepts(g, cat, "carrots"));
}

TEST_CASE("a farm staffed all year harvests 8 loads in its harvest month and delivers them one by one") {
    Simulation sim(bo::test::catalog_no_migration(), SimulationConfig{1, Difficulty::Hero, 5000, valley(), true});
    sim.issue(PlaceBuilding{"carrot_farm", 0, 3});  // harvest in month 4 (index 3)
    sim.issue(PlaceBuilding{"granary", 30, 2});
    sim.step(1);
    sim.run_days(16 * 12 + 16 * 4);  // one full year of growth, then into the next harvest month
    const Building& g = at(sim, {30, 2});
    std::uint32_t delivered = 0;
    for (const auto& [good, loads] : g.storage->loads) {
        delivered += loads;
    }
    const Building& farm = at(sim, {0, 3});
    CHECK(delivered + farm.producer->output_loads + (farm.producer->deliveryman != kNoWalker ? 1U : 0U) >= 8);
    CHECK(delivered >= 1);
}

TEST_CASE("food vendor fills its stall from a Granary within reach; the peddler feeds huts into shacks") {
    Simulation sim(bo::test::catalog_no_migration(), SimulationConfig{1, Difficulty::Hero, 5000, valley(), true});
    sim.issue(PlaceBuilding{"granary", 20, 2});
    // Common agora over the main street: road lane is row 6 (rotation 1: long axis along x).
    sim.issue(PlaceBuilding{"common_agora", 10, 4, 1});
    sim.issue(PlaceBuilding{"common_housing", 12, 7});
    sim.step(1);
    REQUIRE(sim.state().city.map.at({10, 4}).building != kNoBuilding);
    const Building& agora = at(sim, {10, 4});
    REQUIRE(agora.slot_origins.size() == 3);
    sim.issue(PlaceBuilding{"food_vendor_stall", agora.slot_origins[0].x, agora.slot_origins[0].y});
    sim.step(1);
    at(sim, {20, 2}).storage->loads["carrots"] = 6;
    at(sim, {12, 7}).house->population = 8;
    sim.run_days(4);
    const Building& stall = sim.state().city.buildings.items.at(at(sim, {10, 4}).slots[0]);  // stalls own no tiles
    REQUIRE(stall.stall);
    CHECK(at(sim, {20, 2}).storage->loads["carrots"] == 2);  // the vendor took 4 loads (L-100)
    sim.run_days(10);
    CHECK(at(sim, {12, 7}).house->goods[static_cast<std::size_t>(HouseGood::Food)] > 0);
    CHECK(at(sim, {12, 7}).house->level >= 1);  // shack
}

TEST_CASE("house target: two months of the food of the current or next level (L-100)") {
    const auto& cat = bo::test::catalog();
    HouseState hut;
    CHECK(house_target(cat, hut, HouseGood::Food, Difficulty::Hero) == 8);  // shack eats 4 a month
    CHECK(house_target(cat, hut, HouseGood::Fleece, Difficulty::Hero) == 0);
    HouseState hovel;
    hovel.level = 2;
    CHECK(house_target(cat, hovel, HouseGood::Fleece, Difficulty::Hero) == 4);  // homestead needs fleece
}

TEST_CASE("elite plots need 2 food, 1 fleece and 1 olive oil in storage (doc 18)") {
    Simulation sim(bo::test::catalog_no_migration(), SimulationConfig{1, Difficulty::Hero, 5000, valley(), true});
    sim.issue(PlaceBuilding{"granary", 10, 2});
    sim.issue(PlaceBuilding{"storehouse", 16, 3});
    sim.step(1);
    at(sim, {10, 2}).storage->loads["fish"] = 2;
    at(sim, {16, 3}).storage->loads["fleece"] = 1;
    sim.issue(PlaceBuilding{"elite_housing", 22, 2});
    sim.step(1);
    CHECK(sim.state().city.map.at({22, 2}).building == kNoBuilding);
    at(sim, {16, 3}).storage->loads["olive_oil"] = 1;
    sim.issue(PlaceBuilding{"elite_housing", 22, 2});
    sim.step(1);
    CHECK(sim.state().city.map.at({22, 2}).building != kNoBuilding);
}

TEST_CASE("whole chain on real labour and migration: farm, granary, agora, fountain, gymnasium -> hovels") {
    std::vector<std::string> r(16, std::string(48, '.'));
    for (std::size_t y = 0; y < 6; ++y) {
        for (std::size_t x = 0; x < 9; ++x) {
            r[y][x] = ',';
        }
    }
    r[6] = std::string(48, '=');
    Simulation sim(bo::test::catalog(), SimulationConfig{9, Difficulty::Hero, 8000, r});
    sim.issue(PlaceBuilding{"carrot_farm", 0, 3});
    sim.issue(PlaceBuilding{"onion_farm", 3, 3});
    sim.issue(PlaceBuilding{"wheat_farm", 6, 3});
    sim.issue(PlaceBuilding{"granary", 40, 2});
    sim.issue(PlaceBuilding{"common_agora", 20, 4, 1});
    sim.issue(PlaceBuilding{"fountain", 18, 7});
    sim.issue(PlaceBuilding{"gymnasium", 27, 7});
    for (int x = 12; x < 40; x += 2) {
        if (x < 18 || x > 30) {
            sim.issue(PlaceBuilding{"common_housing", x, 4});
        }
        sim.issue(PlaceBuilding{"common_housing", x, 7});  // overlaps with the fountain and gymnasium are refused
    }
    sim.step(1);
    const auto& agora = sim.state().city.buildings.items.at(sim.state().city.map.at({20, 4}).building);
    sim.issue(PlaceBuilding{"food_vendor_stall", agora.slot_origins[0].x, agora.slot_origins[0].y});
    // The Granary takes deliveries only when fully staffed (Z:66): the player raises distribution first.
    sim.issue(SetSectorPriority{static_cast<std::uint8_t>(Sector::Distribution), 5});
    sim.run_days(16 * 12 * 2);
    int hovels = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        hovels += b.house && b.house->level >= 2 ? 1 : 0;
    }
    CHECK(hovels > 0);
}

TEST_CASE("save v7: storage, stalls and walkers with cargo survive a round trip") {
    Simulation a(bo::test::catalog_no_migration(), SimulationConfig{1, Difficulty::Hero, 5000, valley(), true});
    a.issue(PlaceBuilding{"granary", 20, 2});
    a.issue(PlaceBuilding{"common_agora", 10, 4, 1});
    a.issue(PlaceBuilding{"carrot_farm", 0, 3});
    a.step(1);
    const auto slot = a.state().city.buildings.items.at(a.state().city.map.at({10, 4}).building).slot_origins[0];
    a.issue(PlaceBuilding{"food_vendor_stall", slot.x, slot.y});
    a.step(1);
    at(a, {20, 2}).storage->loads["carrots"] = 6;
    at(a, {20, 2}).storage->orders["onions"] = StorageOrder{StorageOrderKind::DontAccept, 0};
    at(a, {0, 3}).producer->output_loads = 2;
    a.run_days(1);
    a.step(40);
    bool cargo = false;
    for (const auto& [id, w] : a.state().walkers.items) {
        cargo = cargo || !w.cargo.empty() || w.job != WalkerJob::None;
    }
    CHECK(cargo);
    std::string error;
    auto b = Simulation::load(bo::test::catalog_no_migration(), a.save(), error);
    REQUIRE(b);
    CHECK(a.state_hash() == b->state_hash());
    a.run_days(20);
    b->run_days(20);
    CHECK(a.state_hash() == b->state_hash());
}

TEST_CASE("storage orders: Don't Accept stops deliveries; Get and Empty wait for stage 2") {
    Simulation sim(bo::test::catalog_no_migration(), SimulationConfig{1, Difficulty::Hero, 5000, valley(), true});
    sim.issue(PlaceBuilding{"granary", 10, 2});
    sim.step(52);
    const BuildingId g = sim.state().city.map.at({10, 2}).building;
    sim.issue(SetStorageOrder{g, "carrots", 3, 0});
    sim.issue(SetStorageOrder{g, "fish", 2, 0});
    sim.step(1);
    CHECK_FALSE(storage_accepts(sim.state().city.buildings.items.at(g), bo::test::catalog(), "carrots"));
    CHECK(storage_accepts(sim.state().city.buildings.items.at(g), bo::test::catalog(), "fish"));
}

TEST_CASE("whole chain with fleece and gardens: houses reach homestead (Z:30-31)") {
    std::vector<std::string> r(18, std::string(48, '.'));
    for (std::size_t y = 0; y < 6; ++y) {
        for (std::size_t x = 0; x < 12; ++x) {
            r[y][x] = ',';
        }
    }
    r[6] = std::string(48, '=');
    Simulation sim(bo::test::catalog(), SimulationConfig{9, Difficulty::Hero, 20000, r});
    sim.issue(PlaceBuilding{"carrot_farm", 0, 3});
    sim.issue(PlaceBuilding{"onion_farm", 3, 3});
    sim.issue(PlaceBuilding{"carding_shed", 9, 4});
    sim.issue(PlaceBuilding{"granary", 42, 2});
    sim.issue(PlaceBuilding{"storehouse", 38, 3});
    sim.issue(PlaceBuilding{"common_agora", 20, 4, 1});
    sim.issue(PlaceBuilding{"fountain", 18, 7});
    sim.issue(PlaceBuilding{"gymnasium", 27, 7});
    for (int x = 12; x < 18; x += 2) {
        sim.issue(PlaceBuilding{"common_housing", x, 4});
        sim.issue(PlaceBuilding{"common_housing", x, 7});
        sim.issue(PlaceBuilding{"flower_garden", x, 9});
    }
    for (int x = 30; x < 38; x += 2) {
        sim.issue(PlaceBuilding{"common_housing", x, 4});
        sim.issue(PlaceBuilding{"common_housing", x, 7});
        sim.issue(PlaceBuilding{"flower_garden", x, 9});
    }
    sim.step(1);
    for (int i = 0; i < 8; ++i) {
        sim.issue(PlaceBuilding{"sheep", 6 + (i % 3), 1});
    }
    const auto& agora = sim.state().city.buildings.items.at(sim.state().city.map.at({20, 4}).building);
    sim.issue(PlaceBuilding{"food_vendor_stall", agora.slot_origins[0].x, agora.slot_origins[0].y});
    sim.issue(PlaceBuilding{"fleece_vendor_stall", agora.slot_origins[1].x, agora.slot_origins[1].y});
    sim.issue(SetSectorPriority{static_cast<std::uint8_t>(Sector::Distribution), 5});
    sim.run_days(16 * 12 * 3);
    int homesteads = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        homesteads += b.house && b.house->level >= 3 ? 1 : 0;
    }
    CHECK(homesteads > 0);
}
