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
    w.u32(static_cast<std::uint32_t>(s.trade.partners.size()));  // since v14
    for (const auto& p : s.trade.partners) {
        w.str(p.id);
        w.str(p.relation);
        w.u8(p.by_sea ? 1 : 0);
        for (const auto* offers : {&p.sells, &p.buys}) {
            w.u32(static_cast<std::uint32_t>(offers->size()));
            for (const auto& [good, o] : *offers) {
                w.str(good);
                w.i32(o.price);
                w.u32(o.yearly_limit);
                w.u32(o.traded);
            }
        }
    }
    w.u64(s.trade.last_trader_day);
    w.u32(s.trade.traders_today);
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
    if (version >= 14) {
        std::uint32_t partners = 0;
        if (!r.u32(partners) || partners > 256) {
            return false;
        }
        s.trade.partners.resize(partners);
        for (auto& p : s.trade.partners) {
            std::uint8_t sea = 0;
            if (!r.str(p.id) || !r.str(p.relation) || !r.u8(sea)) {
                return false;
            }
            p.by_sea = sea != 0;
            for (auto* offers : {&p.sells, &p.buys}) {
                std::uint32_t n = 0;
                if (!r.u32(n) || n > 4096) {
                    return false;
                }
                for (std::uint32_t i = 0; i < n; ++i) {
                    std::string good;
                    TradeOffer o;
                    if (!r.str(good) || !r.i32(o.price) || !r.u32(o.yearly_limit) || !r.u32(o.traded)) {
                        return false;
                    }
                    (*offers)[good] = o;
                }
            }
        }
        if (!r.u64(s.trade.last_trader_day) || !r.u32(s.trade.traders_today)) {
            return false;
        }
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
