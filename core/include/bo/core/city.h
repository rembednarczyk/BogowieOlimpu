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
    std::uint8_t happiness = 0;        // C8: 0-100, set to the difficulty base when people move in
    std::uint8_t hungry_updates = 0;   // C8: mood updates in a row without food
    std::uint8_t days_without_road = 0;  // C8: people leave after more than 4 days (doc 24)
    std::uint16_t quarantine_days = 0;   // C7: after a plague nobody moves in (Z:85)

    friend bool operator==(const HouseState& a, const HouseState& b) {
        return a.elite == b.elite && a.level == b.level && a.population == b.population && a.goods == b.goods &&
               a.food_remainder == b.food_remainder && a.failed_checks == b.failed_checks &&
               a.abandoned == b.abandoned && a.happiness == b.happiness && a.hungry_updates == b.hungry_updates &&
               a.days_without_road == b.days_without_road && a.quarantine_days == b.quarantine_days;
    }
};

// Storage orders (Z:66-69). Stored in saves: append only.
enum class StorageOrderKind : std::uint8_t { Accept = 0, Get, Empty, DontAccept };

struct StorageOrder {
    StorageOrderKind kind = StorageOrderKind::Accept;
    std::uint32_t limit = 32;  // loads (Accept and Get)
    friend bool operator==(const StorageOrder& a, const StorageOrder& b) {
        return a.kind == b.kind && a.limit == b.limit;
    }
};

// Granary and Storehouse (module C5): stock in loads, orders per good (missing entry = default order).
struct StorageState {
    std::map<std::string, std::uint32_t> loads;
    std::map<std::string, StorageOrder> orders;
    WalkerId deliveryman = kNoWalker;  // storage deliveryman (up to 4 loads, doc 19)
    friend bool operator==(const StorageState& a, const StorageState& b) {
        return a.loads == b.loads && a.orders == b.orders && a.deliveryman == b.deliveryman;
    }
};

// Producer (module C4): yearly progress, finished loads waiting for the deliveryman.
struct ProducerState {
    std::uint32_t progress = 0;      // farms and processors: loads_per_year x staffing percent per day
    std::uint32_t output_loads = 0;  // finished loads at the building
    WalkerId deliveryman = kNoWalker;
    std::uint32_t input_loads = 0;                // processors: raw loads waiting (olive press, winery)
    std::map<std::string, std::uint32_t> units;   // lodges: fruit in units (100 = one load)
    std::vector<WalkerId> field_workers;          // shepherds, pruners, harvesters out in the fields
    friend bool operator==(const ProducerState& a, const ProducerState& b) {
        return a.progress == b.progress && a.output_loads == b.output_loads && a.deliveryman == b.deliveryman &&
               a.input_loads == b.input_loads && a.units == b.units && a.field_workers == b.field_workers;
    }
};

// Sheep and goats (Z:44-45): no tiles of their own (any number fit on one meadow tile, S-004 trivia).
struct AnimalState {
    BuildingId shed = kNoBuilding;
    std::uint16_t days_to_ready = 96;  // first shearing or milking after 6 months, then yearly (doc 19)
    bool worker_coming = false;
    friend bool operator==(const AnimalState& a, const AnimalState& b) {
        return a.shed == b.shed && a.days_to_ready == b.days_to_ready && a.worker_coming == b.worker_coming;
    }
};

// Olive trees and grapevines (Z:41-43, doc 19): ripeness grows from pruner visits; percent = units to pick.
struct OrchardState {
    std::uint32_t growth = 0;            // ripeness x 192 (100 % after a year of full growth)
    std::uint16_t days_since_tended = 0;
    bool worker_coming = false;
    friend bool operator==(const OrchardState& a, const OrchardState& b) {
        return a.growth == b.growth && a.days_since_tended == b.days_since_tended && a.worker_coming == b.worker_coming;
    }
};

// Agora stall (module C5): goods in units (100 per load) and its vendor.
struct StallState {
    std::map<std::string, std::uint32_t> units;
    WalkerId vendor = kNoWalker;
    friend bool operator==(const StallState& a, const StallState& b) {
        return a.units == b.units && a.vendor == b.vendor;
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
    std::optional<StorageState> storage;    // Granary, Storehouse
    std::optional<ProducerState> producer;  // buildings with outputs
    std::optional<StallState> stall;        // agora stalls
    std::optional<AnimalState> animal;      // sheep, goats
    std::optional<OrchardState> orchard;    // olive trees, grapevines
    // Risks (C7): counters grow by the building's risk each day; a visit of the right walker clears them.
    std::uint16_t fire_counter = 0;
    std::uint16_t damage_counter = 0;
    std::uint16_t disease_counter = 0;
    std::uint16_t crime_counter = 0;
    std::uint8_t burning_days = 0;  // 0: not burning
    std::optional<TilePos> road_access;   // exit and entry tile (recomputed on every road or building change)
    std::uint8_t staffing_percent = 100;  // set daily by the labour module (C3)
    std::uint32_t workers = 0;            // assigned by the labour module (C3)
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

// Animals at a spawn point (doc 19: a boar herd of 7, a young boar every 26 days below 7; deer one a month).
struct SpawnState {
    std::uint8_t animals = 0;
    std::uint16_t timer = 0;
    friend bool operator==(const SpawnState& a, const SpawnState& b) { return a.animals == b.animals && a.timer == b.timer; }
};

struct CityState {
    TileMap map;
    std::map<std::uint32_t, SpawnState> spawns;  // tile index -> herd
    std::uint8_t wind = 0;                       // C7: direction fire spreads, drawn each year (S-006)
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
// Empty when all tiles lie on one level without ramps [ZAŁOŻENIE, L-099], otherwise the reason key.
std::string level_check(const CityState& city, const std::vector<TilePos>& tiles);

// Special placements (placement.cpp). Each validates, then calls add_building.
BuildResult place_on_coast(CityState& city, Building b);
BuildResult place_agora(CityState& city, Building b, bool road_in_center, std::int32_t slots);
BuildResult place_stall(CityState& city, Building b);
BuildResult place_over_road(CityState& city, Building b, bool both_sides);
BuildResult place_crossing(CityState& city, Building b, bool no_adjacent_same);
// Sheep and goats: on a free meadow tile, joining the nearest shed of `shed_type` with room in its herd.
BuildResult place_animal(CityState& city, const DataCatalog& catalog, Building b, const std::string& shed_type);

// Undo (undo.cpp). `before` is the city as it was before the tick's build commands.
void record_undo(CityState& city, const CityState& before, std::uint64_t tick, std::int64_t cost);
// Reverts the record. Returns the refund, or nullopt when there is nothing to undo.
std::optional<std::int64_t> apply_undo(CityState& city);

// Every building whose footprint intersects the square of `radius` around `center`, in ascending id order.
std::vector<BuildingId> buildings_in_square(const CityState& city, TilePos center, std::int32_t radius);

}  // namespace bo::core
