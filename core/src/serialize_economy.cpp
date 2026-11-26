// Labour, migration and finance blocks of the save (v5, v6).
#include "serialize_city.h"

namespace bo::core {

// Labour and finance details (v5). Written after the city block, before the walkers.
void write_economy(ByteWriter& w, const WorldState& s) {
    const LabourState& l = s.labour;
    for (const std::uint8_t p : l.priority) {
        w.u8(p);
    }
    w.u32(static_cast<std::uint32_t>(l.disabled_types.size()));
    for (const auto& t : l.disabled_types) {
        w.str(t);
    }
    w.u8(l.full_staffing ? 1 : 0);
    w.u32(l.workforce);
    w.u32(l.employed);
    for (std::size_t i = 0; i < kSectors; ++i) {
        w.u32(l.sector_need[i]);
        w.u32(l.sector_employed[i]);
    }
    const MigrationState& m = s.migration;  // since v6
    w.u8(m.popularity);
    w.i32(m.pool);
    w.u32(m.arrived_this_month);
    w.u32(m.left_this_month);
    const FinanceState& f = s.finance;
    w.i64(f.tax_remainder);
    w.i64(f.wage_remainder);
    for (const FinanceYear* y : {&f.this_year, &f.last_year}) {
        w.i64(y->taxes);
        w.i64(y->wages);
        w.i64(y->construction);
    }
}

bool read_economy(ByteReader& r, WorldState& s, std::uint32_t version) {
    LabourState& l = s.labour;
    for (auto& p : l.priority) {
        if (!r.u8(p) || p >= kPriorityLevels) {
            return false;
        }
    }
    std::uint32_t n = 0;
    if (!r.u32(n) || n > 4096) {
        return false;
    }
    for (std::uint32_t i = 0; i < n; ++i) {
        std::string t;
        if (!r.str(t)) {
            return false;
        }
        l.disabled_types.insert(t);
    }
    std::uint8_t full = 0;
    if (!r.u8(full) || !r.u32(l.workforce) || !r.u32(l.employed)) {
        return false;
    }
    l.full_staffing = full != 0;
    for (std::size_t i = 0; i < kSectors; ++i) {
        if (!r.u32(l.sector_need[i]) || !r.u32(l.sector_employed[i])) {
            return false;
        }
    }
    MigrationState& m = s.migration;
    if (version >= 6 &&
        (!r.u8(m.popularity) || !r.i32(m.pool) || !r.u32(m.arrived_this_month) || !r.u32(m.left_this_month))) {
        return false;
    }
    FinanceState& f = s.finance;
    if (!r.i64(f.tax_remainder) || !r.i64(f.wage_remainder)) {
        return false;
    }
    for (FinanceYear* y : {&f.this_year, &f.last_year}) {
        if (!r.i64(y->taxes) || !r.i64(y->wages) || !r.i64(y->construction)) {
            return false;
        }
    }
    return true;
}

}  // namespace bo::core
