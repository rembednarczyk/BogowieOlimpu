// Module D3 stage 1 (docs/gameplay/11, 22; Z:135-140): companies formed from houses, standards, muster,
// march and send home. Combat, morale losses and invaders come with stage 2 (assumptions L-109).
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "bo/core/city.h"
#include "bo/core/data.h"
#include "bo/core/map.h"

namespace bo::core {

struct WorldState;

// Stored in saves: append only.
enum class CompanyStatus : std::uint8_t { AtHome = 0, Marching, Still, GoingHome, Fighting, Fleeing, ServingAbroad };

struct CompanyMember {
    BuildingId house = kNoBuilding;
    std::uint8_t soldiers = 0;
    friend bool operator==(const CompanyMember& a, const CompanyMember& b) {
        return a.house == b.house && a.soldiers == b.soldiers;
    }
};

// Company tactics (Z:138-139). Stored in saves: append only.
enum class Tactic : std::uint8_t { Defensive = 0, Offensive, Special };

struct Company {
    std::string unit;  // rabble, hoplite, horseman (walker type of its soldiers)
    std::vector<CompanyMember> members;
    TilePos standard{0, 0};
    CompanyStatus status = CompanyStatus::AtHome;
    std::uint8_t morale = 100;
    std::vector<WalkerId> soldiers;  // soldiers in the field
    bool fled = false;               // fled in this battle: cannot be mustered until it ends (Z:140)
    // A god's company (Z:121, Z:140; since v34): the unit's stats come from this nation of enemies.json ("divine":
    // Ares's Sparti, "amazon": Artemis's Amazons); empty for the player's own soldiers. Its morale never drops.
    std::string nation;
    Tactic tactic = Tactic::Defensive;  // since v38; defensive by default (Z:138)
    // Lent by a partner city for the defence (Z:140, Z:174; since v40): its index, -1 for the player's own.
    // It comes in by the land entry, only defends the city and leaves when the fighting ends.
    std::int32_t lender = -1;
    friend bool operator==(const Company& a, const Company& b) {
        return a.unit == b.unit && a.members == b.members && a.standard == b.standard && a.status == b.status &&
               a.morale == b.morale && a.soldiers == b.soldiers && a.fled == b.fled && a.nation == b.nation &&
               a.tactic == b.tactic && a.lender == b.lender;
    }
};

// An invasion of the city (Z:137, Z:144). Started by the scenario or the world (D4/D5); for now by command.
struct Invasion {
    bool active = false;
    std::string nation;
    std::vector<WalkerId> invaders;
    TilePos origin{0, 0};  // where they came in and where they leave
    std::int32_t city = -1;  // the partner city that sent them (D4), -1: none (since v21)
    // By sea (Z:144; since v39): transports land the soldiers at `landing` (water by the shore), warships escort.
    bool by_sea = false;
    TilePos landing{0, 0};
    friend bool operator==(const Invasion& a, const Invasion& b) {
        return a.active == b.active && a.nation == b.nation && a.invaders == b.invaders && a.origin == b.origin &&
               a.city == b.city && a.by_sea == b.by_sea && a.landing == b.landing;
    }
};

// Trireme orders (Z:142-143). Stored in saves: append only.
enum class ShipOrder : std::uint8_t { Defend = 0, AttackAll, Repair, Patrol };
// Crew (Z:141, Z:150; since v42): none (Unmanned), aboard, or going home with the ship to the wharf.
enum class Crew : std::uint8_t { None = 0, Aboard, GoingHome };

// A player's warship (Z:141-144; since v39): one per wharf, the walker on the water, its post.
struct Ship {
    WalkerId walker = kNoWalker;
    BuildingId wharf = kNoBuilding;
    ShipOrder order = ShipOrder::Defend;
    TilePos station{0, 0};  // Defend Location: where it holds; the wharf's mooring at first
    Crew crew = Crew::None;  // launched unmanned (Z:150); Call Crew to Ship takes workers from their jobs
    friend bool operator==(const Ship& a, const Ship& b) {
        return a.walker == b.walker && a.wharf == b.wharf && a.order == b.order && a.station == b.station &&
               a.crew == b.crew;
    }
};

struct MilitaryState {
    std::vector<Company> companies;
    Invasion invasion;  // since v18
    std::vector<Ship> ships;  // since v39
    friend bool operator==(const MilitaryState& a, const MilitaryState& b) {
        return a.companies == b.companies && a.invasion == b.invasion && a.ships == b.ships;
    }
};

// Soldiers a house gives (doc 22): round(population x %S / 100) of the unit of its level; hoplites need one
// armor each, horsemen armor and a horse each (Z:136-137).
std::uint32_t house_soldiers(const DataCatalog& catalog, const HouseState& h, Difficulty d);

// Daily, while every company is at home: companies of up to 8 soldiers (horsemen, hoplites, rabble; houses in
// id order), at most 20, standards around the Palace; no Palace, no army (Z:135-136).
void form_companies(WorldState& state, const DataCatalog& catalog);
// Daily: a marching company whose soldiers all stand at the standard is Still; a company going home whose
// soldiers are all back is At Home.
void update_companies(WorldState& state);

// Commands. Return an error code for CommandRejected, or an empty string.
// Companies a partner city lends (Z:140, Z:174): `count` of the heavy infantry of the lending army
// (world.rival_attack_nation [ZAŁOŻENIE, L-125]), mustered at once from the land entry to standards by the
// Palace. Returns an error key or "".
std::string lend_companies(WorldState& state, const DataCatalog& catalog, std::int32_t city, std::uint32_t count,
                           std::vector<WalkerId>& spawned);
// The fighting is over: the lent companies go home (Z:140).
void release_borrowed(WorldState& state);

std::string muster_company(WorldState& state, const DataCatalog& catalog, std::uint32_t company,
                           std::vector<WalkerId>& spawned);
std::string move_company(WorldState& state, std::uint32_t company, TilePos to);
std::string send_company_home(WorldState& state, std::uint32_t company);

// Workers missing while rabble is in the field: 6 per soldier (S-004 rabble_workers_per_soldier, doc 22).
std::uint32_t workers_in_the_field(const WorldState& state, const DataCatalog& catalog);

// Stage 2 (combat.cpp; doc 22, 24; Z:137-144; assumptions L-110).
struct CombatEvents {
    std::vector<WalkerId> spawned;
    std::optional<std::uint8_t> ended;  // InvasionEnded::result
    std::int32_t city = -1;             // the city that sent the army that left
    std::vector<BuildingId> removed;    // walls and gates broken through
};

// Invaders of `nation` enter at the invasion point (map symbol 'i'; else the land entry): `soldiers` split by
// the nation's composition, rounded down (doc 22), at most 256. They march on the Palace. By sea (Z:144): see
// start_sea_invasion (naval.h).
std::string start_invasion(WorldState& state, const DataCatalog& catalog, const std::string& nation,
                           std::uint32_t soldiers, CombatEvents& ev, std::int32_t city = -1, bool by_sea = false);
// Bribe (doc 22): 100 dr a foot soldier, 200 a mounted one, x the difficulty percent; they walk away.
std::int64_t bribe_cost(const WorldState& state, const DataCatalog& catalog);

// The tile of soldier k of a company standing at `standard` [ZAŁOŻENIE, L-109].
TilePos company_slot(const CityState& city, TilePos standard, std::size_t k);
std::string bribe_invaders(WorldState& state, const DataCatalog& catalog, CombatEvents& ev);
// Surrender (Z:137): they walk away; the tribute belongs to the world module (D4).
std::string surrender(WorldState& state, CombatEvents& ev);
// Every tick during an invasion: blows and shots (hit = max(0, attack - armor), doc 24), chases, deaths,
// morale, the Palace taken.
void combat_tick(WorldState& state, const DataCatalog& catalog, CombatEvents& ev);
// Daily without an invasion: morale recovers (Z:140).
void recover_morale(WorldState& state);

inline constexpr std::int32_t kEngageTiles = 6;      // [ZAŁOŻENIE, L-110] soldiers turn on enemies this near
inline constexpr std::uint8_t kMoraleLossPerDeath = 10;  // doc 24: 5-20 [ZAŁOŻENIE, L-062]
inline constexpr std::uint8_t kFleeMorale = 20;          // doc 24: flees at 20 or less
inline constexpr std::uint8_t kMoraleRecoveryPerDay = 5;  // [ZAŁOŻENIE, L-062]

inline constexpr std::size_t kCompanySize = 8;
inline constexpr std::size_t kMaxCompanies = 20;

}  // namespace bo::core
