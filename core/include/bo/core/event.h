// Module A3: events are the only way out of the simulation (docs/PRAKTYKI.md ch. 2).
// Pattern: Event Queue (R. Nystrom, "Game Programming Patterns"). Presentation (render, audio, HUD, haptics)
// drains the queue after each step; events never feed back into the simulation.
#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "bo/core/types.h"

namespace bo::core {

struct DayStarted {
    std::uint64_t day = 0;
};

struct MonthStarted {
    std::uint32_t month = 0;  // 0 = January
    std::uint64_t year = 0;
};

struct YearStarted {
    std::uint64_t year = 0;
};

struct TaxRateChanged {
    TaxRate rate = TaxRate::Normal;
};

struct WageRateChanged {
    WageRate rate = WageRate::Normal;
};

struct CommandRejected {
    std::uint32_t command_index = 0;
    std::string reason_key;  // i18n key, never player-facing text (docs/PRAKTYKI.md ch. 12)
};

struct BuildingPlaced {
    std::uint32_t id = 0;
    std::string type;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 1;
    std::int32_t height = 1;
    std::uint8_t rotation = 0;
    std::uint32_t parent = 0;  // agora of a stall
};

struct BuildingRemoved {
    std::uint32_t id = 0;
};

struct TileChanged {  // road, roadblock or terrain changed on one tile
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct WalkerSpawned {
    std::uint32_t id = 0;
    std::string type;
};

struct WalkerRemoved {
    std::uint32_t id = 0;
};

struct UndoApplied {
    std::int64_t refund = 0;
};

struct HouseChanged {  // level change or abandonment (C2)
    std::uint32_t id = 0;
    std::uint8_t level = 0;
    bool elite = false;
    bool abandoned = false;
};

struct MonthlyFinance {  // C6: taxes collected and wages paid at the start of a month
    std::int64_t taxes = 0;
    std::int64_t wages = 0;
};

struct PeopleMoved {  // C8: daily migration result
    std::uint32_t arrived = 0;
    std::uint32_t left = 0;
};

// C7: fires, collapses, plagues and crimes, for messages and the hazards view. Values stored in replays: append.
enum class DisasterKind : std::uint8_t { FireStarted = 0, BurnedDown, Collapsed, Plague, Crime };

struct Disaster {
    DisasterKind kind = DisasterKind::FireStarted;
    std::uint32_t building = 0;
};

struct TradeDone {  // C9: a trader finished at a trade building
    std::uint32_t building = 0;
    std::uint32_t loads_bought = 0;  // imported
    std::uint32_t loads_sold = 0;    // exported
    std::int64_t money = 0;          // + earned, - paid
};

struct InvasionStarted {  // D3: an army enters the city (Z:137)
    std::string nation;
    std::uint32_t soldiers = 0;
    std::int64_t bribe = 0;  // drachmas to buy them off (doc 22)
};

// Stored in no save; values for presentation. 0: invaders defeated, 1: bribed, 2: surrendered, 3: Palace taken.
struct InvasionEnded {
    std::uint8_t result = 0;
};

struct SanctuaryCompleted {  // D1: the artisans have built in the last load (Z:118)
    std::uint32_t building = 0;
};

struct PrayerAnswered {  // D1: the god granted the prayer (Z:124)
    std::uint32_t building = 0;
};

struct TributePaid {  // D4: yearly tribute; amount > 0 received from a vassal or colony, < 0 paid to a rival
    std::uint32_t city = 0;
    std::int64_t amount = 0;
};

struct ExpeditionReturned {  // D4: an army back from abroad
    std::uint32_t city = 0;
    bool won = false;
    std::uint32_t losses = 0;
    std::uint32_t loot = 0;
};

struct EpisodeLost {};  // D4: taken twice by the same rival (Z:172), or years of debt (L-034)

struct CityRebelled {  // L-121: a vassal or colony rises after a year of bad opinion (Z:169, Z:178)
    std::uint32_t city = 0;
};

struct CityGift {  // L-121: a friendly city's goodwill gift or debt aid, in drachmas (Z:100, Z:178-179)
    std::uint32_t city = 0;
    std::int64_t amount = 0;
};

struct GamesHeld {  // L-123: the year's Panhellenic games (Z:110-112)
    std::string game;
    std::int32_t chance = 0;
    bool invited = false;
    bool won = false;
    bool hosted = false;
    bool right_lost = false;
};

struct DebtNotice {  // L-034: another year of continuous debt (eZeus debtAnniversary); immigration stops
    std::uint32_t years = 0;
};

struct CityRequestReceived {  // D5: a leader asks for something by a deadline (Z:161)
    std::uint32_t request = 0;    // index into ScenarioState::requests at the time
    std::uint32_t city = 0;
    std::string good;
    std::uint32_t amount = 0;
};

struct RequestClosed {  // D5: fulfilled, or withdrawn after the deadline
    std::uint32_t city = 0;
    bool fulfilled = false;
};

struct MilitaryHelpGiven {  // a partner city lent companies or struck another city (Z:174)
    std::uint32_t city = 0;
    bool strike = false;
};

// The oracle's prophecy (Z:124) [L-126]: the next scheduled threat of the scenario, `months` from now; an empty
// `kind` when none is foreseen. `subject`: the nation, god, monster or disaster kind.
struct OracleSpoke {
    std::uint32_t building = 0;
    std::string kind;  // invasion, god_attack, monster, disaster
    std::string subject;
    std::uint32_t months = 0;
};

struct EpisodeWon {};  // D5: all goals met

struct MonsterAppeared {  // D5: a monster comes to the city (Z:127)
    std::string monster;
};

struct MonsterSlain {
    std::string monster;
    bool by_hero = false;
};

struct HeroArrived {  // D5: a hero moves into his Hall (Z:130)
    std::string hero;
};

struct QuestGiven {  // D5: a god asks for a quest (Z:124)
    std::string god;
};

struct QuestCompleted {  // D5: the hero is back; the god grants a statue (Z:156)
    std::string god;
};

struct GodAttacked {  // D5: a hostile god came (Z:125); repelled by a god with a sanctuary (Z:119, Z:122)
    std::string god;
    bool repelled = false;
};

using Event = std::variant<DayStarted, MonthStarted, YearStarted, TaxRateChanged, WageRateChanged, CommandRejected,
                           BuildingPlaced, BuildingRemoved, TileChanged, WalkerSpawned, WalkerRemoved, UndoApplied,
                           HouseChanged, MonthlyFinance, PeopleMoved, Disaster, TradeDone, InvasionStarted,
                           InvasionEnded, SanctuaryCompleted, PrayerAnswered,
                           TributePaid, ExpeditionReturned, EpisodeLost, DebtNotice, GamesHeld, CityRebelled, CityGift,
                           CityRequestReceived, RequestClosed, EpisodeWon, MilitaryHelpGiven, OracleSpoke,
                           GodAttacked, MonsterAppeared, MonsterSlain, HeroArrived, QuestGiven, QuestCompleted>;

struct TimedEvent {
    std::uint64_t tick = 0;
    Event event;
};

using EventQueue = std::vector<TimedEvent>;

}  // namespace bo::core
