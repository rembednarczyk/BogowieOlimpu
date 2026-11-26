// Module D5 stage 2: hostile gods and disasters (docs/gameplay/10, 21, 23; Z:119-127; L-054b, L-115).
#include "bo/core/labour.h"
#include "bo/core/sanctuary.h"
#include "bo/core/scenario.h"
#include "bo/core/simulation.h"
#include "bo/core/wrath.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// 60x40, a main street on row 20, a meadow at x 30-40, y 22-27, an invasion point; a Palace, houses,
// a winery and a storehouse.
Simulation city(const DataCatalog& catalog = bo::test::catalog_isolated()) {
    std::vector<std::string> rows(40, std::string(60, '.'));
    rows[20] = std::string(60, '=');
    rows[30][0] = 'i';
    for (std::size_t y = 22; y < 28; ++y) {
        for (std::size_t x = 30; x < 40; ++x) {
            rows[y][x] = ',';
        }
    }
    Simulation sim(catalog, SimulationConfig{13, Difficulty::Hero, 30000, rows, true});
    sim.issue(PlaceBuilding{"palace", 30, 14});
    sim.issue(PlaceBuilding{"storehouse", 50, 17});
    sim.issue(PlaceBuilding{"winery", 14, 17});
    sim.issue(PlaceBuilding{"carding_shed", 42, 21});
    for (int x = 20; x < 28; x += 2) {
        sim.issue(PlaceBuilding{"common_housing", x, 18});
    }
    sim.step(1);
    sim.issue(PlaceBuilding{"sheep", 32, 24});
    sim.step(1);
    return sim;
}

bool has(const Simulation& sim, const std::string& type) {
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        if (b.type == type) {
            return true;
        }
    }
    return false;
}

const Building& find(const Simulation& sim, const std::string& type) {
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        if (b.type == type) {
            return b;
        }
    }
    FAIL("no " << type);
    return sim.state().city.buildings.items.begin()->second;
}

// A finished sanctuary of `god` (marble put in storage first).
void bless(Simulation& sim, const std::string& god) {
    INFO("sanctuary of " << god);
    auto& store = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({50, 17}).building);
    store.storage->loads["marble"] = 60;
    sim.issue(PlaceBuilding{"sanctuary_" + god, 2, 21});  // south of the main street
    sim.step(1);
    bool placed = false;
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        if (b.sanctuary) {
            b.sanctuary->complete = true;
            b.sanctuary->needed.clear();
            placed = true;
        }
    }
    REQUIRE(placed);
}

}  // namespace

TEST_CASE("Zeus destroys the Palace first, then other buildings (Z:125)") {
    auto sim = city();
    REQUIRE(has(sim, "palace"));
    const auto before = sim.state().city.buildings.items.size();
    const WrathEvents ev = god_attack(sim.mutable_state(), sim.catalog(), "zeus");
    CHECK_FALSE(ev.repelled);
    CHECK_FALSE(has(sim, "palace"));
    CHECK(sim.state().city.buildings.items.size() == before - 4);  // the Palace and 3 more [L-115]
}

TEST_CASE("defenders: Zeus against any god, Hera against Zeus, Aphrodite against the lesser gods (Z:119-122, P:21)") {
    auto zeus = city();
    bless(zeus, "zeus");
    CHECK(god_attack(zeus.mutable_state(), zeus.catalog(), "demeter").repelled);
    auto hera = city();
    bless(hera, "hera");
    CHECK(god_attack(hera.mutable_state(), hera.catalog(), "zeus").repelled);
    CHECK_FALSE(god_attack(hera.mutable_state(), hera.catalog(), "hermes").repelled);
    auto love = city();
    bless(love, "aphrodite");
    CHECK(god_attack(love.mutable_state(), love.catalog(), "dionysus").repelled);
    CHECK_FALSE(god_attack(love.mutable_state(), love.catalog(), "apollo").repelled);
}

TEST_CASE("curses stop work for 45 days (L-054b); Demeter takes the flocks; Hermes steals (Z:126-127)") {
    auto sim = city();
    god_attack(sim.mutable_state(), sim.catalog(), "dionysus");
    const Building& winery = find(sim, "winery");
    CHECK(winery.cursed_days == 45);
    CHECK(workers_needed(winery, bo::test::catalog(), sim.state().labour) == 0);
    sim.run_days(46);
    CHECK(find(sim, "winery").cursed_days == 0);
    CHECK(workers_needed(find(sim, "winery"), bo::test::catalog(), sim.state().labour) > 0);

    REQUIRE(has(sim, "sheep"));
    god_attack(sim.mutable_state(), sim.catalog(), "demeter");
    CHECK_FALSE(has(sim, "sheep"));
    const std::int64_t money = sim.state().finance.treasury;
    god_attack(sim.mutable_state(), sim.catalog(), "hermes");
    CHECK(sim.state().finance.treasury == money - money / 4);
}

TEST_CASE("Hephaestus sets fires; Ares comes with his Sparti from a scenario event (Z:126-127)") {
    auto sim = city(bo::test::catalog());  // real risk thresholds: the fires must start
    const WrathEvents ev = god_attack(sim.mutable_state(), sim.catalog(), "hephaestus");
    CHECK(ev.fires.size() == 3);
    sim.run_days(2);
    bool burning = false;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        burning = burning || b.burning_days > 0 || b.type == "ruins";
    }
    CHECK(burning);

    auto ares = city();
    ScenarioEvent e;
    e.type = "god_attack";
    e.god = "ares";
    e.month = 0;
    e.year = 0;
    ares.mutable_state().scenario.events.push_back(e);
    ares.run_days(1);  // they march on the undefended Palace and take it within days
    CHECK(ares.state().military.invasion.active);
    CHECK(ares.state().military.invasion.nation == "divine");
    CHECK(ares.state().military.invasion.invaders.size() == 16);
}

TEST_CASE("disasters destroy what stands within the radius; an earthquake leaves rock (doc 23, L-115)") {
    auto sim = city();
    const auto ev = disaster(sim.mutable_state(), "earthquake", {21, 19}, 1);
    CHECK_FALSE(ev.removed.empty());
    CHECK(sim.state().city.map.at({21, 20}).terrain == Terrain::Rock);
    CHECK_FALSE(sim.state().city.map.at({21, 20}).road);
    auto flood = city();
    const auto wave = disaster(flood.mutable_state(), "tidal_wave", {21, 19}, 1);
    CHECK_FALSE(wave.removed.empty());
    CHECK(flood.state().city.map.at({21, 20}).road);
}
