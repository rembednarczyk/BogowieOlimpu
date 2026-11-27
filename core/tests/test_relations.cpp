// Long-term relations (L-121; docs/gameplay/14-swiat.md, Z:169, Z:172, Z:178; Z:100, Z:179).
#include <cstdio>

#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

TradePartner partner(const std::string& id, const std::string& relation, std::int32_t opinion) {
    TradePartner p;
    p.id = id;
    p.relation = relation;
    p.diplomacy.opinion = opinion;
    p.diplomacy.military = 2;
    return p;
}

Simulation with(std::vector<TradePartner> partners, std::int64_t treasury = 20000) {
    std::vector<std::string> rows(30, std::string(40, '.'));
    rows[6] = std::string(40, '=');
    rows[20][0] = 'i';
    SimulationConfig cfg{12, Difficulty::Hero, treasury, rows, true};
    cfg.trade_partners = std::move(partners);
    Simulation sim(bo::test::catalog_isolated(), cfg);
    REQUIRE(sim.setup_error().empty());
    sim.issue(PlaceBuilding{"palace", 10, 0});
    sim.step(1);
    return sim;
}

template <typename T>
std::size_t count(Simulation& sim) {
    std::size_t n = 0;
    for (const auto& e : sim.drain_events()) {
        n += std::holds_alternative<T>(e.event) ? 1 : 0;
    }
    return n;
}

constexpr std::uint64_t kMonth = 16;

}  // namespace

TEST_CASE("a vassal unhappy for a year stops its tribute and rises (Z:172, Z:178, L-121)") {
    auto sim = with({partner("megara", "vassal", 20)});
    sim.run_days(11 * kMonth);
    CHECK_FALSE(sim.state().trade.partners[0].diplomacy.rebelling);
    sim.run_days(kMonth + 1);
    CHECK(sim.state().trade.partners[0].diplomacy.rebelling);
    CHECK(count<CityRebelled>(sim) == 1);
}

TEST_CASE("a content vassal does not rise (L-121)") {
    auto sim = with({partner("megara", "vassal", 40)});
    sim.run_days(30 * kMonth);
    CHECK_FALSE(sim.state().trade.partners[0].diplomacy.rebelling);
}

TEST_CASE("a rival that hates the player for a year marches on the city (Z:178, L-121)") {
    auto sim = with({partner("sparta", "rival", 5)});
    sim.run_days(12 * kMonth + 1);
    CHECK(count<InvasionStarted>(sim) == 1);  // without defenders the battle may already be over (L-110)
}

TEST_CASE("an ally fond of the player for two years sends a gift; allies help a city in debt (L-121)") {
    auto fond = with({partner("athens", "ally", 90)});
    const std::int64_t before = fond.state().finance.treasury;
    fond.run_days(24 * kMonth + 1);
    CHECK(count<CityGift>(fond) == 1);
    CHECK(fond.state().finance.treasury > before);  // the gift outweighs the Palace's few wages here

    auto debt = with({partner("athens", "ally", 60), partner("sparta", "rival", 60)}, -500);
    debt.run_days(7 * kMonth);
    CHECK(count<CityGift>(debt) == 1);  // after half a year of debt, from the ally only, once a year
    CHECK(debt.state().finance.treasury > 0);
}

TEST_CASE("a city's call for troops is answered by sending companies; unanswered, it lapses without penalty (Z:161, L-122)") {
    auto sim = with({partner("athens", "ally", 50)});
    auto& m = sim.mutable_state().military;
    Company rabble;
    rabble.unit = "rabble";
    rabble.members.push_back(CompanyMember{9999, 8});
    Company hoplites;
    hoplites.unit = "hoplite";
    hoplites.members.push_back(CompanyMember{9999, 8});
    m.companies = {rabble, hoplites};
    CityRequest r;
    r.city = 0;
    r.good = "army";
    r.amount = 1;
    r.due_day = sim.state().time.total_days() + 30;
    sim.mutable_state().scenario.requests.push_back(r);
    sim.issue(FulfillRequest{0});
    sim.step(1);
    CHECK(sim.state().scenario.requests.empty());
    REQUIRE(sim.state().expeditions.size() == 1);
    CHECK(sim.state().expeditions[0].aid);
    CHECK(sim.state().expeditions[0].companies == std::vector<std::uint32_t>{1});  // the hoplites, not the rabble
    CHECK(sim.state().trade.partners[0].diplomacy.opinion == 60);

    auto late = with({partner("athens", "ally", 50)});
    r.due_day = late.state().time.total_days() + 5;
    late.mutable_state().scenario.requests.push_back(r);
    late.run_days(7);
    CHECK(late.state().scenario.requests.empty());
    CHECK(late.state().trade.partners[0].diplomacy.opinion == 50);  // no penalty: it was never overdue
}
