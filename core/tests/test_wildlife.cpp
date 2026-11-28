// Hunting and fishing (docs/gameplay/04 Z:46-48, P:8; 19 S-005; assumptions L-103).
#include "bo/core/logistics.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

std::uint32_t loads_of(const Simulation& sim, const std::string& good) {
    std::uint32_t n = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        if (b.storage) {
            const auto it = b.storage->loads.find(good);
            n += it == b.storage->loads.end() ? 0 : it->second;
        }
        if (b.producer) {
            const auto u = b.producer->units.find(good);
            n += u == b.producer->units.end() ? 0 : u->second / 100;
            const auto& outs = bo::test::catalog().buildings.at(b.type).outputs;
            if (outs.size() == 1 && outs.front() == good) {
                n += b.producer->output_loads;
            }
        }
    }
    return n;
}

}  // namespace

TEST_CASE("boar herds: 7 at the spawn point, a young boar every 26 days; hunters bring pork (Z:46)") {
    std::vector<std::string> r(12, std::string(40, '.'));
    r[6] = std::string(40, '=');
    r[2][8] = 'h';
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{3, Difficulty::Hero, 9000, r, true});
    sim.issue(PlaceBuilding{"boar_hunters_lodge", 2, 4});
    sim.issue(PlaceBuilding{"granary", 20, 2});
    sim.step(1);
    sim.run_days(192);
    const auto pork = loads_of(sim, "pork");
    MESSAGE("pork loads in a year: " << pork);
    CHECK(pork >= 10);  // S-005: about 12 a year, limited by the herd's growth
    CHECK(pork <= 22);
    const auto& spawn = sim.state().city.spawns.at(static_cast<std::uint32_t>(sim.state().city.map.index({8, 2})));
    CHECK(spawn.animals <= kBoarHerd);
}

TEST_CASE("a fishery builds its boat, then sails to the fishing ground; output falls with distance (S-005)") {
    auto yearly = [](std::int32_t distance) {
        std::vector<std::string> r(12, std::string(48, '.'));
        r[6] = std::string(48, '=');
        for (std::size_t y = 9; y < 12; ++y) {
            r[y] = std::string(48, '~');
        }
        r[10][static_cast<std::size_t>(4 + distance)] = 'f';
        Simulation sim(bo::test::catalog_isolated(), SimulationConfig{3, Difficulty::Hero, 9000, r, true});
        sim.issue(PlaceBuilding{"fishery", 3, 7});
        sim.issue(PlaceBuilding{"granary", 6, 2});  // close: the fishery holds one load, the deliveryman must keep up
        sim.step(1);
        REQUIRE(sim.state().city.map.at({3, 7}).building != kNoBuilding);
        sim.run_days(192 + 16);  // a year after the boat is ready
        return loads_of(sim, "fish");
    };
    const auto close = yearly(2);
    const auto far = yearly(20);
    MESSAGE("fish loads in a year: close " << close << ", 20 tiles " << far);
    CHECK(close >= 11);
    CHECK(close <= 15);
    CHECK(far >= 6);
    CHECK(far < close);
}
