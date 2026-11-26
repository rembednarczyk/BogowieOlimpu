// Internal to the data loader: the economy sections of the catalog (data_economy.cpp).
#pragma once

#include <map>
#include <optional>
#include <string>

#include "bo/core/data.h"
#include "nlohmann/json.hpp"

namespace bo::core::detail {

inline std::optional<std::int32_t> opt_int(const nlohmann::json& j) {
    if (j.is_number_integer()) {
        return j.get<std::int32_t>();
    }
    return std::nullopt;
}

void load_migration(const nlohmann::json& m, const nlohmann::json& diff, MigrationParams& p);
void load_risks(const std::map<std::string, nlohmann::json>& docs, RiskParams& p);
void load_production(const std::map<std::string, nlohmann::json>& docs, DataCatalog& cat);

}  // namespace bo::core::detail
