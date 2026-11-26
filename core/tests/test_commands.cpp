#include <variant>

#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

TEST_CASE("commands execute at the next step in issue order and emit events") {
    Simulation sim(bo::test::catalog(), SimulationConfig{1, std::nullopt, 0});
    sim.issue(SetTaxRate{TaxRate::High});
    sim.issue(SetTaxRate{TaxRate::Low});
    CHECK(sim.state().finance.tax_rate == TaxRate::Normal);  // not applied before the step
    sim.step();
    CHECK(sim.state().finance.tax_rate == TaxRate::Low);    // later command wins
    const auto events = sim.drain_events();
    int tax_events = 0;
    for (const auto& e : events) {
        tax_events += std::holds_alternative<TaxRateChanged>(e.event) ? 1 : 0;
    }
    CHECK(tax_events == 2);
    CHECK(sim.executed_commands().size() == 2);
}

TEST_CASE("scheduled commands wait for their tick") {
    Simulation sim(bo::test::catalog(), SimulationConfig{1, std::nullopt, 0});
    sim.issue_at(100, SetWageRate{WageRate::High});
    sim.step(100);
    CHECK(sim.state().finance.wage_rate == WageRate::Normal);
    sim.step();
    CHECK(sim.state().finance.wage_rate == WageRate::High);
}

TEST_CASE("calendar emits day, month and year events") {
    Simulation sim(bo::test::catalog(), SimulationConfig{1, std::nullopt, 0});
    sim.run_days(kDaysPerMonth * kMonthsPerYear + 1);
    int days = 0;
    int months = 0;
    int years = 0;
    for (const auto& e : sim.drain_events()) {
        days += std::holds_alternative<DayStarted>(e.event) ? 1 : 0;
        months += std::holds_alternative<MonthStarted>(e.event) ? 1 : 0;
        years += std::holds_alternative<YearStarted>(e.event) ? 1 : 0;
    }
    CHECK(days == 193);
    CHECK(months == 13);
    CHECK(years == 2);
}

TEST_CASE("default difficulty comes from data (R-013: Hero)") {
    Simulation sim(bo::test::catalog(), SimulationConfig{});
    CHECK(sim.state().difficulty == Difficulty::Hero);
}
