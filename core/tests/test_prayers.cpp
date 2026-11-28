// Prayers and the gods' help without one, god by god (Z:119-124; prayers.cpp, god_help.cpp; L-112, L-130).
#include <algorithm>

#include "bo/core/gods.h"
#include "bo/core/simulation.h"
#include "bo/core/storage.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

BuildingId sanctuary_of(const Simulation& sim) {
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        if (b.sanctuary) {
            return id;
        }
    }
    return kNoBuilding;
}

bool answered(Simulation& sim) {
    for (const auto& e : sim.drain_events()) {
        if (std::holds_alternative<PrayerAnswered>(e.event)) {
            return true;
        }
    }
    return false;
}

std::uint32_t stock(const Simulation& sim, const char* good) { return city_stock(sim.state().city, sim.catalog(), good); }

// A prayer at the god's finished sanctuary, a granary by the street for food gifts.
Simulation praying_to(const std::string& god) {
    auto sim = bo::test::with_god(god);
    // with_god's storehouse held the sanctuary's marble; empty, it takes the gifts of goods.
    sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({50, 17}).building).storage->loads.clear();
    sim.issue(PlaceBuilding{"granary", 44, 16});
    sim.step(1);
    sim.drain_events();
    return sim;
}

// The days until the god looks at the city again (the first day of a month), each with `before` run first.
template <class F>
std::vector<std::string> helped_over(Simulation& sim, int days, F before) {
    std::vector<std::string> gods;
    for (int d = 0; d < days; ++d) {
        before();
        sim.run_days(1);
        for (const auto& e : sim.drain_events()) {
            if (const auto* h = std::get_if<GodHelped>(&e.event)) {
                gods.push_back(h->god);
            }
        }
    }
    return gods;
}

// Runs to the last day before the gods look at the city: what is set up then is still there when they look.
void to_month_end(Simulation& sim) {
    while (sim.state().time.day_in_month() != kDaysPerMonth - 1) {
        sim.run_days(1);
    }
    sim.drain_events();
}

// Runs to the first day of a month just before the gods look (the mythology slot, tick 33 of the day): an
// unopposed invasion started then is still under way when they look.
void to_the_look(Simulation& sim) {
    to_month_end(sim);
    sim.run_days(1);
    sim.step(30);
    sim.drain_events();
}

}  // namespace

TEST_CASE("prayers: Poseidon sends fish, Demeter wheat, Dionysus wine, 24 loads each (L-112)") {
    for (const auto& [god, good] : {std::pair<const char*, const char*>{"poseidon", "fish"}, {"demeter", "wheat"},
                                    {"dionysus", "wine"}}) {
        INFO(god);
        auto sim = praying_to(god);
        const auto before = stock(sim, good);
        sim.issue(Pray{sanctuary_of(sim)});
        sim.step(1);
        CHECK(answered(sim));
        CHECK(stock(sim, good) == before + static_cast<std::uint32_t>(sim.catalog().prayer_goods_loads));
    }
}

TEST_CASE("prayers: Athena gives olives and olive oil, half each; Apollo cures the plague (Z:120, Z:121)") {
    auto athena = praying_to("athena");
    athena.issue(Pray{sanctuary_of(athena)});
    athena.step(1);
    CHECK(answered(athena));
    const auto half = static_cast<std::uint32_t>(athena.catalog().prayer_goods_loads / 2);
    CHECK(stock(athena, "olives") == half);
    CHECK(stock(athena, "olive_oil") == half);

    auto apollo = praying_to("apollo");
    apollo.issue(PlaceBuilding{"common_housing", 20, 18});
    apollo.step(1);
    auto& house = apollo.mutable_state().city.buildings.items.at(apollo.state().city.map.at({20, 18}).building);
    house.house->quarantine_days = 20;
    house.disease_counter = 500;
    apollo.drain_events();
    apollo.issue(Pray{sanctuary_of(apollo)});
    apollo.step(1);
    CHECK(answered(apollo));
    const Building& cured = apollo.state().city.buildings.items.at(apollo.state().city.map.at({20, 18}).building);
    CHECK(cured.house->quarantine_days == 0);
    CHECK(cured.disease_counter == 0);
}

TEST_CASE("prayers: a god with nothing to give keeps his turn; no room for the gift, no cooldown (L-112)") {
    auto sim = bo::test::with_god("demeter");  // no granary: wheat has nowhere to go
    sim.drain_events();
    sim.issue(Pray{sanctuary_of(sim)});
    sim.step(1);
    CHECK_FALSE(answered(sim));
    CHECK(sim.state().city.buildings.items.at(sanctuary_of(sim)).sanctuary->prayer_cooldown == 0);
}

TEST_CASE("spontaneous help: Poseidon and Demeter bring food when the granaries run low (L-130)") {
    for (const auto& [god, good] : {std::pair<const char*, const char*>{"poseidon", "fish"}, {"demeter", "wheat"}}) {
        INFO(god);
        auto sim = praying_to(god);
        const auto gods = helped_over(sim, static_cast<int>(kDaysPerMonth) + 1, [] {});
        CHECK(gods == std::vector<std::string>{god});
        CHECK(stock(sim, good) >= static_cast<std::uint32_t>(sim.catalog().prayer_goods_loads));
    }
}

TEST_CASE("spontaneous help: nothing when the city lacks nothing; Hades when the money runs out (L-130)") {
    auto rich = praying_to("hades");  // 30 000 drachmas: no need
    CHECK(helped_over(rich, static_cast<int>(kDaysPerMonth) + 1, [] {}).empty());
    auto poor = praying_to("hades");
    const auto gods = helped_over(poor, static_cast<int>(kDaysPerMonth) + 1,
                                  [&poor] { poor.mutable_state().finance.treasury = 100; });
    CHECK(gods == std::vector<std::string>{"hades"});
}

TEST_CASE("spontaneous help: Athena when olives and oil run out, Dionysus when the wine does (L-130)") {
    for (const char* god : {"athena", "dionysus"}) {
        INFO(god);
        auto sim = praying_to(god);
        CHECK(helped_over(sim, static_cast<int>(kDaysPerMonth) + 1, [] {}) == std::vector<std::string>{god});
    }
}

TEST_CASE("spontaneous help: Ares with an army abroad, Hephaestus's Talos and Zeus in an invasion (L-130)") {
    auto ares = praying_to("ares");
    TradePartner enemy;
    enemy.id = "sparta";
    enemy.relation = "rival";
    ares.mutable_state().trade.partners.push_back(enemy);
    Expedition away;
    away.city = 0;
    away.days_left = 200;  // still on the road when the god looks
    ares.mutable_state().expeditions.push_back(away);
    CHECK(helped_over(ares, static_cast<int>(kDaysPerMonth) + 1, [] {}) == std::vector<std::string>{"ares"});
    CHECK(ares.state().ares_with_army);

    auto heph = praying_to("hephaestus");
    heph.mutable_state().city.map.at({59, 20}).feature = static_cast<std::uint8_t>(TileFeature::InvasionPoint);
    to_the_look(heph);  // an unopposed invasion is over within a day or two
    heph.issue(StartInvasion{"greek", 20});
    heph.step(1);
    const auto talos = helped_over(heph, static_cast<int>(kDaysPerMonth) + 1, [] {});
    CHECK(std::count(talos.begin(), talos.end(), "hephaestus") == 1);

    auto zeus = praying_to("zeus");  // no army at all: any invasion is stronger (Z:119)
    zeus.mutable_state().city.map.at({59, 20}).feature = static_cast<std::uint8_t>(TileFeature::InvasionPoint);
    to_the_look(zeus);
    zeus.issue(StartInvasion{"greek", 20});
    zeus.step(1);
    const auto great = helped_over(zeus, static_cast<int>(kDaysPerMonth) + 1, [] {});
    CHECK(std::count(great.begin(), great.end(), "zeus") == 1);
}

TEST_CASE("spontaneous help: Aphrodite fills a city of half-empty houses; Hermes a request about to lapse (L-130)") {
    auto love = praying_to("aphrodite");
    love.issue(PlaceBuilding{"common_housing", 20, 18});
    love.step(1);
    const BuildingId house = love.state().city.map.at({20, 18}).building;
    const auto gods = helped_over(love, static_cast<int>(kDaysPerMonth) + 1, [&love, house] {
        love.mutable_state().city.buildings.items.at(house).house->population = 1;  // far below its room
    });
    CHECK(std::count(gods.begin(), gods.end(), "aphrodite") == 1);

    auto hermes = praying_to("hermes");
    TradePartner p;
    p.id = "corinth";
    p.relation = "ally";
    hermes.mutable_state().trade.partners.push_back(p);
    to_month_end(hermes);
    CityRequest r;
    r.city = 0;
    r.good = "drachmas";
    r.amount = 100;
    r.due_day = hermes.state().time.total_days() + 10;  // lapses within 16 days
    hermes.mutable_state().scenario.requests.push_back(r);
    const auto free = helped_over(hermes, static_cast<int>(kDaysPerMonth) + 1, [] {});
    CHECK(std::count(free.begin(), free.end(), "hermes") == 1);
    CHECK(hermes.state().scenario.requests.empty());
}

TEST_CASE("spontaneous help: Artemis when food is very low, not merely low (L-130)") {
    auto sim = praying_to("artemis");
    CHECK(helped_over(sim, static_cast<int>(kDaysPerMonth) + 1, [] {}) == std::vector<std::string>{"artemis"});
    auto fed = praying_to("artemis");
    auto& granary = *fed.mutable_state().city.buildings.items.at(fed.state().city.map.at({44, 16}).building).storage;
    granary.loads["wheat"] = static_cast<std::uint32_t>(fed.catalog().spontaneous.very_low_food_loads);  // not below
    CHECK(helped_over(fed, static_cast<int>(kDaysPerMonth) + 1, [] {}).empty());
}
