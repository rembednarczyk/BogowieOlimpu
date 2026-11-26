// Simulation state (everything that is saved and hashed). Presentation reads it, never writes it.
#pragma once

#include <cstdint>

#include "bo/core/city.h"
#include "bo/core/command.h"
#include "bo/core/labour.h"
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

struct WorldState {
    GameTime time;
    RngSet rng;
    Difficulty difficulty = Difficulty::Hero;
    FinanceState finance;
    CityState city;
    LabourState labour;
    MigrationState migration;
    WalkerStore walkers;
    std::uint64_t next_command_sequence = 0;
    CommandLog pending_commands;  // sorted by (tick, sequence)
};

}  // namespace bo::core
