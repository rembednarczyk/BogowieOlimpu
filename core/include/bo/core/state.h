// Simulation state (everything that is saved and hashed). Presentation reads it, never writes it.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "bo/core/city.h"
#include "bo/core/command.h"
#include "bo/core/labour.h"
#include "bo/core/military.h"
#include "bo/core/monsters.h"
#include "bo/core/scenario.h"
#include "bo/core/rng.h"
#include "bo/core/time.h"
#include "bo/core/types.h"
#include "bo/core/walker.h"

namespace bo::core {

struct FinanceYear {
    std::int64_t taxes = 0;
    std::int64_t wages = 0;
    std::int64_t construction = 0;
};

struct FinanceState {
    std::int64_t treasury = 0;  // drachmas
    TaxRate tax_rate = TaxRate::Normal;
    WageRate wage_rate = WageRate::Normal;
    std::int64_t tax_remainder = 0;   // thousandths of a drachma carried to the next month
    std::int64_t wage_remainder = 0;  // yearly thousandths carried (divide by 12000 for drachmas)
    FinanceYear this_year;
    FinanceYear last_year;
};

// Module C8 (doc 24): city mood and the migration pool.
struct MigrationState {
    std::uint8_t popularity = 0;  // mean happiness of inhabited houses (set to the base at start)
    std::int32_t pool = 0;        // people waiting to arrive (+) or leave (-), applied in batches of >= 4
    std::uint32_t arrived_this_month = 0;
    std::uint32_t left_this_month = 0;
};

// Module C9: partner cities as the scenario sets them (world map data arrives with module D).
struct TradeOffer {
    std::int32_t price = 0;            // drachmas per load
    std::uint32_t yearly_limit = 0;    // loads a year (Z:75)
    std::uint32_t traded = 0;          // loads traded this year
};

// Module D4: what a leader thinks of the player and owes or demands (Z:167-178; doc 24; L-064, L-113).
struct CityDiplomacy {
    std::int32_t opinion = 60;      // 0-100, scenario "favour" (doc 24)
    std::int32_t military = 1;      // shields 1-6 (Z:171, S-008)
    std::int32_t economy = 1;       // coins 1-6
    std::int64_t tribute = 0;       // what a vassal or colony pays a year (Z:172)
    std::int64_t tribute_demand = 0;  // what this rival demands a year after taking the player's city
    bool rebelling = false;
    std::uint8_t player_defeats = 0;  // times this rival took the player's city (two: the episode is lost)
    std::map<std::string, std::uint32_t> gift_units_this_year;
    std::uint16_t request_cooldown = 0;  // days until a new request is heard
    // Long-term relations (L-121, since v30): months in a row of a bad or a good opinion, months until debt aid.
    std::uint8_t months_bad = 0;
    std::uint8_t months_good = 0;
    std::uint8_t aid_cooldown = 0;
    friend bool operator==(const CityDiplomacy& a, const CityDiplomacy& b) {
        return a.opinion == b.opinion && a.military == b.military && a.economy == b.economy &&
               a.tribute == b.tribute && a.tribute_demand == b.tribute_demand && a.rebelling == b.rebelling &&
               a.player_defeats == b.player_defeats && a.gift_units_this_year == b.gift_units_this_year &&
               a.request_cooldown == b.request_cooldown && a.months_bad == b.months_bad &&
               a.months_good == b.months_good && a.aid_cooldown == b.aid_cooldown;
    }
};

struct TradePartner {
    std::string id;
    std::string relation = "ally";     // ally, colony, vassal, rival, distant (Z:167-170)
    bool by_sea = false;               // sea route (blue) or land route (red-brown) (Z:75)
    std::map<std::string, TradeOffer> sells;  // what we can import
    std::map<std::string, TradeOffer> buys;   // what we can export
    CityDiplomacy diplomacy;           // D4; saved in the world block (since v21)
};

// What a city sends at the player's request, on its way (eZeus: 90 days; owner's choice, L-113).
struct PendingDelivery {
    std::uint32_t city = 0;
    std::string good;          // "drachmas": money
    std::uint32_t amount = 0;  // loads, or drachmas
    std::uint32_t days_left = 0;
    friend bool operator==(const PendingDelivery& a, const PendingDelivery& b) {
        return a.city == b.city && a.good == b.good && a.amount == b.amount && a.days_left == b.days_left;
    }
};

// An army sent abroad (Z:177-178): out, battle, back (doc 24: 200 days each way, L-063).
struct Expedition {
    std::uint32_t city = 0;
    bool raid = false;
    std::string good;                   // raid: what to bring back
    std::vector<std::uint32_t> companies;
    std::uint32_t days_left = 0;
    bool returning = false;
    bool won = false;
    std::uint32_t losses = 0;           // soldiers lost in the battle
    std::uint32_t loot = 0;             // loads (or drachmas for "drachmas")
    friend bool operator==(const Expedition& a, const Expedition& b) {
        return a.city == b.city && a.raid == b.raid && a.good == b.good && a.companies == b.companies &&
               a.days_left == b.days_left && a.returning == b.returning && a.won == b.won && a.losses == b.losses &&
               a.loot == b.loot;
    }
};

struct TradeWorld {
    std::vector<TradePartner> partners;
    std::uint64_t last_trader_day = ~std::uint64_t{0};  // at most one new trader a day (doc 24)
    std::uint32_t traders_today = 0;
};

struct WorldState {
    GameTime time;
    RngSet rng;
    Difficulty difficulty = Difficulty::Hero;
    FinanceState finance;
    CityState city;
    LabourState labour;
    MigrationState migration;
    TradeWorld trade;
    WalkerStore walkers;
    MilitaryState military;  // since v17 (module D3)
    std::vector<Expedition> expeditions;  // since v21 (module D4)
    std::vector<PendingDelivery> deliveries;  // since v27
    bool episode_lost = false;            // since v21: taken twice by the same rival (Z:172), or debt (L-034)
    std::uint32_t debt_days = 0;          // since v29: days of continuous debt (L-034)
    ScenarioState scenario;               // since v22 (module D5)
    std::vector<MonsterState> monsters;   // since v24
    std::vector<HeroState> heroes;        // since v24
    std::uint64_t next_command_sequence = 0;
    CommandLog pending_commands;  // sorted by (tick, sequence)
};

}  // namespace bo::core
