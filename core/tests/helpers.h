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

}  // namespace bo::test
