// Auto-Defend (Z:22, Z:145). Z:145 says only that the generals and admirals command on their own and that the
// switch goes on before the fighting starts; what they do is assumed (L-129): muster everyone, march on the
// nearest enemy, crew every trireme and attack all, go home when the fighting is over.
#include "bo/core/auto_defend.h"

#include <cstdlib>
#include <optional>

#include "bo/core/naval.h"

namespace bo::core {

namespace {

std::int32_t chebyshev(TilePos a, TilePos b) { return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y)); }

// The nearest invader on the land to `from` (ties: the lower walker id, the invaders' order).
std::optional<TilePos> nearest_invader(const WorldState& st, TilePos from) {
    std::optional<std::pair<std::int32_t, TilePos>> best;
    for (const WalkerId id : st.military.invasion.invaders) {
        const auto it = st.walkers.items.find(id);
        if (it == st.walkers.items.end() || is_ship(it->second) || it->second.hp <= 0) {
            continue;
        }
        const std::int32_t d = chebyshev(from, it->second.tile);
        if (!best || d < best->first) {
            best = std::make_pair(d, it->second.tile);
        }
    }
    return best ? std::optional<TilePos>(best->second) : std::nullopt;
}

bool own(const Company& c) { return c.lender < 0; }  // lent companies obey their own generals (Z:140)

}  // namespace

void advance_auto_defend(WorldState& st, const DataCatalog& catalog, CombatEvents& ev) {
    if (!st.auto_defend || !st.military.invasion.active) {
        return;
    }
    for (std::uint32_t i = 0; i < st.military.ships.size(); ++i) {  // the admirals (Z:145)
        const Ship& sh = st.military.ships[i];
        if (sh.crew == Crew::None) {
            set_ship_order(st, catalog, i, 4, TilePos{-1, -1});
        }
        if (st.military.ships[i].crew == Crew::Aboard && sh.order == ShipOrder::Defend) {  // a patrol fights on its way
            set_ship_order(st, catalog, i, 1, TilePos{-1, -1});
        }
    }
    const auto& companies = st.military.companies;
    if (!nearest_invader(st, TilePos{0, 0})) {
        return;  // only ships so far
    }
    for (std::uint32_t i = 0; i < companies.size(); ++i) {  // the generals (Z:145)
        const Company& c = companies[i];
        if (!own(c) || c.status != CompanyStatus::AtHome || c.fled || c.members.empty()) {
            continue;
        }
        muster_company(st, catalog, i, ev.spawned);  // a company that cannot muster (no Palace) stays at home
    }
    for (std::uint32_t i = 0; i < companies.size(); ++i) {
        const Company& c = companies[i];
        if (!own(c) || (c.status != CompanyStatus::Still && c.status != CompanyStatus::Marching)) {
            continue;
        }
        if (const auto foe = nearest_invader(st, c.standard); foe && chebyshev(*foe, c.standard) > 1) {
            move_company(st, i, *foe);
        }
    }
}

void auto_defend_stand_down(WorldState& st) {
    if (!st.auto_defend) {
        return;
    }
    for (std::uint32_t i = 0; i < st.military.companies.size(); ++i) {
        const Company& c = st.military.companies[i];
        if (own(c) && (c.status == CompanyStatus::Still || c.status == CompanyStatus::Marching)) {
            send_company_home(st, i);
        }
    }
}

std::string set_auto_defend(WorldState& st, bool on) {
    if (on && !st.auto_defend && st.military.invasion.active) {
        return "military.battle_started";  // Z:145: before the fighting starts
    }
    st.auto_defend = on;
    return {};
}

}  // namespace bo::core
