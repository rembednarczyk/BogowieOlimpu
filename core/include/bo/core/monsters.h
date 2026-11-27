// Module D5 stage 3 (docs/gameplay/10, 21, 23; Z:127-131; assumptions L-057b, L-116): monsters sent by the
// scenario, Hero's Halls and heroes who slay them.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "bo/core/city.h"
#include "bo/core/data.h"

namespace bo::core {

struct WorldState;

struct MonsterState {
    std::string id;                 // monsters.json
    WalkerId walker = kNoWalker;
    TilePos home{0, 0};             // the monster point it comes back to (S-004 scendesign)
    std::string target_kind;        // food (default), citizens, military, resources (doc 21)
    BuildingId target = kNoBuilding;
    std::uint16_t days = 0;         // days attacking the target, or resting at home
    bool resting = false;
    bool slain = false;
    friend bool operator==(const MonsterState& a, const MonsterState& b) {
        return a.id == b.id && a.walker == b.walker && a.home == b.home && a.target_kind == b.target_kind &&
               a.target == b.target && a.days == b.days && a.resting == b.resting && a.slain == b.slain;
    }
};

struct HeroState {
    std::string id;                 // heroes.json
    BuildingId hall = kNoBuilding;
    WalkerId walker = kNoWalker;
    friend bool operator==(const HeroState& a, const HeroState& b) {
        return a.id == b.id && a.hall == b.hall && a.walker == b.walker;
    }
};

struct MonsterEvents {
    std::vector<WalkerId> spawned;
    std::vector<BuildingId> removed;
    std::vector<std::pair<std::string, bool>> slain;  // monster, by a hero
};

// A monster appears at the monster point ('M'; else the land entry) with hit points x strength (L-057b). A sea
// monster (Kraken, Scylla) needs water: the monster point if it lies on water, else the sea entry (L-116).
std::string spawn_monster(WorldState& state, const DataCatalog& catalog, const std::string& id, std::uint32_t strength,
                          const std::string& target_kind, MonsterEvents& ev);
// Daily: a monster walks to its target, wrecks it after destroy_days, goes home and rests rest_days (L-116);
// Apollo from his sanctuary kills it (Z:129); a hero walks to the monster he slays; deaths are noted.
void advance_monsters(WorldState& state, const DataCatalog& catalog, MonsterEvents& ev);
// Every tick: a monster that petrifies (Medusa) turns to stone the city's walkers next to it (Z:128); they
// end now. Returns (monster, walker) for each.
std::vector<std::pair<std::string, WalkerId>> petrify(WorldState& state, const DataCatalog& catalog);

// Hero's Hall is built only while a monster threatens the city (Z:130, doc 21).
bool hero_needed(const WorldState& state);
// Requirements of the hero who slays a monster now in the city (doc 21, L-116). Returns the unmet ones.
std::vector<std::string> unmet_requirements(const WorldState& state, const DataCatalog& catalog, const std::string& hero,
                                            BuildingId hall);
// A hero's walker standing at his Hall (after summoning or coming back from a quest).
WalkerId hero_walker(WorldState& state, const DataCatalog& catalog, const HeroState& hero);
// Summons the hero to a Hall: all requirements met; Perseus takes his 3000 drachmas (doc 21).
std::string summon_hero(WorldState& state, const DataCatalog& catalog, BuildingId hall, MonsterEvents& ev);

}  // namespace bo::core
