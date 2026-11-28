#include "bo/core/finance.h"

#include "bo/core/housing.h"

namespace bo::core {

MonthResult settle_month(WorldState& st, const DataCatalog& catalog) {
    FinanceState& f = st.finance;
    const auto di = static_cast<std::size_t>(st.difficulty);
    const std::int64_t rate = catalog.tax_rate_per_mille[static_cast<std::size_t>(f.tax_rate)];
    std::int64_t tax_milli = f.tax_remainder;
    for (const auto& [id, b] : st.city.buildings.items) {
        if (!b.house || b.house->population == 0) {
            continue;
        }
        const auto clerk = b.coverage.find("clerk");
        if (clerk == b.coverage.end() || clerk->second == 0) {
            continue;  // not registered: pays nothing (doc 18)
        }
        const HouseModel& m = levels_of(catalog, *b.house)[b.house->level].model[di];
        tax_milli += static_cast<std::int64_t>(m.tax_multiplier) * b.house->population * rate;
    }
    MonthResult r;
    r.taxes = tax_milli / 1000;
    f.tax_remainder = tax_milli % 1000;
    const std::int64_t wage_milli = static_cast<std::int64_t>(st.labour.employed) *
                                        catalog.wage_per_mille[static_cast<std::size_t>(f.wage_rate)] +
                                    f.wage_remainder;
    r.wages = wage_milli / 12000;  // yearly thousandths -> one month of drachmas
    f.wage_remainder = wage_milli % 12000;
    f.treasury += r.taxes - r.wages;
    f.this_year.taxes += r.taxes;
    f.this_year.wages += r.wages;
    return r;
}

void close_year(FinanceState& finance) {
    finance.last_year = finance.this_year;
    finance.this_year = FinanceYear{};
}

}  // namespace bo::core
