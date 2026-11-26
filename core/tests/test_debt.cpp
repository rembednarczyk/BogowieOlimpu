// Debt (L-034, doc 24; Z:16): a notice every year of continuous debt, no immigrants after a year, the episode
// is lost after three years (eZeus model for the first two, S-008).
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

std::size_t count_notices(Simulation& sim) {
    std::size_t n = 0;
    for (const auto& e : sim.drain_events()) {
        n += std::holds_alternative<DebtNotice>(e.event) ? 1 : 0;
    }
    return n;
}

Simulation in_debt() {
    std::vector<std::string> rows(12, std::string(30, '.'));
    rows[5] = std::string(30, '=');
    rows[5][0] = 'E';
    Simulation sim(bo::test::catalog(), SimulationConfig{5, Difficulty::Hero, -1000, rows, true});
    REQUIRE(sim.setup_error().empty());
    return sim;
}

}  // namespace

TEST_CASE("L-034: a year of debt brings a notice and stops immigration") {
    auto sim = in_debt();
    sim.run_days(191);
    CHECK(count_notices(sim) == 0);
    sim.run_days(2);
    CHECK(count_notices(sim) == 1);
    for (std::int32_t x = 2; x < 28; x += 2) {
        sim.issue(PlaceBuilding{"common_housing", x, 3});
    }
    sim.run_days(30);
    std::uint32_t people = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        people += b.house ? b.house->population + b.house->incoming : 0;
    }
    CHECK(people == 0);
    CHECK_FALSE(sim.state().episode_lost);
}

TEST_CASE("L-034: three years of continuous debt lose the episode; paying it off starts the count again") {
    auto sim = in_debt();
    sim.run_days(2 * 192);
    sim.mutable_state().finance.treasury = 10;  // out of debt for a day
    sim.run_days(1);
    CHECK(sim.state().debt_days == 0);
    sim.mutable_state().finance.treasury = -10;
    sim.run_days(3 * 192 - 2);
    CHECK_FALSE(sim.state().episode_lost);
    sim.run_days(3);
    CHECK(sim.state().episode_lost);
    bool lost_event = false;
    for (const auto& e : sim.drain_events()) {
        lost_event = lost_event || std::holds_alternative<EpisodeLost>(e.event);
    }
    CHECK(lost_event);
}
