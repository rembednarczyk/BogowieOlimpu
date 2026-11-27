// Module D3 stage 2: invasions, bribe, surrender, combat, morale (docs/gameplay/11, 22, 24; Z:137-144; L-110).
#include "bo/core/migration.h"
#include "bo/core/military.h"
#include "bo/core/serialize.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// A main street on row 6, an invasion point at the west edge of row 20, three hovels and (optionally) a Palace.
Simulation city(bool palace, std::int64_t treasury = 20000) {
    std::vector<std::string> rows(30, std::string(40, '.'));
    rows[6] = std::string(40, '=');
    rows[20][0] = 'i';
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{5, Difficulty::Hero, treasury, rows});
    for (int x = 2; x < 8; x += 2) {
        sim.issue(PlaceBuilding{"common_housing", x, 4});
    }
    if (palace) {
        sim.issue(PlaceBuilding{"palace", 20, 7});
    }
    sim.step(1);
    return sim;
}

void keep_hovels(Simulation& sim) {
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        if (b.house) {
            b.house->level = 2;  // Hovel: 5 rabble each (doc 22)
            b.house->population = std::max<std::uint32_t>(b.house->population, 6);
            b.house->failed_checks = 0;
        }
    }
}

void fill_hovels(Simulation& sim) {
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        if (b.house) {
            b.house->population = 24;
        }
    }
}

// Runs until the invasion ends (or `days` pass) and returns InvasionEnded::result, -1 if it did not end.
int run_battle(Simulation& sim, int days) {
    for (int t = 0; t < days * static_cast<int>(kTicksPerDay); ++t) {
        if (t % static_cast<int>(kTicksPerDay) == 0) {
            keep_hovels(sim);
        }
        sim.step(1);
        for (const auto& e : sim.drain_events()) {
            if (const auto* end = std::get_if<InvasionEnded>(&e.event)) {
                return end->result;
            }
        }
    }
    return -1;
}

std::string rejection(Simulation& sim) {
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
            return r->reason_key;
        }
    }
    return {};
}

}  // namespace

TEST_CASE("invaders: composition rounded down, bribe 100 / 200 dr x difficulty (doc 22)") {
    auto sim = city(true);
    sim.issue(StartInvasion{"greek", 10});  // 60/20/20: 6 hoplites, 2 rabble, 2 horsemen
    sim.step(1);
    std::uint32_t started = 0;
    std::int64_t bribe = 0;
    for (const auto& e : sim.drain_events()) {
        if (const auto* s = std::get_if<InvasionStarted>(&e.event)) {
            started = s->soldiers;
            bribe = s->bribe;
        }
    }
    CHECK(started == 10);
    CHECK(bribe == 8 * 100 + 2 * 200);  // Hero: 100 %
    sim.issue(StartInvasion{"persian", 7});  // 30/40/30 of 7: 2 + 2 + 2
    sim.step(1);
    CHECK(rejection(sim) == "military.bad_invasion");  // one at a time
    CHECK(sim.state().military.invasion.invaders.size() == 10);
}

TEST_CASE("bribe: paid from the treasury and they leave; not enough money is refused (Z:137)") {
    auto poor = city(true, 500);
    poor.issue(StartInvasion{"greek", 10});
    poor.issue(BribeInvaders{});
    poor.step(1);
    CHECK(rejection(poor) == "military.cannot_afford_bribe");

    auto sim = city(true);
    sim.issue(StartInvasion{"greek", 10});
    sim.step(1);
    const std::int64_t before = sim.state().finance.treasury;
    sim.issue(BribeInvaders{});
    CHECK(run_battle(sim, 1) == 1);
    CHECK(sim.state().finance.treasury == before - 1200);
    CHECK_FALSE(sim.state().military.invasion.active);
    sim.run_days(20);
    CHECK(sim.state().walkers.items.empty());  // they walked out
}

TEST_CASE("surrender ends the invasion; with no defenders the invaders take the Palace (Z:137, Z:144)") {
    auto giving = city(true);
    giving.issue(StartInvasion{"greek", 5});
    giving.issue(Surrender{});
    CHECK(run_battle(giving, 1) == 2);

    auto sim = city(true);
    sim.issue(StartInvasion{"greek", 5});
    CHECK(run_battle(sim, 30) == 3);
}

TEST_CASE("rabble defeats a lone hoplite with stones (missiles 6 - 2 armor, doc 22); companies fight") {
    auto sim = city(true);
    fill_hovels(sim);
    run_battle(sim, 2);
    REQUIRE(sim.state().military.companies.size() == 2);
    sim.issue(MusterCompany{0});
    sim.issue(MusterCompany{1});
    run_battle(sim, 8);
    sim.issue(StartInvasion{"greek", 2});  // 60 % of 2: one hoplite
    CHECK(run_battle(sim, 30) == 0);
    CHECK(sim.state().military.invasion.invaders.empty());
}

TEST_CASE("a soldier's death costs his house 6 people; low morale makes the company flee (doc 22, Z:140)") {
    auto sim = city(true);
    fill_hovels(sim);
    run_battle(sim, 2);
    REQUIRE_FALSE(sim.state().military.companies.empty());
    sim.issue(MusterCompany{0});
    run_battle(sim, 8);
    sim.mutable_state().military.companies[0].morale = 30;  // one more death breaks it
    std::uint32_t people = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        people += b.house ? b.house->population : 0;
    }
    sim.issue(StartInvasion{"greek", 40});
    bool fled = false;
    for (int d = 0; d < 30 && !fled; ++d) {
        sim.step(kTicksPerDay);
        fled = sim.state().military.companies[0].fled;
    }
    REQUIRE(fled);
    std::uint32_t after = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        after += b.house ? b.house->population : 0;
    }
    CHECK(after <= people - 6);
    sim.drain_events();
    sim.issue(MusterCompany{0});
    sim.step(1);
    const std::string why = rejection(sim);
    CHECK((why == "military.company_fled" || why == "military.bad_company"));
}

TEST_CASE("save v18: a battle in progress survives a round trip with the same future") {
    auto a = city(true);
    fill_hovels(a);
    run_battle(a, 2);
    a.issue(MusterCompany{0});
    run_battle(a, 6);
    a.issue(StartInvasion{"greek", 12});
    a.step(kTicksPerDay * 6);
    std::string error;
    auto b = Simulation::load(bo::test::catalog_isolated(), a.save(), error);
    REQUIRE(b.has_value());
    CHECK(b->state().military == a.state().military);
    a.step(kTicksPerDay * 3);
    b->step(kTicksPerDay * 3);
    CHECK(a.state_hash() == b->state_hash());
}

TEST_CASE("an army in the field with no enemy in the city lowers happiness (Z:159, doc 24: -2 per update)") {
    auto sim = city(true);
    WorldState& st = sim.mutable_state();
    for (auto& [id, b] : st.city.buildings.items) {
        if (b.house) {
            b.house->level = 2;
            b.house->population = 150;  // above the 300 of a city that does not complain (doc 24)
            b.house->happiness = 50;
        }
    }
    auto at_home = st;
    Company c;
    c.unit = "rabble";
    c.status = CompanyStatus::Still;
    st.military.companies.push_back(c);
    at_home.military.companies.push_back(Company{});
    update_moods(st, bo::test::catalog_isolated());
    update_moods(at_home, bo::test::catalog_isolated());
    const auto& a = st.city.buildings.items.begin()->second;
    const auto& b = at_home.city.buildings.items.begin()->second;
    REQUIRE(a.house);
    CHECK(static_cast<int>(a.house->happiness) == static_cast<int>(b.house->happiness) - 2);
    st.military.invasion.active = true;  // a real battle: no penalty
    auto fighting = st;
    fighting.city = at_home.city;
    for (auto& [id, h] : fighting.city.buildings.items) {
        if (h.house) {
            h.house->happiness = 50;
        }
    }
    auto calm = fighting;
    calm.military.companies.clear();
    update_moods(fighting, bo::test::catalog_isolated());
    update_moods(calm, bo::test::catalog_isolated());
    CHECK(fighting.city.buildings.items.begin()->second.house->happiness ==
          calm.city.buildings.items.begin()->second.house->happiness);
}

namespace {

// How far a lone hoplite of a company with `tactic` walks in a day when an invader is four tiles away.
std::int32_t hoplite_moves(Tactic tactic) {
    auto sim = city(true);
    sim.issue(StartInvasion{"greek", 2});  // 60 % of 2: one hoplite (doc 22)
    sim.step(1);
    REQUIRE(!sim.state().military.invasion.invaders.empty());
    auto& st = sim.mutable_state();
    for (const WalkerId id : st.military.invasion.invaders) {
        st.walkers.items.at(id).speed_raw = 0;  // the invaders stay put for the test
    }
    Walker& enemy = st.walkers.items.at(st.military.invasion.invaders.front());
    const TilePos at{enemy.tile.x + 4, enemy.tile.y};
    Walker w;
    w.id = st.walkers.next_id++;
    w.type = "hoplite";
    w.job = WalkerJob::Soldier;
    w.phase = WalkerPhase::AtTarget;
    w.tile = at;
    w.hp = 150;
    Company c;
    c.unit = "hoplite";
    c.status = CompanyStatus::Still;
    c.standard = at;
    c.soldiers.push_back(w.id);
    c.tactic = tactic;
    st.walkers.items.emplace(w.id, w);
    st.military.companies.push_back(c);
    sim.run_days(1);
    const auto it = sim.state().walkers.items.find(w.id);
    return it == sim.state().walkers.items.end() ? 99 : std::abs(it->second.tile.x - at.x) + std::abs(it->second.tile.y - at.y);
}

}  // namespace

TEST_CASE("tactics: a defensive company holds its ground, an offensive one charges the enemy (Z:138-139)") {
    CHECK(hoplite_moves(Tactic::Defensive) == 0);
    CHECK(hoplite_moves(Tactic::Offensive) > 0);
}

TEST_CASE("tactics: the order is kept in the company and in the save (Z:138)") {
    auto sim = city(true);
    fill_hovels(sim);
    run_battle(sim, 2);
    REQUIRE(!sim.state().military.companies.empty());
    CHECK(sim.state().military.companies[0].tactic == Tactic::Defensive);  // the default (Z:138)
    sim.issue(SetCompanyTactic{0, 2});
    sim.step(1);
    CHECK(sim.state().military.companies[0].tactic == Tactic::Special);
    std::string error;
    auto copy = Simulation::load(bo::test::catalog_isolated(), sim.save(), error);
    REQUIRE(copy.has_value());
    CHECK(copy->state().military.companies[0].tactic == Tactic::Special);
}

TEST_CASE("B-017: the companies formed anew each day keep the order the player gave them (Z:138)") {
    auto sim = city(true);
    fill_hovels(sim);
    for (int day = 0; day < 2; ++day) {
        keep_hovels(sim);
        sim.run_days(1);
    }
    REQUIRE(!sim.state().military.companies.empty());
    sim.issue(SetCompanyTactic{0, 1});
    for (int day = 0; day < 3; ++day) {  // the companies at home are formed again every day
        keep_hovels(sim);
        sim.run_days(1);
    }
    CHECK(sim.state().military.companies[0].tactic == Tactic::Offensive);
}

namespace {

// A lone soldier of `unit` with `tactic`, `gap` tiles east of a standing invader; his standard `standard_gap`
// tiles east of the invader.
struct Duel {
    Simulation sim;
    WalkerId soldier = kNoWalker;
    WalkerId enemy = kNoWalker;
};

Duel duel(const std::string& unit, Tactic tactic, std::int32_t gap, std::int32_t standard_gap) {
    Duel d{city(true)};
    d.sim.issue(StartInvasion{"greek", 2});
    d.sim.step(1);
    REQUIRE(!d.sim.state().military.invasion.invaders.empty());
    auto& st = d.sim.mutable_state();
    for (const WalkerId id : st.military.invasion.invaders) {
        st.walkers.items.at(id).speed_raw = 0;
    }
    d.enemy = st.military.invasion.invaders.front();
    Walker& enemy = st.walkers.items.at(d.enemy);
    enemy.hp = 100000;  // it must outlast the test
    const UnitStats& s = d.sim.catalog().combat.nations.at("player_greek").units.at(unit);
    Walker w;
    w.id = st.walkers.next_id++;
    w.type = unit;
    w.job = WalkerJob::Soldier;
    w.phase = WalkerPhase::AtTarget;
    w.tile = {enemy.tile.x + gap, enemy.tile.y};
    w.hp = 100000;
    w.speed_raw = s.speed_raw;
    Company c;
    c.unit = unit;
    c.status = CompanyStatus::Still;
    c.standard = {enemy.tile.x + standard_gap, enemy.tile.y};
    c.soldiers.push_back(w.id);
    c.tactic = tactic;
    d.soldier = w.id;
    st.walkers.items.emplace(w.id, w);
    st.military.companies.push_back(c);
    return d;
}

std::int32_t gap(const Duel& d) {
    const auto& items = d.sim.state().walkers.items;
    const TilePos a = items.at(d.soldier).tile;
    const TilePos b = items.at(d.enemy).tile;
    return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

}  // namespace

TEST_CASE("special tactics: rabble skirmish, stepping back from an enemy at hand to throw again (Z:139)") {
    auto special = duel("rabble", Tactic::Special, 1, 1);
    auto defensive = duel("rabble", Tactic::Defensive, 1, 1);
    special.sim.step(60);
    defensive.sim.step(60);
    CHECK(gap(special) > 1);
    CHECK(gap(defensive) <= 1);
}

TEST_CASE("special tactics: hoplites in a wedge strike harder than in an offensive line (Z:139)") {
    auto wedge = duel("hoplite", Tactic::Special, 1, 1);
    auto line = duel("hoplite", Tactic::Offensive, 1, 1);
    wedge.sim.step(200);
    line.sim.step(200);
    const auto hp = [](const Duel& d) { return d.sim.state().walkers.items.at(d.enemy).hp; };
    CHECK(hp(wedge) < hp(line));
}

TEST_CASE("special tactics: horsemen harass, riding back to their standard after a blow (Z:139)") {
    auto harass = duel("horseman", Tactic::Special, 1, 5);
    auto charge = duel("horseman", Tactic::Offensive, 1, 5);
    harass.sim.step(10);
    charge.sim.step(10);
    CHECK(gap(harass) > 1);
    CHECK(gap(charge) <= 1);
}
