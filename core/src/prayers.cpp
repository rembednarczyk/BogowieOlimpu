// Prayers (Z:119-124, Z:132; doc 10, 21; P:20-21): what each god grants, and how likely the god is to listen
// [ZAŁOŻENIE, L-053, L-112].
#include <algorithm>

#include "bo/core/gods.h"
#include "bo/core/housing.h"
#include "bo/core/sanctuary.h"
#include "bo/core/scenario.h"
#include "bo/core/state.h"
#include "bo/core/storage.h"

namespace bo::core {

namespace {

// Effects that act at once on the city; returns whether the god could give anything.
bool grant_now(WorldState& st, const DataCatalog& catalog, const std::string& effect) {
    const auto loads = static_cast<std::uint32_t>(catalog.prayer_goods_loads);
    if (effect == "food_to_granaries" || effect == "pork" || effect == "seafood_when_low_food" ||
        effect == "wine_by_satyrs") {
        const char* good = effect == "food_to_granaries" ? "wheat"
                           : effect == "pork"            ? "pork"
                           : effect == "wine_by_satyrs"  ? "wine"
                                                         : "fish";
        return store_in_city(st.city, catalog, good, loads) > 0;
    }
    if (effect == "olives_and_olive_oil") {
        return store_in_city(st.city, catalog, "olives", loads / 2) + store_in_city(st.city, catalog, "olive_oil", loads / 2) > 0;
    }
    if (effect == "money") {
        st.finance.treasury += catalog.prayer_money;
        return true;
    }
    if (effect == "cures_plague_improves_health") {
        for (auto& [bid, b] : st.city.buildings.items) {
            b.disease_counter = 0;
            if (b.house) {
                b.house->quarantine_days = 0;
            }
        }
        return true;
    }
    if (effect == "more_residents_all_houses") {  // Aphrodite (Z:122): every inhabited house filled up
        bool any = false;
        for (auto& [bid, b] : st.city.buildings.items) {
            if (b.house && b.house->population > 0 && !b.house->abandoned) {
                const std::uint32_t cap = house_capacity(catalog, *b.house, st.difficulty);
                any = any || b.house->population < cap;
                b.house->population = std::max(b.house->population, cap);
            }
        }
        return any;
    }
    if (effect == "fulfills_one_outstanding_request_free") {  // Hermes (Z:122)
        return !st.scenario.requests.empty() && fulfill_request(st, catalog, 0, true).empty();
    }
    if (effect == "restock_agoras_staffed_stalls") {  // Hera (P:20)
        bool any = false;
        for (auto& [bid, b] : st.city.buildings.items) {
            if (b.stall && b.staffing_percent > 0) {
                for (const std::string& g : catalog.buildings.at(b.type).sells) {
                    auto& units = b.stall->units[g == "food" ? "wheat" : g];
                    any = any || units < catalog.hera_stall_units;
                    units = std::max(units, catalog.hera_stall_units);
                }
            }
        }
        return any;
    }
    if (effect == "speeds_construction_materials_arrive_instantly") {  // Atlas (P:21): the materials are on site
        bool any = false;
        for (auto& [bid, b] : st.city.buildings.items) {
            if (b.sanctuary && !b.sanctuary->complete) {
                for (auto& [good, n] : b.sanctuary->needed) {
                    const std::uint32_t missing = n > b.sanctuary->on_site[good] ? n - b.sanctuary->on_site[good] : 0;
                    any = any || missing > 0;
                    b.sanctuary->on_site[good] += missing;
                }
            }
        }
        return any;
    }
    return false;
}

}  // namespace

std::int32_t prayer_chance(const SanctuaryState& s, const DataCatalog& catalog) {
    const std::int32_t days = s.days_since_sacrifice;
    const std::int32_t full = catalog.prayer_chance_full_days;
    const std::int32_t lost = kFavorLostAfterDays;
    if (s.favor_lost) {
        return 0;
    }
    if (days <= full) {
        return 100;
    }
    const std::int32_t lo = catalog.prayer_chance_min;
    return std::max(lo, 100 - (100 - lo) * (days - full) / std::max(1, lost - full));
}

std::string pray(WorldState& st, const DataCatalog& catalog, BuildingId id) {
    const auto it = st.city.buildings.items.find(id);
    if (it == st.city.buildings.items.end() || !it->second.sanctuary || !it->second.sanctuary->complete ||
        catalog.sanctuaries.at(it->second.type).monument) {  // a monument has no god to pray to (L-128)
        return "myth.no_sanctuary";
    }
    SanctuaryState& s = *it->second.sanctuary;
    if (s.favor_lost) {
        return "myth.god_turned_away";
    }
    if (s.prayer_cooldown > 0) {
        return "myth.prayer_too_soon";  // the god ignores requests that come too often (Z:124)
    }
    const SanctuaryParams& p = catalog.sanctuaries.at(it->second.type);
    const std::string& effect = p.prayer;
    const bool later = effect == "defeats_next_invading_army" || effect == "accompanies_army_abroad" ||
                       effect == "talos_defends_few_months";
    const auto roll = static_cast<std::int32_t>(st.rng.stream(RngStream::Gods).next_u32() % 100U);
    if (roll >= prayer_chance(s, catalog)) {
        s.prayer_cooldown = static_cast<std::uint16_t>(p.prayer_cooldown_days / 2);
        return "myth.prayer_denied";  // the god did not listen this time (Z:132)
    }
    if (effect == "defeats_next_invading_army") {
        st.zeus_guard = true;  // Z:119
    } else if (effect == "accompanies_army_abroad") {
        st.ares_with_army = true;  // Z:121
    } else if (effect == "talos_defends_few_months") {
        st.talos_days = catalog.talos_days;  // Z:120
    }
    if (!later && !grant_now(st, catalog, effect)) {
        return "myth.no_room_for_gift";  // nothing to give now: no room, no request, no site; no cooldown
    }
    s.prayer_cooldown = static_cast<std::uint16_t>(p.prayer_cooldown_days);
    return {};
}

}  // namespace bo::core
