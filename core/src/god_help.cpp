// The gods' help without a prayer (Z:119-122; docs/gameplay/10-mitologia.md "Pomoc samorzutna"). The manual says
// only that many gods give their prayer's blessing on their own when they see the problem; what they see and how
// often is assumed (L-130, data/gods_rules.json spontaneous_help).
#include <algorithm>

#include "bo/core/gods.h"
#include "bo/core/housing.h"
#include "bo/core/scenario.h"
#include "bo/core/state.h"
#include "bo/core/storage.h"
#include "combat_internal.h"

namespace bo::core {

namespace {

std::int32_t loads(const WorldState& st, const DataCatalog& catalog, const char* good) {
    return static_cast<std::int32_t>(city_stock(st.city, catalog, good));
}

bool mostly_vacant(const WorldState& st, const DataCatalog& catalog) {
    std::uint64_t people = 0;
    std::uint64_t room = 0;
    for (const auto& [id, b] : st.city.buildings.items) {
        if (b.house && b.house->population > 0 && !b.house->abandoned) {
            people += b.house->population;
            room += house_capacity(catalog, *b.house, st.difficulty);
        }
    }
    return room > 0 && people * 100 < room * static_cast<std::uint64_t>(catalog.spontaneous.vacant_percent);
}

// The problem a god looks out for (gods.json spontaneous_help).
bool sees_problem(const WorldState& st, const DataCatalog& catalog, const std::string& when) {
    const SpontaneousParams& p = catalog.spontaneous;
    if (when == "when_city_in_great_danger") {  // Zeus: an army stronger than the player's (Z:119)
        return st.military.invasion.active && !st.zeus_guard && combat_detail::invaders_stronger(st, catalog);
    }
    if (when == "when_low_food") {
        return loads(st, catalog, "food") < p.low_food_loads;
    }
    if (when == "when_very_low_food") {
        return loads(st, catalog, "food") < p.very_low_food_loads;
    }
    if (when == "money_or_ally_donation_when_low_money") {
        return st.finance.treasury < p.low_money;
    }
    if (when == "when_low_olives_or_oil") {
        return loads(st, catalog, "olives") + loads(st, catalog, "olive_oil") < p.low_goods_loads;
    }
    if (when == "when_low_wine") {
        return loads(st, catalog, "wine") < p.low_goods_loads;
    }
    if (when == "when_plague") {
        return std::any_of(st.city.buildings.items.begin(), st.city.buildings.items.end(),
                           [](const auto& kv) { return kv.second.house && kv.second.house->quarantine_days > 0; });
    }
    if (when == "accompanies_army_abroad") {  // Ares: with an army abroad (Z:121)
        return !st.expeditions.empty() && !st.ares_with_army;
    }
    if (when == "talos_when_in_trouble") {  // Hephaestus: the city attacked (Z:120)
        return st.military.invasion.active && st.talos_days == 0;
    }
    if (when == "when_housing_mostly_vacant") {
        return mostly_vacant(st, catalog);
    }
    if (when == "fulfills_request") {  // Hermes: a request about to lapse (Z:122)
        const std::uint64_t today = st.time.total_days();
        return std::any_of(st.scenario.requests.begin(), st.scenario.requests.end(), [&](const CityRequest& r) {
            return r.due_day >= today && r.due_day - today <= static_cast<std::uint64_t>(p.request_days_left);
        });
    }
    if (when == "when_agoras_low") {  // Hera (P:20): a staffed stall with nothing left
        return std::any_of(st.city.buildings.items.begin(), st.city.buildings.items.end(), [](const auto& kv) {
            const Building& b = kv.second;
            return b.stall && b.staffing_percent > 0 && b.stall->units.empty();
        });
    }
    return false;
}

}  // namespace

std::vector<std::string> advance_god_help(WorldState& st, const DataCatalog& catalog) {
    std::vector<std::string> helped;
    if (st.time.day_in_month() != 0) {
        return helped;  // the gods look at the city once a month [ZAŁOŻENIE, L-130]
    }
    for (auto& [id, b] : st.city.buildings.items) {
        if (!b.sanctuary || !b.sanctuary->complete || b.sanctuary->favor_lost || b.sanctuary->prayer_cooldown > 0) {
            continue;
        }
        const SanctuaryParams& p = catalog.sanctuaries.at(b.type);
        if (p.monument || p.spontaneous.empty() || !sees_problem(st, catalog, p.spontaneous)) {
            continue;
        }
        const std::string effect = p.god == "hermes" ? "fulfills_one_outstanding_request_free" : p.prayer;
        if (effect == "fulfills_one_outstanding_request_free") {
            const std::uint64_t today = st.time.total_days();
            const auto& rq = st.scenario.requests;
            const auto it = std::find_if(rq.begin(), rq.end(), [&](const CityRequest& r) {
                return r.due_day >= today && r.due_day - today <= static_cast<std::uint64_t>(catalog.spontaneous.request_days_left);
            });
            if (it == rq.end() || !fulfill_request(st, catalog, static_cast<std::uint32_t>(it - rq.begin()), true).empty()) {
                continue;
            }
        } else if (!grant_blessing(st, catalog, effect)) {
            continue;  // nothing to give now (no room in the granaries, no request)
        }
        b.sanctuary->prayer_cooldown = static_cast<std::uint16_t>(p.prayer_cooldown_days);
        helped.push_back(p.god);
    }
    return helped;
}

}  // namespace bo::core
