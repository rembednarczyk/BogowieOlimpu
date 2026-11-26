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
        const auto& rr = o.value("requires_road", json(true));
        t.requires_road = rr.is_boolean() ? rr.get<bool>() : true;
        t.rotatable = o.value("rotatable", false);
        t.return_north = o.value("return_tile", "") == "north";
        const auto& pl = o.value("placement", json());
        t.placement = pl.is_string() ? pl.get<std::string>() : "";
        t.stall_slots = o.value("stall_slots", 0);
        t.no_adjacent_same = o.value("no_adjacent_same", false);
        t.plot_needs_road = o.value("plot_needs_road", false);
        t.stats_id = o.value("stats_from", t.id);
        auto strings = [&o](const char* key) {
            std::vector<std::string> v;
            const auto& a = o.value(key, json());
            if (a.is_array()) {
                for (const auto& e : a) {
                    if (e.is_string()) {
                        v.push_back(e.get<std::string>());
                    }
                }
            }
            return v;
        };
        t.stores_class = strings("stores_class");
        t.outputs = strings("outputs");
        t.delivery_priority = strings("delivery_priority");
        t.sells = strings("sells");
        t.sources = strings("sources");
        t.accepts_only_when_fully_staffed = o.value("accepts_only_when_fully_staffed", false);
        const auto& order = o.value("default_order", json());
        t.storage_wheat_only_for_horses = order.is_object() && order.value("wheat", "") == "dont_accept";
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

void load_walkers(const json& walkers, DataCatalog& cat) {
    for (const auto& o : walkers) {
        WalkerType w;
        w.id = o.at("id").get<std::string>();
        const auto kind = o.value("type", "destination");
        w.kind = kind == "roaming" ? WalkerKind::Roaming : kind == "two_phase" ? WalkerKind::TwoPhase : WalkerKind::Destination;
        const auto& from = o.value("from", json());
        if (from.is_string()) {
            w.from.push_back(from.get<std::string>());
        } else if (from.is_array()) {
            for (const auto& f : from) {
                w.from.push_back(f.get<std::string>());
            }
        }
        if (o.contains("roam_ticks")) {
            w.roam_ticks = o.at("roam_ticks").get<std::uint32_t>();
        }
        // Speed in tiles per month -> steps per tick, as Q.16 fixed point: tiles * 15 / 816 ticks.
        // Decimal speeds (54.4, 72.53...) are converted once at load time in integer thousandths.
        const auto milli = static_cast<std::int64_t>(std::llround(o.value("speed_tiles_per_month", 54.4) * 1000.0));
        w.speed_raw = (milli * 15 * 65536) / (816 * 1000);
        w.culture_points = o.value("culture_points", 0);
        w.coverage_days = o.value("coverage_days", static_cast<std::uint8_t>(96));
        w.max_source_distance = opt_int(o.value("max_source_distance_tiles", json()));
        cat.walkers.emplace(w.id, w);
    }
}

void load_housing(const json& housing, DataCatalog& cat) {
    constexpr const char* kGoodKeys[] = {"Fd", "Fl", "Oil", "Wi", "Ar", "Hrs"};
    for (const auto& h : housing) {
        HousingLevel lv;
        lv.id = h.at("id").get<std::string>();
        lv.elite = h.value("class", "common") == "elite";
        lv.level = h.value("level", 1);
        cat.housing_levels.push_back(lv.id);
        const auto& use = h.at("consumption_units_per_month_full");
        constexpr const char* kUseKeys[] = {"food", "fleece", "olive_oil", "wine"};
        for (std::size_t g = 0; g < 4; ++g) {
            lv.consumption[g] = use.value(kUseKeys[g], 0);
        }
        const auto& models = h.at("model_by_difficulty");
        for (std::size_t d = 0; d < lv.model.size(); ++d) {
            const auto& m = models.at(std::string(difficulty_code(static_cast<Difficulty>(d))));
            HouseModel& hm = lv.model[d];
            hm.appeal_devolve = m.at("AppD").get<std::int32_t>();
            hm.appeal_evolve = m.at("AppE").get<std::int32_t>();
            hm.culture = m.at("Ent").get<std::int32_t>();
            hm.water = m.at("W").get<std::int32_t>() != 0;
            for (std::size_t g = 0; g < kHouseGoods; ++g) {
                hm.needs[g] = m.at(kGoodKeys[g]).get<std::int32_t>() != 0;
            }
            hm.population = m.at("Pop").get<std::int32_t>();
            hm.tax_multiplier = m.at("TRM").get<std::int32_t>();
            hm.disease_risk = opt_int(m.value("DRI", json())).value_or(0);
            hm.crime_risk = opt_int(m.value("CRI", json())).value_or(0);
            hm.soldiers_percent = opt_int(m.value("%S", json())).value_or(0);
        }
        (lv.elite ? cat.elite_levels : cat.common_levels).push_back(lv);
    }
}

void load_migration(const json& m, const json& diff, MigrationParams& p) {
    for (std::size_t d = 0; d < p.base_happiness.size(); ++d) {
        p.base_happiness[d] = m.at("base_happiness").at(std::string(difficulty_code(static_cast<Difficulty>(d))));
    }
    p.small_city_population = m.at("small_city_population");
    for (const auto& row : m.at("unemployment_mood")) {
        p.unemployment_mood.emplace_back(row.at("above_percent"), row.at("mood"));
    }
    constexpr const char* kWageKeys[] = {"none", "very_low", "low", "normal", "high", "very_high"};
    for (std::size_t i = 0; i < p.wage_mood.size(); ++i) {
        p.wage_mood[i] = m.at("wage_mood").at(kWageKeys[i]);
    }
    p.food_stocked = m.at("food_mood").at("stocked");
    p.food_missing_first = m.at("food_mood").at("missing_first");
    p.food_missing_max = m.at("food_mood").at("missing_max");
    p.debt_mood = m.at("debt_mood");
    for (const auto& row : m.at("migration_table")) {
        p.migration_table.emplace_back(row.at("above"), row.at("percent"));
    }
    p.batch_people = m.at("batch_people");
    p.min_batch = m.at("min_batch");
    p.emigration_min_population = m.at("emigration_min_population");
    p.fill_first_free_slots = m.at("fill_first_free_slots");
    constexpr const char* kTaxKeys[] = {"none", "very_low", "low", "normal", "high", "very_high", "outrageous"};
    for (std::size_t t = 0; t < p.tax_mood.size(); ++t) {
        for (std::size_t d = 0; d < p.tax_mood[t].size(); ++d) {
            p.tax_mood[t][d] = diff.at("tax_sentiment").at(kTaxKeys[t]).at(d);
        }
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
    std::map<std::string, std::string> files;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(directory, ec)) {
        if (entry.path().extension() == ".json") {
            std::ifstream f(entry.path(), std::ios::binary);
            std::stringstream buf;
            buf << f.rdbuf();
            files.emplace(entry.path().filename().string(), buf.str());
        }
    }
    if (ec) {
        DataLoadResult result;
        result.errors.push_back("cannot read data directory: " + directory);
        return result;
    }
    return load_data_catalog_from_memory(files);
}

DataLoadResult load_data_catalog_from_memory(const std::map<std::string, std::string>& files) {
    DataLoadResult result;
    DataCatalog cat;
    Fnv1a64 version;
    std::map<std::string, json> docs;
    for (const auto& [name, bytes] : files) {  // std::map: sorted by name
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
                                 "heroes.json", "quests.json", "walkers.json", "goods.json", "difficulty.json",
                                 "migration.json"}) {
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
        load_walkers(docs.at("walkers.json"), cat);
        load_housing(docs.at("housing.json"), cat);
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
        constexpr const char* kWageKeys[] = {"none", "very_low", "low", "normal", "high", "very_high"};
        for (std::size_t i = 0; i < cat.wage_per_mille.size(); ++i) {
            cat.wage_per_mille[i] = static_cast<std::int32_t>(
                std::llround(diff.at("wage_drachmas_per_worker_year").at(kWageKeys[i]).get<double>() * 1000.0));
        }
        load_migration(docs.at("migration.json"), diff, cat.migration);
        for (const auto& g : docs.at("goods.json")) {
            cat.goods[g.at("id").get<std::string>()] = g.value("class", "");
        }
        if (docs.count("production.json") != 0) {
            for (const auto& p : docs.at("production.json")) {
                ProductionParams pp;
                const auto& lpy = p.value("loads_per_year", json());
                pp.loads_per_year = lpy.is_array() ? lpy.at(0).get<std::int32_t>() : opt_int(lpy).value_or(0);
                if (const auto m = opt_int(p.value("harvest_month", json()))) {
                    pp.harvest_month = *m - 1;  // data: 1-12
                }
                pp.max_storage_distance = opt_int(p.value("max_storage_distance", json())).value_or(300);
                cat.production[p.at("id").get<std::string>()] = pp;
            }
        }
        const auto& logistics = docs.count("logistics.json") != 0 ? docs.at("logistics.json") : json::object();
        cat.storage_slots = logistics.value("storage_tiles", 8);
        cat.loads_per_slot = logistics.value("loads_per_storage_tile", 4);
        cat.units_per_load = logistics.value("units_per_load", 100);
        for (std::size_t d = 0; d < cat.worker_percent.size(); ++d) {
            const auto& row = diff.at("worker_percentage").at(std::string(difficulty_code(static_cast<Difficulty>(d))));
            for (std::size_t w = 0; w < cat.worker_percent[d].size(); ++w) {
                cat.worker_percent[d][w] = row.at(w).get<std::int32_t>();
            }
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
