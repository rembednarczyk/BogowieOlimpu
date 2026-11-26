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
    for (auto& [id, b] : city.buildings.items) {
        b.cursed_days = static_cast<std::uint16_t>(b.cursed_days > 0 ? b.cursed_days - 1 : 0);
    }
    st.talos_days = static_cast<std::uint16_t>(st.talos_days > 0 ? st.talos_days - 1 : 0);
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

}  // namespace bo::core
