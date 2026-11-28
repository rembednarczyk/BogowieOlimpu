// The expansion's gods, monsters and heroes (P:20-25; docs/gameplay/10, 21; data/gods.json, heroes.json; L-134).
#include <algorithm>

#include "bo/core/monsters.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;
using bo::test::rejection;

namespace {

std::size_t count_of(const Simulation& sim, const char* type) {
    return static_cast<std::size_t>(std::count_if(sim.state().city.buildings.items.begin(), sim.state().city.buildings.items.end(),
                                                  [type](const auto& kv) { return kv.second.type == type; }));
}


// Loads a masonry shop on a quarry cuts in a year, with or without Atlas.
std::uint32_t stone_in_a_year(bool atlas) {
    auto sim = bo::test::with_god(atlas ? "atlas" : "");
    for (int y = 10; y < 14; ++y) {
        for (int x = 40; x < 48; ++x) {
            sim.mutable_state().city.map.at({x, y}).terrain = Terrain::MarbleQuarry;
        }
    }
    sim.issue(PlaceBuilding{"masonry_shop", 42, 18});
    sim.step(1);
    auto& shop = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({42, 18}).building);
    std::uint32_t cut = 0;
    for (int d = 0; d < 192; ++d) {
        sim.run_days(1);
        cut += shop.producer->output_loads;
        shop.producer->output_loads = 0;  // carried away
    }
    return cut;
}

}  // namespace

TEST_CASE("Hera: her sanctuary brings two orange groves (P:21, L-134)") {
    auto sim = bo::test::with_god("hera");
    CHECK(count_of(sim, "orange_tree") == static_cast<std::size_t>(2 * sim.catalog().blessings.grove_plants));
}

TEST_CASE("Atlas: stonecutters work faster; a guild sends two artisans at a time (P:22, L-134)") {
    const auto plain = stone_in_a_year(false);
    const auto blessed = stone_in_a_year(true);
    MESSAGE("masonry loads a year: " << plain << " plain, " << blessed << " with Atlas");
    CHECK(blessed > plain);

    for (const bool atlas : {false, true}) {
        INFO("atlas " << atlas);
        auto sim = bo::test::with_god(atlas ? "atlas" : "");
        if (!atlas) {
            sim.issue(PlaceBuilding{"storehouse", 50, 17});
            sim.step(1);
        }
        auto& store = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({50, 17}).building);
        store.storage->loads["marble"] = 60;
        sim.issue(PlaceBuilding{"sanctuary_hermes", 40, 21});
        sim.issue(PlaceBuilding{"artisans_guild", 30, 18});
        sim.step(1);
        CHECK(rejection(sim).empty());
        std::size_t most = 0;
        for (int d = 0; d < 60; ++d) {
            sim.run_days(1);
            most = std::max(most, static_cast<std::size_t>(std::count_if(
                                      sim.state().walkers.items.begin(), sim.state().walkers.items.end(),
                                      [](const auto& kv) { return kv.second.job == WalkerJob::Build; })));
        }
        CHECK(most == (atlas ? 2U : 1U));
    }
}

TEST_CASE("Citadel of Poseidon: only in Atlantis, built with orichalc too (P:26)") {
    auto greek = bo::test::with_god("");
    greek.issue(PlaceBuilding{"sanctuary_poseidon_citadel", 2, 0});
    greek.step(1);
    CHECK(rejection(greek) == "build.atlantean_only");
    auto atl = bo::test::with_god("");
    atl.mutable_state().city.atlantean = true;
    atl.issue(PlaceBuilding{"storehouse", 50, 17});
    atl.step(1);
    atl.mutable_state().city.buildings.items.at(atl.state().city.map.at({50, 17}).building).storage->loads["marble"] = 40;
    atl.issue(PlaceBuilding{"sanctuary_poseidon_citadel", 2, 0});
    atl.step(1);
    const std::string why = rejection(atl);
    INFO(why);
    CHECK(why.empty());
    const auto& p = atl.catalog().sanctuaries.at("sanctuary_poseidon_citadel");
    CHECK(p.god == "poseidon");
    CHECK(p.materials.at("orichalc") > 0);  // 16 (S-004)
}

TEST_CASE("sacrifices: the priests take cattle too (P:22, Z:124)") {
    auto sim = bo::test::with_god("hera");
    sim.issue(PlaceBuilding{"corral", 30, 16});
    sim.step(1);
    for (int i = 0; i < 4; ++i) {
        sim.issue(PlaceBuilding{"cattle", 31 + i, 24});
    }
    sim.step(1);
    REQUIRE(count_of(sim, "cattle") == 4);
    sim.run_days(80);
    CHECK(count_of(sim, "cattle") < 4);  // gone to the altar; the corral does not breed them back (P:7)
}

TEST_CASE("heroes in Atlantis: the hippodrome for the games, orichalc for bronze (P:24-25, heroes.json)") {
    auto sim = bo::test::with_god("");
    sim.issue(PlaceBuilding{"storehouse", 50, 17});
    sim.step(1);
    sim.mutable_state().scenario.monuments["athlete"] = 1;  // a game won
    auto& store = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({50, 17}).building);
    store.storage->loads["bronze"] = 24;
    const auto lacks = [&sim](const char* hero, const char* prefix) {
        const auto u = unmet_requirements(sim.state(), sim.catalog(), hero, kNoBuilding);
        return std::any_of(u.begin(), u.end(), [prefix](const std::string& r) { return r.rfind(prefix, 0) == 0; });
    };
    CHECK_FALSE(lacks("hercules", "win_any_panhellenic_game"));
    CHECK_FALSE(lacks("bellerophon", "bronze:"));
    sim.mutable_state().city.atlantean = true;
    CHECK(lacks("hercules", "win_any_panhellenic_game"));  // no working hippodrome
    CHECK(lacks("bellerophon", "bronze:"));                // orichalc, not bronze
    store.storage->loads["orichalc"] = 24;
    CHECK_FALSE(lacks("bellerophon", "bronze:"));
}

TEST_CASE("the expansion's monsters come and go for the city: Sphinx, Chimera, Echidna, Harpies (P:23-24)") {
    for (const char* monster : {"sphinx", "chimera", "echidna", "harpies"}) {
        INFO(monster);
        std::vector<std::string> rows(30, std::string(40, '.'));
        rows[15] = std::string(40, '=');
        rows[5][0] = 'M';
        Simulation sim(bo::test::catalog_isolated(), SimulationConfig{3, Difficulty::Hero, 30000, rows, true});
        sim.issue(PlaceBuilding{"granary", 20, 11});
        sim.step(1);
        MonsterEvents ev;
        REQUIRE(spawn_monster(sim.mutable_state(), sim.catalog(), monster, 1, "", ev).empty());
        bool wrecked = false;
        for (int d = 0; d < 40 && !wrecked; ++d) {
            sim.run_days(1);
            wrecked = sim.state().city.map.at({20, 11}).building == kNoBuilding;
        }
        CHECK(wrecked);
        CHECK(sim.catalog().monster_slayer.at(monster) == (std::string(monster) == "sphinx" || std::string(monster) == "harpies" ? "atalanta" : "bellerophon"));
    }
}

TEST_CASE("the Citadel of Poseidon is finished with its orichalc (P:26, B-025)") {
    auto sim = bo::test::with_god("");
    sim.mutable_state().city.atlantean = true;
    sim.issue(PlaceBuilding{"storehouse", 50, 17});
    sim.step(1);
    auto& store = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({50, 17}).building);
    store.storage->loads["marble"] = 40;
    sim.issue(PlaceBuilding{"sanctuary_poseidon_citadel", 2, 0});
    sim.issue(PlaceBuilding{"artisans_guild", 30, 18});
    sim.step(1);
    const BuildingId site = sim.state().city.map.at({2, 0}).building;
    REQUIRE(site != kNoBuilding);
    auto& s = *sim.mutable_state().city.buildings.items.at(site).sanctuary;
    for (auto& [g, n] : s.needed) {
        n = g == "orichalc" ? 4 : 0;  // all built but the orichalc
    }
    s.on_site["orichalc"] = 4;
    sim.run_days(60);
    CHECK(sim.state().city.buildings.items.at(site).sanctuary->complete);
}
