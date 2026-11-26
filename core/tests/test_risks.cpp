// Module C7: fire, collapse, disease, crime (docs/gameplay/07, 20, 24; data/risks.json; assumptions L-104).
#include "bo/core/risks.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// The real data with migration off (populations are set by hand); risks stay on.
const DataCatalog& risky() {
    static const DataCatalog kCatalog = [] {
        DataCatalog c = bo::test::catalog();
        c.migration.batch_people = 0;
        return c;
    }();
    return kCatalog;
}

std::vector<std::string> street() {
    std::vector<std::string> r(10, std::string(30, '.'));
    r[4] = std::string(30, '=');
    return r;
}

int count(const Simulation& sim, const std::string& type) {
    int n = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        n += b.type == type ? 1 : 0;
    }
    return n;
}

}  // namespace

TEST_CASE("without a superintendent a granary collapses after about 60 / damage_risk months (S-004)") {
    Simulation sim(risky(), SimulationConfig{1, Difficulty::Hero, 9000, street(), true});
    sim.issue(PlaceBuilding{"granary", 2, 0});
    sim.step(1);
    const auto& stats = bo::test::catalog().building_stats.at("granary");
    const std::int32_t risk = stats.damage_risk[static_cast<std::size_t>(Difficulty::Hero)].value_or(0);
    REQUIRE(risk > 0);
    const std::int32_t days = 960 / risk;  // mean; a daily random multiplier 0-2 spreads it
    sim.run_days(static_cast<std::uint64_t>(days * 2 + 32));
    CHECK(count(sim, "granary") == 0);
    CHECK(count(sim, "ruins") == 1);
}

TEST_CASE("a superintendent's visits keep buildings standing (Z:89)") {
    Simulation sim(risky(), SimulationConfig{1, Difficulty::Hero, 9000, street(), true});
    sim.issue(PlaceBuilding{"granary", 2, 0});
    sim.issue(PlaceBuilding{"maintenance_office", 8, 2});
    sim.step(1);
    sim.run_days(192 * 2);
    CHECK(count(sim, "granary") == 1);
    CHECK(count(sim, "ruins") == 0);
}

TEST_CASE("fire: a building burns 32 days and becomes ruins; ruins can be cleared, not built") {
    Simulation sim(risky(), SimulationConfig{1, Difficulty::Hero, 9000, street(), true});
    sim.issue(PlaceBuilding{"fountain", 2, 2});
    sim.step(1);
    auto& b = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({2, 2}).building);
    b.burning_days = 1;
    sim.run_days(33);
    CHECK(count(sim, "fountain") == 0);
    CHECK(count(sim, "ruins") == 1);
    sim.issue(PlaceBuilding{"ruins", 10, 0});
    sim.issue(Demolish{2, 2});
    sim.step(1);
    CHECK(count(sim, "ruins") == 0);
}

TEST_CASE("a superintendent passing within 8 days puts the fire out (Z:90, L-104)") {
    std::string err;
    CityState c;
    c.map = *parse_text_map(street(), err);
    update_roads(c);
    const auto r = place_building(c, risky(), Difficulty::Hero, "fountain", {2, 2});
    Walker w;
    w.tile = {3, 4};
    w.resets = 1;
    c.buildings.items.at(r.building).burning_days = 5;
    provide_coverage(c, w);
    CHECK(c.buildings.items.at(r.building).burning_days == 0);
    c.buildings.items.at(r.building).burning_days = 12;
    provide_coverage(c, w);
    CHECK(c.buildings.items.at(r.building).burning_days == 12);  // too late
}

TEST_CASE("plague: half the household dies, quarantine keeps newcomers out; a healer prevents it") {
    Simulation sim(risky(), SimulationConfig{1, Difficulty::Hero, 9000, street(), true});
    sim.issue(PlaceBuilding{"common_housing", 2, 2});
    sim.step(1);
    auto& b = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({2, 2}).building);
    b.house->population = 8;
    b.disease_counter = 3800;
    b.fire_counter = 0;
    sim.run_days(4);
    CHECK(b.house->population == 4);
    CHECK(b.house->quarantine_days > 90);
    Walker healer;
    healer.tile = {3, 4};
    healer.resets = 4;
    b.disease_counter = 3800;
    provide_coverage(sim.mutable_state().city, healer);
    CHECK(b.disease_counter == 0);
}

TEST_CASE("crime: an unhappy house sends an outlaw; a watchman stops it; a vandal ruins an elite house") {
    Simulation sim(risky(), SimulationConfig{1, Difficulty::Hero, 9000, street(), true});
    sim.issue(PlaceBuilding{"common_housing", 2, 2});
    sim.step(1);
    auto& b = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({2, 2}).building);
    b.house->population = 8;
    sim.run_days(1);           // past the day-0 mood update (small cities sit at the base mood)
    b.house->happiness = 20;  // < 30: thief; no elite house to rob, so it roams
    b.crime_counter = 959;
    sim.run_days(1);
    WalkerId outlaw = kNoWalker;
    for (const auto& [id, w] : sim.state().walkers.items) {
        if (w.type == "outlaw") {
            outlaw = id;
            CHECK(w.variant == "thief");
        }
    }
    REQUIRE(outlaw != kNoWalker);
    Walker guard;
    guard.type = "watchman";
    guard.id = 999;
    guard.tile = sim.state().walkers.items.at(outlaw).tile;
    sim.mutable_state().walkers.items.emplace(999, guard);
    const auto caught = catch_outlaws(sim.mutable_state(), risky());
    CHECK(caught == std::vector<WalkerId>{outlaw});

    Building elite = b;
    elite.house->elite = true;
    elite.house->population = 6;
    const BuildingId target = add_building(sim.mutable_state().city, elite);
    Walker vandal;
    vandal.variant = "vandal";
    vandal.target = target;
    RiskEvents ev;
    crime_at_target(sim.mutable_state(), risky(), vandal, ev);
    CHECK(sim.state().city.buildings.items.count(target) == 0);
    CHECK(sim.state().city.homeless >= 6);
}
