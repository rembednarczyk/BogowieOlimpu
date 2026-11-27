// Module D5: adventures of several episodes, colonies (docs/gameplay/23; L-119).
#include <fstream>
#include <sstream>

#include "bo/core/adventure.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

std::string rows(int w, int h) {
    std::string out;
    for (int y = 0; y < h; ++y) {
        out += (y ? "," : "") + std::string("\"") + std::string(static_cast<std::size_t>(w), '.') + "\"";
    }
    return out;
}

std::string colony(const std::string& id, int width) {
    return R"({"id": ")" + id + R"(", "treasury": 2000, "map": [)" + rows(width, 12) +
           R"(], "goals": [{"type": "treasury", "value": 2500}]})";
}

// Parent, colony (choice of two), parent, colony, victory (doc 23: P, ..., K, ..., Victory).
const std::string kAdventure = R"({"id": "test_adventure", "treasury": 1000, "map": [)" + rows(30, 20) + R"(],
  "cities": [{"id": "argos", "relation": "distant"}, {"id": "corcyra", "relation": "distant"}],
  "steps": [
    {"type": "parent", "goals": [{"type": "treasury", "value": 5000}]},
    {"type": "colony", "colonies": [)" + colony("argos", 24) + "," + colony("corcyra", 26) + R"(]},
    {"type": "parent", "goals": [{"type": "treasury", "value": 9000}],
     "events": [{"type": "gift", "city": "argos", "good": "drachmas", "amount": 100, "month": 1}]},
    {"type": "colony", "colonies": [)" + colony("argos", 24) + "," + colony("corcyra", 26) + R"(]},
    {"type": "victory"}]})";

void win(std::optional<Simulation>& sim, std::int64_t treasury) {
    sim->mutable_state().finance.treasury = treasury;
    sim->run_days(17);
    REQUIRE(sim->state().scenario.won);
}

}  // namespace

TEST_CASE("an adventure: parent episodes in one city, colonies on their own maps, then victory (doc 23)") {
    std::string error;
    const auto adventure = parse_adventure(kAdventure, error);
    REQUIRE_MESSAGE(adventure.has_value(), error);
    CHECK(adventure->steps.size() == 5);
    AdventureRun run;
    std::optional<Simulation> sim;
    sim.emplace(begin_adventure(bo::test::catalog_isolated(), *adventure, run));
    CHECK(sim->state().city.map.width() == 30);
    sim->issue(PlaceBuilding{"granary", 5, 5});
    sim->step(1);
    win(sim, 6000);

    CHECK(advance_adventure(*adventure, run, sim) == NextEpisode::ChooseColony);
    CHECK(colony_choices(*adventure, run) == std::vector<std::string>{"argos", "corcyra"});
    CHECK(found_colony(bo::test::catalog_isolated(), *adventure, run, "sparta", sim) == "adventure.bad_colony");
    CHECK(found_colony(bo::test::catalog_isolated(), *adventure, run, "argos", sim).empty());
    CHECK(sim->state().city.map.width() == 24);  // the colony's own map
    win(sim, 3000);

    CHECK(advance_adventure(*adventure, run, sim) == NextEpisode::ParentEpisode);
    CHECK(sim->state().city.map.width() == 30);  // back in the parent city, as it was left
    CHECK(sim->state().city.map.at({5, 5}).building != kNoBuilding);
    CHECK(sim->state().trade.partners[0].relation == "colony");
    CHECK(sim->state().scenario.monuments.at("colony") == 1);  // B-005, Z:156
    CHECK(sim->state().scenario.id == "test_adventure#2");      // B-006
    CHECK_FALSE(sim->state().scenario.won);
    REQUIRE(sim->state().scenario.goals.size() == 1);
    CHECK(sim->state().scenario.goals[0].value == 9000);
    win(sim, 9500);

    CHECK(advance_adventure(*adventure, run, sim) == NextEpisode::ChooseColony);
    CHECK(colony_choices(*adventure, run) == std::vector<std::string>{"corcyra"});  // each colony once
    CHECK(found_colony(bo::test::catalog_isolated(), *adventure, run, "corcyra", sim).empty());
    win(sim, 3000);
    CHECK(advance_adventure(*adventure, run, sim) == NextEpisode::Victory);
}

TEST_CASE("a single episode file is an adventure of one parent episode; an adventure must end in victory") {
    std::string error;
    const auto one = parse_adventure(R"({"map": [".."], "goals": [{"type": "treasury", "value": 1}]})", error);
    REQUIRE(one.has_value());
    CHECK(one->steps.size() == 2);
    CHECK(one->steps.back().kind == "victory");
    CHECK_FALSE(parse_adventure(R"({"map": [".."], "steps": [{"type": "parent"}]})", error).has_value());
    CHECK(error == "adventure.must_start_in_parent_and_end_in_victory");
}

TEST_CASE("the example adventure (scenarios/example_adventure.json) parses") {
    std::ifstream f(std::string(BO_DATA_DIR) + "/../scenarios/example_adventure.json");
    REQUIRE(f.good());
    std::stringstream text;
    text << f.rdbuf();
    std::string error;
    const auto a = parse_adventure(text.str(), error);
    REQUIRE_MESSAGE(a.has_value(), error);
    CHECK(a->steps.size() == 4);
    CHECK(a->steps[1].colonies.size() == 2);
}

TEST_CASE("a save in the middle of a colony keeps the adventure and the parent city (B-007)") {
    std::string error;
    const auto adventure = parse_adventure(kAdventure, error);
    REQUIRE(adventure.has_value());
    AdventureRun run;
    std::optional<Simulation> sim;
    sim.emplace(begin_adventure(bo::test::catalog_isolated(), *adventure, run));
    sim->issue(PlaceBuilding{"granary", 5, 5});
    sim->step(1);
    win(sim, 6000);
    REQUIRE(advance_adventure(*adventure, run, sim) == NextEpisode::ChooseColony);
    REQUIRE(found_colony(bo::test::catalog_isolated(), *adventure, run, "argos", sim).empty());
    sim->run_days(3);

    const auto bytes = save_adventure(kAdventure, run, *sim);
    CHECK(is_adventure_save(bytes));
    auto loaded = load_adventure(bo::test::catalog_isolated(), bytes, error);
    REQUIRE_MESSAGE(loaded.has_value(), error);
    CHECK(loaded->run.in_colony);
    CHECK(loaded->run.step == run.step);
    CHECK(loaded->sim->state_hash() == sim->state_hash());
    win(loaded->sim, 3000);
    CHECK(advance_adventure(loaded->data, loaded->run, loaded->sim) == NextEpisode::ParentEpisode);
    CHECK(loaded->sim->state().city.map.at({5, 5}).building != kNoBuilding);  // the parent city came back
    CHECK(loaded->sim->state().trade.partners[0].relation == "colony");
    CHECK_FALSE(is_adventure_save(sim->save()));
}
