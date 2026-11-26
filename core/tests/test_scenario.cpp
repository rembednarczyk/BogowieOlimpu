// Module D5 stage 1: adventure format, events, city requests, goals (docs/gameplay/13, 23; Z:161-164; L-114).
#include <fstream>
#include <sstream>

#include "bo/core/scenario.h"
#include "bo/core/serialize.h"
#include "bo/core/simulation.h"
#include "bo/core/storage.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// A small adventure: a street, an invasion point, an ally and a rival; events and goals as given.
std::string adventure(const std::string& events, const std::string& goals) {
    std::string map;
    for (int y = 0; y < 20; ++y) {
        std::string row(30, '.');
        if (y == 6) {
            row = std::string(30, '=');
        }
        if (y == 15) {
            row[0] = 'i';
        }
        map += (y ? "," : "") + std::string("\"") + row + "\"";
    }
    return R"({"id": "test", "treasury": 5000, "difficulty": "H", "seed": 3, "map": [)" + map + R"(],
      "cities": [{"id": "athens", "relation": "ally", "sells": {"wheat": [40, 16]}, "buys": {"wine": [200, 8]},
                  "opinion": 50, "military": 2, "economy": 3},
                 {"id": "sparta", "relation": "rival", "opinion": 20, "military": 3, "economy": 2}],
      "events": [)" + events + R"(], "goals": [)" + goals + "]}";
}

Simulation start(const std::string& text) {
    std::string error;
    auto cfg = parse_scenario(text, error);
    REQUIRE_MESSAGE(cfg.has_value(), error);
    cfg->full_staffing = true;
    Simulation sim(bo::test::catalog_isolated(), *cfg);
    REQUIRE(sim.setup_error().empty());
    return sim;
}

template <class E>
int count(Simulation& sim) {
    int n = 0;
    for (const auto& e : sim.drain_events()) {
        n += std::holds_alternative<E>(e.event) ? 1 : 0;
    }
    return n;
}

}  // namespace

TEST_CASE("an adventure file sets the map, treasury, difficulty and cities (doc 23)") {
    auto sim = start(adventure("", ""));
    CHECK(sim.state().city.map.width() == 30);
    CHECK(sim.state().finance.treasury == 5000);
    REQUIRE(sim.state().trade.partners.size() == 2);
    CHECK(sim.state().trade.partners[1].relation == "rival");
    CHECK(sim.state().trade.partners[0].diplomacy.opinion == 50);
    std::string error;
    CHECK_FALSE(parse_scenario("{\"map\": []}", error).has_value());
    CHECK(error == "scenario.no_map");
    CHECK_FALSE(parse_scenario("not json", error).has_value());
}

TEST_CASE("dated events: a gift in month 2, an invasion by the rival in year 1, a yearly request (doc 23)") {
    auto sim = start(adventure(R"({"type": "gift", "city": "athens", "good": "drachmas", "amount": 700, "month": 2},
        {"type": "invasion", "city": "sparta", "nation": "greek", "amount": 5, "year": 1, "month": 0},
        {"type": "request", "city": "athens", "good": "wheat", "amount": 4, "month": 3, "repeat": true})",
                               ""));
    sim.issue(PlaceBuilding{"granary", 10, 2});  // something for the invaders to march on
    sim.run_days(2 * 16 + 1);
    CHECK(sim.state().finance.treasury >= 5000 + 700 - 400 - 500);  // minus the granary and a little in wages
    sim.run_days(16);
    CHECK(sim.state().scenario.requests.size() == 1);
    CHECK(count<CityRequestReceived>(sim) == 1);
    sim.run_days(9 * 16);
    CHECK(sim.state().military.invasion.active);
    CHECK(sim.state().military.invasion.city == 1);
    sim.run_days(3 * 16);
    CHECK(sim.state().scenario.requests.size() == 2);  // the request comes again a year later
}

TEST_CASE("requests: on time +10, withdrawn 96 days after the deadline -15 (doc 24, Z:162, L-114)") {
    auto sim = start(adventure(R"({"type": "request", "city": "athens", "good": "wheat", "amount": 4, "month": 0,
                                   "deadline_months": 2})",
                               ""));
    sim.issue(PlaceBuilding{"granary", 10, 2});
    sim.run_days(1);
    REQUIRE(sim.state().scenario.requests.size() == 1);
    sim.issue(FulfillRequest{0});
    sim.step(1);
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
            CHECK(r->reason_key == "scenario.not_in_storage");
        }
    }
    auto& granary = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({10, 2}).building);
    granary.storage->loads["wheat"] = 6;
    sim.issue(FulfillRequest{0});
    sim.step(1);
    CHECK(sim.state().scenario.requests.empty());
    CHECK(sim.state().trade.partners[0].diplomacy.opinion == 60);
    CHECK(city_stock(sim.state().city, bo::test::catalog(), "wheat") == 2);

    auto ignored = start(adventure(R"({"type": "request", "city": "athens", "good": "wheat", "amount": 4, "month": 0,
                                       "deadline_months": 2})",
                                   ""));
    ignored.run_days(2 * 16 + 96 + 2);
    CHECK(ignored.state().scenario.requests.empty());
    CHECK(ignored.state().trade.partners[0].diplomacy.opinion == 35);
}

TEST_CASE("goals are checked monthly; all met wins the episode (doc 23)") {
    auto sim = start(adventure("", R"({"type": "treasury", "value": 5500}, {"type": "vassal", "target": "sparta"})"));
    sim.run_days(20);
    CHECK_FALSE(sim.state().scenario.won);
    sim.mutable_state().finance.treasury = 9000;
    sim.mutable_state().trade.partners[1].relation = "vassal";
    sim.run_days(16);
    CHECK(sim.state().scenario.won);
    CHECK(sim.state().scenario.goals[0].met);
}

TEST_CASE("save v22: scenario progress survives a round trip with the same future") {
    auto a = start(adventure(R"({"type": "request", "city": "athens", "good": "wheat", "amount": 4, "month": 1},
        {"type": "invasion", "city": "sparta", "nation": "greek", "amount": 5, "year": 0, "month": 4})",
                             R"({"type": "population", "value": 100})"));
    a.run_days(40);
    std::string error;
    auto b = Simulation::load(bo::test::catalog_isolated(), a.save(), error);
    REQUIRE(b.has_value());
    CHECK(b->state().scenario == a.state().scenario);
    a.run_days(60);
    b->run_days(60);
    CHECK(a.state_hash() == b->state_hash());
}

TEST_CASE("the demo adventure (scenarios/demo.json) parses and starts") {
    std::ifstream f(std::string(BO_DATA_DIR) + "/../scenarios/demo.json");
    REQUIRE(f.good());
    std::stringstream text;
    text << f.rdbuf();
    auto sim = start(text.str());
    CHECK(sim.state().city.map.width() == 64);
    CHECK(sim.state().trade.partners.size() == 2);
    CHECK(sim.state().scenario.goals.size() == 2);
    CHECK(sim.state().scenario.events.size() == 6);
}

TEST_CASE("a request fulfilled after its deadline costs 5 opinion (eZeus; owner, L-114)") {
    auto sim = start(adventure(R"({"type": "request", "city": "athens", "good": "wheat", "amount": 4, "month": 0,
                                   "deadline_months": 1})",
                               ""));
    sim.issue(PlaceBuilding{"granary", 10, 2});
    sim.run_days(1 + 16 + 5);  // past the deadline, not yet withdrawn
    REQUIRE(sim.state().scenario.requests.size() == 1);
    auto& granary = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({10, 2}).building);
    granary.storage->loads["wheat"] = 6;
    sim.issue(FulfillRequest{0});
    sim.step(1);
    CHECK(sim.state().scenario.requests.empty());
    CHECK(sim.state().trade.partners[0].diplomacy.opinion == 45);
}
