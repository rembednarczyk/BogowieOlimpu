// Lasting gifts of the gods (core/src/blessings.cpp; Z:119-122; assumptions L-131).
#include <algorithm>

#include "bo/core/blessings.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

TEST_CASE("Athena: the city's soldiers strike harder; her grove of four olive trees grows by the sanctuary (Z:120, L-131)") {
    auto plain = bo::test::with_god("");
    auto sim = bo::test::with_god("athena");
    Walker soldier;
    soldier.job = WalkerJob::Soldier;
    Walker invader;
    invader.job = WalkerJob::Invade;
    CHECK(blessed_attack(plain.state(), plain.catalog(), soldier, 8) == 8);
    CHECK(blessed_attack(sim.state(), sim.catalog(), soldier, 8) == 10);  // 125 %
    CHECK(blessed_attack(sim.state(), sim.catalog(), invader, 8) == 8);   // only the city's own
    std::size_t trees = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        if (b.type == "olive_tree") {
            ++trees;
            CHECK(sim.state().city.map.at(b.origin).terrain == Terrain::Meadow);
        }
    }
    CHECK(trees == 4);
}

TEST_CASE("Hermes: traders come more often and walk faster, and so do the deliverymen (Z:122, L-131)") {
    auto plain = bo::test::with_god("");
    auto sim = bo::test::with_god("hermes");
    CHECK(blessed_trader_interval(plain.state().city, plain.catalog(), 30) == 30);
    CHECK(blessed_trader_interval(sim.state().city, sim.catalog(), 30) == 20);
    CHECK(blessed_speed(plain.state().city, plain.catalog(), 1000) == 1000);
    CHECK(blessed_speed(sim.state().city, sim.catalog(), 1000) == 1500);
}

TEST_CASE("Ares: his dragon eats an invader every eight days (Z:121, L-131)") {
    auto sim = bo::test::with_god("ares");
    sim.mutable_state().city.map.at({59, 20}).feature = static_cast<std::uint8_t>(TileFeature::InvasionPoint);
    sim.issue(StartInvasion{"greek", 20});
    sim.step(1);
    REQUIRE(sim.state().military.invasion.active);
    auto fallen = [&sim] {
        std::size_t n = 0;
        for (const WalkerId id : sim.state().military.invasion.invaders) {
            n += sim.state().walkers.items.at(id).hp <= 0 ? 1 : 0;
        }
        return n;
    };
    const auto every = static_cast<std::uint64_t>(sim.catalog().blessings.dragon_every_days);
    auto& st = sim.mutable_state();
    if (st.time.total_days() % every == 0) {
        sim.run_days(1);
    }
    feed_dragon(st, sim.catalog());  // not the dragon's day
    CHECK(fallen() == 0);
    while (st.time.total_days() % every != 0) {
        for (std::uint64_t t = 0; t < kTicksPerDay; ++t) st.time.advance();
    }
    feed_dragon(st, sim.catalog());
    CHECK(fallen() == 1);
    const auto count = sim.state().military.invasion.invaders.size();
    sim.step(1);  // the eaten one leaves the invasion with the day's finished walkers (no zombie invader)
    CHECK(sim.state().military.invasion.invaders.size() < count);
    auto none = bo::test::with_god("athena");  // no Ares, no dragon
    none.mutable_state().city.map.at({59, 20}).feature = static_cast<std::uint8_t>(TileFeature::InvasionPoint);
    none.issue(StartInvasion{"greek", 20});
    none.step(1);
    while (none.state().time.total_days() % every != 0) {
        for (std::uint64_t t = 0; t < kTicksPerDay; ++t) none.mutable_state().time.advance();
    }
    feed_dragon(none.mutable_state(), none.catalog());
    std::size_t n = 0;
    for (const WalkerId id : none.state().military.invasion.invaders) {
        n += none.state().walkers.items.at(id).hp <= 0 ? 1 : 0;
    }
    CHECK(n == 0);
}

namespace {
std::size_t tiles_of(const Simulation& sim, Terrain t) {
    std::size_t n = 0;
    for (const Tile& tile : sim.state().city.map.tiles()) {
        n += tile.terrain == t ? 1 : 0;
    }
    return n;
}
}  // namespace

TEST_CASE("Hades, Hephaestus, Demeter: silver ore, copper ore and meadow by the new sanctuary (Z:119-121, L-131)") {
    const auto meadow = tiles_of(bo::test::with_god(""), Terrain::Meadow);
    CHECK(tiles_of(bo::test::with_god("hades"), Terrain::SilverOre) == 4);
    CHECK(tiles_of(bo::test::with_god("hephaestus"), Terrain::CopperOre) == 4);
    CHECK(tiles_of(bo::test::with_god("demeter"), Terrain::Meadow) > meadow + 10);
    CHECK(tiles_of(bo::test::with_god("athena"), Terrain::SilverOre) == 0);
}

TEST_CASE("Zeus raises every leader's opinion a month; Aphrodite makes the city lovelier (Z:119, Z:122, L-131)") {
    auto sim = bo::test::with_god("zeus");
    TradePartner p;
    p.id = "corinth";
    p.relation = "ally";
    p.diplomacy.opinion = 50;
    sim.mutable_state().trade.partners.push_back(p);
    sim.run_days(kDaysPerMonth);
    CHECK(sim.state().trade.partners[0].diplomacy.opinion == 50 + sim.catalog().blessings.zeus_opinion);
    auto plain = bo::test::with_god("");
    auto lovely = bo::test::with_god("aphrodite");
    CHECK(blessed_appeal(plain.state().city, plain.catalog()) == 0);
    CHECK(blessed_appeal(lovely.state().city, lovely.catalog()) == lovely.catalog().blessings.aphrodite_appeal);
    CHECK(blessed_fishing_ticks(plain.state().city, plain.catalog(), 300) == 300);
    CHECK(blessed_fishing_ticks(bo::test::with_god("poseidon").state().city, plain.catalog(), 300) == 201);  // Poseidon: 67 %
}

TEST_CASE("Hades's Cerberus kills an invader; Artemis hunts down a wolf (Z:120-121, L-131)") {
    auto sim = bo::test::with_god("hades");
    sim.mutable_state().city.map.at({59, 20}).feature = static_cast<std::uint8_t>(TileFeature::InvasionPoint);
    sim.issue(StartInvasion{"greek", 20});
    sim.step(1);
    REQUIRE(sim.state().military.invasion.active);
    auto& st = sim.mutable_state();
    while (st.time.total_days() % static_cast<std::uint64_t>(sim.catalog().blessings.cerberus_every_days) != 0) {
        for (std::uint64_t t = 0; t < kTicksPerDay; ++t) st.time.advance();
    }
    const auto acts = advance_blessings(st, sim.catalog());
    CHECK(std::count(acts.begin(), acts.end(), std::make_pair(std::string("hades"), std::string("cerberus"))) == 1);
    std::size_t gone = 0;
    for (const WalkerId id : st.military.invasion.invaders) {
        gone += st.walkers.items.at(id).phase == WalkerPhase::Done ? 1 : 0;
    }
    CHECK(gone == 1);

    auto hunt = bo::test::with_god("artemis");
    Walker wolf;
    wolf.id = hunt.state().walkers.next_id;
    wolf.type = "wolf";
    hunt.mutable_state().walkers.next_id++;
    hunt.mutable_state().walkers.items.emplace(wolf.id, wolf);
    auto& hs = hunt.mutable_state();
    while (hs.time.total_days() % static_cast<std::uint64_t>(hunt.catalog().blessings.hunt_every_days) != 0) {
        for (std::uint64_t t = 0; t < kTicksPerDay; ++t) hs.time.advance();
    }
    advance_blessings(hs, hunt.catalog());
    CHECK(hs.walkers.items.at(wolf.id).phase == WalkerPhase::Done);
}

TEST_CASE("Poseidon's Kraken sinks an enemy ship with the soldiers aboard (Z:119, L-131)") {
    auto sim = bo::test::with_god("poseidon");
    auto& st = sim.mutable_state();
    Walker ship;
    ship.id = st.walkers.next_id++;
    ship.type = "transport";
    ship.job = WalkerJob::Invade;
    ship.hp = 100;
    ship.aboard = 32;
    st.walkers.items.emplace(ship.id, ship);
    st.military.invasion.active = true;
    st.military.invasion.invaders.push_back(ship.id);
    while (st.time.total_days() % static_cast<std::uint64_t>(sim.catalog().blessings.kraken_every_days) != 0) {
        for (std::uint64_t t = 0; t < kTicksPerDay; ++t) st.time.advance();
    }
    const auto acts = advance_blessings(st, sim.catalog());
    CHECK(std::count(acts.begin(), acts.end(), std::make_pair(std::string("poseidon"), std::string("kraken"))) == 1);
    CHECK(st.walkers.items.at(ship.id).phase == WalkerPhase::Done);
    CHECK(st.walkers.items.at(ship.id).aboard == 0);
}
