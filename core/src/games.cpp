#include "bo/core/games.h"

#include <algorithm>

#include "bo/core/gods.h"

namespace bo::core {

std::int32_t games_chance(const WorldState& st, const DataCatalog& catalog, const GameParams& game) {
    const GamesParams& p = catalog.games;
    std::int64_t people = 0;
    std::int64_t covered_sum = 0;  // sum over the game's performers of the people they reach
    for (const auto& [id, b] : st.city.buildings.items) {
        if (!b.house || b.house->population == 0) {
            continue;
        }
        people += b.house->population;
        for (const std::string& w : game.walkers) {
            const auto it = b.coverage.find(w);
            covered_sum += it != b.coverage.end() && it->second > 0 ? b.house->population : 0;
        }
    }
    if (people == 0 || game.walkers.empty()) {
        return 0;
    }
    const std::int64_t coverage = 100 * covered_sum / (people * static_cast<std::int64_t>(game.walkers.size()));
    std::int64_t mult = 0;  // per mille
    for (const auto& [from, m] : p.population_multiplier_per_mille) {
        mult = people >= from ? m : mult;
    }
    if (mult * coverage < static_cast<std::int64_t>(p.min_score) * 1000) {
        return 0;  // not invited: too few performers (Z:111)
    }
    std::int64_t chance = mult * coverage * coverage / 1000 / 100;  // percent
    if (god_blesses(st.city, catalog, "apollo")) {
        chance = chance * p.apollo_multiplier_per_mille / 1000;  // Apollo helps (Z:112)
    }
    return static_cast<std::int32_t>(std::clamp<std::int64_t>(chance, 1, 100));
}

namespace {

bool working_stadium(const CityState& city, const DataCatalog& catalog) {
    (void)catalog;
    return std::any_of(city.buildings.items.begin(), city.buildings.items.end(), [](const auto& kv) {
        const Building& b = kv.second;
        return b.type == "stadium" && b.staffing_percent > 0 && b.road_access && b.performing_days > 0;
    });
}

// Hosting (Z:111): the parade of the athletes of all Greece; other leaders and the city's own people are pleased.
void host(WorldState& st, const GamesParams& p) {
    ++st.scenario.monuments["games_host"];
    for (TradePartner& partner : st.trade.partners) {
        partner.diplomacy.opinion = std::min(100, partner.diplomacy.opinion + p.host_opinion);
    }
    for (auto& [id, b] : st.city.buildings.items) {
        if (b.house && b.house->population > 0) {
            b.house->happiness = static_cast<std::uint8_t>(std::min(100, b.house->happiness + p.host_happiness));
        }
    }
}

}  // namespace

std::optional<GamesResult> advance_games(WorldState& st, const DataCatalog& catalog) {
    const GamesParams& p = catalog.games;
    if (st.time.day_in_month() != 0 || st.time.month() != p.end_month) {
        return std::nullopt;
    }
    const auto year = static_cast<std::uint32_t>(st.time.year());
    const auto game = std::find_if(p.games.begin(), p.games.end(),
                                   [year](const GameParams& g) { return g.year_mod_4 == year % 4; });
    if (game == p.games.end()) {
        return std::nullopt;
    }
    GamesResult r;
    r.game = game->id;
    if (game->id == "olympian" && st.games_host_year == static_cast<std::int32_t>(year)) {
        st.games_host_year = -1;
        if (!working_stadium(st.city, catalog)) {
            r.right_lost = true;  // another city hosts and the player is not invited (Z:111)
            return r;
        }
        host(st, p);
        r.hosted = true;
    }
    r.chance = games_chance(st, catalog, *game);
    r.invited = r.chance > 0;
    // Draw only for an invited city: a city without performers leaves the random stream as it was.
    r.won = r.invited && static_cast<std::int32_t>(st.rng.stream(RngStream::World).next_u32() % 100U) < r.chance;
    if (r.won) {
        ++st.scenario.monuments[game->monument];  // Scholar or Athlete (Z:155-156)
        if (game->id == "olympian") {
            for (TradePartner& partner : st.trade.partners) {  // other leaders think better of the city (Z:111)
                partner.diplomacy.opinion = std::min(100, partner.diplomacy.opinion + p.olympian_opinion);
            }
            if (st.games_host_year < 0) {
                st.games_host_year = static_cast<std::int32_t>(year) + 4;  // the right to host in four years (Z:111)
            }
        }
    }
    return r;
}

}  // namespace bo::core
