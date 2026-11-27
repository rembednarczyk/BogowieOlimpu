// Modules C4 (production) and C5 (distribution), stage 1: farms, Granary and Storehouse deliveries, agora
// vendors and peddlers. Rules: docs/gameplay/04, 06, 19, 24. Assumptions: L-100 (vendor load, stall refill
// threshold, how much a peddler leaves in a house).
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "bo/core/data.h"
#include "bo/core/state.h"

namespace bo::core {

// Vendor fetches up to this many loads [ZAŁOŻENIE, L-100: same as a storage deliveryman, doc 19].
inline constexpr std::uint32_t kVendorLoads = 4;
// A stall sends its vendor when it holds less than this many units of its goods [ZAŁOŻENIE, L-100].
inline constexpr std::uint32_t kStallRefillUnits = 200;
// Default vendor reach when the data gives none (doc 24: 40 tiles [RODZINA]).
inline constexpr std::int32_t kVendorReach = 40;

// Daily production. Farms gain loads_per_year x staffing percent per day and harvest once a year at the start
// of their harvest month: loads = progress / (100 x 192); a full year at full staffing gives loads_per_year
// (S-004, doc 19). Other producers arrive in stage 2.
void advance_production(WorldState& state, const DataCatalog& catalog);

struct LogisticsEvents {
    std::vector<WalkerId> spawned;
};

// Daily: producers with finished loads send one deliveryman (1 load, doc 19) to the nearest receiver by road,
// in the order of the building's delivery_priority; stalls below the refill level send their vendor to the
// nearest storage of their source types within reach.
LogisticsEvents dispatch_workers(WorldState& state, const DataCatalog& catalog);

// Every tick: working walkers at their target hand over or take goods and turn home; walkers back home unload.
void handle_arrivals(WorldState& state, const DataCatalog& catalog);
void unload_at_home(WorldState& state, const DataCatalog& catalog, const Walker& w);

// Peddler visit (doc 06): every house in the 5x5 square around `w` gets the goods its current or next level
// needs from the stalls of the peddler's agora, up to the house target (L-100).
// The water tile next to a coast building on its water side (fisheries, wharves), if any.
std::optional<TilePos> mooring(const CityState& city, const Building& b);

void peddler_sells(WorldState& state, const DataCatalog& catalog, const Walker& w);

// Stage 2 (husbandry.cpp): animals ready for shearing or milking, orchards ripening from pruner visits and
// losing unpicked fruit after the harvest season, processors turning input loads into output loads.
void advance_husbandry(WorldState& state, const DataCatalog& catalog);

// Stage 2: sends shepherds and goatherds to ready animals, a pruner once a month, and up to 3 harvesters in
// the harvest months (doc 19). Field workers walk across open land.
void dispatch_field_workers(WorldState& state, const DataCatalog& catalog, LogisticsEvents& ev);

// Stage 2: what a field worker does at its target and brings home.
void field_work_at_target(WorldState& state, const DataCatalog& catalog, Walker& w);
void field_work_unload(WorldState& state, const DataCatalog& catalog, const Walker& w, Building& home);

// Processors accept a delivery of their input while below the input capacity (3 loads, doc 19); the horse
// ranch takes wheat in units, the chariot factory wood and horses in pieces.
bool processor_accepts(const Building& b, const DataCatalog& catalog, const std::string& good);
std::uint32_t processor_room_loads(const Building& b, const DataCatalog& catalog, const std::string& good);
void processor_add(Building& b, const DataCatalog& catalog, const std::string& good, std::uint32_t loads);

// A pruner visit marks every tree within this radius of the visited tree as tended [ZAŁOŻENIE, L-101].
inline constexpr std::int32_t kPrunerRadius = 2;
// Harvesters out at once per lodge (doc 19: 3 harvesters in the harvest season).
inline constexpr std::size_t kHarvesters = 3;

// Stage 3 (extraction.cpp, L-102): timber mill, foundry, refinery and mint send up to 3 workers (scaled by
// staffing) to the nearest free resource tile in reach; each works work_days_per_trip and brings
// units_per_trip back; every 100 units make a load (Foundry: tin is built in). The Mint turns 200 silver into
// 100 drachmas for the treasury. Masonry and black marble workshops make loads_per_year while a quarry is in
// reach. Sculpture studios need 4 bronze per sculpture.
void dispatch_extractors(WorldState& state, const DataCatalog& catalog, LogisticsEvents& ev);
// Returns true while the worker is still working at the resource.
bool extraction_at_target(WorldState& state, const DataCatalog& catalog, Walker& w);
void extraction_unload(WorldState& state, const DataCatalog& catalog, const Walker& w, Building& home);
// Daily: mint batches, quarry workshops. Returns drachmas minted today.
std::int64_t advance_extraction(WorldState& state, const DataCatalog& catalog);
// Herd births (L-101): a herd of 2 to 7 animals gets a young one every kBirthDays; returns new animal ids.
std::vector<BuildingId> herd_births(WorldState& state, const DataCatalog& catalog);
inline constexpr std::uint32_t kBirthDays = 32;
// Extraction stops while this many finished loads wait at the building [ZAŁOŻENIE, L-102].
inline constexpr std::uint32_t kExtractionStock = 4;

// Hunting and fishing (wildlife.cpp, L-103): boar spawn points keep a herd of 7 (a young boar every 26 days
// below 7), deer spawn points add one deer a month; a lodge sends one hunter at a time to the nearest spawn
// point with animals in reach. A fishery first builds its boat, then the boat sails to the nearest fishing
// ground, fishes, sails back; an urchin quay's gatherer wades to the urchins. Each trip brings one load.
void advance_wildlife(WorldState& state, const DataCatalog& catalog);
void dispatch_hunters_and_boats(WorldState& state, const DataCatalog& catalog, LogisticsEvents& ev);
bool wildlife_at_target(WorldState& state, const DataCatalog& catalog, Walker& w);  // true: still working
void wildlife_unload(WorldState& state, const DataCatalog& catalog, const Walker& w, Building& home);
inline constexpr std::uint8_t kBoarHerd = 7;
inline constexpr std::uint16_t kBoarBirthDays = 26;

// Stalls (agora.cpp): the vendor goes to the nearest storage, trade building or pickup producer of its sources.
void dispatch_vendor(WorldState& state, const DataCatalog& catalog, BuildingId stall, LogisticsEvents& ev);
std::string walker_type_from(const DataCatalog& catalog, const std::string& building_type);

// Horses and chariots (horses.cpp; doc 05, 06, 19; P:18): the horse ranch and the chariot factory keep their
// output for a pickup. A chariot factory short of horses sends its hostler to the nearest ranch with a horse
// within the hostler's reach (44 tiles, S-005); the hostler brings one horse [ZAŁOŻENIE, L-106].
void dispatch_hostlers(WorldState& state, const DataCatalog& catalog, LogisticsEvents& ev);
void fetch_from_producer(WorldState& state, const DataCatalog& catalog, Walker& w, Building& producer);
void hostler_unload(const DataCatalog& catalog, const Walker& w, Building& home);

// Wolves (wolves.cpp, Z:45, L-107): daily, a den with wolves sends one on average once in hunt_one_in_days
// to the nearest sheep or goat in reach; at the prey it takes the animal unless a shepherd of its shed is
// near, then goes back to the den. Packs refill one wolf every refill_days.
struct WolfEvents {
    std::vector<WalkerId> spawned;
    std::vector<BuildingId> removed;
};
WolfEvents advance_wolves(WorldState& state, const DataCatalog& catalog);

// Units a house wants to hold of each house good [ZAŁOŻENIE, L-100: eZeus, owner's decision 2026-11-26]: food
// 2 units a resident (8 months at 0.25), 8 units (4 months) of fleece, oil, wine when the current or next level
// needs them; armor for each soldier of the level (doc 22), horses up to HS.
std::uint32_t house_target(const DataCatalog& catalog, const HouseState& h, HouseGood g, Difficulty d);

}  // namespace bo::core
