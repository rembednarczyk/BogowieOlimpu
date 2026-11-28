// Module D5 stage 3: monsters, Hero's Halls, heroes (docs/gameplay/10, 21, 23; Z:127-131; L-057b, L-116).
#include "bo/core/military.h"
#include "bo/core/monsters.h"
#include "bo/core/serialize.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;
using bo::test::rejection;

namespace {

// 60x40: a main street on row 20, a monster point at the west edge; a granary with food, a storehouse.
Simulation town() {
    std::vector<std::string> rows(40, std::string(60, '.'));
    rows[20] = std::string(60, '=');
    rows[10][0] = 'M';
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{14, Difficulty::Hero, 30000, rows, true});
    sim.issue(PlaceBuilding{"granary", 20, 16});
    sim.issue(PlaceBuilding{"storehouse", 50, 17});
    sim.step(1);
    auto& granary = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({20, 16}).building);
    granary.storage->loads["wheat"] = 8;
    return sim;
}

void send_monster(Simulation& sim, const std::string& id, std::uint32_t strength = 1) {
    ScenarioEvent e;
    e.type = "monster";
    e.nation = id;
    e.amount = strength;
    e.good = "food";
    sim.mutable_state().scenario.events.push_back(e);
}

// A finished sanctuary of `god` at x (marble put in storage first).
void bless(Simulation& sim, const std::string& god, int x) {
    auto& store = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({50, 17}).building);
    store.storage->loads["marble"] = 40;
    sim.issue(PlaceBuilding{"sanctuary_" + god, x, 21});
    sim.step(1);
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        if (b.sanctuary) {
            b.sanctuary->complete = true;
            b.sanctuary->needed.clear();
        }
    }
}


}  // namespace

TEST_CASE("a monster goes for the food, wrecks the granary and goes home to rest (Z:127, doc 21, L-116)") {
    auto sim = town();
    send_monster(sim, "medusa");
    sim.run_days(1);
    REQUIRE(sim.state().monsters.size() == 1);
    bool gone = false;
    for (int d = 0; d < 40 && !gone; ++d) {
        sim.run_days(1);
        gone = sim.state().city.map.at({20, 16}).building == kNoBuilding;
    }
    CHECK(gone);
    CHECK(sim.state().monsters[0].resting);
    sim.run_days(12);
    const Walker& w = sim.state().walkers.items.at(sim.state().monsters[0].walker);
    CHECK(w.tile == TilePos{0, 10});  // back at the monster point
}

TEST_CASE("Apollo from his sanctuary kills a monster that attacks the city (Z:129)") {
    auto sim = town();
    bless(sim, "apollo", 2);
    send_monster(sim, "cyclops", 3);
    sim.run_days(2);
    REQUIRE(sim.state().monsters.size() == 1);
    CHECK(sim.state().monsters[0].slain);
}

TEST_CASE("Hero's Hall only with a monster in the city; the hero comes when his demands are met (Z:130, doc 21)") {
    auto sim = town();
    sim.issue(PlaceBuilding{"heros_hall", 30, 16});
    sim.step(1);
    CHECK(rejection(sim) == "build.no_hero_needed");
    send_monster(sim, "medusa");  // slain by Perseus: 2 sanctuaries of four, 3000 dr taken, 16 fleece, 6 sculptures
    sim.run_days(1);
    sim.issue(PlaceBuilding{"heros_hall", 30, 16});
    sim.step(1);
    CHECK(rejection(sim).empty());
    const BuildingId hall = sim.state().city.map.at({30, 16}).building;
    REQUIRE(hall != kNoBuilding);
    sim.issue(SummonHero{hall});
    sim.step(1);
    CHECK(rejection(sim) == "myth.hero_requirements");
    CHECK(unmet_requirements(sim.state(), sim.catalog(), "perseus", hall).size() == 3);
    bless(sim, "hermes", 2);
    bless(sim, "athena", 14);
    auto& store = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({50, 17}).building);
    store.storage->loads["fleece"] = 16;
    store.storage->loads["sculpture"] = 6;
    CHECK(unmet_requirements(sim.state(), sim.catalog(), "perseus", hall).empty());
    const std::int64_t before = sim.state().finance.treasury;
    sim.drain_events();
    sim.issue(SummonHero{hall});
    sim.step(1);
    CHECK(rejection(sim).empty());
    REQUIRE(sim.state().heroes.size() == 1);
    CHECK(sim.state().heroes[0].id == "perseus");
    CHECK(sim.state().finance.treasury <= before - 3000);
    bool slain = false;
    for (int d = 0; d < 60 && !slain; ++d) {
        sim.run_days(1);
        for (const auto& e : sim.drain_events()) {
            if (const auto* m = std::get_if<MonsterSlain>(&e.event)) {
                slain = m->by_hero;
            }
        }
    }
    CHECK(slain);
    CHECK(sim.state().scenario.monuments.at("hero") == 1);  // a Hero monument (Z:131)
}

TEST_CASE("save v24: a monster on its way survives a round trip with the same future") {
    auto a = town();
    send_monster(a, "hydra", 2);
    a.run_days(3);
    std::string error;
    auto b = Simulation::load(bo::test::catalog_isolated(), a.save(), error);
    REQUIRE(b.has_value());
    CHECK(b->state().monsters == a.state().monsters);
    a.run_days(20);
    b->run_days(20);
    CHECK(a.state_hash() == b->state_hash());
}

TEST_CASE("morale does not recover while a monster fights in the city (B-004)") {
    auto sim = town();
    WorldState& st = sim.mutable_state();
    Company c;
    c.unit = "hoplite";
    c.morale = 50;
    st.military.companies.push_back(c);
    st.monsters.push_back(MonsterState{"medusa", kNoWalker, {0, 10}, "food", kNoBuilding, 0, false, false});
    recover_morale(st);
    CHECK(st.military.companies[0].morale == 50);
    st.monsters[0].slain = true;
    recover_morale(st);
    CHECK(st.military.companies[0].morale == 55);
}

namespace {

// 60x30: a street on row 8, the sea from row 12 with its entry at (59,20); a granary with food inland at (30,2),
// a storehouse on the shore at (10,9).
Simulation harbour(bool sea_entry = true) {
    std::vector<std::string> rows(30, std::string(60, '.'));
    rows[8] = std::string(60, '=');
    for (std::size_t y = 12; y < 30; ++y) {
        rows[y] = std::string(60, '~');
    }
    if (sea_entry) {
        rows[20][59] = 'S';
    }
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{14, Difficulty::Hero, 30000, rows, true});
    sim.issue(PlaceBuilding{"granary", 30, 4});
    sim.issue(PlaceBuilding{"storehouse", 10, 9});
    sim.step(1);
    sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({30, 4}).building).storage->loads["wheat"] = 8;
    return sim;
}

const Walker* monster_walker(const Simulation& sim) {
    const auto it = sim.state().walkers.items.find(sim.state().monsters.at(0).walker);
    return it == sim.state().walkers.items.end() ? nullptr : &it->second;
}

}  // namespace

TEST_CASE("the Kraken comes in from the sea, keeps to the water and wrecks what stands on the shore (Z:128, L-116)") {
    auto sim = harbour();
    send_monster(sim, "kraken");
    sim.run_days(1);
    REQUIRE(sim.state().monsters.size() == 1);
    bool wrecked = false;
    for (int d = 0; d < 60 && !wrecked; ++d) {
        sim.step(kTicksPerDay / 3);
        const Walker* w = monster_walker(sim);
        REQUIRE(w != nullptr);
        CHECK(sim.state().city.map.at(w->tile).terrain == Terrain::Water);  // never on land
        sim.step(kTicksPerDay - kTicksPerDay / 3);
        wrecked = sim.state().city.map.at({10, 9}).building == kNoBuilding;
    }
    CHECK(wrecked);  // the shore storehouse, though it asked for food: the granary stands inland
    CHECK(sim.state().city.map.at({30, 4}).building != kNoBuilding);
}

TEST_CASE("a sea monster without the sea does not appear; Scylla too keeps to the water (S-008, L-116)") {
    auto dry = harbour(false);
    MonsterEvents ev;
    CHECK(spawn_monster(dry.mutable_state(), dry.catalog(), "kraken", 1, "", ev) == "myth.no_sea_point");
    CHECK(dry.state().monsters.empty());
    auto sim = harbour();
    REQUIRE(spawn_monster(sim.mutable_state(), sim.catalog(), "scylla", 1, "", ev).empty());
    CHECK(sim.state().city.map.at(monster_walker(sim)->tile).terrain == Terrain::Water);
}

TEST_CASE("Medusa turns to stone the people next to her, not the soldiers (Z:128, L-116)") {
    auto sim = town();
    send_monster(sim, "medusa");
    sim.run_days(1);
    REQUIRE(!sim.state().monsters.empty());
    const TilePos at = monster_walker(sim)->tile;
    WorldState& st = sim.mutable_state();
    Walker carrier;
    carrier.id = st.walkers.next_id++;
    carrier.job = WalkerJob::Deliver;
    carrier.tile = at;
    carrier.phase = WalkerPhase::AtTarget;
    Walker far = carrier;
    far.id = st.walkers.next_id++;
    far.tile = {at.x + 5, at.y};
    Walker soldier = carrier;
    soldier.id = st.walkers.next_id++;
    soldier.job = WalkerJob::Soldier;
    soldier.type = "hoplite";
    for (const Walker& w : {carrier, far, soldier}) {
        st.walkers.items.emplace(w.id, w);
    }
    sim.drain_events();
    sim.step(1);
    std::vector<std::uint32_t> stone;
    for (const auto& e : sim.drain_events()) {
        if (const auto* p = std::get_if<WalkerPetrified>(&e.event)) {
            stone.push_back(p->walker);
            CHECK(p->monster == "medusa");
        }
    }
    CHECK(stone == std::vector<std::uint32_t>{carrier.id});  // not the one 5 tiles off, not the soldier
    CHECK(sim.state().walkers.items.count(carrier.id) == 0);
    CHECK(sim.state().walkers.items.count(soldier.id) == 1);
}

TEST_CASE("Perseus fights the Kraken from the shore and slays it (Z:130, L-116)") {
    auto sim = harbour();
    send_monster(sim, "kraken");
    sim.run_days(1);
    sim.issue(PlaceBuilding{"heros_hall", 40, 4});
    sim.step(1);
    const BuildingId hall = sim.state().city.map.at({40, 4}).building;
    REQUIRE(hall != kNoBuilding);
    WorldState& st = sim.mutable_state();
    st.heroes.push_back(HeroState{"perseus", hall, kNoWalker});
    st.heroes.back().walker = hero_walker(st, sim.catalog(), st.heroes.back());
    REQUIRE(st.heroes.back().walker != kNoWalker);
    bool slain = false;
    for (int d = 0; d < 120 && !slain; ++d) {
        sim.run_days(1);
        for (const auto& e : sim.drain_events()) {
            if (const auto* m = std::get_if<MonsterSlain>(&e.event)) {
                slain = m->by_hero && m->monster == "kraken";
            }
        }
    }
    CHECK(slain);
}
