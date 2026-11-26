// Golden hash (docs/PRAKTYKI.md ch. 3): a small fixed scenario, hash updated ONLY by hand, and the commit that
// changes it must name the rule that changed it. Never regenerate it automatically.
#include <cstdio>

#include "bo/core/hash.h"
#include "bo/core/serialize.h"
#include "bo/core/simulation.h"
#include "bo/core/storage.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

Simulation golden_scenario() {
    Simulation sim(bo::test::catalog(), SimulationConfig{20261126, Difficulty::Hero, 10000});
    sim.issue_at(10, SetTaxRate{TaxRate::High});
    sim.issue_at(kTicksPerMonth * 3, SetWageRate{WageRate::VeryHigh});
    sim.issue_at(kTicksPerYear, SetTaxRate{TaxRate::Low});
    sim.step(kTicksPerYear * 2 + 17);
    return sim;
}

}  // namespace

TEST_CASE("replay: the same seed and command log give the same hash") {
    CHECK(golden_scenario().state_hash() == golden_scenario().state_hash());
}

TEST_CASE("golden hash") {
    // The data version is part of the hash, so this test covers the logic, not the data: it hashes a state
    // whose data_version field is fixed to 0.
    const auto sim = golden_scenario();
    const auto bytes = bo::core::serialize_state(sim.state(), 0);
    bo::core::Fnv1a64 h;
    h.add_bytes(bytes.data(), bytes.size());
    // 2026-11-26: initial value (modules A1-A5).
    // 2026-11-26: save format v2 appends the (empty) city and walker blocks (module B, serialize.cpp).
    // 2026-11-26: save format v3 appends the (empty) undo record (Z:20, serialize_city.cpp).
    // 2026-11-26: save format v4 appends the homeless count (module C2, serialize_city.cpp).
    // 2026-11-26: save format v5 adds labour and finance blocks (modules C3, C6).
    // 2026-11-26: save format v6 adds the migration state; popularity starts at the base (module C8).
    // 2026-11-26: save format v7 (version field; empty city has no new blocks), modules C4-C5 stage 1.
    // 2026-11-26: save format v8 (version field; terrain height per tile, L-099).
    // 2026-11-26: save format v9 (version field; herds, orchards, processors).
    // 2026-11-26: save format v10 (version field; extraction trips, Get orders).
    // 2026-11-26: save format v11 (version field; spawn herds block).
    // 2026-11-26: save format v12 (version field; wind byte), module C7 risks in the daily schedule.
    // 2026-11-26: save format v13 (version field; city hygiene byte), L-104 revised from S-010 and S-007.
    // 2026-11-26: save format v14 (version field; empty trade block), module C9.
    // 2026-11-26: save format v15 (version field; immigrants on the way).
    // 2026-11-26: save format v16 (version field; hunting wolf per spawn point), wolves (Z:45, L-107).
    // 2026-11-26: save format v17 (version field; empty army block), module D3 stage 1.
    // 2026-11-26: save format v18 (version field; invasion block), module D3 stage 2.
    // 2026-11-26: save format v19 (version field; sanctuary state), module D1 stage 1.
    // 2026-11-26: save format v20 (version field; sanctification and prayer fields), module D1 stage 2.
    // 2026-11-26: save format v21 (version field; invasion city, empty world block), module D4 stage 1.
    // 2026-11-26: save format v22 (version field; empty scenario block), module D5 stage 1.
    // 2026-11-26: save format v23 (version field; curse days, event fields), module D5 stage 2.
    // 2026-11-26: save format v24 (version field; empty monster block), module D5 stage 3.
    // 2026-11-26: save format v25 (version field; hits and cooldown per building), module D3 stage 3.
    // 2026-11-26: save format v26 (version field; quests and monuments earned), module D5 quests.
    // 2026-11-26: save format v27 (version field; requested goods on their way), owner's decisions L-108..L-118.
    // 2026-11-26: save format v28 (version field; performances and schools per building), B-008 culture.
    // 2026-11-26: save format v29 (version field; days of debt), L-034 debt.
    // 2026-11-26: save format v30 (version field; no partners here), L-121 long-term relations.
    const std::uint64_t kGolden = 0x2bb5380fe316e7aULL;
    if (h.value() != kGolden) {
        std::printf("golden hash is 0x%016llx\n", static_cast<unsigned long long>(h.value()));
    }
    CHECK(h.value() == kGolden);
}

namespace {

// 32x32 city: a road grid with a roadblock, forest, meadow, four service buildings, an agora with a stall,
// a boulevard, an undone road, six house plots filled by immigrants, a farm feeding a granary, two years.
Simulation golden_city() {
    std::vector<std::string> rows(32, std::string(32, '.'));
    for (std::size_t i = 2; i < 30; ++i) {
        rows[4][i] = '=';
        rows[16][i] = '=';
        rows[28][i] = '=';
        rows[i][4] = '=';
        rows[i][16] = '=';
        rows[i][28] = '=';
    }
    rows[16][22] = 'x';
    for (std::size_t i = 20; i < 26; ++i) {
        rows[8][i] = 'T';
        rows[22][i] = ',';
        rows[26][i] = ',';
    }
    // A one-level plateau on the two east columns, reached by a ramp at (30, 16) (L-099).
    rows[16][30] = '>';
    rows[20][1] = 'h';  // boar spawn point
    rows[4][1] = 'E';   // land entry: the road to Greece
    const std::vector<std::string> heights(32, std::string(31, '0') + "1");
    SimulationConfig cfg{20261126, Difficulty::Hero, 3000, rows, false, heights};
    TradePartner corinth;
    corinth.id = "corinth";
    corinth.sells["fleece"] = TradeOffer{60, 8, 0};
    corinth.buys["wood"] = TradeOffer{75, 8, 0};
    cfg.trade_partners.push_back(corinth);
    Simulation sim(bo::test::catalog(), cfg);
    sim.issue(PlaceRoad{30, 16});
    sim.issue(PlaceRoad{31, 16});
    sim.issue(PlaceRoad{31, 15});
    sim.issue(PlaceBuilding{"carrot_farm", 20, 25, 0});
    sim.issue(PlaceBuilding{"carding_shed", 17, 20, 0});
    sim.issue(PlaceBuilding{"growers_lodge", 17, 23, 0});
    sim.issue(PlaceBuilding{"storehouse", 24, 17, 0});
    sim.issue(PlaceBuilding{"timber_mill", 17, 9, 0});
    sim.issue(PlaceBuilding{"boar_hunters_lodge", 5, 21, 0});
    // Maintenance and health (C7): without superintendents buildings burn or collapse in a few months (S-004).
    sim.issue(PlaceBuilding{"maintenance_office", 20, 5, 0});
    sim.issue(PlaceBuilding{"maintenance_office", 5, 13, 0});
    sim.issue(PlaceBuilding{"maintenance_office", 12, 26, 0});
    sim.issue(PlaceBuilding{"infirmary", 12, 22, 0});
    sim.issue(PlaceBuilding{"trading_post", 24, 22, 0});
    sim.issue_at(2, PlaceBuilding{"sheep", 24, 22, 0});
    sim.issue_at(2, PlaceBuilding{"sheep", 25, 22, 0});
    sim.issue_at(2, PlaceBuilding{"olive_tree", 20, 22, 0});
    sim.issue_at(2, PlaceBuilding{"olive_tree", 21, 22, 0});
    sim.issue(PlaceBuilding{"granary", 5, 24, 0});
    sim.issue(SetSectorPriority{2, 5});  // distribution first: the Granary needs full staffing (Z:66)
    sim.issue(SetSectorPriority{3, 5});  // maintenance too: unattended buildings burn (C7)
    for (int x = 6; x < 28; x += 2) {
        sim.issue(PlaceBuilding{"common_housing", x, 29, 0});  // the farm and the granary need hands
    }
    for (int x = 5; x < 15; x += 2) {
        sim.issue(PlaceBuilding{"common_housing", x, 17, 0});  // immigrants bring the workers (C8, C3)
    }
    sim.issue(PlaceBuilding{"fountain", 6, 5});
    sim.issue(PlaceBuilding{"maintenance_office", 17, 17});
    sim.issue(PlaceBuilding{"watchpost", 10, 14});
    sim.issue_at(kTicksPerMonth, PlaceBuilding{"gymnasium", 24, 12});
    sim.issue_at(kTicksPerMonth * 2, ClearLand{21, 8});
    sim.issue_at(kTicksPerMonth * 2, PlaceRoad{10, 10});
    sim.issue_at(kTicksPerMonth * 5, Demolish{16, 10});
    sim.issue_at(kTicksPerYear, PlaceRoadblock{4, 10});
    sim.issue_at(kTicksPerMonth * 3, PlaceBuilding{"common_agora", 28, 6, 0});
    sim.issue_at(kTicksPerMonth * 3 + 1, PlaceBuilding{"food_vendor_stall", 29, 8, 0});
    sim.issue_at(kTicksPerMonth * 4, PlaceBuilding{"boulevard", 8, 4, 1});
    sim.issue_at(kTicksPerMonth * 6, PlaceRoad{12, 10});
    sim.issue_at(kTicksPerMonth * 6 + 5, Undo{});
    sim.issue_at(kTicksPerMonth * 7, PlaceBuilding{"common_housing", 5, 2, 0});
    sim.issue_at(kTicksPerMonth * 7, PlaceBuilding{"flower_garden", 8, 1, 0});
    sim.step(1);
    const BuildingId post = sim.state().city.map.at({24, 22}).building;
    sim.issue(SetTradePartner{post, 0});
    sim.issue(SetTradeOrder{post, "wood", false, 0, true, 4});
    sim.issue(SetTradeOrder{post, "fleece", true, 4, false, 0});
    sim.step(kTicksPerYear * 2 + 16);
    return sim;
}

}  // namespace

TEST_CASE("golden hash: city with buildings and walkers") {
    const auto sim = golden_city();
    REQUIRE(sim.setup_error().empty());
    // The economy is covered by dedicated tests (test_goods, test_husbandry, test_risks); here: structure only.
    // 40 placed (the plot on the x = 16 road is refused) + lambs, minus what burned down (ruins stay).
    CHECK(sim.state().city.buildings.items.size() == 38);
    CHECK(sim.state().trade.partners[0].sells.at("fleece").traded + sim.state().trade.partners[0].buys.at("wood").traded > 0);
    CHECK(sim.state().labour.workforce > 0);
    CHECK(sim.state().city.map.at({31, 15}).road_network == sim.state().city.map.at({29, 16}).road_network);
    CHECK_FALSE(sim.state().city.map.at({12, 10}).road);  // undone
    CHECK(sim.state().city.map.at({4, 10}).roadblock);
    const auto bytes = bo::core::serialize_state(sim.state(), 0);
    bo::core::Fnv1a64 h;
    h.add_bytes(bytes.data(), bytes.size());
    // 2026-11-26: initial value (module B: map, construction, roads, A*, roaming and destination walkers).
    // 2026-11-26: module B completed: save format v3, scenario extended with an agora, a stall, a boulevard and
    // an undo; wide-road rule (S-006) and Fountain return tile (S-004) change walker routes.
    // 2026-11-26: modules C1-C2: save format v4 (house state), scenario extended with a house plot and a garden.
    // 2026-11-26: modules C3, C6: save format v5, full staffing switch, monthly finance, clerk counter 50 days.
    // 2026-11-26: module C8: save format v6, immigration fills six house plots, real labour replaces full staffing.
    // 2026-11-26: modules C4-C5 stage 1: save format v7 (storage, producers, stalls, cargo), scenario extended
    // with a farm, a granary and eleven more house plots; the agora stall now fetches food.
    // 2026-11-26: terrain height (L-099): save format v8, a plateau with a ramp and a road over it.
    // 2026-11-26: module C4 stage 2: save format v9, a carding shed with sheep, a growers' lodge with olive
    // trees and a storehouse in the scenario.
    // 2026-11-26: module C4/C5 stage 3: save format v10, herd births, a timber mill in the scenario.
    // 2026-11-26: hunting and fishing (L-103): save format v11, a boar spawn point and a hunters' lodge.
    // 2026-11-26: module C7 (fire, collapse, disease, crime): save format v12, maintenance offices and an
    // infirmary in the scenario, maintenance sector first; two buildings burn down.
    // 2026-11-26: L-104 revised (S-010, S-007 OG fire model, family hygiene, plague 1 in 3, crime escalation,
    // firefighters sent to fires, Hygiene & Safety first at equal priority): save format v13.
    // 2026-11-26: module C9 trade: save format v14, a land entry, a trading post and a partner city.
    // 2026-11-26: immigrants walk from the land entry (O-006), workers need a walking link to it (S-005).
    // 2026-11-26: save format v16: the boar spawn point stores a (no) hunting wolf field (L-107).
    // 2026-11-26: houses no longer count days toward a roaming walker unless they are inhabited elite houses
    // sending a nobleman (Z:25, Z:88, L-108).
    // 2026-11-26: save format v17: army block (no Palace in the scenario, so no companies), module D3.
    // 2026-11-26: save format v18: hit points and cooldown per walker, invasion block (D3 stage 2, L-110).
    // 2026-11-26: save format v19 (version field only: no sanctuary in the scenario), module D1 stage 1.
    // 2026-11-26: save format v20: a sanctification flag and base per building (D1 stage 2, L-112).
    // 2026-11-26: save format v21: diplomacy of the partner city (opinion 60), module D4 stage 1.
    // 2026-11-26: save format v22 (version field; empty scenario block), module D5 stage 1.
    // 2026-11-26: save format v23: curse days per building (D5 stage 2, L-115).
    // 2026-11-26: save format v24 (version field; empty monster block), module D5 stage 3.
    // 2026-11-26: save format v25: hits and cooldown per building (D3 stage 3, L-117).
    // 2026-11-26: save format v26 (version field; quests and monuments earned), module D5 quests.
    // 2026-11-26: save format v27 (version field; requested goods on their way), owner's decisions L-108..L-118.
    // 2026-11-26: save format v28 (two culture fields per building; no school with a venue here), B-008.
    // 2026-11-26: save format v29 (days of debt; the treasury here never goes below zero), L-034.
    // 2026-11-26: save format v30 (three relation counters for the one partner; opinion 60 starts none), L-121.
    const std::uint64_t kGolden = 0x57ca4986aa855b55ULL;
    if (h.value() != kGolden) {
        std::printf("golden city hash is 0x%016llx\n", static_cast<unsigned long long>(h.value()));
    }
    CHECK(h.value() == kGolden);
}
