// Module D4 stage 1: opinions, gifts, requests, tribute, armies abroad (docs/gameplay/14, 24; Z:167-178;
// L-063, L-064, L-065, L-113).
#include "bo/core/military.h"
#include "bo/core/simulation.h"
#include "bo/core/storage.h"
#include "bo/core/world.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

TradePartner city(const std::string& id, const std::string& relation) {
    TradePartner p;
    p.id = id;
    p.relation = relation;
    p.sells["olive_oil"] = TradeOffer{145, 12, 0};
    p.sells["wheat"] = TradeOffer{40, 16, 0};
    p.buys["wine"] = TradeOffer{200, 8, 0};
    p.diplomacy.military = 2;
    p.diplomacy.economy = 3;
    return p;
}

// A main street on row 6 with a storehouse and a granary; partners: two allies, a vassal, a rival.
Simulation world(std::int64_t treasury = 20000) {
    std::vector<std::string> rows(30, std::string(40, '.'));
    rows[6] = std::string(40, '=');
    rows[20][0] = 'i';
    SimulationConfig cfg{12, Difficulty::Hero, treasury, rows, true};
    cfg.trade_partners = {city("athens", "ally"), city("thebes", "ally"), city("megara", "vassal"),
                          city("sparta", "rival")};
    cfg.trade_partners[2].diplomacy.tribute = 500;
    Simulation sim(bo::test::catalog_isolated(), cfg);
    sim.issue(PlaceBuilding{"storehouse", 10, 3});
    sim.issue(PlaceBuilding{"granary", 20, 2});
    sim.step(1);
    return sim;
}

std::string outcome(Simulation& sim) {
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
            return r->reason_key;
        }
    }
    return "ok";
}

const CityDiplomacy& dip(const Simulation& sim, std::size_t i) { return sim.state().trade.partners[i].diplomacy; }

}  // namespace

TEST_CASE("gifts: +3 a needed load, +1.5 otherwise, at most 3 a year; allies hear of it (doc 24, Z:177)") {
    auto sim = world();
    auto& store = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({10, 3}).building);
    store.storage->loads["wine"] = 4;
    store.storage->loads["fleece"] = 4;
    sim.issue(GiftToCity{0, "wine", 2});  // Athens buys wine: 2 x 3
    sim.step(1);
    CHECK(outcome(sim) == "ok");
    CHECK(dip(sim, 0).opinion == 66);
    CHECK(dip(sim, 1).opinion == 63);  // the other ally: half
    CHECK(dip(sim, 2).opinion == 60);  // a vassal: another state
    sim.issue(GiftToCity{0, "wine", 2});  // 4 in a year: too many
    sim.step(1);
    CHECK(outcome(sim) == "world.gift_refused");
    sim.issue(GiftToCity{0, "fleece", 2});  // not needed: 2 x 1.5
    sim.step(1);
    CHECK(dip(sim, 0).opinion == 69);
    sim.issue(GiftToCity{0, "drachmas", 1000});  // money is always welcome: 2 units x 3
    sim.step(1);
    CHECK(dip(sim, 0).opinion == 75);
    CHECK(sim.state().finance.treasury < 20000 - 1000 + 1);
}

TEST_CASE("requests: allies give food at any opinion, goods only when fairly happy; every city -10 (Z:174, doc 24)") {
    auto sim = world();
    sim.mutable_state().trade.partners[0].diplomacy.opinion = 20;
    sim.issue(RequestFromCity{0, "olive_oil"});
    sim.step(1);
    CHECK(outcome(sim) == "world.request_refused");
    sim.issue(RequestFromCity{0, "wheat"});
    sim.step(1);
    CHECK(outcome(sim) == "ok");
    CHECK(city_stock(sim.state().city, bo::test::catalog(), "wheat") == 8);
    CHECK(dip(sim, 1).opinion == 50);
    sim.issue(RequestFromCity{0, "wheat"});
    sim.step(1);
    CHECK(outcome(sim) == "world.asked_too_recently");
    sim.issue(RequestFromCity{1, "wine"});
    sim.step(1);
    CHECK(outcome(sim) == "world.city_does_not_make_it");
    sim.issue(RequestFromCity{3, "wheat"});  // a rival without a stronger army against it
    sim.step(1);
    CHECK(outcome(sim) == "world.request_refused");
}

TEST_CASE("tribute: a vassal pays every year while it does not hate the player (Z:172)") {
    auto sim = world();
    const std::int64_t before = sim.state().finance.treasury;
    sim.mutable_state().trade.partners[2].diplomacy.opinion = 40;
    sim.run_days(192);
    bool paid = false;
    for (const auto& e : sim.drain_events()) {
        if (const auto* t = std::get_if<TributePaid>(&e.event)) {
            paid = paid || (t->city == 2 && t->amount == 500);
        }
    }
    CHECK(paid);
    CHECK(sim.state().finance.treasury != before);
    sim.mutable_state().trade.partners[2].diplomacy.opinion = 10;
    sim.run_days(192);
    bool again = false;
    for (const auto& e : sim.drain_events()) {
        again = again || std::holds_alternative<TributePaid>(e.event);
    }
    CHECK_FALSE(again);
}

TEST_CASE("a rival that takes the city demands tribute; twice and the episode is lost; beaten off it stops (Z:172)") {
    auto sim = world();
    sim.issue(PlaceBuilding{"palace", 20, 7});
    sim.step(1);
    sim.issue(StartInvasion{"greek", 6, 3});
    sim.issue(Surrender{});
    sim.step(2);
    CHECK(dip(sim, 3).player_defeats == 1);
    CHECK(dip(sim, 3).tribute_demand == 3 * 200);
    CHECK_FALSE(sim.state().episode_lost);
    sim.issue(StartInvasion{"greek", 6, 3});
    sim.issue(Surrender{});
    sim.step(2);
    bool lost = false;
    for (const auto& e : sim.drain_events()) {
        lost = lost || std::holds_alternative<EpisodeLost>(e.event);
    }
    CHECK(lost);
    CHECK(sim.state().episode_lost);
}

TEST_CASE("armies abroad: no rabble; 200 days out, a battle by strength, 200 back; conquest makes a vassal (doc 24)") {
    auto sim = world();
    WorldState& st = sim.mutable_state();
    Company hoplites;
    hoplites.unit = "hoplite";
    hoplites.members.push_back(CompanyMember{kNoBuilding, 8});
    Company more = hoplites;
    Company stones;
    stones.unit = "rabble";
    stones.members.push_back(CompanyMember{kNoBuilding, 8});
    st.military.companies = {hoplites, more, stones};
    sim.issue(DispatchArmy{3, false, "", {2}});
    sim.step(1);
    CHECK(outcome(sim) == "world.bad_company");
    sim.issue(DispatchArmy{2, false, "", {0}});  // a loyal vassal
    sim.step(1);
    CHECK(outcome(sim) == "world.not_at_war");
    sim.issue(DispatchArmy{3, false, "", {0, 1}});  // 16 hoplites against 2 shields = 20 [L-063]
    sim.step(1);
    CHECK(outcome(sim) == "ok");
    CHECK(sim.state().military.companies[0].status == CompanyStatus::ServingAbroad);
    sim.run_days(395);
    CHECK(sim.state().trade.partners[3].relation == "rival");
    sim.run_days(10);
    bool back = false;
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<ExpeditionReturned>(&e.event)) {
            back = r->won == false;  // 16 < 20: lost
        }
    }
    CHECK(back);
    CHECK(sim.state().expeditions.empty());

    sim.mutable_state().trade.partners[3].diplomacy.military = 1;  // 16 > 10
    sim.mutable_state().military.companies = {hoplites, more};     // the companies form again at home
    sim.issue(DispatchArmy{3, false, "", {0, 1}});
    sim.run_days(405);
    CHECK(sim.state().trade.partners[3].relation == "vassal");
    CHECK(dip(sim, 3).tribute == 3 * 200);
}

TEST_CASE("a successful raid brings back the city's yearly amount x the difficulty percent (S-004 trivia)") {
    auto sim = world();
    WorldState& st = sim.mutable_state();
    Company horse;
    horse.unit = "horseman";
    horse.members.push_back(CompanyMember{kNoBuilding, 8});  // 12 strength > 10
    st.trade.partners[3].diplomacy.military = 1;
    st.military.companies = {horse};
    sim.issue(DispatchArmy{3, true, "wheat", {0}});
    sim.run_days(405);
    CHECK(city_stock(sim.state().city, bo::test::catalog(), "wheat") == 16);  // Hero: 100 % of 16
    CHECK(dip(sim, 3).opinion == 0);
}

TEST_CASE("save v21: diplomacy and an army abroad survive a round trip with the same future") {
    auto a = world();
    Company hoplites;
    hoplites.unit = "hoplite";
    hoplites.members.push_back(CompanyMember{kNoBuilding, 8});
    a.mutable_state().military.companies = {hoplites};
    a.issue(DispatchArmy{3, false, "", {0}});
    a.issue(GiftToCity{0, "drachmas", 500});
    a.run_days(30);
    std::string error;
    auto b = Simulation::load(bo::test::catalog_isolated(), a.save(), error);
    REQUIRE(b.has_value());
    CHECK(b->state().expeditions == a.state().expeditions);
    CHECK(b->state().trade.partners[0].diplomacy == a.state().trade.partners[0].diplomacy);
    a.run_days(200);
    b->run_days(200);
    CHECK(a.state_hash() == b->state_hash());
}
