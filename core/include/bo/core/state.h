// Simulation state (everything that is saved and hashed). Presentation reads it, never writes it.
#pragma once

#include <cstdint>

#include "bo/core/command.h"
#include "bo/core/rng.h"
#include "bo/core/time.h"
#include "bo/core/types.h"

namespace bo::core {

struct FinanceState {
    std::int64_t treasury = 0;  // drachmas
    TaxRate tax_rate = TaxRate::Normal;
    WageRate wage_rate = WageRate::Normal;
};

struct WorldState {
    GameTime time;
    RngSet rng;
    Difficulty difficulty = Difficulty::Hero;
    FinanceState finance;
    std::uint64_t next_command_sequence = 0;
    CommandLog pending_commands;  // sorted by (tick, sequence)
};

}  // namespace bo::core
