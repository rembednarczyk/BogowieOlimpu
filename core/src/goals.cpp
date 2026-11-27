// Module D5: episode goals (docs/gameplay/23; doc 23; L-093b). The types of the reference's editor as eZeus
// reads them (S-008, eepisodegoal): population, treasury, sanctuary, rule (vassal), quest, housing (houses),
// support, slay, set aside goods, survive until, complete before, trading partners, yearly profit. Pyramids
// (monuments) and the hippodrome wait for those buildings (L-093b).
#include "bo/core/scenario.h"

#include <algorithm>

#include "bo/core/military.h"
#include "bo/core/migration.h"
#include "bo/core/monsters.h"
#include "bo/core/state.h"
#include "bo/core/storage.h"

namespace bo::core {
namespace {

// Soldiers the houses can give of `unit` or better (S-008: rabble counts hoplites and horsemen, hoplites count
// horsemen); "trireme": ships on the water.
std::int64_t support(const WorldState& st, const DataCatalog& catalog, const std::string& unit) {
    if (unit == "trireme") {
        return static_cast<std::int64_t>(st.military.ships.size());
    }
    const auto rank = [](const std::string& u) { return u == "horseman" ? 2 : u == "hoplite" ? 1 : u == "rabble" ? 0 : -1; };
    std::int64_t n = 0;
    for (const auto& [id, b] : st.city.buildings.items) {
        if (!b.house) {
            continue;
        }
        const auto& levels = b.house->elite ? catalog.elite_levels : catalog.common_levels;
        const std::string& gives = b.house->level < levels.size() ? levels[b.house->level].military : std::string();
        if (rank(gives) >= 0 && rank(gives) >= rank(unit)) {
            n += house_soldiers(catalog, *b.house, st.difficulty);
        }
    }
    return n;
}

// Partner cities trading with the city: named by a trading post or pier (Z:75-79).
std::int64_t trading_partners(const WorldState& st) {
    std::vector<std::int32_t> seen;
    for (const auto& [id, b] : st.city.buildings.items) {
        if (b.trade && b.trade->partner >= 0 &&
            std::find(seen.begin(), seen.end(), b.trade->partner) == seen.end()) {
            seen.push_back(b.trade->partner);
        }
    }
    return static_cast<std::int64_t>(seen.size());
}

}  // namespace

bool goal_met(const WorldState& st, const DataCatalog& catalog, const ScenarioGoal& g) {
    if (g.type == "population") {
        return city_population(st.city) >= static_cast<std::uint64_t>(g.value);
    }
    if (g.type == "treasury") {
        return st.finance.treasury >= g.value;
    }
    if (g.type == "sanctuary") {
        // B-011, owner's decision: the sanctuary must be finished and standing; the god's favour does not count.
        return std::any_of(st.city.buildings.items.begin(), st.city.buildings.items.end(), [&](const auto& kv) {
            const auto p = catalog.sanctuaries.find(kv.second.type);
            return kv.second.sanctuary && kv.second.sanctuary->complete && p != catalog.sanctuaries.end() &&
                   p->second.god == g.target;
        });
    }
    if (g.type == "vassal") {
        return std::any_of(st.trade.partners.begin(), st.trade.partners.end(),
                           [&g](const TradePartner& p) { return p.id == g.target && p.relation == "vassal"; });
    }
    if (g.type == "quest") {
        return std::any_of(st.scenario.quests.begin(), st.scenario.quests.end(),
                           [&g](const Quest& q) { return q.god == g.target && q.status == QuestStatus::Done; });
    }
    if (g.type == "houses") {
        std::int64_t n = 0;
        for (const auto& [id, b] : st.city.buildings.items) {
            if (!b.house || b.house->population == 0) {
                continue;
            }
            const auto& levels = b.house->elite ? catalog.elite_levels : catalog.common_levels;
            const auto it = std::find_if(levels.begin(), levels.end(),
                                         [&g](const HousingLevel& lv) { return lv.id == g.target; });
            if (it != levels.end() && b.house->level >= static_cast<std::size_t>(it - levels.begin())) {
                ++n;
            }
        }
        return n >= g.value;
    }
        if (g.type == "support") {  // e.g. 24 hoplites the houses can muster, or 2 triremes (S-008)
        return support(st, catalog, g.target) >= g.value;
    }
    if (g.type == "slay") {  // a named monster, or any `value` of them (S-008)
        const auto slain = std::count_if(st.monsters.begin(), st.monsters.end(), [&g](const MonsterState& m) {
            return m.slain && (g.target.empty() || m.id == g.target);
        });
        return slain >= std::max<std::int64_t>(1, g.value);
    }
    if (g.type == "set_aside_goods") {  // loads of a good in storage (S-008)
        return static_cast<std::int64_t>(city_stock(st.city, catalog, g.target)) >= g.value;
    }
    if (g.type == "survive_until") {  // `value`: the year (from 0) the city must reach (S-008)
        return static_cast<std::int64_t>(st.time.year()) >= g.value;
    }
    if (g.type == "complete_before") {  // met until year `value`; then the episode is lost (advance_scenario)
        return static_cast<std::int64_t>(st.time.year()) < g.value;
    }
    if (g.type == "trading_partners") {
        return trading_partners(st) >= g.value;
    }
    if (g.type == "yearly_profit") {  // last year's taxes less wages and building [ZAŁOŻENIE, L-093b]
        const FinanceYear& y = st.finance.last_year;
        return y.taxes - y.wages - y.construction >= g.value;
    }
    return false;
}

}  // namespace bo::core
