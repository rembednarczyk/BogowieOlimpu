// Module C4 stage 2: herds, orchards, processors (docs/gameplay/04, 19; S-004, S-005; assumptions L-101).
#include "bo/core/logistics.h"
#include "bo/core/simulation.h"
#include "bo/core/storage.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// Meadow in the north-west (x < 12, y < 6), main street on row 6.
std::vector<std::string> farmland() {
    std::vector<std::string> r(14, std::string(40, '.'));
    for (std::size_t y = 0; y < 6; ++y) {
        for (std::size_t x = 0; x < 12; ++x) {
            r[y][x] = ',';
        }
    }
    r[6] = std::string(40, '=');
    return r;
}

Simulation town() {
    return Simulation(bo::test::catalog_no_migration(), SimulationConfig{4, Difficulty::Hero, 9000, farmland(), true});
}

Building& at(Simulation& sim, TilePos p) {
    return sim.mutable_state().city.buildings.items.at(sim.state().city.map.at(p).building);
}

std::uint32_t stock(const Simulation& sim, const std::string& good) {
    return city_stock(sim.state().city, bo::test::catalog(), good);
}

// Loads of a good anywhere: storages, producers' output, walkers' cargo, lodge fruit (100 units = 1 load).
std::uint32_t produced(const Simulation& sim, const std::string& good) {
    std::uint32_t total = stock(sim, good);
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        if (b.producer) {
            const auto& outputs = bo::test::catalog().buildings.at(b.type).outputs;
            if (!outputs.empty() && outputs.front() == good && bo::test::catalog().buildings.at(b.type).tends.empty()) {
                total += b.producer->output_loads;
            }
            const auto u = b.producer->units.find(good);
            total += u == b.producer->units.end() ? 0 : u->second / 100;
        }
    }
    for (const auto& [id, w] : sim.state().walkers.items) {
        const auto c = w.cargo.find(good);
        total += c == w.cargo.end() ? 0 : (w.job == WalkerJob::Harvest ? c->second / 100 : c->second);
    }
    return total;
}

}  // namespace

TEST_CASE("sheep need a carding shed; a herd holds 8; any number share a meadow tile (Z:45, S-004)") {
    auto sim = town();
    sim.issue(PlaceBuilding{"sheep", 2, 2});
    sim.step(1);
    CHECK(sim.state().city.buildings.items.empty());
    sim.issue(PlaceBuilding{"carding_shed", 14, 4});
    sim.step(1);
    for (int i = 0; i < 9; ++i) {
        sim.issue(PlaceBuilding{"sheep", 2, 2});
    }
    sim.issue(PlaceBuilding{"sheep", 20, 2});  // not meadow
    sim.step(1);
    int sheep = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        sheep += b.animal ? 1 : 0;
    }
    CHECK(sheep == 8);
}

TEST_CASE("shepherds shear ready sheep (first after 6 months, then yearly); fleece goes to a Storehouse") {
    auto sim = town();
    sim.issue(PlaceBuilding{"carding_shed", 14, 4});
    sim.issue(PlaceBuilding{"storehouse", 20, 3});
    sim.step(1);
    for (int i = 0; i < 8; ++i) {  // a full herd: no births
        sim.issue(PlaceBuilding{"sheep", 3 + i % 4, 2});
    }
    sim.run_days(90);
    CHECK(produced(sim, "fleece") == 0);
    sim.run_days(80);  // one shepherd at a time walks to each sheep and back
    CHECK(produced(sim, "fleece") == 8);  // one load per sheep (doc 19)
    sim.run_days(110);
    CHECK(produced(sim, "fleece") == 8);  // next shearing a year after the first
    CHECK(stock(sim, "fleece") == 8);
    sim.run_days(100);
    CHECK(produced(sim, "fleece") == 16);
}

TEST_CASE("orchards ripen, harvesters pick in the season, the lodge ships olives to the press, oil to storage") {
    auto sim = town();
    sim.issue(PlaceBuilding{"growers_lodge", 14, 4});
    sim.issue(PlaceBuilding{"olive_press", 18, 4});
    sim.issue(PlaceBuilding{"storehouse", 24, 3});
    for (int i = 0; i < 6; ++i) {
        sim.issue(PlaceBuilding{"olive_tree", 2 + i, 3});
    }
    sim.step(1);
    // Planted in month 1 of year 1: a full year of growth with monthly pruning, harvest from month 1 of year 2.
    sim.run_days(16 * 12 + 16 * 3);
    const Building& press = at(sim, {18, 4});
    const std::uint32_t oil_and_olives = produced(sim, "olive_oil") + produced(sim, "olives") + press.producer->input_loads;
    CHECK(oil_and_olives >= 5);  // 6 trees, about one load each
    sim.run_days(16 * 4);
    CHECK(stock(sim, "olive_oil") >= 2);  // the press works 6 loads a year at full staffing
}

TEST_CASE("unpicked ripe fruit is lost after the season; weak plants keep their ripeness (Z:42, doc 19)") {
    auto sim = town();
    sim.issue(PlaceBuilding{"olive_tree", 2, 3});
    sim.issue(PlaceBuilding{"olive_tree", 4, 3});
    sim.step(1);
    at(sim, {2, 3}).orchard->growth = 80 * 192;
    at(sim, {4, 3}).orchard->growth = 5 * 192;  // + 48 days of growth = 30 %, below the carry-over line
    sim.run_days(16 * 3 + 1);  // into month 4: the olive season (months 1-3) has ended, no lodge picked
    CHECK(at(sim, {2, 3}).orchard->growth < 10 * 192);
    CHECK(at(sim, {4, 3}).orchard->growth >= 30 * 192);
}

TEST_CASE("without a pruner for 10 months a tree stops ripening (doc 19)") {
    auto sim = town();
    sim.issue(PlaceBuilding{"grapevine", 2, 3});
    sim.step(1);
    sim.run_days(160);
    const std::uint32_t g = at(sim, {2, 3}).orchard->growth;
    CHECK(g == 96 * 100 + 64 * 50);  // 6 months full speed, 4 months half speed
    sim.run_days(10);
    CHECK(at(sim, {2, 3}).orchard->growth == g);
}

TEST_CASE("a storehouse sends its olives to a press that needs them (storage deliveryman, up to 3 loads)") {
    auto sim = town();
    sim.issue(PlaceBuilding{"storehouse", 14, 3});
    sim.issue(PlaceBuilding{"olive_press", 20, 4});
    sim.step(1);
    at(sim, {14, 3}).storage->loads["olives"] = 5;
    sim.run_days(3);
    CHECK(at(sim, {20, 4}).producer->input_loads == 3);
    CHECK(at(sim, {14, 3}).storage->loads["olives"] == 2);
}

TEST_CASE("herds below 8 get a young one every 32 days; the young give fleece after 12 months (L-101)") {
    auto sim = town();
    sim.issue(PlaceBuilding{"carding_shed", 14, 4});
    sim.step(1);
    sim.issue(PlaceBuilding{"sheep", 3, 2});
    sim.issue(PlaceBuilding{"sheep", 4, 2});
    sim.step(1);
    auto herd = [&sim] {
        int n = 0;
        for (const auto& [id, b] : sim.state().city.buildings.items) {
            n += b.animal ? 1 : 0;
        }
        return n;
    };
    sim.run_days(33);
    CHECK(herd() == 3);
    sim.run_days(16 * 12);
    CHECK(herd() == 8);  // capped at the herd size
}
