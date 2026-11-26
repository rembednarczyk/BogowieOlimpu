// Modules C3 (labour) and C6 (taxes, wages). Numbers: doc 18 (S-004), allocation: doc 24 and K-031.
#include "bo/core/finance.h"
#include "bo/core/labour.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

struct Town {
    Simulation sim;
    Town() : sim(bo::test::catalog_no_migration(), SimulationConfig{3, Difficulty::Hero, 5000, rows()}) {}
    static std::vector<std::string> rows() {
        std::vector<std::string> r(14, std::string(20, '.'));
        r[4] = std::string(20, '=');
        r[10] = std::string(20, '=');
        return r;
    }
    BuildingId at(TilePos p) { return sim.state().city.map.at(p).building; }
    Building& b(TilePos p) { return sim.mutable_state().city.buildings.items.at(at(p)); }
    void house(TilePos p, std::uint32_t people) {
        sim.issue(PlaceBuilding{"common_housing", p.x, p.y});
        sim.step(1);
        b(p).house->population = people;
    }
};

}  // namespace

TEST_CASE("workforce = residents x worker percentage (Hero, normal wages: 42%; very high: 47%)") {
    Town t;
    t.house({0, 2}, 60);
    t.house({2, 2}, 40);
    t.sim.run_days(1);
    CHECK(t.sim.state().labour.workforce == 42);
    t.sim.issue(SetWageRate{WageRate::VeryHigh});
    t.sim.run_days(1);
    CHECK(t.sim.state().labour.workforce == 47);
}

TEST_CASE("shortage: sectors are served in priority order (Z:62, K-031)") {
    Town t;
    t.sim.issue(PlaceBuilding{"fountain", 4, 2});             // hygiene and safety, 4 workers
    t.sim.issue(PlaceBuilding{"maintenance_office", 6, 2});   // hygiene and safety, 5 workers
    t.sim.issue(PlaceBuilding{"gymnasium", 8, 5});            // culture, 7 workers
    t.house({0, 2}, 10);                                      // 4 workers
    t.sim.issue(SetSectorPriority{static_cast<std::uint8_t>(Sector::Culture), 5});
    t.sim.run_days(1);
    const auto& l = t.sim.state().labour;
    CHECK(l.workforce == 4);
    CHECK(l.sector_employed[static_cast<std::size_t>(Sector::Culture)] == 4);
    CHECK(l.sector_employed[static_cast<std::size_t>(Sector::HygieneSafety)] == 0);
    CHECK(t.b({4, 2}).staffing_percent == 0);
}

TEST_CASE("inside a sector every building gets the same share; remainder by fractional part") {
    Town t;
    t.sim.issue(PlaceBuilding{"fountain", 4, 2});            // 4
    t.sim.issue(PlaceBuilding{"maintenance_office", 6, 2});  // 5
    t.house({0, 2}, 15);                                     // floor(15 x 0.42) = 6
    t.sim.run_days(1);
    // 6 of 9: fountain 6x4/9 = 2.67, office 6x5/9 = 3.33 -> 2 and 3, one left for the fountain (0.67 > 0.33).
    CHECK(t.b({4, 2}).workers == 3);
    CHECK(t.b({6, 2}).workers == 3);
    CHECK(t.b({6, 2}).staffing_percent == 60);
}

TEST_CASE("a switched-off industry gets no workers (Z:62-63)") {
    Town t;
    t.sim.issue(PlaceBuilding{"fountain", 4, 2});
    t.house({0, 2}, 60);
    t.sim.issue(SetIndustryEnabled{"fountain", false});
    t.sim.run_days(1);
    CHECK(t.b({4, 2}).workers == 0);
    CHECK(t.sim.state().labour.employed == 0);
}

TEST_CASE("taxes: TRM x residents x rate, only for houses a clerk registered (doc 18)") {
    Town t;
    t.house({0, 2}, 60);
    t.b({0, 2}).house->level = 6;            // townhouse, TRM 2 on Hero
    t.b({0, 2}).coverage["clerk"] = 50;
    t.sim.mutable_state().finance.tax_rate = TaxRate::Normal;  // 0.09
    const MonthResult r = settle_month(t.sim.mutable_state(), bo::test::catalog());
    CHECK(r.taxes == 10);  // 2 x 60 x 0.09 = 10.8 -> 10, 0.8 carried
    CHECK(t.sim.state().finance.tax_remainder == 800);
    t.b({0, 2}).coverage["clerk"] = 0;
    CHECK(settle_month(t.sim.mutable_state(), bo::test::catalog()).taxes == 0);
}

TEST_CASE("wages: yearly wage per worker paid monthly, fractions carried (doc 18)") {
    Town t;
    t.sim.mutable_state().labour.employed = 100;
    t.sim.mutable_state().finance.wage_rate = WageRate::Normal;  // 3 per year
    std::int64_t paid = 0;
    for (int m = 0; m < 12; ++m) {
        paid += settle_month(t.sim.mutable_state(), bo::test::catalog()).wages;
    }
    CHECK(paid == 300);
}

TEST_CASE("clerk coverage lasts 50 days (doc 24)") {
    CHECK(bo::test::catalog().walkers.at("clerk").coverage_days == 50);
    CHECK(bo::test::catalog().walkers.at("water_bearer").coverage_days == 96);
}
