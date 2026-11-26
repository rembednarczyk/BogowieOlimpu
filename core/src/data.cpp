#include "bo/core/data.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

#include "bo/core/hash.h"
#include "nlohmann/json.hpp"

namespace bo::core {
namespace {

using nlohmann::json;
namespace fs = std::filesystem;

std::optional<std::int32_t> opt_int(const json& j) {
    if (j.is_number_integer()) {
        return j.get<std::int32_t>();
    }
    return std::nullopt;
}

PerDifficulty per_difficulty(const json& j) {
    PerDifficulty out{};
    if (!j.is_object()) {
        return out;
    }
    for (std::size_t i = 0; i < out.size(); ++i) {
        const auto code = std::string(difficulty_code(static_cast<Difficulty>(i)));
        if (j.contains(code)) {
            out[i] = opt_int(j.at(code));
        }
    }
    return out;
}

std::set<std::string> ids_of(const json& arr) {
    std::set<std::string> ids;
    for (const auto& o : arr) {
        ids.insert(o.at("id").get<std::string>());
    }
    return ids;
}

void check_unique_ids(const std::string& file, const json& arr, std::vector<std::string>& errors) {
    std::set<std::string> seen;
    for (const auto& o : arr) {
        const auto id = o.at("id").get<std::string>();
        if (!seen.insert(id).second) {
            errors.push_back(file + ": duplicate id '" + id + "'");
        }
    }
}

void load_buildings(const json& buildings, const json& stats, DataCatalog& cat) {
    for (const auto& o : buildings) {
        BuildingType t;
        t.id = o.at("id").get<std::string>();
        t.tab = o.value("tab", "");
        t.culture = o.value("culture", "");
        cat.buildings.emplace(t.id, t);
    }
    for (const auto& o : stats) {
        BuildingStats s;
        s.id = o.at("id").get<std::string>();
        s.size_text = o.value("size", "");
        s.footprint = parse_footprint(s.size_text);
        s.workers = opt_int(o.value("workers", json()));
        s.cost = per_difficulty(o.value("cost", json()));
        s.fire_risk = per_difficulty(o.value("fire_risk", json()));
        s.damage_risk = per_difficulty(o.value("damage_risk", json()));
        s.expansion = o.value("expansion", false);
        const auto& a = o.value("appeal", json());
        if (a.is_object()) {
            // "–" in the source table (null here) means: no step change / default step size.
            s.appeal = AppealParams{opt_int(a.value("initial", json())).value_or(0),
                                    opt_int(a.value("step_tiles", json())).value_or(1),
                                    opt_int(a.value("step_change", json())).value_or(0),
                                    opt_int(a.value("range", json())).value_or(0)};
        }
        cat.building_stats.emplace(s.id, s);
    }
}

void validate(const std::map<std::string, json>& docs, std::vector<std::string>& errors) {
    const auto& buildings = docs.at("buildings.json");
    const auto& housing = docs.at("housing.json");
    const auto& gods = docs.at("gods.json");
    const auto& monsters = docs.at("monsters.json");
    const auto& heroes = docs.at("heroes.json");
    for (const auto* name : {"buildings.json", "housing.json", "gods.json", "monsters.json", "heroes.json",
                             "building_stats.json", "walkers.json", "goods.json"}) {
        check_unique_ids(name, docs.at(name), errors);
    }
    const auto building_ids = ids_of(buildings);
    const auto housing_ids = ids_of(housing);
    const auto god_ids = ids_of(gods);
    const auto monster_ids = ids_of(monsters);
    const auto hero_ids = ids_of(heroes);
    for (const auto& s : docs.at("building_stats.json")) {
        const auto id = s.at("id").get<std::string>();
        if (building_ids.count(id) == 0 && housing_ids.count(id) == 0) {
            errors.push_back("building_stats.json: '" + id + "' not in buildings.json or housing.json");
        }
    }
    for (const auto& b : buildings) {
        for (const char* key : {"sanctified_by", "cursed_by"}) {
            for (const auto& g : b.value(key, json::array())) {
                if (god_ids.count(g.get<std::string>()) == 0) {
                    errors.push_back("buildings.json: " + b.at("id").get<std::string>() + "." + key + " unknown god");
                }
            }
        }
    }
    for (const auto& g : gods) {
        const auto m = g.at("monster").get<std::string>();
        if (monster_ids.count(m) == 0) {
            errors.push_back("gods.json: " + g.at("id").get<std::string>() + " unknown monster '" + m + "'");
        }
    }
    for (const auto& m : monsters) {
        const auto hero = m.at("slain_by").get<std::string>();
        if (hero_ids.count(hero) == 0) {
            errors.push_back("monsters.json: " + m.at("id").get<std::string>() + " unknown hero '" + hero + "'");
        }
    }
    for (const auto& h : heroes) {
        for (const auto& m : h.at("slays")) {
            if (monster_ids.count(m.get<std::string>()) == 0) {
                errors.push_back("heroes.json: " + h.at("id").get<std::string>() + " unknown monster");
            }
        }
    }
    for (const auto& q : docs.at("quests.json")) {
        if (god_ids.count(q.at("god").get<std::string>()) == 0 ||
            hero_ids.count(q.at("traditional_hero").get<std::string>()) == 0) {
            errors.push_back("quests.json: unknown god or hero in '" + q.at("ref_title").get<std::string>() + "'");
        }
    }
}

}  // namespace

std::optional<Footprint> parse_footprint(const std::string& text) {
    // Accept 'x' and the UTF-8 multiplication sign U+00D7 (0xC3 0x97).
    std::string t;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (static_cast<unsigned char>(text[i]) == 0xC3 && i + 1 < text.size() &&
            static_cast<unsigned char>(text[i + 1]) == 0x97) {
            t.push_back('x');
            ++i;
        } else {
            t.push_back(text[i]);
        }
    }
    std::int32_t w = 0;
    std::int32_t h = 0;
    char sep = 0;
    std::istringstream in(t);
    if (in >> w >> sep >> h && sep == 'x' && w > 0 && h > 0) {
        return Footprint{w, h};
    }
    return std::nullopt;
}

DataLoadResult load_data_catalog(const std::string& directory) {
    DataLoadResult result;
    std::vector<fs::path> paths;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(directory, ec)) {
        if (entry.path().extension() == ".json") {
            paths.push_back(entry.path());
        }
    }
    if (ec) {
        result.errors.push_back("cannot read data directory: " + directory);
        return result;
    }
    std::sort(paths.begin(), paths.end());
    DataCatalog cat;
    Fnv1a64 version;
    std::map<std::string, json> docs;
    for (const auto& p : paths) {
        std::ifstream f(p, std::ios::binary);
        std::stringstream buf;
        buf << f.rdbuf();
        const std::string bytes = buf.str();
        const auto name = p.filename().string();
        version.add_string(name);
        version.add_string(bytes);
        cat.files.push_back(name);
        try {
            docs.emplace(name, json::parse(bytes));
        } catch (const json::exception& e) {
            result.errors.push_back(name + ": " + e.what());
        }
    }
    cat.version = version.value();
    for (const auto* required : {"buildings.json", "building_stats.json", "housing.json", "gods.json", "monsters.json",
                                 "heroes.json", "quests.json", "walkers.json", "goods.json", "difficulty.json"}) {
        if (docs.count(required) == 0) {
            result.errors.push_back(std::string("missing required file: ") + required);
        }
    }
    if (!result.errors.empty()) {
        return result;
    }
    try {
        validate(docs, result.errors);
        load_buildings(docs.at("buildings.json"), docs.at("building_stats.json"), cat);
        for (const auto& h : docs.at("housing.json")) {
            cat.housing_levels.push_back(h.at("id").get<std::string>());
        }
        const auto& diff = docs.at("difficulty.json");
        const auto level = difficulty_from_code(diff.value("default_level", "H"));
        if (!level) {
            result.errors.push_back("difficulty.json: invalid default_level");
        } else {
            cat.default_difficulty = *level;
        }
        constexpr const char* kTaxKeys[] = {"none", "very_low", "low", "normal", "high", "very_high", "outrageous"};
        for (std::size_t i = 0; i < cat.tax_rate_per_mille.size(); ++i) {
            // Parsing a decimal literal is exact and platform independent; rounding to per mille happens once at load.
            cat.tax_rate_per_mille[i] =
                static_cast<std::int32_t>(std::llround(diff.at("tax_rates").at(kTaxKeys[i]).get<double>() * 1000.0));
        }
    } catch (const json::exception& e) {
        result.errors.push_back(std::string("schema error: ") + e.what());
    }
    if (result.errors.empty()) {
        result.catalog = std::move(cat);
    }
    return result;
}

}  // namespace bo::core
