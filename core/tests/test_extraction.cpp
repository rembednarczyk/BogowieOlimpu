// Module C4 stage 3: extraction, mint, workshops (docs/gameplay/05, 19; S-005 rates; assumptions L-102).
#include "bo/core/logistics.h"
#include "bo/core/simulation.h"
#include "bo/core/storage.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// A road on row 6; a resource column `distance` tiles east of a building at x = 2.
std::vector<std::string> quarry_map(char resource, std::int32_t distance) {
    std::vector<std::string> r(12, std::string(48, '.'));
    r[6] = std::string(48, '=');
    for (std::size_t y = 0; y < 5; ++y) {
        r[y][static_cast<std::size_t>(3 + distance)] = resource;
    }
    return r;
}

std::uint32_t yearly_loads(const std::string& building, char resource, std::int32_t distance) {
    Simulation sim(bo::test::catalog_no_migration(),
                   SimulationConfig{2, Difficulty::Hero, 9000, quarry_map(resource, distance), true});
    sim.issue(PlaceBuilding{building, 2, 4});
    sim.issue(PlaceBuilding{"storehouse", 20, 7});
    sim.step(1);
    sim.run_days(192);
    std::uint32_t total = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        if (b.storage) {
            for (const auto& [g, l] : b.storage->loads) {
                total += l;
            }
        }
        if (b.producer) {
            total += b.producer->output_loads;
        }
    }
    return total;
}

}  // namespace

TEST_CASE("foundry output falls with the distance to the ore: about 13 / 10 / 7-8 loads a year (S-005)") {
    const auto close = yearly_loads("foundry", 'c', 2);
    const auto mid = yearly_loads("foundry", 'c', 10);
    const auto far = yearly_loads("foundry", 'c', 20);
    MESSAGE("foundry loads per year: " << close << " " << mid << " " << far);
    CHECK(close >= 11);
    CHECK(close <= 15);
    CHECK(mid >= 8);
    CHECK(mid <= 11);
    CHECK(far >= 6);
    CHECK(far <= 9);
}

TEST_CASE("a timber mill fells nearby forest and ships wood to the storehouse") {
    CHECK(yearly_loads("timber_mill", 'T', 3) >= 10);
}

TEST_CASE("masonry shops make 12 slabs a year while a quarry is within 20 tiles (L-102)") {
    CHECK(yearly_loads("masonry_shop", 'q', 5) >= 11);
    CHECK(yearly_loads("masonry_shop", 'q', 30) == 0);
}

TEST_CASE("the mint turns 200 silver into 100 drachmas for the treasury (doc 19)") {
    Simulation sim(bo::test::catalog_no_migration(), SimulationConfig{2, Difficulty::Hero, 0, quarry_map('s', 2), true});
    sim.issue(PlaceBuilding{"mint", 2, 4});
    sim.step(1);
    const std::int64_t after_build = sim.state().finance.treasury;
    sim.run_days(192);
    const std::int64_t minted = sim.state().finance.treasury - after_build;
    MESSAGE("minted in a year: " << minted);
    CHECK(minted >= 400);  // S-005: 600-700 close to the ore
    CHECK(minted % 100 == 0);
}

TEST_CASE("a sculpture studio waits for 4 bronze per sculpture (doc 19)") {
    Simulation sim(bo::test::catalog_no_migration(), SimulationConfig{2, Difficulty::Hero, 9000, quarry_map('.', 2), true});
    sim.issue(PlaceBuilding{"sculpture_studio", 2, 4});
    sim.step(1);
    auto& b = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({2, 4}).building);
    b.producer->input_loads = 3;
    sim.run_days(64);
    CHECK(b.producer->output_loads == 0);
    b.producer->input_loads = 4;
    sim.run_days(33);
    CHECK(b.producer->output_loads == 1);
    CHECK(b.producer->input_loads == 0);
}
