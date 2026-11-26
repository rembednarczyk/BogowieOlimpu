#include "bo/core/migration.h"

#include <algorithm>
#include <vector>

#include "bo/core/housing.h"

namespace bo::core {
namespace {

std::int32_t unemployment_mood(const MigrationParams& p, const LabourState& l) {
    const std::int64_t jobless = static_cast<std::int64_t>(l.workforce) - static_cast<std::int64_t>(l.employed);
    const std::int64_t percent = l.workforce == 0 ? 0 : std::max<std::int64_t>(0, jobless) * 100 / l.workforce;
    for (const auto& [above, mood] : p.unemployment_mood) {
        if (percent > above) {
            return mood;
        }
    }
    return 0;
}

std::uint32_t free_places(const DataCatalog& catalog, const HouseState& h, Difficulty d) {
    const std::uint32_t cap = house_capacity(catalog, h, d);
    return cap > h.population ? cap - h.population : 0;
}

// Moves up to `people` into common houses; houses with many free places first (doc 24). Returns how many.
std::uint32_t fill_houses(WorldState& st, const DataCatalog& catalog, std::uint32_t people) {
    const auto& p = catalog.migration;
    std::uint32_t placed = 0;
    for (const bool big_first : {true, false}) {
        for (auto& [id, b] : st.city.buildings.items) {
            if (placed == people) {
                return placed;
            }
            if (!b.house || b.house->elite || b.house->abandoned || !b.road_access || b.house->quarantine_days > 0) {
                continue;  // elite households arrive with module C5 (their plots need city stock)
            }
            HouseState& h = *b.house;
            const std::uint32_t room = free_places(catalog, h, st.difficulty);
            if (room == 0 || (big_first && room < static_cast<std::uint32_t>(p.fill_first_free_slots))) {
                continue;
            }
            const std::uint32_t n = std::min(room, people - placed);
            if (h.population == 0) {
                h.happiness = static_cast<std::uint8_t>(p.base_happiness[static_cast<std::size_t>(st.difficulty)]);
            }
            h.population += n;
            placed += n;
        }
    }
    return placed;
}

// Removes up to `people` from the lowest house levels first (ties: building id).
std::uint32_t empty_houses(WorldState& st, std::uint32_t people) {
    std::vector<std::pair<std::uint8_t, BuildingId>> order;
    for (const auto& [id, b] : st.city.buildings.items) {
        if (b.house && !b.house->elite && b.house->population > 0) {
            order.emplace_back(b.house->level, id);
        }
    }
    std::sort(order.begin(), order.end());
    std::uint32_t removed = 0;
    for (const auto& [level, id] : order) {
        HouseState& h = *st.city.buildings.items.at(id).house;
        const std::uint32_t n = std::min(h.population, people - removed);
        h.population -= n;
        removed += n;
        if (removed == people) {
            break;
        }
    }
    return removed;
}

}  // namespace

std::uint32_t city_population(const CityState& city) {
    std::uint32_t total = 0;
    for (const auto& [id, b] : city.buildings.items) {
        if (b.house) {
            total += b.house->population;
        }
    }
    return total;
}

void update_moods(WorldState& st, const DataCatalog& catalog) {
    const MigrationParams& p = catalog.migration;
    const auto di = static_cast<std::size_t>(st.difficulty);
    const std::int32_t base = p.base_happiness[di];
    const bool small = static_cast<std::int32_t>(city_population(st.city)) < p.small_city_population;
    const std::int32_t city_terms = p.tax_mood[static_cast<std::size_t>(st.finance.tax_rate)][di] +
                                    p.wage_mood[static_cast<std::size_t>(st.finance.wage_rate)] +
                                    unemployment_mood(p, st.labour) + (st.finance.treasury < 0 ? p.debt_mood : 0);
    std::int64_t sum = 0;
    std::int64_t houses = 0;
    for (auto& [id, b] : st.city.buildings.items) {
        if (!b.house || b.house->population == 0) {
            continue;
        }
        HouseState& h = *b.house;
        if (small) {
            h.happiness = static_cast<std::uint8_t>(base);  // small cities do not complain (doc 24)
        } else {
            std::int32_t food = 0;
            const bool eats = levels_of(catalog, h)[h.level].consumption[0] > 0;
            if (eats && h.goods[static_cast<std::size_t>(HouseGood::Food)] > 0) {
                food = p.food_stocked;
                h.hungry_updates = 0;
            } else if (eats) {
                h.hungry_updates = static_cast<std::uint8_t>(std::min(h.hungry_updates + 1, 100));
                food = std::max(p.food_missing_max, p.food_missing_first * h.hungry_updates);
            }
            h.happiness = static_cast<std::uint8_t>(std::clamp(h.happiness + city_terms + food, 0, 100));
        }
        sum += h.happiness;
        ++houses;
    }
    st.migration.popularity = static_cast<std::uint8_t>(houses == 0 ? base : sum / houses);
}

std::int32_t migration_percent(const MigrationParams& p, std::int32_t popularity) {
    for (const auto& [above, percent] : p.migration_table) {
        if (popularity > above) {
            return percent;
        }
    }
    return 0;
}

MigrationResult migrate(WorldState& st, const DataCatalog& catalog) {
    const MigrationParams& p = catalog.migration;
    MigrationResult r;
    for (auto& [id, b] : st.city.buildings.items) {
        if (!b.house || b.house->population == 0) {
            continue;
        }
        HouseState& h = *b.house;
        h.days_without_road = b.road_access ? 0 : static_cast<std::uint8_t>(std::min(h.days_without_road + 1, 255));
        if (h.days_without_road > 4) {
            st.city.homeless += h.population;  // no road for more than 4 days (doc 24)
            h.population = 0;
        }
    }
    if (st.city.homeless > 0) {
        r.rehoused = fill_houses(st, catalog, st.city.homeless);
        st.city.homeless -= r.rehoused;
    }
    st.migration.pool += p.batch_people * migration_percent(p, st.migration.popularity) / 100;
    if (st.migration.pool >= p.min_batch) {
        r.arrived = fill_houses(st, catalog, static_cast<std::uint32_t>(st.migration.pool));
        st.migration.pool = 0;  // no room: immigration is blocked, the batch does not wait (doc 24)
    } else if (st.migration.pool <= -p.min_batch) {
        if (static_cast<std::int32_t>(city_population(st.city)) > p.emigration_min_population) {
            r.left = empty_houses(st, static_cast<std::uint32_t>(-st.migration.pool));
        }
        st.migration.pool = 0;
    }
    st.migration.arrived_this_month += r.arrived;
    st.migration.left_this_month += r.left;
    return r;
}

}  // namespace bo::core
