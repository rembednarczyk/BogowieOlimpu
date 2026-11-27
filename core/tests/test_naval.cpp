// Fleets (docs/gameplay/11-wojsko.md Z:141-144; naval.cpp; assumptions L-124).
#include <algorithm>

#include "bo/core/military.h"
#include "bo/core/naval.h"
#include "bo/core/serialize.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// Land in the north (Palace at 2,2, a road along y = 8), the sea from y = 12, ships enter at the south-east corner.
Simulation sea_city() {
    std::vector<std::string> rows(20, std::string(30, '.'));
    rows[8] = std::string(30, '=');
    for (std::size_t y = 12; y < 20; ++y) {
        rows[y] = std::string(30, '~');
    }
    rows[19][29] = 'S';
    Simulation sim(bo::test::catalog_isolated(), SimulationConfig{16, Difficulty::Hero, 30000, rows, true});
    sim.issue(PlaceBuilding{"palace", 2, 2});
    sim.step(1);
    sim.issue(PlaceBuilding{"trireme_wharf", 10, 9});
    sim.step(1);
    return sim;
}

BuildingId wharf_of(const Simulation& sim) {
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        if (b.type == "trireme_wharf") {
            return id;
        }
    }
    return kNoBuilding;
}

// The wharf has built its trireme: it is launched the next day, unmanned (Z:150); `crew`: its crew called.
Simulation with_trireme(bool crew = true) {
    auto sim = sea_city();
    REQUIRE(wharf_of(sim) != kNoBuilding);
    sim.mutable_state().city.buildings.items.at(wharf_of(sim)).producer->output_loads = 1;
    sim.run_days(1);
    REQUIRE(sim.state().military.ships.size() == 1);
    CHECK(sim.state().military.ships[0].crew == Crew::None);
    if (crew) {
        sim.issue(SetShipOrder{0, 4, -1, -1});  // Call Crew to Ship (Z:141)
        sim.step(1);
    }
    return sim;
}

Walker& trireme(Simulation& sim) { return sim.mutable_state().walkers.items.at(sim.state().military.ships[0].walker); }

std::uint32_t landed(const Simulation& sim) {
    std::uint32_t n = 0;
    for (const WalkerId id : sim.state().military.invasion.invaders) {
        const Walker& w = sim.state().walkers.items.at(id);
        n += is_ship(w) ? 0 : 1;
    }
    return n;
}

std::vector<WalkerId> ships_of_type(const Simulation& sim, const std::string& type) {
    std::vector<WalkerId> out;
    for (const WalkerId id : sim.state().military.invasion.invaders) {
        if (sim.state().walkers.items.at(id).type == type) {
            out.push_back(id);
        }
    }
    return out;
}

// Runs until no transport has soldiers aboard (at most 30 days).
void until_landed(Simulation& sim) {
    for (int day = 0; day < 30; ++day) {
        const auto& inv = sim.state().military.invasion.invaders;
        if (std::none_of(inv.begin(), inv.end(), [&sim](WalkerId id) {
                const Walker& w = sim.state().walkers.items.at(id);
                return is_transport(w) && w.aboard > 0 && w.hp > 0;
            })) {
            return;
        }
        sim.run_days(1);
    }
}

}  // namespace

TEST_CASE("fleet: a wharf launches its trireme at the mooring and builds another when it is sunk (Z:141, S-008)") {
    auto sim = with_trireme();
    const Walker& t = trireme(sim);
    CHECK(t.type == "trireme");
    CHECK(t.tile.y == 12);  // on the water by the wharf
    CHECK(sim.state().military.ships[0].order == ShipOrder::Defend);  // the default (Z:142)
    trireme(sim).hp = 0;
    trireme(sim).phase = WalkerPhase::Done;  // sunk
    sim.run_days(1);
    CHECK(sim.state().military.ships.empty());
    CHECK(sim.state().city.buildings.items.at(wharf_of(sim)).producer->output_loads == 0);  // building again
}

TEST_CASE("fleet: an army by sea comes in transports of 32 with warships and lands by the Palace (Z:144, S-008)") {
    auto sim = sea_city();
    sim.issue(StartInvasion{"greek", 40, -1, true});
    sim.step(1);
    const Invasion& inv = sim.state().military.invasion;
    REQUIRE(inv.active);
    CHECK(inv.by_sea);
    CHECK(ships_of_type(sim, "transport").size() == 2);  // 32 + 8
    CHECK(ships_of_type(sim, "warship").size() == 1);    // one for every two transports [L-124]
    CHECK(inv.landing.y == 12);
    CHECK(inv.landing.x <= 4);  // the water nearest the Palace
    CHECK(landed(sim) == 0);
    until_landed(sim);
    CHECK(landed(sim) == 40);  // all ashore, marching on the Palace
}

TEST_CASE("fleet: the soldiers of a sunk transport never land (S-008)") {
    auto sim = sea_city();
    sim.issue(StartInvasion{"greek", 40, -1, true});
    sim.step(1);
    for (const WalkerId id : ships_of_type(sim, "transport")) {
        Walker& t = sim.mutable_state().walkers.items.at(id);
        if (t.aboard == 32) {
            t.hp = 0;  // sunk at sea
        }
    }
    sim.step(1);
    until_landed(sim);
    CHECK(landed(sim) == 8);
}

TEST_CASE("fleet: a trireme shoots a transport with soldiers before a warship (Z:144)") {
    auto sim = with_trireme();
    sim.issue(StartInvasion{"greek", 40, -1, true});
    sim.step(1);
    auto& st = sim.mutable_state();
    const TilePos at = trireme(sim).tile;
    const WalkerId warship = ships_of_type(sim, "warship").front();
    const WalkerId transport = ships_of_type(sim, "transport").front();
    for (const WalkerId id : st.military.invasion.invaders) {
        st.walkers.items.at(id).speed_raw = 0;
        st.walkers.items.at(id).cooldown = 60000;  // the enemy holds fire
    }
    st.walkers.items.at(warship).tile = {at.x + 2, at.y + 1};    // nearer
    st.walkers.items.at(transport).tile = {at.x + 5, at.y + 1};  // farther, but with soldiers aboard
    const auto warship_hp = st.walkers.items.at(warship).hp;
    const auto transport_hp = st.walkers.items.at(transport).hp;
    sim.step(100);
    CHECK(sim.state().walkers.items.at(warship).hp == warship_hp);
    CHECK(sim.state().walkers.items.at(transport).hp < transport_hp);
}

TEST_CASE("fleet: a battered trireme goes back to the wharf, mends and returns to its post (Z:142)") {
    auto sim = with_trireme();
    const TilePos post{20, 15};
    sim.issue(SetShipOrder{0, 0, post.x, post.y});
    sim.run_days(10);
    REQUIRE(trireme(sim).tile == post);
    trireme(sim).hp = 50;  // below 30 % of 300
    sim.run_days(1);
    CHECK(sim.state().military.ships[0].order == ShipOrder::Repair);
    sim.run_days(20);
    CHECK(trireme(sim).hp == 300);
    CHECK(sim.state().military.ships[0].order == ShipOrder::Defend);
    sim.run_days(10);
    CHECK(trireme(sim).tile == post);
}

TEST_CASE("fleet: orders are checked, kept and saved (Z:142)") {
    auto sim = with_trireme();
    sim.issue(SetShipOrder{0, 0, 5, 5});  // land
    sim.issue(SetShipOrder{3, 1, -1, -1});  // no such ship
    sim.step(1);
    std::size_t rejected = 0;
    for (const auto& e : sim.drain_events()) {
        rejected += std::holds_alternative<CommandRejected>(e.event) ? 1 : 0;
    }
    CHECK(rejected == 2);
    sim.issue(SetShipOrder{0, 1, -1, -1});
    sim.step(1);
    CHECK(sim.state().military.ships[0].order == ShipOrder::AttackAll);
    std::string error;
    auto copy = Simulation::load(bo::test::catalog_isolated(), sim.save(), error);
    REQUIRE(copy.has_value());
    CHECK(copy->state().military.ships == sim.state().military.ships);
    CHECK(copy->state_hash() == sim.state_hash());
}

TEST_CASE("fleet: when the soldiers ashore are beaten the invasion is over and the ships sail away (L-124)") {
    auto sim = sea_city();
    sim.issue(StartInvasion{"greek", 40, -1, true});
    sim.step(1);
    until_landed(sim);
    for (const WalkerId id : sim.state().military.invasion.invaders) {
        Walker& w = sim.mutable_state().walkers.items.at(id);
        w.phase = is_ship(w) ? w.phase : WalkerPhase::Done;  // beaten
    }
    sim.step(2);
    CHECK_FALSE(sim.state().military.invasion.active);
    sim.run_days(20);
    const auto& walkers = sim.state().walkers.items;
    CHECK(std::none_of(walkers.begin(), walkers.end(), [](const auto& kv) {
        return kv.second.type == "transport" || kv.second.type == "warship";
    }));
}

TEST_CASE("fleet: an unmanned trireme takes no orders; its crew comes from the workforce and goes back (Z:141)") {
    auto sim = with_trireme(false);
    const auto workforce = workers_in_the_field(sim.state(), sim.catalog());
    sim.drain_events();
    sim.issue(SetShipOrder{0, 1, -1, -1});
    sim.step(1);
    std::size_t rejected = 0;
    for (const auto& e : sim.drain_events()) {
        rejected += std::holds_alternative<CommandRejected>(e.event) ? 1 : 0;
    }
    CHECK(rejected == 1);  // military.no_crew
    sim.issue(SetShipOrder{0, 4, -1, -1});
    sim.step(1);
    CHECK(sim.state().military.ships[0].crew == Crew::Aboard);
    CHECK(workers_in_the_field(sim.state(), sim.catalog()) == workforce + 20);  // 20 rowers and marines [L-124]
    sim.issue(SetShipOrder{0, 0, 20, 15});
    sim.run_days(10);
    REQUIRE(trireme(sim).tile == TilePos{20, 15});
    sim.issue(SetShipOrder{0, 5, -1, -1});  // Send Crew Home
    sim.step(1);
    CHECK(sim.state().military.ships[0].crew == Crew::GoingHome);
    sim.run_days(10);
    CHECK(trireme(sim).tile.y == 12);  // at the wharf
    CHECK(sim.state().military.ships[0].crew == Crew::None);
    CHECK(workers_in_the_field(sim.state(), sim.catalog()) == workforce);
}

TEST_CASE("fleet: on patrol a trireme sails between its post and the wharf (Z:142, L-124)") {
    auto sim = with_trireme();
    sim.issue(SetShipOrder{0, 3, 24, 16});
    sim.step(1);
    bool post = false;
    bool wharf = false;
    for (int day = 0; day < 40; ++day) {
        sim.run_days(1);
        post = post || trireme(sim).tile == TilePos{24, 16};
        wharf = wharf || (post && trireme(sim).tile.y == 12);
    }
    CHECK(post);
    CHECK(wharf);
}

TEST_CASE("fleet: crewed triremes sail with an army to a city on a sea route and come back (Z:123, Z:177, L-124)") {
    std::vector<std::string> rows(20, std::string(30, '.'));
    rows[8] = std::string(30, '=');
    for (std::size_t y = 12; y < 20; ++y) {
        rows[y] = std::string(30, '~');
    }
    SimulationConfig cfg{16, Difficulty::Hero, 30000, rows, true};
    TradePartner island;
    island.id = "melos";
    island.relation = "rival";
    island.by_sea = true;
    island.diplomacy.military = 1;  // 10 x 2 = 20 against a trireme's 16 and nothing else: lost
    TradePartner inland = island;
    inland.id = "thebes";
    inland.by_sea = false;
    cfg.trade_partners = {island, inland};
    Simulation sim(bo::test::catalog_isolated(), cfg);
    sim.issue(PlaceBuilding{"palace", 2, 2});
    sim.step(1);
    sim.issue(PlaceBuilding{"trireme_wharf", 10, 9});
    sim.step(1);
    sim.mutable_state().city.buildings.items.at(wharf_of(sim)).producer->output_loads = 1;
    sim.run_days(1);
    REQUIRE(sim.state().military.ships.size() == 1);
    sim.issue(SetShipOrder{0, 4, -1, -1});
    sim.step(1);
    sim.drain_events();
    sim.issue(DispatchArmy{1, false, "", {}, true});  // Thebes: no water leads there (Z:123)
    sim.step(1);
    std::string reason;
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
            reason = r->reason_key;
        }
    }
    CHECK(reason == "world.no_sea_route");
    sim.issue(DispatchArmy{0, true, "drachmas", {}, true});  // a raid on Melos by sea, triremes only
    sim.step(1);
    REQUIRE(sim.state().expeditions.size() == 1);
    CHECK(sim.state().expeditions[0].ships.size() == 1);
    CHECK(sim.state().military.ships[0].crew == Crew::Abroad);
    CHECK(sim.state().military.ships[0].walker == kNoWalker);  // off the map
    std::string error;
    auto copy = Simulation::load(bo::test::catalog_isolated(), sim.save(), error);
    REQUIRE(copy.has_value());
    CHECK(copy->state().expeditions == sim.state().expeditions);
    sim.run_days(2 * 200 + 2);  // there and back (march_days)
    CHECK(sim.state().expeditions.empty());
    REQUIRE(sim.state().military.ships.size() == 1);
    CHECK(sim.state().military.ships[0].crew == Crew::Aboard);
    CHECK(trireme(sim).tile.y == 12);  // at its mooring again
}

TEST_CASE("fleet: a crew takes the free workers there are; short-handed a ship is slower, under half it stays (Z:141)") {
    auto sim = with_trireme(false);
    sim.mutable_state().labour.full_staffing = false;  // the city's real workforce
    const auto pct = sim.catalog().worker_percent[static_cast<std::size_t>(sim.state().difficulty)]
                                                 [static_cast<std::size_t>(sim.state().finance.wage_rate)];
    const BuildingId house = sim.mutable_state().city.buildings.next_id++;
    // A house that keeps `workers` workers whatever happens to it (no roads, no food in this city).
    auto keep = [&sim, house, pct](std::uint32_t workers) {
        Building h;
        h.id = house;
        h.type = "common_housing";
        h.house = HouseState{};
        h.house->population = static_cast<std::uint32_t>(workers * 100 / pct + 1);
        sim.mutable_state().city.buildings.items[house] = h;
    };
    auto days = [&sim, &keep](int n, std::uint32_t workers) {
        for (int d = 0; d < n; ++d) {
            keep(workers);
            sim.run_days(1);
        }
    };
    keep(12);
    REQUIRE(labour_pool(sim.state().city, sim.catalog(), sim.state().difficulty, sim.state().finance.wage_rate) == 12);
    sim.issue(SetShipOrder{0, 4, -1, -1});  // Call Crew to Ship: 12 of 20 places
    sim.step(1);
    CHECK(sim.state().military.ships[0].hands == 12);
    CHECK(workers_in_the_field(sim.state(), sim.catalog()) == 12);
    const auto& full = sim.catalog().combat.nations.at("player_greek").units.at("trireme");
    CHECK(trireme(sim).speed_raw == full.speed_raw * 12 / 20);  // slower (Z:141, L-124)
    sim.issue(SetShipOrder{0, 0, 20, 15});  // 12 of 20 is above half: it takes orders
    days(20, 12);
    CHECK(trireme(sim).tile == TilePos{20, 15});
    days(1, 6);  // workers leave the city: 6 hands left
    CHECK(sim.state().military.ships[0].hands == 6);
    sim.drain_events();
    sim.issue(SetShipOrder{0, 1, -1, -1});  // under half: no orders (Z:141)
    sim.step(1);
    std::string reason;
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
            reason = r->reason_key;
        }
    }
    CHECK(reason == "military.crew_short");
    sim.issue(SetShipOrder{0, 0, 25, 15});
    days(10, 6);
    CHECK(trireme(sim).tile == TilePos{20, 15});  // it stays where it was
    std::string error;
    auto copy = Simulation::load(bo::test::catalog_isolated(), sim.save(), error);
    REQUIRE(copy.has_value());
    CHECK(copy->state().military.ships[0].hands == 6);
    CHECK(copy->state_hash() == sim.state_hash());
}
