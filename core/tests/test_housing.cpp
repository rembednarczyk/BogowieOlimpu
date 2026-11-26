// Module C2: house plots, evolution with appeal hysteresis, devolution, consumption (docs/gameplay/03, 18, 24).
#include "bo/core/housing.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

constexpr auto kFood = static_cast<std::size_t>(HouseGood::Food);
constexpr auto kFleece = static_cast<std::size_t>(HouseGood::Fleece);
constexpr auto kOil = static_cast<std::size_t>(HouseGood::OliveOil);

struct Fixture {
    Simulation sim;
    BuildingId house = kNoBuilding;

    Fixture() : sim(bo::test::catalog_no_migration(), SimulationConfig{11, Difficulty::Hero, 1000, rows()}) {
        sim.issue(PlaceBuilding{"common_housing", 2, 1});
        sim.step(1);
        house = sim.state().city.map.at({2, 1}).building;
        REQUIRE(house != kNoBuilding);
    }
    static std::vector<std::string> rows() {
        std::vector<std::string> r(10, std::string(10, '.'));
        r[3] = "==========";
        return r;
    }
    Building& b() { return sim.mutable_state().city.buildings.items.at(house); }
    HouseState& h() { return *b().house; }
};

}  // namespace

TEST_CASE("house plots touch a road and cost as a Hut (Z:29, S-004)") {
    Fixture f;
    CHECK(f.sim.state().finance.treasury == 1000 - 20);
    CHECK(f.h().population == 0);
    CHECK_FALSE(f.h().elite);
    f.sim.issue(PlaceBuilding{"common_housing", 6, 6});  // no road next to it
    f.sim.step(1);
    CHECK(f.sim.state().city.map.at({6, 6}).building == kNoBuilding);
}

TEST_CASE("a vacant plot does not evolve; a hut with food becomes a shack the next day") {
    Fixture f;
    f.h().goods[kFood] = 100;
    f.sim.run_days(2);
    CHECK(f.h().level == 0);
    f.h().population = 8;
    f.sim.run_days(1);
    CHECK(f.h().level == 1);  // shack
}

TEST_CASE("hovel needs water and 15 culture points; evolution goes one level per day") {
    Fixture f;
    f.h().population = 8;
    f.h().goods[kFood] = 500;
    f.h().goods[kFleece] = 100;
    f.b().coverage["water_bearer"] = 96;
    f.b().coverage["philosopher"] = 96;
    f.sim.run_days(1);
    CHECK(f.h().level == 1);
    f.sim.run_days(1);
    CHECK(f.h().level == 2);  // hovel
    f.sim.run_days(1);
    CHECK(f.h().level == 2);  // stays a hovel: fleece is there, but appeal 0 < AppE(hovel) 10
}

TEST_CASE("devolution waits for 3 failed checks and drops to the highest level still satisfied (Z:31-32)") {
    Fixture f;
    f.h().population = 60;
    f.h().level = 6;  // townhouse
    f.h().goods[kFleece] = 100;
    f.h().goods[kOil] = 100;
    f.sim.run_days(2);
    CHECK(f.h().level == 6);
    f.sim.run_days(1);
    CHECK(f.h().level == 0);  // no food: straight down to Hut
    CHECK(f.h().population == 8);
    CHECK(f.sim.state().city.homeless == 52);
}

TEST_CASE("appeal hysteresis: a level holds between AppD and AppE (doc 18)") {
    const auto& cat = bo::test::catalog();
    const auto& apt = cat.common_levels[5].model[static_cast<std::size_t>(Difficulty::Hero)];
    CHECK(apt.appeal_devolve == 15);
    CHECK(apt.appeal_evolve == 30);
    Fixture f;
    f.h().population = 48;
    f.h().level = 5;  // apartment
    f.h().goods = {1000, 100, 100, 0, 0, 0};
    f.b().coverage["water_bearer"] = 96;
    f.b().coverage["philosopher"] = 96;
    f.b().coverage["athlete"] = 96;
    f.sim.run_days(5);
    CHECK(f.h().level == 3);  // appeal 0 < AppD 15: down to homestead (AppD -2, needs met); tenement AppD 5
}

TEST_CASE("consumption: a full shack eats 4 food a month, 2 on day 0 and 2 on day 8 (doc 18)") {
    Fixture f;
    f.h().population = 16;
    f.h().level = 1;
    f.h().goods[kFood] = 100;
    f.sim.run_days(16);
    CHECK(f.h().goods[kFood] == 100 - 4);
}

TEST_CASE("elite plots need goods in storage; storage arrives with C5") {
    Fixture f;
    f.sim.issue(PlaceBuilding{"elite_housing", 0, 4});
    f.sim.step(1);
    bool rejected = false;
    for (const auto& e : f.sim.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
            rejected = rejected || r->reason_key == "build.elite_needs_goods";
        }
    }
    CHECK(rejected);
}
