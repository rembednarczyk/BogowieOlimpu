#include <filesystem>
#include <fstream>

#include "bo/core/data.h"
#include "nlohmann/json.hpp"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;
namespace fs = std::filesystem;

TEST_CASE("the real data directory loads without validation errors") {
    const auto& cat = bo::test::catalog();
    CHECK(cat.version != 0);
    CHECK(cat.buildings.size() > 100);
    CHECK(cat.building_stats.size() > 130);
    CHECK(cat.housing_levels.size() == 11);
}

TEST_CASE("values from the spec are reachable through the catalog") {
    const auto& cat = bo::test::catalog();
    const auto& granary = cat.building_stats.at("granary");
    REQUIRE(granary.footprint.has_value());
    CHECK(granary.footprint->width == 4);
    CHECK(granary.workers == 18);
    CHECK(granary.cost[static_cast<int>(Difficulty::Hero)] == 100);
    REQUIRE(granary.appeal.has_value());
    CHECK(granary.appeal->initial == -12);
    CHECK(cat.building_stats.at("sanctuary_poseidon").footprint->width == 21);  // K-021
    CHECK(cat.tax_rate_per_mille[static_cast<int>(TaxRate::Normal)] == 90);
}

TEST_CASE("footprint parser") {
    CHECK(parse_footprint("24\xC3\x97" "16")->height == 16);
    CHECK(parse_footprint("3x3m")->width == 3);
    CHECK(parse_footprint("4\xC3\x97" "4+ 3\xC3\x97" "3")->width == 4);
    CHECK_FALSE(parse_footprint("m").has_value());
    CHECK_FALSE(parse_footprint("2xn").has_value());
}

TEST_CASE("validation catches a broken reference (proof that the check bites)") {
    const fs::path tmp = fs::temp_directory_path() / "bo_data_broken";
    fs::remove_all(tmp);
    fs::copy(BO_DATA_DIR, tmp, fs::copy_options::recursive);
    {
        std::ifstream in(tmp / "monsters.json");
        std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        const auto pos = text.find("\"slain_by\": \"odysseus\"");
        REQUIRE(pos != std::string::npos);
        text.replace(pos, 22, "\"slain_by\": \"nobody__\"");
        std::ofstream out(tmp / "monsters.json", std::ios::trunc);
        out << text;
    }
    const auto r = load_data_catalog(tmp.string());
    CHECK_FALSE(r.catalog.has_value());
    CHECK_FALSE(r.errors.empty());
    fs::remove_all(tmp);
}

TEST_CASE("B-023: every production distance and rate reads as written, in any of its forms (production.json)") {
    std::ifstream f(std::string(BO_DATA_DIR) + "/production.json");
    const auto doc = nlohmann::json::parse(f);
    const auto& cat = bo::test::catalog();
    // A number, a [low, high] range (the last), or measurements by distance {"close": n, "dist20": m}:
    // the distance a deliveryman goes is the longest measured, the rate the one by the source (L-103).
    const auto expected = [](const nlohmann::json& j, bool longest) {
        if (j.is_number_integer()) {
            return j.get<std::int32_t>();
        }
        if (j.is_array()) {
            return longest ? j.back().get<std::int32_t>() : j.front().get<std::int32_t>();
        }
        std::int32_t best = 0;
        for (const auto& [k, v] : j.items()) {
            const std::int32_t n = v.is_array() ? (longest ? v.back().get<std::int32_t>() : v.front().get<std::int32_t>())
                                                : v.get<std::int32_t>();
            best = longest ? std::max(best, n) : (k == "close" ? n : best);
        }
        return best;
    };
    for (const auto& p : doc) {
        const std::string id = p.at("id").get<std::string>();
        INFO(id);
        REQUIRE(cat.production.count(id) != 0);
        if (p.contains("max_storage_distance")) {
            CHECK(cat.production.at(id).max_storage_distance == expected(p.at("max_storage_distance"), true));
        }
        if (p.contains("loads_per_year") && !p.at("loads_per_year").is_null()) {
            CHECK(cat.production.at(id).loads_per_year == expected(p.at("loads_per_year"), false));
        }
    }
}
