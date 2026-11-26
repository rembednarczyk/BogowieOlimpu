// Modules B5 (roaming walker) and B6 (destination walker).
// Roaming algorithm: engine family (S-006 Julius figure/movement.c, S-007 Akhenaten movement.cpp) with Zeus
// numbers (docs/gameplay/24-algorytmy-silnika.md). Differences from the family, by choice:
//  - 4-direction grid indices (N, E, S, W) instead of the family's 0/2/4/6 on an 8-direction compass;
//  - no roam history (Pharaoh only, S-007), as in Caesar III (S-006).
// Zeus addition: Fountain and Infirmary walkers return to the road tile north of the building that does not
// touch it (doc 17, S-004).
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "bo/core/city.h"
#include "bo/core/data.h"
#include "bo/core/map.h"
#include "bo/core/path.h"

namespace bo::core {

inline constexpr std::uint8_t kServiceCoverageDays = 96;  // a visit lasts 96 days (doc 24)
inline constexpr std::int32_t kServiceRadius = 2;          // 5x5 square around the walker (doc 24)
inline constexpr std::int32_t kRoamTargetDistance = 8;     // initial aim, tiles from the building
inline constexpr std::int32_t kRoamTargetRoadRadius = 6;

// AtTarget: a working walker (deliveryman, vendor) reached its target and waits for the logistics step.
enum class WalkerPhase : std::uint8_t { ToTarget = 0, Roaming, Returning, Routed, Done, AtTarget };

// What a destination walker carries out (module C5). Stored in saves: append only.
enum class WalkerJob : std::uint8_t { None = 0, Deliver, Fetch, Shear, Tend, Harvest, Extract, Hunt, Fish };

struct Walker {
    WalkerId id = kNoWalker;
    std::string type;
    WalkerKind kind = WalkerKind::Destination;
    BuildingId home = kNoBuilding;
    WalkerPhase phase = WalkerPhase::Routed;
    TilePos tile;
    Dir dir = Dir::North;
    std::optional<Dir> came_from;
    std::int64_t speed_raw = 65536;    // steps per tick, Q.16
    std::int64_t step_accum_raw = 0;   // accumulated steps toward the next tile, Q.16
    std::vector<TilePos> path;         // remaining routed path (front = next tile)
    std::uint32_t path_pos = 0;
    // Roaming state
    std::uint32_t roam_ticks = 0;
    std::uint32_t max_roam_ticks = 0;
    TilePos roam_target;
    std::int8_t turn = 1;                     // +1 clockwise (right), -1 counter-clockwise
    std::int32_t ticks_until_next_turn = -1;  // failed-pick budget before re-aiming; -1 = never aimed
    std::uint32_t random_counter = 0;
    bool return_north = false;  // S-004: Fountain, Infirmary
    std::uint8_t coverage_days = kServiceCoverageDays;  // counter a visit sets (clerk: 50, doc 24)
    // Logistics (C5)
    WalkerJob job = WalkerJob::None;
    BuildingId target = kNoBuilding;
    std::map<std::string, std::uint32_t> cargo;  // good -> loads
    std::string want_good;          // Fetch for a Get order: the good and loads to take
    std::uint32_t want_loads = 0;
    TilePos resource;               // Extract: the resource tile worked
    std::uint32_t work_ticks = 0;   // Extract: ticks of work left at the resource
};

struct WalkerStore {
    std::uint32_t next_id = 1;
    std::map<WalkerId, Walker> items;
};

// Spawns a roaming walker of `type` from `home` (needs road access). Returns kNoWalker when impossible.
WalkerId spawn_roaming_walker(CityState& city, WalkerStore& walkers, const DataCatalog& catalog,
                              const std::string& type, BuildingId home, Dir start_dir);

// Spawns a destination walker from `home` to tile `to` along roads (roadblocks allowed, Z:19-20), or across
// open land in PathMode::Walkable (shepherds, growers; `goal` is the target building whose tiles are passable).
WalkerId spawn_destination_walker(CityState& city, WalkerStore& walkers, const DataCatalog& catalog,
                                  const std::string& type, BuildingId home, TilePos to,
                                  PathMode mode = PathMode::Road, BuildingId goal = kNoBuilding);

// Advances one walker by one tick. Sets phase Done when the walker should be removed.
void step_walker(CityState& city, Walker& w);

// Sends a walker back to its home building along roads (phase Returning), or Done when there is no way.
void send_home(CityState& city, Walker& w);

// Gives service coverage around `w` to every building in the 5x5 square.
void provide_coverage(CityState& city, const Walker& w);

}  // namespace bo::core
