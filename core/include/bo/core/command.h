// Module A3: commands are the only way into the simulation (docs/PRAKTYKI.md ch. 2).
// Pattern: Command (GoF) as a closed std::variant; dispatch uses std::visit with one handler per alternative,
// so a missing handler is a compile error ("typ wymuszający komplet"). Player input, autopilot, scenario
// scripts and tests all issue the same commands; replay = seed + command log.
// Append new command types at the end of the variant only (the index is stored in saves and logs).
#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "bo/core/types.h"

namespace bo::core {

struct SetTaxRate {
    TaxRate rate = TaxRate::Normal;
};

struct SetWageRate {
    WageRate rate = WageRate::Normal;
};

struct PlaceBuilding {
    std::string type;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::uint8_t rotation = 0;  // see place_building in city.h
};

struct PlaceRoad {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct PlaceRoadblock {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct ClearLand {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct Demolish {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

// Reverts the build commands of the last tick that had any (Z:20), within kUndoWindowTicks.
struct Undo {};

// Labour panel (Z:62-63).
struct SetSectorPriority {
    std::uint8_t sector = 0;    // bo::core::Sector
    std::uint8_t priority = 3;  // 0 none .. 5 very high
};

struct SetIndustryEnabled {
    std::string type;  // building type
    bool enabled = true;
};

// Granary or Storehouse order for one good (Z:66-69). kind: 0 Accept, 1 Get, 2 Empty, 3 Don't Accept.
struct SetStorageOrder {
    std::uint32_t building = 0;
    std::string good;
    std::uint8_t kind = 0;
    std::uint32_t limit = 32;  // loads
};

// Trade (Z:76-79): the partner city of a Trading Post or Pier, and its import and export orders per good.
struct SetTradePartner {
    std::uint32_t building = 0;
    std::int32_t partner = -1;
};

struct SetTradeOrder {
    std::uint32_t building = 0;
    std::string good;
    bool import_on = false;
    std::uint32_t import_limit = 0;
    bool export_on = false;
    std::uint32_t export_keep = 0;
};

// Stockpiling (Z:80): keep a good for the player's own use. Not Buying (Z:72): an agora stall stops fetching.
struct SetStockpile {
    std::string good;
    bool on = true;
};

struct SetNotBuying {
    std::uint32_t building = 0;
    bool on = true;
};

// Company tactics (Z:138-139): 0 defensive, 1 offensive, 2 special.
struct SetCompanyTactic {
    std::uint32_t company = 0;
    std::uint8_t tactic = 0;
};

// Man All (Z:146): sentries to every tower, or back home.
struct ManTowers {
    bool on = true;
};

// Auto-Defend (Z:22, Z:145): the generals and admirals command on their own; on only before the fighting.
struct SetAutoDefend {
    bool on = true;
};

// Hold or resume the building of a monument (P:26-28): its carts and artisans stay away while held.
struct SetConstructionPaused {
    std::uint32_t building = 0;
    bool on = true;
};

// Army (Z:137-139): muster a company, move its standard, send its soldiers home. `company` indexes the
// companies in MilitaryState.
struct MusterCompany {
    std::uint32_t company = 0;
};

struct MoveCompany {
    std::uint32_t company = 0;
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct SendCompanyHome {
    std::uint32_t company = 0;
};

// An invasion (Z:137). Issued by the scenario or the world module (D4/D5); for now also by tests and tools.
struct StartInvasion {
    std::string nation;  // data/enemies.json
    std::uint32_t soldiers = 0;
    std::int32_t city = -1;  // the partner city that sends them (D4), -1: none
    bool by_sea = false;     // in transports with warships (Z:144; since v39)
};

// A request to a partner city (Z:174): defensive aid (lent companies while an invasion is on) or, with `strike`,
// an attack of that city on `target`.
struct RequestMilitaryHelp {
    std::uint32_t city = 0;
    bool strike = false;
    std::uint32_t target = 0;
};

// Consult Oracle (Z:124): at a complete sanctuary of a god with an oracle (Zeus, Apollo).
struct ConsultOracle {
    std::uint32_t building = 0;
};

// Trireme orders (Z:142): 0 Defend Location at (x, y) (-1: where it is), 1 Attack All Enemies, 2 Carry Out Repairs,
// 3 Patrol Area (between the post and the wharf), 4 Call Crew to Ship, 5 Send Crew Home (Z:141).
struct SetShipOrder {
    std::uint32_t ship = 0;
    std::uint8_t order = 0;
    std::int32_t x = -1;
    std::int32_t y = -1;
};

// The player's answer to an invasion (Z:137): buy them off or give in. Fighting needs no command.
struct BribeInvaders {};
struct Surrender {};

// A prayer at a finished sanctuary (Z:124).
struct Pray {
    std::uint32_t building = 0;
};

// World (D4, Z:173-178): `city` indexes the partner cities of the scenario; "drachmas" is money.
struct GiftToCity {
    std::uint32_t city = 0;
    std::string good;
    std::uint32_t amount = 0;  // loads, or drachmas
};

struct RequestFromCity {
    std::uint32_t city = 0;
    std::string good;
};

struct DispatchArmy {
    std::uint32_t city = 0;
    bool raid = false;
    std::string good;  // raid only
    std::vector<std::uint32_t> companies;
    bool ships = false;  // with every crewed trireme, to a city on a sea route (Z:123, Z:177)
};

// Sends what an open city request asks for (Z:162); `request` indexes ScenarioState::requests.
struct FulfillRequest {
    std::uint32_t request = 0;
};

// Calls the hero who slays the monster in the city to a Hero's Hall (Z:130).
struct SummonHero {
    std::uint32_t hall = 0;
};

// Sends the hero of an open quest to the enchanted place (Z:124); `quest` indexes ScenarioState::quests.
struct SendHeroOnQuest {
    std::uint32_t quest = 0;
};

using Command = std::variant<SetTaxRate, SetWageRate, PlaceBuilding, PlaceRoad, PlaceRoadblock, ClearLand, Demolish,
                             Undo, SetSectorPriority, SetIndustryEnabled, SetStorageOrder, SetTradePartner,
                             SetTradeOrder, MusterCompany, MoveCompany, SendCompanyHome,
                             StartInvasion, BribeInvaders, Surrender, Pray, GiftToCity, RequestFromCity, DispatchArmy,
                             FulfillRequest, SummonHero, SendHeroOnQuest, SetStockpile, SetNotBuying, ManTowers, SetCompanyTactic, SetShipOrder,
                             RequestMilitaryHelp, ConsultOracle, SetAutoDefend, SetConstructionPaused>;

// A command scheduled for execution at the start of `tick`, in issue order (`sequence`).
struct ScheduledCommand {
    std::uint64_t tick = 0;
    std::uint64_t sequence = 0;
    Command command;
};

using CommandLog = std::vector<ScheduledCommand>;

}  // namespace bo::core
