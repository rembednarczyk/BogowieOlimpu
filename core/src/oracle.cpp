// Module D1: the oracle (Z:124; docs/gameplay/10-mitologia.md; assumptions L-126). Zeus's Stronghold and
// Apollo's Oracle foretell what is coming: here the next threat on the scenario's calendar (an invasion, a god's
// attack, a monster, a disaster), as eZeus's prophecies do (S-008, oracle prediction messages); then the oracle
// keeps silent for oracle_silent_days (counted down with the prayer cooldown, gods.cpp).
#include "bo/core/oracle.h"

#include <optional>
#include <tuple>

namespace bo::core {

std::string consult_oracle(WorldState& st, const DataCatalog& catalog, BuildingId id, OracleSpoke& out) {
    const auto it = st.city.buildings.items.find(id);
    const auto params = it != st.city.buildings.items.end() ? catalog.sanctuaries.find(it->second.type)
                                                             : catalog.sanctuaries.end();
    if (params == catalog.sanctuaries.end() || !it->second.sanctuary || !it->second.sanctuary->complete ||
        !params->second.oracle) {
        return "myth.no_oracle";
    }
    SanctuaryState& s = *it->second.sanctuary;
    if (s.oracle_silent > 0) {
        return "myth.oracle_silent";  // it does not answer when it spoke not long ago (Z:124)
    }
    const auto year = static_cast<std::int64_t>(st.time.year());
    const auto month = static_cast<std::int64_t>(st.time.month());
    const std::int64_t now = year * 12 + month;
    std::optional<std::tuple<std::int64_t, std::size_t>> best;  // month index, event order
    for (std::size_t i = 0; i < st.scenario.events.size(); ++i) {
        const ScenarioEvent& e = st.scenario.events[i];
        if (e.type != "invasion" && e.type != "god_attack" && e.type != "monster" && e.type != "disaster") {
            continue;
        }
        std::int64_t y = e.year;
        if (e.repeat) {
            y = std::max<std::int64_t>(y, year);
            if (y * 12 + e.month < now || e.last_year >= y) {
                ++y;  // this year's has passed or fired: next year's
            }
        } else if (e.last_year >= 0) {
            continue;  // already happened
        }
        const std::int64_t at = y * 12 + e.month;
        if (at >= now && (!best || std::make_tuple(at, i) < *best)) {
            best = std::make_tuple(at, i);
        }
    }
    out = OracleSpoke{id, "", "", 0};
    if (best) {
        const ScenarioEvent& e = st.scenario.events[std::get<1>(*best)];
        out.kind = e.type;
        out.subject = e.type == "god_attack" ? e.god : e.nation;  // monster and disaster kind are in `nation`
        out.months = static_cast<std::uint32_t>(std::get<0>(*best) - now);
    }
    s.oracle_silent = static_cast<std::uint16_t>(params->second.oracle_silent_days);
    return {};
}

}  // namespace bo::core
