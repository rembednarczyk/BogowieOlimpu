// Module C7: fire, collapse, disease, crime (docs/gameplay/07, 20, 24; data/risks.json; assumptions L-104).
#include <algorithm>

#include "bo/core/risks.h"
#include "bo/core/walker.h"
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

TEST_CASE("a superintendent passing within 12 days puts the fire out (Z:90, L-104)") {
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
    c.buildings.items.at(r.building).burning_days = 13;
    provide_coverage(c, w);
    CHECK(c.buildings.items.at(r.building).burning_days == 13);  // too late
}

TEST_CASE("plague needs low city hygiene; a third of the household dies, quarantine follows (S-010, Z:85)") {
    Simulation sim(risky(), SimulationConfig{1, Difficulty::Hero, 9000, street(), true});
    sim.issue(PlaceBuilding{"common_housing", 2, 2});
    sim.issue(PlaceBuilding{"common_housing", 6, 2});
    sim.step(1);
    auto& a = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({2, 2}).building);
    auto& b = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({6, 2}).building);
    a.house->population = 9;
    b.house->population = 9;
    a.disease_counter = 950;
    sim.run_days(2);
    CHECK(a.house->population == 9);  // a small city keeps hygiene at 50: no plague
    sim.mutable_state().city.hygiene = 20;
    a.disease_counter = 950;
    sim.mutable_state().city.buildings.items.at(a.id).house->population = 9;
    sim.run_days(1);
    CHECK(a.house->population == 6);  // one in three dies (S-010)
    CHECK(a.house->quarantine_days >= 47);
    Walker healer;
    healer.tile = {3, 4};
    healer.resets = 4;
    a.disease_counter = 900;
    provide_coverage(sim.mutable_state().city, healer);
    CHECK(a.disease_counter == 0);
}

TEST_CASE("city hygiene moves 2 a month toward the share of residents with a healer and food (S-006)") {
    static const DataCatalog kNoSmallCity = [] {
        DataCatalog c = risky();
        c.migration.small_city_population = 0;  // measure hygiene in a town of 15 houses
        return c;
    }();
    Simulation sim(kNoSmallCity, SimulationConfig{1, Difficulty::Hero, 9000, street(), true});
    for (int x = 0; x < 30; x += 2) {
        sim.issue(PlaceBuilding{"common_housing", x, 2});
    }
    sim.step(1);
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        b.house->population = 8;  // huts: no healer; huts do not eat, so they count as fed (1/4)
    }
    sim.run_days(16 * 3 + 1);
    CHECK(sim.state().city.hygiene == 50 - 2 * 4);  // days 0, 16, 32, 48: target 25 (fed, no healer), 2 a month
}

TEST_CASE("fire grows on 1 day in 8 by risk x 1-5: a hut burns after months, not weeks (S-007 OG)") {
    Simulation sim(risky(), SimulationConfig{5, Difficulty::Hero, 9000, street(), true});
    for (int x = 0; x < 30; x += 2) {
        sim.issue(PlaceBuilding{"common_housing", x, 2});
    }
    sim.step(1);
    int first_fire = -1;
    for (int d = 0; d < 16 * 30 && first_fire < 0; ++d) {
        for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
            if (b.house) {
                b.house->population = 8;
            }
        }
        sim.run_days(1);
        for (const auto& e : sim.drain_events()) {
            const auto* ds = std::get_if<Disaster>(&e.event);
            if (ds && ds->kind == DisasterKind::FireStarted && first_fire < 0) {
                first_fire = d;
            }
        }
    }
    MESSAGE("first hut fire after " << first_fire << " days");
    CHECK(first_fire > 16 * 4);   // later than the old 'like collapse' assumption (4 months)
    CHECK(first_fire < 16 * 24);  // S-007: about 10 months for a house
}

TEST_CASE("a superintendent is sent to a fire and saves the building in time (Z:90, S-007)") {
    Simulation sim(risky(), SimulationConfig{1, Difficulty::Hero, 9000, street(), true});
    sim.issue(PlaceBuilding{"fountain", 2, 2});
    sim.issue(PlaceBuilding{"maintenance_office", 12, 2});
    sim.step(1);
    auto& f = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({2, 2}).building);
    f.fire_counter = 959;
    f.fire_counter = 2000;  // ignite at the next risk update
    sim.run_days(1);
    bool sent = false;
    for (const auto& [id, w] : sim.state().walkers.items) {
        sent = sent || w.job == WalkerJob::Extinguish;
    }
    CHECK(sent);
    sim.run_days(10);
    CHECK(f.burning_days == 0);
    CHECK(count(sim, "fountain") == 1);
}

TEST_CASE("crime escalates: disgruntled, mugger, thief, vandal (S-010); a watchman stops outlaws") {
    Simulation sim(risky(), SimulationConfig{1, Difficulty::Hero, 9000, street(), true});
    sim.issue(PlaceBuilding{"common_housing", 2, 2});
    sim.step(1);
    auto& b = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({2, 2}).building);
    b.house->population = 8;
    sim.run_days(1);           // past the day-0 mood update (small cities sit at the base mood)
    b.house->happiness = 20;
    std::vector<std::string> seen;
    for (int i = 0; i < 3; ++i) {
        b.crime_counter = 959;
        sim.run_days(1);
        for (const auto& [id, w] : sim.state().walkers.items) {
            if (std::find(seen.begin(), seen.end(), w.variant) == seen.end() && !w.variant.empty()) {
                seen.push_back(w.variant);
            }
        }
    }
    CHECK(seen == std::vector<std::string>{"disgruntled_citizen", "mugger", "thief"});
    WalkerId outlaw = kNoWalker;
    for (const auto& [id, w] : sim.state().walkers.items) {
        if (w.type == "outlaw" && w.phase != WalkerPhase::Done) {
            outlaw = id;
        }
    }
    REQUIRE(outlaw != kNoWalker);
    Walker guard;
    guard.type = "watchman";
    guard.id = 999;
    guard.tile = sim.state().walkers.items.at(outlaw).tile;
    sim.mutable_state().walkers.items.emplace(999, guard);
    const auto caught = catch_outlaws(sim.mutable_state(), risky());
    CHECK(std::find(caught.begin(), caught.end(), outlaw) != caught.end());

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

TEST_CASE("at equal priority Hygiene & Safety fills first (S-010)") {
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{1, Difficulty::Hero, 9000, street()});
    sim.issue(PlaceBuilding{"maintenance_office", 2, 2});  // 5 workers, hygiene & safety
    sim.issue(PlaceBuilding{"gymnasium", 6, 1});           // 7 workers, culture
    sim.issue(PlaceBuilding{"common_housing", 12, 2});
    sim.step(1);
    sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({12, 2}).building).house->population = 14;
    sim.run_days(1);  // workforce floor(14 x 0.42) = 5
    CHECK(sim.state().city.buildings.items.at(sim.state().city.map.at({2, 2}).building).workers == 5);
    CHECK(sim.state().city.buildings.items.at(sim.state().city.map.at({6, 1}).building).workers == 0);
}

TEST_CASE("disgruntled citizens ignore roadblocks (S-010)") {
    std::string err;
    CityState c;
    c.map = *parse_text_map({"..........", "====x=====", "..........", ".........."}, err);
    update_roads(c);
    const auto r = place_building(c, bo::test::catalog(), Difficulty::Hero, "fountain", {0, 2});
    WalkerStore ws;
    const WalkerId id = spawn_roaming_walker(c, ws, bo::test::catalog(), "disgruntled_citizen", r.building, Dir::East);
    REQUIRE(id != kNoWalker);
    bool crossed = false;
    for (int t = 0; t < 3000 && ws.items.at(id).phase != WalkerPhase::Done; ++t) {
        step_walker(c, ws.items.at(id));
        crossed = crossed || ws.items.at(id).tile.x > 4;
    }
    CHECK(crossed);
}
