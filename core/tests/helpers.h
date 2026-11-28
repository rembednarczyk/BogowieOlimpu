#pragma once

#include <fstream>
#include <sstream>
#include <string>

#include "bo/core/data.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"

namespace bo::test {

// Loads the real game data once per test binary.
inline const bo::core::DataCatalog& catalog() {
    static const bo::core::DataCatalog kCatalog = [] {
        auto r = bo::core::load_data_catalog(BO_DATA_DIR);
        for (const auto& e : r.errors) {
            MESSAGE(e);
        }
        REQUIRE(r.catalog.has_value());
        return *r.catalog;
    }();
    return kCatalog;
}

// The real data with migration and risks switched off (batch size 0, unreachable risk thresholds): for tests
// of one system that set populations by hand and run for months without maintenance offices.
inline const bo::core::DataCatalog& catalog_isolated() {
    static const bo::core::DataCatalog kCatalog = [] {
        bo::core::DataCatalog c = catalog();
        c.migration.batch_people = 0;
        c.risks.hazard_threshold = 0x7FFFFFFF;
        c.risks.disease_threshold = 0x7FFFFFFF;
        c.risks.crime_threshold = 0x7FFFFFFF;
        return c;
    }();
    return kCatalog;
}

// 60x40: a main street on row 20, a meadow at x 30-40, y 22-27; a finished sanctuary of `god` at (2, 14).
inline bo::core::Simulation with_god(const std::string& god) {
    std::vector<std::string> rows(40, std::string(60, '.'));
    rows[20] = std::string(60, '=');
    for (std::size_t y = 22; y < 28; ++y) {
        for (std::size_t x = 30; x < 40; ++x) {
            rows[y][x] = ',';
        }
    }
    bo::core::Simulation sim(catalog_isolated(), bo::core::SimulationConfig{11, bo::core::Difficulty::Hero, 30000, rows, true});
    if (god.empty()) {
        return sim;
    }
    sim.issue(bo::core::PlaceBuilding{"storehouse", 50, 17});
    sim.step(1);
    auto& store = sim.mutable_state().city.buildings.items.at(sim.state().city.map.at({50, 17}).building);
    store.storage->loads["marble"] = 60;
    sim.issue(bo::core::PlaceBuilding{"sanctuary_" + god, 2, 20 - catalog().building_stats.at("sanctuary_" + god).footprint->height});
    sim.step(1);
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        if (b.sanctuary) {
            for (auto& [g, n] : b.sanctuary->needed) {
                n = 0;
            }
        }
    }
    sim.run_days(1);
    return sim;
}

// The building on tile p (REQUIREs a building there).
inline bo::core::Building& at(bo::core::Simulation& sim, bo::core::TilePos p) {
    return sim.mutable_state().city.buildings.items.at(sim.state().city.map.at(p).building);
}
inline const bo::core::Building& at(const bo::core::Simulation& sim, bo::core::TilePos p) {
    return sim.state().city.buildings.items.at(sim.state().city.map.at(p).building);
}

// The reason key of the first command rejected since the last drain, empty if none; drains the events.
inline std::string rejection(bo::core::Simulation& sim) {
    std::string first;
    for (const auto& e : sim.drain_events()) {
        if (const auto* r = std::get_if<bo::core::CommandRejected>(&e.event); r != nullptr && first.empty()) {
            first = r->reason_key;
        }
    }
    return first;
}

// A whole file as text (scenarios and build orders of the demo gates).
inline std::string read_text(const std::string& path) {
    std::ifstream f(path);
    std::stringstream s;
    s << f.rdbuf();
    return s.str();
}

}  // namespace bo::test
