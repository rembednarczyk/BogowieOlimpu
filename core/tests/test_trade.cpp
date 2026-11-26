// Module C9: trade (docs/gameplay/06 Z:74-80, 24; data/trade.json; assumptions L-105).
#include "bo/core/simulation.h"
#include "bo/core/storage.h"
#include "bo/core/trade.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// A road from the land entry 'E' on the west edge; sea on the south with a sea entry 'S'.
std::vector<std::string> port_map() {
    std::vector<std::string> r(16, std::string(40, '.'));
    r[6] = "E" + std::string(39, '=');
    for (std::size_t y = 11; y < 16; ++y) {
        r[y] = std::string(40, '~');
    }
    r[15][39] = 'S';
    return r;
}

TradePartner land_partner() {
    TradePartner p;
    p.id = "corinth";
    p.relation = "ally";
    p.sells["fleece"] = TradeOffer{60, 12, 0};
    p.buys["olive_oil"] = TradeOffer{145, 16, 0};
    return p;
}

SimulationConfig config(std::vector<TradePartner> partners) {
    SimulationConfig c{3, Difficulty::Hero, 5000, port_map(), true};
    c.trade_partners = std::move(partners);
    return c;
}

Building& at(Simulation& sim, TilePos p) {
    return sim.mutable_state().city.buildings.items.at(sim.state().city.map.at(p).building);
}

}  // namespace

TEST_CASE("a caravan comes from the land entry every 4 days, sells fleece and buys oil (Z:77-79, doc 24)") {
    Simulation sim(bo::test::catalog_isolated(), config({land_partner()}));
    sim.issue(PlaceBuilding{"trading_post", 20, 2});
    sim.step(1);
    const BuildingId tp = sim.state().city.map.at({20, 2}).building;
    sim.issue(SetTradePartner{tp, 0});
    sim.issue(SetTradeOrder{tp, "fleece", true, 6, false, 0});
    sim.issue(SetTradeOrder{tp, "olive_oil", false, 0, true, 10});
    sim.step(1);
    at(sim, {20, 2}).storage->loads["olive_oil"] = 10;
    const std::int64_t before = sim.state().finance.treasury;
    std::int64_t earned = 0;
    std::uint32_t bought = 0;
    std::uint32_t sold = 0;
    for (int d = 0; d < 32; ++d) {  // a caravan at most every 4 days, one at a time per building
        sim.run_days(1);
        for (const auto& e : sim.drain_events()) {
            if (const auto* t = std::get_if<TradeDone>(&e.event)) {
                earned += t->money;
                bought += t->loads_bought;
                sold += t->loads_sold;
            }
        }
    }
    CHECK(bought == 6);     // up to the import limit
    CHECK(sold == 10);      // 8 per caravan, then the rest on the next visit
    CHECK(earned == 10 * 145 - 6 * 60);
    CHECK(sim.state().finance.treasury - before <= earned);  // wages and the like may also move the treasury
    CHECK(sim.state().trade.partners[0].sells.at("fleece").traded == 6);
}

TEST_CASE("yearly limits cap trade and start over each year (Z:75)") {
    TradePartner p = land_partner();
    p.buys["olive_oil"].yearly_limit = 3;
    Simulation sim(bo::test::catalog_isolated(), config({p}));
    sim.issue(PlaceBuilding{"trading_post", 20, 2});
    sim.step(1);
    const BuildingId tp = sim.state().city.map.at({20, 2}).building;
    sim.issue(SetTradePartner{tp, 0});
    sim.issue(SetTradeOrder{tp, "olive_oil", false, 0, true, 20});
    sim.step(1);
    at(sim, {20, 2}).storage->loads["olive_oil"] = 20;
    sim.run_days(40);
    CHECK(at(sim, {20, 2}).storage->loads["olive_oil"] == 17);
    sim.run_days(192);
    CHECK(at(sim, {20, 2}).storage->loads["olive_oil"] == 14);
}

TEST_CASE("rivals do not trade (Z:74); goods kept for export are not city stock (Z:79)") {
    TradePartner p = land_partner();
    p.relation = "rival";
    Simulation sim(bo::test::catalog_isolated(), config({p}));
    sim.issue(PlaceBuilding{"trading_post", 20, 2});
    sim.step(1);
    const BuildingId tp = sim.state().city.map.at({20, 2}).building;
    sim.issue(SetTradePartner{tp, 0});
    sim.issue(SetTradeOrder{tp, "olive_oil", false, 0, true, 10});
    sim.step(1);
    at(sim, {20, 2}).storage->loads["olive_oil"] = 10;
    sim.run_days(20);
    CHECK(at(sim, {20, 2}).storage->loads["olive_oil"] == 10);
    CHECK(city_stock(sim.state().city, bo::test::catalog(), "olive_oil") == 0);
}

TEST_CASE("producers ship to export first; trade deliverymen carry imports to storage (Z:78, doc 05)") {
    Simulation sim(bo::test::catalog_isolated(), config({land_partner()}));
    sim.issue(PlaceBuilding{"trading_post", 20, 2});
    sim.issue(PlaceBuilding{"storehouse", 30, 3});
    sim.issue(PlaceBuilding{"olive_press", 10, 4});
    sim.step(1);
    const BuildingId tp = sim.state().city.map.at({20, 2}).building;
    sim.issue(SetTradePartner{tp, 0});
    sim.issue(SetTradeOrder{tp, "olive_oil", false, 0, true, 4});
    sim.issue(SetTradeOrder{tp, "fleece", true, 6, false, 0});
    sim.step(1);
    at(sim, {10, 4}).producer->output_loads = 2;  // oil: delivery_priority starts with trade_exporting
    sim.run_days(20);
    const auto oil_sold_or_kept = sim.state().trade.partners[0].buys.at("olive_oil").traded +
                                  at(sim, {20, 2}).storage->loads["olive_oil"];
    CHECK(oil_sold_or_kept == 2);
    CHECK(at(sim, {30, 3}).storage->loads["fleece"] > 0);  // imported fleece moved to the storehouse
}

TEST_CASE("a pier has a dock on the water; ships come from the sea entry every 30 days with 12 loads") {
    TradePartner p = land_partner();
    p.by_sea = true;
    p.buys["olive_oil"].yearly_limit = 40;
    Simulation sim(bo::test::catalog_isolated(), config({p}));
    sim.issue(PlaceBuilding{"pier", 18, 7});  // land part on rows 7-10 by the road on row 6
    sim.step(1);
    REQUIRE(sim.state().city.map.at({18, 7}).building != kNoBuilding);
    CHECK(sim.state().city.map.at({19, 11}).building == sim.state().city.map.at({18, 7}).building);  // dock
    const BuildingId pier = sim.state().city.map.at({18, 7}).building;
    sim.issue(SetTradePartner{pier, 0});
    sim.issue(SetTradeOrder{pier, "olive_oil", false, 0, true, 30});
    sim.step(1);
    at(sim, {18, 7}).storage->loads["olive_oil"] = 30;
    sim.run_days(32 + 20);
    CHECK(at(sim, {18, 7}).storage->loads["olive_oil"] == 18);  // one ship: 12 loads
}
