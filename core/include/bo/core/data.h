// Module A4: game data (Type Object pattern, R. Nystrom). Types live in data/*.json, behaviour in systems.
// The catalog is immutable after load. The data version (hash of all files) is stored in saves.
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "bo/core/types.h"

namespace bo::core {

struct Footprint {
    std::int32_t width = 0;
    std::int32_t height = 0;
};

using PerDifficulty = std::array<std::optional<std::int32_t>, static_cast<std::size_t>(Difficulty::Count)>;

struct AppealParams {
    std::int32_t initial = 0;
    std::int32_t step_tiles = 1;
    std::int32_t step_change = 0;
    std::int32_t range = 0;
};

struct BuildingStats {
    std::string id;
    std::string size_text;                // as in the source table, e.g. "4×4+ 3×3", "3x3m"
    std::optional<Footprint> footprint;   // main rectangle when it can be parsed
    std::optional<std::int32_t> workers;
    PerDifficulty cost{};
    std::optional<AppealParams> appeal;
    PerDifficulty fire_risk{};
    PerDifficulty damage_risk{};
    bool expansion = false;
};

// House goods (doc 18): stock in units, 100 units = 1 storage load.
enum class HouseGood : std::uint8_t { Food = 0, Fleece, OliveOil, Wine, Armor, Horses, Count };
inline constexpr std::size_t kHouseGoods = static_cast<std::size_t>(HouseGood::Count);

// One row of the S-004 house model for one difficulty (data/housing.json "model_by_difficulty").
struct HouseModel {
    std::int32_t appeal_devolve = 0;  // AppD: below this the house devolves
    std::int32_t appeal_evolve = 0;   // AppE: from this the house may evolve to the next level
    std::int32_t culture = 0;         // Ent: culture points the level needs
    bool water = false;               // W
    std::array<bool, kHouseGoods> needs{};  // Fd, Fl, Oil, Wi, Ar, Hrs
    std::int32_t population = 0;      // Pop
    std::int32_t tax_multiplier = 0;  // TRM
    std::int32_t disease_risk = 0;    // DRI
    std::int32_t crime_risk = 0;      // CRI (base game)
    std::int32_t soldiers_percent = 0;  // %S
};

struct HousingLevel {
    std::string id;
    bool elite = false;
    std::int32_t level = 1;  // 1-based within its class
    std::array<std::int32_t, kHouseGoods> consumption{};  // units per month for a full house (doc 18)
    std::array<HouseModel, static_cast<std::size_t>(Difficulty::Count)> model{};
};

struct BuildingType {
    std::string id;
    std::string tab;
    std::string culture;
    bool requires_road = true;  // Z:24-25; false for aesthetics, walls, trees, animals
    bool rotatable = false;     // Z:24-25: palace, stadium, sanctuaries, gatehouse, horse ranch, pyramids
    bool return_north = false;  // S-004: roaming walker returns to the road north of the building
    std::string placement;      // special placement rule, e.g. "straight_coast", "on_agora" (empty: normal)
    std::int32_t stall_slots = 0;    // agoras (Z:69-70)
    bool no_adjacent_same = false;   // water crossings (Z:102)
    std::string stats_id;            // row in building_stats.json (default: id; house plots use "hut")
    bool plot_needs_road = false;    // house plots must touch a road when placed (Z:29, Z:32)
    // Goods (module C4/C5)
    std::vector<std::string> stores_class;   // storage: goods classes kept (Granary: food; Storehouse: raw...)
    bool storage_wheat_only_for_horses = false;  // Storehouse: wheat only on request, default Don't Accept (Z:68)
    bool accepts_only_when_fully_staffed = false;  // Granary (Z:66)
    std::vector<std::string> outputs;          // goods produced
    std::vector<std::string> delivery_priority;  // destination categories, first match wins (doc 04/05)
    std::vector<std::string> sells;            // agora stall: goods (or class "food") it sells
    std::vector<std::string> sources;          // agora stall: building types its vendor fetches from
    std::map<std::string, std::vector<std::string>> delivery_priority_by_good;  // lodges: per fruit
    std::string requires_building;             // animals: their shed type (Z:44-45)
    std::vector<std::string> tends;            // lodges: tree types, in the order of `outputs`
    std::string footprint_text;                // footprint from buildings.json when the stats size has none
};

enum class WalkerKind : std::uint8_t { Roaming = 0, Destination, TwoPhase };

struct WalkerType {
    std::string id;
    WalkerKind kind = WalkerKind::Destination;
    std::vector<std::string> from;              // building ids (or family ids such as "agora")
    std::optional<std::uint32_t> roam_ticks;    // roaming budget (doc 24)
    std::int64_t speed_raw = 0;                 // steps per tick as Fixed raw value (54.4 tiles/month = 1.0)
    std::int32_t culture_points = 0;            // points a visit gives a house (doc 18)
    std::uint8_t coverage_days = 96;            // service counter set by a visit (doc 24; clerk 50)
    std::optional<std::int32_t> max_source_distance;  // vendors: tiles to a storage (doc 24: 37 measured)
};

// Mood and migration parameters (data/migration.json, doc 24).
struct MigrationParams {
    std::array<std::int32_t, static_cast<std::size_t>(Difficulty::Count)> base_happiness{};
    std::int32_t small_city_population = 300;
    std::vector<std::pair<std::int32_t, std::int32_t>> unemployment_mood;  // (above percent, mood), first match
    std::array<std::int32_t, static_cast<std::size_t>(WageRate::Count)> wage_mood{};
    std::int32_t food_stocked = 1;
    std::int32_t food_missing_first = -1;
    std::int32_t food_missing_max = -3;
    std::int32_t debt_mood = -2;
    std::vector<std::pair<std::int32_t, std::int32_t>> migration_table;  // (popularity above, percent)
    std::int32_t batch_people = 12;
    std::int32_t min_batch = 4;
    std::int32_t emigration_min_population = 100;
    std::int32_t fill_first_free_slots = 8;
    // Tax sentiment per tax rate and difficulty (data/difficulty.json, S-004).
    std::array<std::array<std::int32_t, static_cast<std::size_t>(Difficulty::Count)>,
               static_cast<std::size_t>(TaxRate::Count)>
        tax_mood{};
};

// Production parameters (data/production.json, S-004/S-005).
struct ProductionParams {
    std::int32_t loads_per_year = 0;       // at full staffing (lower bound when the source gives a range)
    std::optional<std::int32_t> harvest_month;  // farms: 0-based month of the yearly harvest
    std::int32_t max_storage_distance = 300;    // tiles a deliveryman may walk to a receiver
    std::string input_good;                     // processors: one input load per output load (olive press, winery)
    std::int32_t input_capacity = 0;            // processors: input loads held (3, doc 19)
    std::int32_t storage_loads = 0;             // sheds and lodges: finished loads held (5, doc 19)
    std::int32_t animals_max = 0;               // sheds: herd size (8, doc 19)
    std::map<std::string, std::vector<std::int32_t>> harvest_months;  // lodges: fruit -> 0-based months
};

struct DataCatalog {
    std::uint64_t version = 0;                      // FNV-1a 64 over file names and bytes, sorted by name
    std::vector<std::string> files;                 // loaded file names, sorted
    std::map<std::string, BuildingType> buildings;  // std::map: deterministic iteration order
    std::map<std::string, BuildingStats> building_stats;
    std::map<std::string, WalkerType> walkers;
    std::vector<std::string> housing_levels;        // in file order
    std::vector<HousingLevel> common_levels;        // hut .. townhouse
    std::vector<HousingLevel> elite_levels;         // residence .. estate
    Difficulty default_difficulty = Difficulty::Hero;
    std::array<std::int32_t, static_cast<std::size_t>(TaxRate::Count)> tax_rate_per_mille{};
    // Share of common-house residents who work, percent [difficulty][wage rate] (S-004, doc 18).
    std::array<std::array<std::int32_t, static_cast<std::size_t>(WageRate::Count)>,
               static_cast<std::size_t>(Difficulty::Count)>
        worker_percent{};
    // Yearly wage per worker in thousandths of a drachma, per wage rate (doc 18).
    std::array<std::int32_t, static_cast<std::size_t>(WageRate::Count)> wage_per_mille{};
    MigrationParams migration;
    std::map<std::string, std::string> goods;  // good id -> class (food, raw, goods, construction, money)
    std::map<std::string, ProductionParams> production;
    std::int32_t storage_slots = 8;            // logistics.json: 8 tiles x 4 loads (doc 19)
    std::int32_t loads_per_slot = 4;
    std::int32_t units_per_load = 100;
};

struct DataLoadResult {
    std::optional<DataCatalog> catalog;
    std::vector<std::string> errors;  // empty when the data is valid
};

// Loads every *.json file in `directory` and validates cross references.
DataLoadResult load_data_catalog(const std::string& directory);

// Same, from file contents already in memory (name -> bytes). Used by the engine layer, which reads files
// through its own virtual filesystem (e.g. res:// inside an Android APK).
DataLoadResult load_data_catalog_from_memory(const std::map<std::string, std::string>& files);

// Parses "24×16", "3x3m", "4×4+ 3×3" (first rectangle). Returns nullopt for "m", "2xn", "–".
std::optional<Footprint> parse_footprint(const std::string& text);

}  // namespace bo::core
