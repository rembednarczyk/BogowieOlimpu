// Modules B2 (construction) and B3 (roads, road networks, building road access).
// Rules: docs/gameplay/01-podstawy.md, 20-atrakcyjnosc-i-budynki.md, 17-czas-i-ruch.md (exit point).
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <utility>
#include <string>
#include <vector>

#include "bo/core/data.h"
#include "bo/core/map.h"

namespace bo::core {

using WalkerId = std::uint32_t;  // 0 = none
inline constexpr WalkerId kNoWalker = 0;

// Undo stays available for one day after the action [ZAŁOŻENIE, L-035: the window is unknown].
inline constexpr std::uint64_t kUndoWindowTicks = 51;
// Longest water span of a crossing [ZAŁOŻENIE, L-097: Z:101 says only "not too far"].
inline constexpr std::int32_t kMaxCrossingSpan = 8;

// A house plot and its people (module C2, docs/gameplay/03, 18). Population 0 = vacant plot.
struct HouseState {
    bool elite = false;
    std::uint8_t level = 0;  // index into DataCatalog::common_levels or elite_levels
    std::uint32_t population = 0;
    std::array<std::uint32_t, kHouseGoods> goods{};  // stock in units (100 = one load)
    std::uint32_t food_remainder = 0;  // carried part of the population-scaled food consumption
    std::uint8_t failed_checks = 0;    // devolution after 3 failed daily checks [RODZINA]
    bool abandoned = false;            // elite house left by its owners (Z:34)

    friend bool operator==(const HouseState& a, const HouseState& b) {
        return a.elite == b.elite && a.level == b.level && a.population == b.population && a.goods == b.goods &&
               a.food_remainder == b.food_remainder && a.failed_checks == b.failed_checks &&
               a.abandoned == b.abandoned;
    }
};

// How a building relates to roads under it.
enum class RoadMode : std::uint8_t {
    None = 0,
    OverRoad,  // built over an existing road that stays when the building goes (agoras, Z:69-70)
    OwnRoad,   // carries its own road that goes with it (water crossings and bridges, Z:101-103)
};

struct Building {
    BuildingId id = kNoBuilding;
    std::string type;
    TilePos origin;  // north-west corner of the footprint
    std::int32_t width = 1;
    std::int32_t height = 1;
    std::uint8_t rotation = 0;  // quarter turns clockwise; meaning per placement rule (docs/gameplay/01, B2)
    std::vector<TilePos> tiles;  // map tiles owned by the building (the whole rectangle for ordinary buildings)
    RoadMode road_mode = RoadMode::None;
    std::optional<TilePos> road_tile;  // boulevard and avenue: the road they decorate (access passes through)
    BuildingId parent = kNoBuilding;   // agora stall: its agora
    std::vector<TilePos> slot_origins;  // agora: north-west corners of the 2x2 stall slots
    std::vector<BuildingId> slots;      // agora: stall per slot (kNoBuilding = free)
    std::optional<HouseState> house;    // houses only
    std::optional<TilePos> road_access;   // exit and entry tile (recomputed on every road or building change)
    std::uint8_t staffing_percent = 100;  // set by the labour module (C3); 100 until it exists
    std::uint32_t days_since_spawn = 0;
    Dir next_roam_dir = Dir::North;       // rotates 90 degrees clockwise per roaming walker (doc 24)
    WalkerId roaming_walker = kNoWalker;  // at most one roaming walker alive per building (doc 24)
    // Service coverage counters, e.g. "water_bearer" -> 96, decaying 1 per day (doc 24).
    std::map<std::string, std::uint8_t> coverage;
};

bool operator==(const Building& a, const Building& b);
inline bool operator!=(const Building& a, const Building& b) { return !(a == b); }

// Buildings with stable ids in ascending order (ids are never reused).
struct BuildingStore {
    std::uint32_t next_id = 1;
    std::map<BuildingId, Building> items;
};

// Undo (Z:20): reverts the build commands executed in one tick (a drag is issued as one batch).
// Stores the difference against the state before that tick, so it can be saved.
struct UndoRecord {
    bool active = false;
    std::uint64_t tick = 0;  // tick of the recorded commands
    std::int64_t cost = 0;   // refunded on undo
    std::vector<std::pair<TilePos, Tile>> tiles_before;
    std::vector<Building> buildings_before;  // removed or changed buildings, as they were
    std::vector<BuildingId> added;           // buildings created by the action
};

struct CityState {
    TileMap map;
    BuildingStore buildings;
    UndoRecord undo;
    std::uint32_t homeless = 0;          // people pushed out of houses, waiting for module C8
    std::vector<std::int32_t> appeal;    // derived (C1): recomputed daily, not saved
};

// Reason keys are i18n keys (docs/PRAKTYKI.md ch. 12). Empty string = success.
struct BuildResult {
    std::string reason;
    BuildingId building = kNoBuilding;  // placed building
    std::vector<BuildingId> removed;    // buildings removed by a demolition (stalls and ornaments included)
    std::int64_t cost = 0;
    [[nodiscard]] bool ok() const { return reason.empty(); }
};

// Validates and places a building with its north-west corner at `origin`. Does not touch the treasury.
// `rotation` (0-3): rotatable buildings swap width and height on odd values; agoras pick their axis the same
// way; boulevards: road axis (even N-S, odd E-W); avenues: side of the ornament; crossings: direction across
// the water. Other buildings need 0.
BuildResult place_building(CityState& city, const DataCatalog& catalog, Difficulty difficulty,
                           const std::string& type, TilePos origin, std::uint8_t rotation = 0);
BuildResult place_road(CityState& city, const DataCatalog& catalog, Difficulty difficulty, TilePos p);
BuildResult place_roadblock(CityState& city, const DataCatalog& catalog, Difficulty difficulty, TilePos p);
BuildResult clear_land(CityState& city, const DataCatalog& catalog, Difficulty difficulty, TilePos p);
// Removes the building covering `p` (an occupied agora slot removes only its stall), or the road on `p`
// together with the boulevards and avenues decorating it.
BuildResult demolish(CityState& city, TilePos p);
// Removes a building and its stalls; clears its tiles (and its own road). Appends removed ids to `removed`.
void remove_building(CityState& city, BuildingId id, std::vector<BuildingId>& removed);
// Registers a building and writes its tiles to the map (no validation).
BuildingId add_building(CityState& city, Building b);

// B3: recomputes road network ids (connected components in tile-index order) and every building's road access.
void update_roads(CityState& city);

// Exit tile search (S-004 units; S-007 road_access): perimeter tiles starting north of the north-west corner,
// clockwise; the first road tile wins. A boulevard or avenue tile counts as its road (Z:154).
std::optional<TilePos> find_road_access(const CityState& city, TilePos origin, std::int32_t width,
                                        std::int32_t height);

// Empty when `p` is free buildable land, otherwise the reason key.
std::string land_check(const CityState& city, TilePos p);

// Special placements (placement.cpp). Each validates, then calls add_building.
BuildResult place_on_coast(CityState& city, Building b);
BuildResult place_agora(CityState& city, Building b, bool road_in_center, std::int32_t slots);
BuildResult place_stall(CityState& city, Building b);
BuildResult place_over_road(CityState& city, Building b, bool both_sides);
BuildResult place_crossing(CityState& city, Building b, bool no_adjacent_same);

// Undo (undo.cpp). `before` is the city as it was before the tick's build commands.
void record_undo(CityState& city, const CityState& before, std::uint64_t tick, std::int64_t cost);
// Reverts the record. Returns the refund, or nullopt when there is nothing to undo.
std::optional<std::int64_t> apply_undo(CityState& city);

// Every building whose footprint intersects the square of `radius` around `center`, in ascending id order.
std::vector<BuildingId> buildings_in_square(const CityState& city, TilePos center, std::int32_t radius);

}  // namespace bo::core
