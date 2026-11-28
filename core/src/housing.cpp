#include "bo/core/housing.h"

#include <algorithm>

#include "bo/core/appeal.h"
#include "bo/core/culture.h"

namespace bo::core {

const std::vector<HousingLevel>& levels_of(const DataCatalog& catalog, const HouseState& h) {
    return h.elite ? catalog.elite_levels : catalog.common_levels;
}

std::int32_t house_culture(const Building& b, const DataCatalog& catalog, std::int32_t city_bonus) {
    std::int32_t points = city_bonus;
    for (const auto& [walker, days] : b.coverage) {
        const auto w = catalog.walkers.find(walker);
        if (days > 0 && w != catalog.walkers.end()) {
            points += w->second.culture_points;
        }
    }
    return points;
}

bool house_needs_met(const Building& b, const DataCatalog& catalog, const HousingLevel& level, Difficulty d,
                     std::int32_t city_bonus) {
    const HouseModel& m = level.model[static_cast<std::size_t>(d)];
    const HouseState& h = *b.house;
    if (m.water) {
        const auto w = b.coverage.find("water_bearer");
        if (w == b.coverage.end() || w->second == 0) {
            return false;
        }
    }
    for (std::size_t g = 0; g < kHouseGoods; ++g) {
        if (m.needs[g] && h.goods[g] == 0) {
            return false;
        }
    }
    return house_culture(b, catalog, city_bonus) >= m.culture;
}

std::uint32_t house_capacity(const DataCatalog& catalog, const HouseState& h, Difficulty d) {
    const auto& levels = levels_of(catalog, h);
    return static_cast<std::uint32_t>(levels[h.level].model[static_cast<std::size_t>(d)].population);
}

std::vector<HouseChange> update_houses(CityState& city, const DataCatalog& catalog, Difficulty d) {
    std::vector<HouseChange> changes;
    const auto di = static_cast<std::size_t>(d);
    const std::int32_t bonus = city_culture_bonus(city, catalog);
    for (auto& [id, b] : city.buildings.items) {
        if (!b.house || b.house->population == 0 || b.house->abandoned) {
            continue;  // vacant plots wait for immigrants (C8)
        }
        HouseState& h = *b.house;
        const auto& levels = levels_of(catalog, h);
        const std::int32_t appeal = building_appeal(city, b);
        const HouseModel& cur = levels[h.level].model[di];
        if (appeal >= cur.appeal_devolve && house_needs_met(b, catalog, levels[h.level], d, bonus)) {
            h.failed_checks = 0;
            h.established = h.established || h.elite;
            const std::size_t next = h.level + 1U;
            if (next < levels.size() && appeal >= cur.appeal_evolve && house_needs_met(b, catalog, levels[next], d, bonus)) {
                h.level = static_cast<std::uint8_t>(next);
                changes.push_back({id, h.level, false});
            }
            continue;
        }
        if (++h.failed_checks < 3) {
            continue;
        }
        h.failed_checks = 0;
        std::optional<std::uint8_t> target;
        for (std::size_t k = h.level; k-- > 0;) {
            if (appeal >= levels[k].model[di].appeal_devolve && house_needs_met(b, catalog, levels[k], d, bonus)) {
                target = static_cast<std::uint8_t>(k);
                break;
            }
        }
        if (!target && h.elite && !h.established) {
            continue;  // B-013: a new family waits for its needs (eZeus: level 0 below Residence); it leaves on a loss
        }
        if (!target && h.elite) {
            city.homeless += h.population;  // the owners leave the city with their household [ZAŁOŻENIE]
            h.population = 0;
            h.abandoned = true;  // ruined residence (Z:34)
            changes.push_back({id, h.level, true});
            continue;
        }
        h.level = target.value_or(0);  // common houses always fit Hut (AppD -99, no needs)
        const std::uint32_t cap = house_capacity(catalog, h, d);
        if (h.population > cap) {
            city.homeless += h.population - cap;
            h.population = cap;
        }
        changes.push_back({id, h.level, false});
    }
    return changes;
}

void consume_house_goods(CityState& city, const DataCatalog& catalog) {
    for (auto& [id, b] : city.buildings.items) {
        if (!b.house || b.house->population == 0) {
            continue;
        }
        HouseState& h = *b.house;
        const HousingLevel& lv = levels_of(catalog, h)[h.level];
        // Full-house monthly figures; capacity on Hero matches the table (doc 18). Food scales with people.
        const auto cap = static_cast<std::uint32_t>(std::max(1, lv.model[static_cast<std::size_t>(Difficulty::Hero)].population));
        const std::uint32_t food_num = static_cast<std::uint32_t>(lv.consumption[0]) * h.population + h.food_remainder;
        const std::uint32_t food = food_num / (2 * cap);
        h.food_remainder = food_num % (2 * cap);
        auto& stock = h.goods;
        stock[0] -= std::min(stock[0], food);
        for (std::size_t g = 1; g < 4; ++g) {
            const auto half = static_cast<std::uint32_t>(lv.consumption[g] / 2);
            stock[g] -= std::min(stock[g], half);
        }
    }
}

}  // namespace bo::core
