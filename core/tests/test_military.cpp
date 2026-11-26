// Module D3 stage 1: companies, standards, muster, move, send home (docs/gameplay/11, 22; Z:135-140; L-109).
#include "bo/core/military.h"
#include "bo/core/serialize.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

constexpr std::uint8_t kHovel = 2;  // common level index: hut, shack, hovel

Simulation town() {
    std::vector<std::string> rows(30, std::string(40, '.'));
    rows[6] = std::string(40, '=');
    return Simulation(bo::test::catalog_isolated(), SimulationConfig{5, Difficulty::Hero, 20000, rows});
}

// Three house plots on the main street, kept at Hovel with 24 residents (5 rabble each, doc 22).
void keep_hovels(Simulation& sim) {
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        if (b.house) {
            b.house->level = kHovel;
            b.house->population = 24;
            b.house->failed_checks = 0;
        }
    }
}

void run(Simulation& sim, int days) {
    for (int d = 0; d < days; ++d) {
        keep_hovels(sim);
        sim.run_days(1);
    }
}

Simulation army_town(bool palace) {
    auto sim = town();
    for (int x = 2; x < 8; x += 2) {
        sim.issue(PlaceBuilding{"common_housing", x, 4});
    }
    if (palace) {
        sim.issue(PlaceBuilding{"palace", 20, 7});
    }
    sim.step(1);
    run(sim, 2);
    return sim;
}

bool rejected(Simulation& sim, const std::string& code) {
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event); r && r->reason_key == code) {
            return true;
        }
    }
    return false;
}

}  // namespace

TEST_CASE("soldiers per house: round(population x %S), elite limited by armor and horses (doc 22, Z:136)") {
    const auto& cat = bo::test::catalog();
    HouseState hovel;
    hovel.level = kHovel;
    hovel.population = 24;
    CHECK(house_soldiers(cat, hovel, Difficulty::Hero) == 5);
    hovel.population = 12;
    CHECK(house_soldiers(cat, hovel, Difficulty::Hero) == 2);
    HouseState mansion;
    mansion.elite = true;
    mansion.level = 1;
    mansion.population = 10;
    CHECK(house_soldiers(cat, mansion, Difficulty::Hero) == 0);  // no armor
    mansion.goods[static_cast<std::size_t>(HouseGood::Armor)] = 1;
    CHECK(house_soldiers(cat, mansion, Difficulty::Hero) == 1);
    mansion.goods[static_cast<std::size_t>(HouseGood::Armor)] = 3;
    CHECK(house_soldiers(cat, mansion, Difficulty::Hero) == 2);
    HouseState estate;
    estate.elite = true;
    estate.level = 3;
    estate.population = 20;
    estate.goods[static_cast<std::size_t>(HouseGood::Armor)] = 4;
    estate.goods[static_cast<std::size_t>(HouseGood::Horses)] = 3;
    CHECK(house_soldiers(cat, estate, Difficulty::Hero) == 3);
}

TEST_CASE("no Palace, no army (Z:135); with one, companies of 8 form with standards around it (Z:136)") {
    auto none = army_town(false);
    CHECK(none.state().military.companies.empty());
    none.issue(MusterCompany{0});
    none.step(1);
    CHECK(rejected(none, "military.no_palace"));

    auto sim = army_town(true);
    const auto& companies = sim.state().military.companies;
    REQUIRE(companies.size() == 2);  // 15 rabble: 8 + 7
    CHECK(companies[0].unit == "rabble");
    std::size_t soldiers = 0;
    for (const auto& m : companies[0].members) {
        soldiers += m.soldiers;
    }
    CHECK(soldiers == 8);
    CHECK(companies[0].standard != companies[1].standard);
    for (const auto& c : companies) {
        CHECK(sim.state().city.map.at(c.standard).building == kNoBuilding);
        CHECK(c.status == CompanyStatus::AtHome);
    }
}

TEST_CASE("muster: soldiers march to the standard, 6 workers each leave work; move; send home (Z:137-139)") {
    auto sim = army_town(true);
    const std::uint32_t workforce = sim.state().labour.workforce;
    REQUIRE(workforce > 0);
    sim.issue(MusterCompany{0});
    sim.step(1);
    CHECK(sim.state().military.companies[0].status == CompanyStatus::Marching);
    CHECK(sim.state().military.companies[0].soldiers.size() == 8);
    run(sim, 6);
    CHECK(sim.state().military.companies[0].status == CompanyStatus::Still);
    CHECK(sim.state().labour.workforce == workforce - std::min<std::uint32_t>(workforce, 8 * 6));  // doc 22
    sim.issue(MusterCompany{0});
    sim.step(1);
    CHECK(rejected(sim, "military.bad_company"));

    sim.issue(MoveCompany{0, 30, 20});
    sim.step(1);
    CHECK(sim.state().military.companies[0].status == CompanyStatus::Marching);
    run(sim, 8);
    CHECK(sim.state().military.companies[0].status == CompanyStatus::Still);
    const Walker& first = sim.state().walkers.items.at(sim.state().military.companies[0].soldiers[0]);
    CHECK(std::max(std::abs(first.tile.x - 30), std::abs(first.tile.y - 20)) <= 2);

    sim.issue(SendCompanyHome{0});
    sim.step(1);
    CHECK(sim.state().military.companies[0].status == CompanyStatus::GoingHome);
    run(sim, 14);  // about 25 tiles at 4.5 tiles a day (rabble 72.53 tiles a month, doc 22)
    CHECK(sim.state().military.companies[0].status == CompanyStatus::AtHome);
    CHECK(sim.state().walkers.items.empty());
    CHECK(sim.state().labour.workforce == workforce);
}

TEST_CASE("save v17: a company in the field survives a round trip with the same future") {
    auto a = army_town(true);
    a.issue(MusterCompany{1});
    a.step(40);
    std::string error;
    auto b = Simulation::load(bo::test::catalog_isolated(), a.save(), error);
    REQUIRE(b.has_value());
    CHECK(b->state().military == a.state().military);
    a.run_days(3);
    b->run_days(3);
    CHECK(a.state_hash() == b->state_hash());
}
