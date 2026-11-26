// Panhellenic games (docs/gameplay/09-kultura.md, Z:110-112; L-123, eZeus model).
#include "bo/core/games.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// Ten houses of 100 people along a road; `visited` of them see a philosopher.
Simulation city(int visited) {
    std::vector<std::string> rows(10, std::string(40, '.'));
    rows[4] = std::string(40, '=');
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{9, Difficulty::Hero, 5000, rows, true});
    for (int i = 0; i < 10; ++i) {
        sim.issue(PlaceBuilding{"common_housing", 2 * i, 2});
    }
    sim.step(1);
    int n = 0;
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        if (b.house) {
            b.house->population = 100;  // stand-ins for big houses: 1 000 people, multiplier 0.5
            if (n++ < visited) {
                b.coverage["philosopher"] = 90;
            }
        }
    }
    return sim;
}

const GameParams& isthmian(const Simulation& sim) {
    for (const auto& g : sim.catalog().games.games) {
        if (g.id == "isthmian") {
            return g;
        }
    }
    FAIL("no isthmian games in data/games.json");
    return sim.catalog().games.games.front();
}

}  // namespace

TEST_CASE("L-123: the chance is multiplier(population) x coverage squared; no performers, no invitation (Z:111)") {
    CHECK(games_chance(city(0).state(), bo::test::catalog(), isthmian(city(0))) == 0);
    auto half = city(5);   // 50 % coverage: 0.5 x 0.5^2 = 12 %
    CHECK(games_chance(half.state(), half.catalog(), isthmian(half)) == 12);
    auto all = city(10);   // 100 %: 50 %
    CHECK(games_chance(all.state(), all.catalog(), isthmian(all)) == 50);
    auto few = city(2);    // 0.5 x 20 = 10 < 15: not invited
    CHECK(games_chance(few.state(), few.catalog(), isthmian(few)) == 0);
}

TEST_CASE("L-123: the games are decided in August; a win earns the discipline's monument (Z:111, Z:155-156)") {
    auto sim = city(10);
    std::size_t held = 0;
    std::size_t won = 0;
    for (int year = 0; year < 8; ++year) {
        sim.run_days(7 * 16 - 1);  // to the last day of July
        for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
            if (b.house) {  // the stand-in houses: their people and the visits, just before the decision
                b.house->population = 100;
                for (const char* w : {"philosopher", "athlete", "actor", "competitor"}) {
                    b.coverage[w] = 90;
                }
            }
        }
        sim.run_days(192 - 7 * 16 + 1);
        for (const auto& e : sim.drain_events()) {
            if (const auto* g = std::get_if<GamesHeld>(&e.event)) {
                ++held;
                won += g->won ? 1 : 0;
                CHECK(g->invited);
            }
        }
    }
    CHECK(held == 8);
    CHECK(won > 0);
    std::uint32_t monuments = 0;
    for (const char* m : {"scholar", "athlete"}) {
        const auto it = sim.state().scenario.monuments.find(m);
        monuments += it == sim.state().scenario.monuments.end() ? 0 : it->second;
    }
    CHECK(monuments == won);
}

TEST_CASE("L-123: a city with the right and a working Stadium hosts the Olympian games; without one it loses the right (Z:111)") {
    for (const bool stadium : {true, false}) {
        auto sim = city(10);
        sim.mutable_state().games_host_year = 0;  // the Olympian games of year 0
        if (stadium) {
            sim.issue(PlaceBuilding{"stadium", 22, 5});
            sim.step(1);
            auto& s = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({22, 5}).building);
            REQUIRE(s.type == "stadium");
            s.performing_days = 200;  // its competitors are performing
        }
        sim.run_days(7 * 16 - 1);
        for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
            if (b.type == "stadium") {
                b.performing_days = 200;
            }
        }
        sim.run_days(2);
        bool hosted = false;
        bool lost = false;
        for (const auto& e : sim.drain_events()) {
            if (const auto* g = std::get_if<GamesHeld>(&e.event)) {
                hosted = g->hosted;
                lost = g->right_lost;
                CHECK((stadium || !g->invited));  // no stadium: not invited this year
            }
        }
        CHECK(hosted == stadium);
        CHECK(lost == !stadium);
        CHECK(sim.state().scenario.monuments.count("games_host") == (stadium ? 1U : 0U));
        CHECK(sim.state().games_host_year != 0);
    }
}
