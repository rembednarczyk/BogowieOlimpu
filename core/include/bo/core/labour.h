// Module C3: labour. Rules: docs/gameplay/05-przemysl-praca.md (Z:60-63), 18 (worker percentages, S-004),
// 24 (sectors, priorities, allocation; conflict K-031 resolved for the manual: strict priority order).
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <set>
#include <string>

#include "bo/core/city.h"
#include "bo/core/data.h"
#include "bo/core/types.h"

namespace bo::core {

// Sectors of the labour panel (names from the original texts, S-008 class A). Stored in saves: append only.
enum class Sector : std::uint8_t {
    Husbandry = 0,
    Industry,
    Distribution,
    HygieneSafety,
    Administration,
    Culture,  // "science" for Atlantean buildings
    Mythology,
    Military,
    Count
};
inline constexpr std::size_t kSectors = static_cast<std::size_t>(Sector::Count);

// Priority levels (S-008): none, very low, low, moderate (default), high, very high.
inline constexpr std::uint8_t kPriorityLevels = 6;
inline constexpr std::uint8_t kDefaultPriority = 3;

std::optional<Sector> sector_of_tab(const std::string& tab);

struct LabourState {
    std::array<std::uint8_t, kSectors> priority{kDefaultPriority, kDefaultPriority, kDefaultPriority,
                                                kDefaultPriority, kDefaultPriority, kDefaultPriority,
                                                kDefaultPriority, kDefaultPriority};
    std::set<std::string> disabled_types;  // industries switched off (Z:62-63)
    bool full_staffing = false;  // development switch: every building fully staffed (tests, demo); not a game rule
    // Results of the last allocation (saved: finance and the panel read them).
    std::uint32_t workforce = 0;
    std::uint32_t employed = 0;
    std::array<std::uint32_t, kSectors> sector_need{};
    std::array<std::uint32_t, kSectors> sector_employed{};
};

// Workers a building needs (0 when it needs none or its industry is switched off).
std::uint32_t workers_needed(const Building& b, const DataCatalog& catalog, const LabourState& labour);

// Daily allocation: workforce from common houses, then sectors in priority order, then buildings inside each
// sector by equal staffing fraction. Sets Building::workers and Building::staffing_percent.
// `absent`: common-house residents in the field as soldiers (D3), not available for work.
// The workers of the common houses (Z:29) at the difficulty's and wage's share (doc 22), before soldiers and crews.
std::uint32_t labour_pool(const CityState& city, const DataCatalog& catalog, Difficulty d, WageRate wage);

void allocate_labour(CityState& city, const DataCatalog& catalog, Difficulty d, WageRate wage, LabourState& labour,
                     std::uint32_t absent = 0);

}  // namespace bo::core
