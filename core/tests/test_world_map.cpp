// The world map block (save v50): each city's place on the map (L-135) and its history of interactions, the
// thumbs up and down of the city panel (Z:171).
#include "bo/core/scenario.h"
#include "bo/core/simulation.h"
#include "bo/core/world.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

TradePartner partner(const std::string& id) {
    TradePartner p;
    p.id = id;
    p.buys["wine"] = TradeOffer{200, 8, 0};
    p.sells["wheat"] = TradeOffer{40, 16, 0};
    return p;
}

Simulation two_allies() {
    std::vector<std::string> rows(20, std::string(30, '.'));
    rows[6] = std::string(30, '=');
    SimulationConfig cfg{5, Difficulty::Hero, 20000, rows, true};
    cfg.trade_partners = {partner("athens"), partner("thebes")};
    cfg.trade_partners[1].map_x = 80;
    cfg.trade_partners[1].map_y = 30;
    return Simulation(bo::test::catalog_isolated(), cfg);
}

const std::vector<Interaction>& history(const Simulation& sim, std::size_t i) {
    return sim.state().trade.partners[i].diplomacy.history;
}

}  // namespace

TEST_CASE("world map: a scenario city's place is read from \"map\", clamped to 0-100; none gives -1 (L-135)") {
    std::string error;
    const auto cfg = parse_scenario(R"({"id": "w", "map": ["....", "===="], "cities": [
        {"id": "a", "map": [12, 140]}, {"id": "b"}]})", error);
    REQUIRE(cfg.has_value());
    CHECK(cfg->trade_partners[0].map_x == 12);
    CHECK(cfg->trade_partners[0].map_y == 100);
    CHECK(cfg->trade_partners[1].map_x == -1);
    CHECK(cfg->trade_partners[1].map_y == -1);
}

TEST_CASE("world map: a gift is noted in the city's history with its opinion change; the others hear of it (Z:171, Z:177)") {
    auto sim = two_allies();
    sim.issue(GiftToCity{0, "drachmas", 500});
    sim.step(1);
    REQUIRE(history(sim, 0).size() == 1);
    CHECK(history(sim, 0)[0].what == "history.gift");
    CHECK(history(sim, 0)[0].delta == sim.state().trade.partners[0].diplomacy.opinion - 60);
    REQUIRE(history(sim, 1).size() == 1);
    CHECK(history(sim, 1)[0].what == "history.gift_heard");
}

TEST_CASE("world map: the history joins changes of one kind in a row, keeps the newest 8 and skips no change (Z:171)") {
    CityDiplomacy d;
    change_opinion(d, 3, "history.gift", 1);
    change_opinion(d, 3, "history.gift", 2);
    REQUIRE(d.history.size() == 1);
    CHECK(d.history[0].delta == 6);
    CHECK(d.history[0].day == 2);
    d.opinion = 100;
    change_opinion(d, 5, "history.zeus", 3);  // clamped at 100: nothing to note
    CHECK(d.history.size() == 1);
    for (std::uint64_t i = 0; i < 10; ++i) {
        change_opinion(d, i % 2 == 0 ? -1 : 1, i % 2 == 0 ? "history.asked" : "history.gift", 10 + i);
    }
    CHECK(d.history.size() == kHistoryLength);
    CHECK(d.history.back().day == 19);
}

TEST_CASE("world map: places and histories survive a save with the same future (save v50)") {
    auto a = two_allies();
    a.issue(GiftToCity{0, "drachmas", 500});
    a.run_days(2);
    std::string error;
    auto b = Simulation::load(bo::test::catalog_isolated(), a.save(), error);
    REQUIRE(b.has_value());
    CHECK(b->state().trade.partners[1].map_x == 80);
    CHECK(b->state().trade.partners[1].map_y == 30);
    CHECK(b->state().trade.partners[0].diplomacy == a.state().trade.partners[0].diplomacy);
    a.run_days(30);
    b->run_days(30);
    CHECK(a.state_hash() == b->state_hash());
}
