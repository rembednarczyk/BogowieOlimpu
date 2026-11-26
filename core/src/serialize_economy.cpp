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

// Army (D3): companies with members, standard, status, morale and soldiers in the field.
void write_military(ByteWriter& w, const MilitaryState& m) {
    w.u32(static_cast<std::uint32_t>(m.companies.size()));
    for (const Company& c : m.companies) {
        w.str(c.unit);
        w.u32(static_cast<std::uint32_t>(c.members.size()));
        for (const CompanyMember& mb : c.members) {
            w.u32(mb.house);
            w.u8(mb.soldiers);
        }
        w.i32(c.standard.x);
        w.i32(c.standard.y);
        w.u8(static_cast<std::uint8_t>(c.status));
        w.u8(c.morale);
        w.u32(static_cast<std::uint32_t>(c.soldiers.size()));
        for (const WalkerId id : c.soldiers) {
            w.u32(id);
        }
        w.u8(c.fled ? 1 : 0);  // since v18
        w.str(c.nation);        // since v34
    }
    const Invasion& inv = m.invasion;  // since v18
    w.u8(inv.active ? 1 : 0);
    w.str(inv.nation);
    w.u32(static_cast<std::uint32_t>(inv.invaders.size()));
    for (const WalkerId id : inv.invaders) {
        w.u32(id);
    }
    w.i32(inv.origin.x);
    w.i32(inv.origin.y);
    w.i32(inv.city);  // since v21
}

bool read_military(ByteReader& r, MilitaryState& m, std::uint32_t version) {
    std::uint32_t count = 0;
    if (!r.u32(count) || count > kMaxCompanies) {
        return false;
    }
    m.companies.resize(count);
    for (Company& c : m.companies) {
        std::uint32_t members = 0;
        std::uint8_t status = 0;
        std::uint32_t soldiers = 0;
        if (!r.str(c.unit) || !r.u32(members) || members > 64) {
            return false;
        }
        c.members.resize(members);
        for (CompanyMember& mb : c.members) {
            if (!r.u32(mb.house) || !r.u8(mb.soldiers)) {
                return false;
            }
        }
        if (!r.i32(c.standard.x) || !r.i32(c.standard.y) || !r.u8(status) || status > 6 || !r.u8(c.morale) ||
            !r.u32(soldiers) || soldiers > 64) {
            return false;
        }
        c.status = static_cast<CompanyStatus>(status);
        c.soldiers.resize(soldiers);
        for (WalkerId& id : c.soldiers) {
            if (!r.u32(id)) {
                return false;
            }
        }
        std::uint8_t fled = 0;
        if (version >= 18 && !r.u8(fled)) {
            return false;
        }
        c.fled = fled != 0;
        if (version >= 34 && !r.str(c.nation)) {
            return false;
        }
    }
    if (version < 18) {
        return true;
    }
    Invasion& inv = m.invasion;
    std::uint8_t active = 0;
    std::uint32_t invaders = 0;
    if (!r.u8(active) || !r.str(inv.nation) || !r.u32(invaders) || invaders > 4096) {
        return false;
    }
    inv.active = active != 0;
    inv.invaders.resize(invaders);
    for (WalkerId& id : inv.invaders) {
        if (!r.u32(id)) {
            return false;
        }
    }
    return r.i32(inv.origin.x) && r.i32(inv.origin.y) && (version < 21 || r.i32(inv.city));
}

// World (D4): diplomacy per partner, expeditions abroad, the lost episode (since v21).
void write_world(ByteWriter& w, const WorldState& s) {
    w.u32(static_cast<std::uint32_t>(s.trade.partners.size()));
    for (const auto& p : s.trade.partners) {
        const CityDiplomacy& d = p.diplomacy;
        w.str(p.relation);
        w.i32(d.opinion);
        w.i32(d.military);
        w.i32(d.economy);
        w.i64(d.tribute);
        w.i64(d.tribute_demand);
        w.u8(d.rebelling ? 1 : 0);
        w.u8(d.player_defeats);
        write_counts(w, d.gift_units_this_year);
        w.u32(d.request_cooldown);
    }
    w.u32(static_cast<std::uint32_t>(s.expeditions.size()));
    for (const Expedition& e : s.expeditions) {
        w.u32(e.city);
        w.u8(static_cast<std::uint8_t>((e.raid ? 1U : 0U) | (e.returning ? 2U : 0U) | (e.won ? 4U : 0U) |
                                       (e.aid ? 8U : 0U)));
        w.str(e.good);
        w.u32(static_cast<std::uint32_t>(e.companies.size()));
        for (const std::uint32_t c : e.companies) {
            w.u32(c);
        }
        w.u32(e.days_left);
        w.u32(e.losses);
        w.u32(e.loot);
    }
    w.u8(s.episode_lost ? 1 : 0);
    w.u32(static_cast<std::uint32_t>(s.deliveries.size()));  // since v27
    for (const PendingDelivery& d : s.deliveries) {
        w.u32(d.city);
        w.str(d.good);
        w.u32(d.amount);
        w.u32(d.days_left);
    }
    w.u32(s.debt_days);  // since v29
    for (const auto& p : s.trade.partners) {  // since v30: long-term relations (L-121)
        w.u8(p.diplomacy.months_bad);
        w.u8(p.diplomacy.months_good);
        w.u8(p.diplomacy.aid_cooldown);
    }
    w.u8(static_cast<std::uint8_t>((s.zeus_guard ? 1U : 0U) | (s.ares_with_army ? 2U : 0U)));  // since v31
    w.u32(s.talos_days);
    w.u32(s.caravan_curse_days);  // since v32
    w.u32(s.ship_curse_days);
    w.u32(static_cast<std::uint32_t>(s.stockpiled.size()));  // since v35
    for (const std::string& g : s.stockpiled) {
        w.str(g);
    }
    w.u8(s.towers_manned ? 1 : 0);  // since v36
}

bool read_world(ByteReader& r, WorldState& s, std::uint32_t version) {
    std::uint32_t n = 0;
    if (!r.u32(n) || n != s.trade.partners.size()) {
        return false;
    }
    for (auto& p : s.trade.partners) {
        CityDiplomacy& d = p.diplomacy;
        std::uint8_t rebel = 0;
        std::uint32_t cooldown = 0;
        if (!r.str(p.relation) || !r.i32(d.opinion) || !r.i32(d.military) || !r.i32(d.economy) || !r.i64(d.tribute) ||
            !r.i64(d.tribute_demand) || !r.u8(rebel) || !r.u8(d.player_defeats) ||
            !read_counts(r, d.gift_units_this_year) || !r.u32(cooldown) || cooldown > 0xFFFF) {
            return false;
        }
        d.rebelling = rebel != 0;
        d.request_cooldown = static_cast<std::uint16_t>(cooldown);
    }
    std::uint32_t count = 0;
    if (!r.u32(count) || count > 64) {
        return false;
    }
    s.expeditions.resize(count);
    for (Expedition& e : s.expeditions) {
        std::uint8_t flags = 0;
        std::uint32_t companies = 0;
        if (!r.u32(e.city) || !r.u8(flags) || !r.str(e.good) || !r.u32(companies) || companies > kMaxCompanies) {
            return false;
        }
        e.raid = (flags & 1U) != 0;
        e.returning = (flags & 2U) != 0;
        e.won = (flags & 4U) != 0;
        e.aid = (flags & 8U) != 0;  // bit 3: an aid expedition (Z:161); older saves have none
        e.companies.resize(companies);
        for (std::uint32_t& c : e.companies) {
            if (!r.u32(c)) {
                return false;
            }
        }
        if (!r.u32(e.days_left) || !r.u32(e.losses) || !r.u32(e.loot)) {
            return false;
        }
    }
    std::uint8_t lost = 0;
    if (!r.u8(lost)) {
        return false;
    }
    s.episode_lost = lost != 0;
    if (version < 27) {
        return true;
    }
    std::uint32_t pending = 0;
    if (!r.u32(pending) || pending > 256) {
        return false;
    }
    s.deliveries.resize(pending);
    for (PendingDelivery& d : s.deliveries) {
        if (!r.u32(d.city) || !r.str(d.good) || !r.u32(d.amount) || !r.u32(d.days_left)) {
            return false;
        }
    }
    if (version < 29) {
        return true;
    }
    if (!r.u32(s.debt_days)) {
        return false;
    }
    for (auto& p : s.trade.partners) {
        if (version >= 30 &&
            (!r.u8(p.diplomacy.months_bad) || !r.u8(p.diplomacy.months_good) || !r.u8(p.diplomacy.aid_cooldown))) {
            return false;
        }
    }
    std::uint8_t help = 0;
    std::uint32_t talos = 0;
    if (version >= 31 && (!r.u8(help) || !r.u32(talos) || talos > 0xFFFF)) {
        return false;
    }
    s.zeus_guard = (help & 1U) != 0;
    s.ares_with_army = (help & 2U) != 0;
    s.talos_days = static_cast<std::uint16_t>(talos);
    std::uint32_t caravans = 0;
    std::uint32_t ships = 0;
    if (version >= 32 && (!r.u32(caravans) || !r.u32(ships) || caravans > 0xFFFF || ships > 0xFFFF)) {
        return false;
    }
    s.caravan_curse_days = static_cast<std::uint16_t>(caravans);
    s.ship_curse_days = static_cast<std::uint16_t>(ships);
    std::uint32_t kept = 0;
    if (version >= 35 && (!r.u32(kept) || kept > 64)) {
        return false;
    }
    for (std::uint32_t i = 0; i < kept; ++i) {
        std::string g;
        if (!r.str(g)) {
            return false;
        }
        s.stockpiled.insert(g);
    }
    std::uint8_t manned = 0;
    if (version >= 36 && !r.u8(manned)) {
        return false;
    }
    s.towers_manned = manned != 0;
    return true;
}

}  // namespace bo::core
