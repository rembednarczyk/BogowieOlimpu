// Module C6: taxes and wages. Rules: docs/gameplay/08-administracja-finanse.md, 18-domy-i-ekonomia-liczby.md:
// monthly tax = TRM(house level, difficulty) x residents x rate, only houses registered by a clerk (counter 50
// days, doc 24); yearly wage per worker by wage rate, paid monthly. Fractions are carried in thousandths.
#pragma once

#include <cstdint>

#include "bo/core/data.h"
#include "bo/core/state.h"

namespace bo::core {

struct MonthResult {
    std::int64_t taxes = 0;
    std::int64_t wages = 0;
};

// Collects taxes and pays wages for one month; updates the treasury and the yearly record.
MonthResult settle_month(WorldState& state, const DataCatalog& catalog);

// Moves this year's record to last year (year start).
void close_year(FinanceState& finance);

}  // namespace bo::core
