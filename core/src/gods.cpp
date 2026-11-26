// Module D1 stage 2: blessings, sanctification, prayers (docs/gameplay/10, 21; Z:119-124; L-053, L-112).
#include "bo/core/gods.h"

#include <algorithm>
#include <cstdlib>
#include <optional>

#include "bo/core/state.h"
#include "bo/core/storage.h"

namespace bo::core {
namespace {

bool sanctifiable(const SanctuaryParams& p, const Building& b) {
    if (!b.producer || b.sanctified) {
        return false;
    }
    return std::any_of(p.sanctifies.begin(), p.sanctifies.end(),
                       [&b](const std::string& t) { return t == "any" || t == b.type; });
}

// Loads of `good` into storages that accept it, in id order; returns the loads placed.
std::uint32_t gift(CityState& city, const DataCatalog& catalog, const std::string& good, std::uint32_t loads) {
    std::uint32_t placed = 0;
    for (auto& [id, b] : city.buildings.items) {
        if (placed == loads || !b.storage || b.trade || !storage_accepts(b, catalog, good)) {
            continue;
        }
        const std::uint32_t n = std::min(loads - placed, storage_room(b, catalog, good));
        b.storage->loads[good] += n;
        placed += n;
    }
    return placed;
}

}  // namespace

bool god_blesses(const CityState& city, const DataCatalog& catalog, const std::string& god) {
    for (const auto& [id, b] : city.buildings.items) {
        if (b.sanctuary && b.sanctuary->complete && !b.sanctuary->favor_lost) {
            const auto p = catalog.sanctuaries.find(b.type);
            if (p != catalog.sanctuaries.end() && p->second.god == god) {
                return true;
            }
        }
    }
    return false;
}

void advance_gods(WorldState& st, const DataCatalog& catalog) {
    CityState& city = st.city;
    std::vector<BuildingId> sanctuaries;
    for (const auto& [id, b] : city.buildings.items) {
        if (b.sanctuary && b.sanctuary->complete) {
            sanctuaries.push_back(id);
        }
    }
    for (const BuildingId id : sanctuaries) {
        Building& s = city.buildings.items.at(id);
        SanctuaryState& state = *s.sanctuary;
        state.prayer_cooldown = static_cast<std::uint16_t>(state.prayer_cooldown > 0 ? state.prayer_cooldown - 1 : 0);
        if (state.favor_lost) {
            continue;
        }
        if (state.days_to_sanctify > 0) {
            --state.days_to_sanctify;
            continue;
        }
        const SanctuaryParams& p = catalog.sanctuaries.at(s.type);
        std::optional<std::pair<std::int32_t, BuildingId>> best;
        for (const auto& [bid, b] : city.buildings.items) {
            const std::int32_t d = std::max(std::abs(b.origin.x - s.origin.x), std::abs(b.origin.y - s.origin.y));
            if (sanctifiable(p, b) && d <= catalog.sanctify_reach_tiles && (!best || d < best->first)) {
                best = std::make_pair(d, bid);
            }
        }
        if (best) {
            Building& b = city.buildings.items.at(best->second);
            b.sanctified = true;
            b.sanctified_base = b.producer->output_loads;
            state.days_to_sanctify = static_cast<std::uint16_t>(catalog.sanctify_every_days);
        }
    }
}

void apply_sanctification(CityState& city) {
    for (auto& [id, b] : city.buildings.items) {
        if (!b.sanctified || !b.producer) {
            continue;
        }
        if (b.producer->output_loads > b.sanctified_base) {
            b.producer->output_loads += 3 * (b.producer->output_loads - b.sanctified_base);  // 4x in all (doc 21)
            b.sanctified = false;
        }
        b.sanctified_base = b.producer->output_loads;
    }
}

std::string pray(WorldState& st, const DataCatalog& catalog, BuildingId id) {
    const auto it = st.city.buildings.items.find(id);
    if (it == st.city.buildings.items.end() || !it->second.sanctuary || !it->second.sanctuary->complete) {
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
    const auto loads = static_cast<std::uint32_t>(catalog.prayer_goods_loads);
    const std::string& effect = p.prayer;
    std::uint32_t given = 0;
    if (effect == "food_to_granaries") {
        given = gift(st.city, catalog, "wheat", loads);
    } else if (effect == "pork") {
        given = gift(st.city, catalog, "pork", loads);
    } else if (effect == "seafood_when_low_food") {
        given = gift(st.city, catalog, "fish", loads);
    } else if (effect == "olives_and_olive_oil") {
        given = gift(st.city, catalog, "olives", loads / 2) + gift(st.city, catalog, "olive_oil", loads / 2);
    } else if (effect == "wine_by_satyrs") {
        given = gift(st.city, catalog, "wine", loads);
    } else if (effect == "money") {
        st.finance.treasury += catalog.prayer_money;
        given = 1;
    } else if (effect == "cures_plague_improves_health") {
        for (auto& [bid, b] : st.city.buildings.items) {
            b.disease_counter = 0;
            if (b.house) {
                b.house->quarantine_days = 0;
            }
        }
        given = 1;
    } else {
        return "myth.prayer_not_available";  // armies, monsters, world requests: later modules
    }
    if (given == 0) {
        return "myth.no_room_for_gift";
    }
    s.prayer_cooldown = static_cast<std::uint16_t>(p.prayer_cooldown_days);
    return {};
}

}  // namespace bo::core
