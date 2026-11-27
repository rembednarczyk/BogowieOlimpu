// Module D5 stage 2: hostile gods and disasters (docs/gameplay/10, 21, 23; Z:119-127; L-054b, L-115).
#include <algorithm>

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

namespace {
// A bare building of `type` in the state (no tiles): enough for curses, which look at the type only.
BuildingId add_bare(Simulation& sim, const std::string& type) {
    auto& items = sim.mutable_state().city.buildings;
    Building b;
    b.id = items.next_id++;
    b.type = type;
    const BuildingId id = b.id;
    items.items.emplace(id, std::move(b));
    return id;
}
// A walker out on the streets (a water carrier), for the gods who take them.
WalkerId add_walker(Simulation& sim) {
    Walker w;
    w.id = sim.state().walkers.next_id;
    w.type = "water_bearer";
    w.job = WalkerJob::None;
    w.tile = {22, 20};
    sim.mutable_state().walkers.next_id++;
    sim.mutable_state().walkers.items.emplace(w.id, w);
    return w.id;
}
std::size_t walkers_of(const Simulation& sim, WalkerJob job) {
    std::size_t n = 0;
    for (const auto& [id, w] : sim.state().walkers.items) {
        n += w.job == job && w.phase != WalkerPhase::Done ? 1 : 0;
    }
    return n;
}
}  // namespace

TEST_CASE("Poseidon curses the fishermen and the wharves and keeps the trade ships away (Z:126)") {
    auto sim = city();
    const BuildingId fishery = add_bare(sim, "fishery");
    const BuildingId wharf = add_bare(sim, "trireme_wharf");
    const WrathEvents ev = god_attack(sim.mutable_state(), sim.catalog(), "poseidon");
    CHECK_FALSE(ev.repelled);
    CHECK(sim.state().city.buildings.items.at(fishery).cursed_days == 45);
    CHECK(sim.state().city.buildings.items.at(wharf).cursed_days == 45);
    CHECK(sim.state().ship_curse_days == 45);
    CHECK(sim.state().caravan_curse_days == 0);  // caravans still come
}

TEST_CASE("Hades takes the walkers off the streets and curses the foundries and the mint (Z:126)") {
    auto sim = city();
    const WalkerId carrier = add_walker(sim);
    const BuildingId mint = add_bare(sim, "mint");
    const BuildingId foundry = add_bare(sim, "foundry");
    const WrathEvents ev = god_attack(sim.mutable_state(), sim.catalog(), "hades");
    CHECK(std::find(ev.walkers_gone.begin(), ev.walkers_gone.end(), carrier) != ev.walkers_gone.end());
    for (const auto& [id, w] : sim.state().walkers.items) {
        if (w.job != WalkerJob::Soldier && w.job != WalkerJob::Invade && w.job != WalkerJob::Prowl) {
            CHECK(w.phase == WalkerPhase::Done);
        }
    }
    CHECK(sim.state().city.buildings.items.at(mint).cursed_days == 45);
    CHECK(sim.state().city.buildings.items.at(foundry).cursed_days == 45);
}

TEST_CASE("Athena curses the olive presses and the growers (Z:126)") {
    auto sim = city();
    const BuildingId press = add_bare(sim, "olive_press");
    const BuildingId lodge = add_bare(sim, "growers_lodge");
    const BuildingId winery = find(sim, "winery").id;
    god_attack(sim.mutable_state(), sim.catalog(), "athena");
    CHECK(sim.state().city.buildings.items.at(press).cursed_days == 45);
    CHECK(sim.state().city.buildings.items.at(lodge).cursed_days == 45);
    CHECK(sim.state().city.buildings.items.at(winery).cursed_days == 0);  // not hers
}

TEST_CASE("Artemis kills the hunters, fills the wolves' dens and sends her Amazons (Z:126)") {
    auto sim = city();
    Walker hunter;
    hunter.id = sim.state().walkers.next_id;
    hunter.type = "hunter";
    hunter.job = WalkerJob::Hunt;
    sim.mutable_state().walkers.next_id++;
    sim.mutable_state().walkers.items.emplace(hunter.id, hunter);
    sim.mutable_state().city.spawns[0] = SpawnState{};
    sim.mutable_state().city.spawns[0].animals = 1;
    const WrathEvents ev = god_attack(sim.mutable_state(), sim.catalog(), "artemis");
    CHECK(walkers_of(sim, WalkerJob::Hunt) == 0);
    CHECK(std::find(ev.walkers_gone.begin(), ev.walkers_gone.end(), hunter.id) != ev.walkers_gone.end());
    CHECK(sim.state().city.spawns.at(0).animals == sim.catalog().wolves.pack);
    CHECK(ev.invasion_nation == "amazon");
    CHECK(ev.invasion_soldiers == 16);
}

TEST_CASE("Apollo strikes three houses with the plague (Z:126)") {
    auto sim = city(bo::test::catalog());  // the isolated catalog keeps risks out of reach of a 16-bit counter
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        if (b.house) {
            b.house->population = 10;
        }
    }
    god_attack(sim.mutable_state(), sim.catalog(), "apollo");
    std::size_t sick = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        sick += b.house && b.disease_counter >= sim.catalog().risks.disease_threshold ? 1 : 0;
    }
    CHECK(sick == 3);
}

TEST_CASE("Aphrodite takes the walkers and a tenth of every house's people (Z:126)") {
    auto sim = city();
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        if (b.house) {
            b.house->population = 20;
        }
    }
    const WalkerId carrier = add_walker(sim);
    const WrathEvents ev = god_attack(sim.mutable_state(), sim.catalog(), "aphrodite");
    std::size_t houses = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        if (b.house) {
            CHECK(b.house->population == 18);
            ++houses;
        }
    }
    CHECK(houses == 4);
    CHECK(std::find(ev.walkers_gone.begin(), ev.walkers_gone.end(), carrier) != ev.walkers_gone.end());
}

TEST_CASE("Dionysus takes the walkers and curses the wineries and the vines (Z:127)") {
    auto sim = city();
    const WalkerId carrier = add_walker(sim);
    const BuildingId vine = add_bare(sim, "grapevine");
    const WrathEvents ev = god_attack(sim.mutable_state(), sim.catalog(), "dionysus");
    CHECK(std::find(ev.walkers_gone.begin(), ev.walkers_gone.end(), carrier) != ev.walkers_gone.end());
    CHECK(sim.state().city.buildings.items.at(vine).cursed_days == 45);
    CHECK(find(sim, "winery").cursed_days == 45);
}
