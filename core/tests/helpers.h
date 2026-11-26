#pragma once

#include "bo/core/data.h"
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

}  // namespace bo::test
