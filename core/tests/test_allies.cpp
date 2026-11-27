// Military help from other cities (Z:140, Z:174; world.cpp request_military_help, military.cpp lend_companies;
// assumptions L-125).
#include "bo/core/military.h"
#include "bo/core/simulation.h"
#include "bo/core/world.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

TradePartner partner(const std::string& id, const std::string& relation, std::int32_t opinion, std::int32_t military) {
    TradePartner p;
    p.id = id;
    p.relation = relation;
    p.diplomacy.opinion = opinion;
    p.diplomacy.military = military;
    return p;
}

// A road from the land entry (west edge, y = 6) past the Palace; an ally, a vassal, a rival.
Simulation allied_city() {
    std::vector<std::string> rows(30, std::string(40, '.'));
    rows[6] = std::string(40, '=');
    rows[6][0] = 'E';
    rows[20][39] = 'i';
    SimulationConfig cfg{21, Difficulty::Hero, 20000, rows, true};
    cfg.trade_partners = {partner("athens", "ally", 85, 4), partner("megara", "vassal", 75, 2),
                          partner("sparta", "rival", 100, 3), partner("argos", "ally", 70, 2)};
    Simulation sim(bo::test::catalog_isolated(), cfg);
    sim.issue(PlaceBuilding{"palace", 20, 7});
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

std::size_t lent(const Simulation& sim) {
    std::size_t n = 0;
    for (const Company& c : sim.state().military.companies) {
        n += c.lender >= 0 ? 1 : 0;
    }
    return n;
}

}  // namespace

TEST_CASE("defensive aid: only while the city is attacked; allies who think very highly of you send it (Z:140, Z:174)") {
    auto sim = allied_city();
    sim.drain_events();
    sim.issue(RequestMilitaryHelp{0, false, 0});
    sim.step(1);
    CHECK(outcome(sim) == "world.no_war");  // lent troops leave when there is no fighting (Z:140)
    sim.issue(StartInvasion{"greek", 8});
    sim.step(1);
    sim.drain_events();
    sim.issue(RequestMilitaryHelp{3, false, 0});  // Argos: an ally at 70, not "very high"
    sim.step(1);
    CHECK(outcome(sim) == "world.request_refused");
    sim.issue(RequestMilitaryHelp{2, false, 0});  // a rival: never
    sim.step(1);
    CHECK(outcome(sim) == "world.request_refused");
    sim.issue(RequestMilitaryHelp{0, false, 0});  // Athens at 85, four shields: three companies [L-125]
    sim.step(1);
    CHECK(outcome(sim) == "ok");
    CHECK(lent(sim) == 3);
    const Company& c = sim.state().military.companies.back();
    CHECK(c.lender == 0);
    CHECK(c.nation == "greek");
    CHECK(c.status == CompanyStatus::Marching);
    CHECK(c.soldiers.size() == 8);
    CHECK(sim.state().walkers.items.at(c.soldiers.front()).tile.x <= 2);  // they come in by the land entry
    sim.issue(RequestMilitaryHelp{0, false, 0});
    sim.step(1);
    CHECK(outcome(sim) == "world.asked_too_recently");
    sim.issue(RequestMilitaryHelp{1, false, 0});  // Megara: a vassal, 65 after the request to Athens (60 enough)
    sim.step(1);
    CHECK(outcome(sim) == "ok");
    CHECK(lent(sim) == 5);
}

TEST_CASE("defensive aid: the lent companies keep their morale and leave when the fighting ends (Z:140)") {
    auto sim = allied_city();
    sim.issue(StartInvasion{"greek", 8});
    sim.step(1);
    sim.issue(RequestMilitaryHelp{0, false, 0});
    sim.step(1);
    REQUIRE(lent(sim) == 3);
    sim.issue(Surrender{});
    sim.step(1);
    CHECK(lent(sim) == 0);
    std::string error;
    auto copy = Simulation::load(bo::test::catalog_isolated(), sim.save(), error);
    REQUIRE(copy.has_value());
    CHECK(copy->state_hash() == sim.state_hash());
}

TEST_CASE("strike: an ally who adores you attacks another city; the weaker army loses a shield (Z:174, L-125)") {
    auto sim = allied_city();
    sim.issue(RequestMilitaryHelp{0, true, 2});  // Athens at 85: a strike needs adoration
    sim.step(1);
    CHECK(outcome(sim) == "world.request_refused");
    sim.mutable_state().trade.partners[0].diplomacy.opinion = 95;
    const auto sparta = sim.state().trade.partners[2].diplomacy.opinion;
    sim.issue(RequestMilitaryHelp{0, true, 2});
    sim.step(1);
    CHECK(outcome(sim) == "ok");
    CHECK(sim.state().trade.partners[2].diplomacy.military == 2);  // 4 shields against 3
    CHECK(sim.state().trade.partners[2].diplomacy.opinion < sparta);
    sim.issue(RequestMilitaryHelp{1, true, 1});  // a city cannot strike itself
    sim.step(1);
    CHECK(outcome(sim) == "world.bad_city");
}

TEST_CASE("defensive aid: lent companies come even to a city with its 20 companies (Z:136, L-125)") {
    auto sim = allied_city();
    sim.issue(StartInvasion{"greek", 8});
    sim.step(1);
    for (int i = 0; i < 20; ++i) {
        Company c;
        c.unit = "rabble";
        sim.mutable_state().military.companies.push_back(c);
    }
    sim.drain_events();
    sim.issue(RequestMilitaryHelp{0, false, 0});
    sim.step(1);
    CHECK(outcome(sim) == "ok");
    CHECK(lent(sim) == 3);
    std::string error;
    auto copy = Simulation::load(bo::test::catalog_isolated(), sim.save(), error);
    REQUIRE(copy.has_value());  // 23 companies in the save
    CHECK(copy->state().military.companies.size() == 23);
}
