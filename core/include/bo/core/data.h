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
};

enum class WalkerKind : std::uint8_t { Roaming = 0, Destination, TwoPhase };

struct WalkerType {
    std::string id;
    WalkerKind kind = WalkerKind::Destination;
    std::vector<std::string> from;              // building ids (or family ids such as "agora")
    std::optional<std::uint32_t> roam_ticks;    // roaming budget (doc 24)
    std::int64_t speed_raw = 0;                 // steps per tick as Fixed raw value (54.4 tiles/month = 1.0)
};

struct DataCatalog {
    std::uint64_t version = 0;                      // FNV-1a 64 over file names and bytes, sorted by name
    std::vector<std::string> files;                 // loaded file names, sorted
    std::map<std::string, BuildingType> buildings;  // std::map: deterministic iteration order
    std::map<std::string, BuildingStats> building_stats;
    std::map<std::string, WalkerType> walkers;
    std::vector<std::string> housing_levels;        // in file order
    Difficulty default_difficulty = Difficulty::Hero;
    std::array<std::int32_t, static_cast<std::size_t>(TaxRate::Count)> tax_rate_per_mille{};
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
