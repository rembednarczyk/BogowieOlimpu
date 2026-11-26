// Module D5: gods' quests and commemorative monuments (docs/gameplay/10, 21; Z:124, Z:155-156; P:24; L-118).
#include "bo/core/monsters.h"
#include "bo/core/scenario.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// 60x40, a main street on row 20, a storehouse; a quest of Hermes (index 0: traditionally Perseus).
Simulation town() {
    std::vector<std::string> rows(40, std::string(60, '.'));
    rows[20] = std::string(60, '=');
    SimulationConfig cfg{17, Difficulty::Hero, 30000, rows, true};
    ScenarioEvent quest;
    quest.type = "quest";
    quest.god = "hermes";
    quest.amount = 0;
    cfg.scenario.events.push_back(quest);
    cfg.scenario.goals.push_back(ScenarioGoal{"quest", "hermes", 0, false});
    Simulation sim(bo::test::catalog_isolated(), cfg);
    sim.issue(PlaceBuilding{"storehouse", 50, 17});
    sim.step(1);
    return sim;
}

// Two finished sanctuaries (Hermes, Athena) and Perseus's goods in storage (doc 21).
void ready_for_perseus(Simulation& sim) {
    auto& store = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({50, 17}).building);
    store.storage->loads["marble"] = 60;
    sim.issue(PlaceBuilding{"sanctuary_hermes", 2, 21});
    sim.issue(PlaceBuilding{"sanctuary_athena", 16, 21});
    sim.step(1);
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        if (b.sanctuary) {
            b.sanctuary->complete = true;
            b.sanctuary->needed.clear();
        }
    }
    store.storage->loads["fleece"] = 16;
    store.storage->loads["sculpture"] = 6;
}

std::string rejection(Simulation& sim) {
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
            return r->reason_key;
        }
    }
    return {};
}

}  // namespace

TEST_CASE("a god's quest calls a hero; he goes to the enchanted place and comes back with a statue (Z:124, Z:156)") {
    auto sim = town();
    sim.run_days(1);
    REQUIRE(sim.state().scenario.quests.size() == 1);
    CHECK(sim.state().scenario.quests[0].hero == "perseus");  // the traditional hero (doc 21)
    sim.issue(PlaceBuilding{"heros_hall", 30, 16});  // a quest is reason enough for a Hall (Z:129-130)
    sim.step(1);
    CHECK(rejection(sim).empty());
    const BuildingId hall = sim.state().city.map.at({30, 16}).building;
    REQUIRE(hall != kNoBuilding);
    sim.issue(SendHeroOnQuest{0});
    sim.step(1);
    CHECK(rejection(sim) == "myth.hero_not_here");
    ready_for_perseus(sim);
    sim.issue(SummonHero{hall});
    sim.step(1);
    CHECK(rejection(sim).empty());
    REQUIRE(sim.state().heroes.size() == 1);
    sim.issue(SendHeroOnQuest{0});
    sim.step(1);
    CHECK(rejection(sim).empty());
    CHECK(sim.state().scenario.quests[0].status == QuestStatus::Underway);
    CHECK(sim.state().heroes[0].walker == kNoWalker);  // away
    sim.run_days(kQuestDays + 16);
    CHECK(sim.state().scenario.quests[0].status == QuestStatus::Done);
    CHECK(sim.state().scenario.monuments.at("god_statue") == 1);
    CHECK(sim.state().heroes[0].walker != kNoWalker);  // back in his Hall
    CHECK(sim.state().scenario.goals[0].met);
}

TEST_CASE("commemorative monuments are earned, never bought; each earned one can be built once (Z:155)") {
    auto sim = town();
    sim.issue(PlaceBuilding{"commemorative_monument", 10, 10});
    sim.step(1);
    CHECK(rejection(sim) == "build.no_monument_earned");
    sim.mutable_state().scenario.monuments["god_statue"] = 1;
    sim.issue(PlaceBuilding{"commemorative_monument", 10, 10});
    sim.step(1);
    CHECK(rejection(sim).empty());
    CHECK(sim.state().city.map.at({10, 10}).building != kNoBuilding);
    CHECK(sim.state().scenario.monuments.at("god_statue") == 0);
    sim.issue(PlaceBuilding{"commemorative_monument", 30, 10});
    sim.step(1);
    CHECK(rejection(sim) == "build.no_monument_earned");
}
