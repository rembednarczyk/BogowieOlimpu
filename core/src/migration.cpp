#include "bo/core/migration.h"

#include "bo/core/hippodrome.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "bo/core/gods.h"
#include "bo/core/housing.h"
#include "bo/core/path.h"

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
    const std::uint32_t taken = h.population + h.incoming;
    return cap > taken ? cap - taken : 0;
}

std::optional<TilePos> land_entry(const CityState& city) {
    for (std::size_t i = 0; i < city.map.size(); ++i) {
        if (city.map.tiles()[i].feature == static_cast<std::uint8_t>(TileFeature::LandEntry)) {
            return city.map.pos(i);  // the first entry in tile order
        }
    }
    return std::nullopt;
}

WalkerId spawn_traveller(WorldState& st, const DataCatalog& catalog, const std::string& type, BuildingId house,
                         TilePos from, TilePos to) {
    auto path = find_path(st.city, {from, to, PathMode::Road});
    const auto t = catalog.walkers.find(type);
    if (!path || t == catalog.walkers.end()) {
        return kNoWalker;
    }
    Walker w;
    w.id = st.walkers.next_id++;
    w.type = type;
    w.home = house;
    w.tile = from;
    w.speed_raw = t->second.speed_raw;
    w.path = std::move(*path);
    w.phase = WalkerPhase::Routed;
    const WalkerId id = w.id;
    st.walkers.items.emplace(id, std::move(w));
    return id;
}

// Moves up to `people` into common houses; houses with many free places first (doc 24). Returns how many.
// With `entry`, they walk from it: the house is reserved and must be reachable (doc 24).
// Elite families (L-108, owner's choice after eZeus): settlers of the common pool, so their pace follows the
// popularity; a group for an elite house brings at most 8 people.
constexpr std::uint32_t kEliteGroup = 8;

std::uint32_t fill_houses(WorldState& st, const DataCatalog& catalog, std::uint32_t people,
                          std::optional<TilePos> entry = std::nullopt, std::vector<WalkerId>* spawned = nullptr,
                          bool elite_too = false) {
    const auto& p = catalog.migration;
    std::uint32_t placed = 0;
    for (const bool big_first : {true, false}) {
        for (auto& [id, b] : st.city.buildings.items) {
            if (placed == people) {
                return placed;
            }
            if (!b.house || (b.house->elite && !elite_too) || b.house->abandoned || !b.road_access ||
                b.house->quarantine_days > 0) {
                continue;  // the homeless only go back to common houses
            }
            HouseState& h = *b.house;
            std::uint32_t room = free_places(catalog, h, st.difficulty);
            if (h.elite) {
                room = std::min(room, kEliteGroup);  // elite families come from the same pool, 8 at most (L-108)
            }
            // An elite family fills its house whole (L-108): elite houses count as roomy in the first pass,
            // or common houses that keep growing take every immigrant first (B-012).
            if (room == 0 || (big_first && !h.elite && room < static_cast<std::uint32_t>(p.fill_first_free_slots))) {
                continue;
            }
            const std::uint32_t n = std::min(room, people - placed);
            if (entry) {
                const WalkerId w = spawn_traveller(st, catalog, "immigrant", id, *entry, *b.road_access);
                if (w == kNoWalker) {
                    continue;  // not reachable from the entry
                }
                Walker& walker = st.walkers.items.at(w);
                walker.job = WalkerJob::Settle;
                walker.target = id;
                walker.want_loads = n;  // people in the group
                h.incoming += n;
                placed += n;
                if (spawned) {
                    spawned->push_back(w);
                }
                continue;
            }
            if (h.population == 0) {
                h.happiness = static_cast<std::uint8_t>(p.base_happiness[static_cast<std::size_t>(st.difficulty)]);
            }
            h.population += n;
            placed += n;
        }
    }
    return placed;
}

// Removes up to `people` from the lowest house levels first (ties: building id). With `entry`, each household
// that leaves walks out to it.
std::uint32_t empty_houses(WorldState& st, const DataCatalog& catalog, std::uint32_t people,
                           std::optional<TilePos> entry, std::vector<WalkerId>& spawned) {
    std::vector<std::pair<std::uint8_t, BuildingId>> order;
    for (const auto& [id, b] : st.city.buildings.items) {
        if (b.house && !b.house->elite && b.house->population > 0) {
            order.emplace_back(b.house->level, id);
        }
    }
    std::sort(order.begin(), order.end());
    std::uint32_t removed = 0;
    for (const auto& [level, id] : order) {
        Building& b = st.city.buildings.items.at(id);
        HouseState& h = *b.house;
        const std::uint32_t n = std::min(h.population, people - removed);
        h.population -= n;
        removed += n;
        if (entry && b.road_access) {
            const WalkerId w = spawn_traveller(st, catalog, "emigrant", id, *b.road_access, *entry);
            if (w != kNoWalker) {
                spawned.push_back(w);
            }
        }
        if (removed == people) {
            break;
        }
    }
    return removed;
}

// Companies in the field while no enemy is in the city (Z:159).
bool idle_army(const MilitaryState& m) {
    return !m.invasion.active && std::any_of(m.companies.begin(), m.companies.end(), [](const Company& c) {
        return c.status == CompanyStatus::Marching || c.status == CompanyStatus::Still;
    });
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
                                    unemployment_mood(p, st.labour) + (st.finance.treasury < 0 ? p.debt_mood : 0) +
                                    (idle_army(st.military) ? p.idle_army_mood : 0) +
                                    hippodrome_mood(st, catalog);  // a big track (P:14, L-127)
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
            st.city.homeless += h.elite ? 0 : h.population;  // no road for more than 4 days (doc 24); elite leave
            h.population = 0;
        }
    }
    if (st.city.homeless > 0) {
        r.rehoused = fill_houses(st, catalog, st.city.homeless);
        st.city.homeless -= r.rehoused;
    }
    const std::int32_t change = p.batch_people * migration_percent(p, st.migration.popularity) / 100;
    // L-034 (eZeus prolongedDebt): after a year of debt nobody moves in; people may still leave.
    st.migration.pool += st.debt_days >= catalog.debt.immigration_block_days ? std::min(change, 0) : change;
    const auto entry = land_entry(st.city);
    if (st.migration.pool >= p.min_batch) {
        r.arrived = fill_houses(st, catalog, static_cast<std::uint32_t>(st.migration.pool), entry, &r.spawned, true);
        st.migration.pool = 0;  // no room: immigration is blocked, the batch does not wait (doc 24)
    } else if (st.migration.pool <= -p.min_batch) {
        if (static_cast<std::int32_t>(city_population(st.city)) > p.emigration_min_population &&
            !god_blesses(st.city, catalog, "aphrodite")) {  // Aphrodite: nobody emigrates (doc 10)
            r.left = empty_houses(st, catalog, static_cast<std::uint32_t>(-st.migration.pool), entry, r.spawned);
        }
        st.migration.pool = 0;
    }
    st.migration.arrived_this_month += r.arrived;
    st.migration.left_this_month += r.left;
    return r;
}

void settle_at_target(WorldState& st, const DataCatalog& catalog, Walker& w) {
    const auto it = st.city.buildings.items.find(w.target);
    std::uint32_t people = w.want_loads;
    if (it != st.city.buildings.items.end() && it->second.house && !it->second.house->abandoned) {
        HouseState& h = *it->second.house;
        h.incoming -= std::min(h.incoming, people);
        const std::uint32_t cap = house_capacity(catalog, h, st.difficulty);
        const std::uint32_t n = std::min(people, cap > h.population ? cap - h.population : 0U);
        if (h.population == 0 && n > 0) {
            h.happiness = static_cast<std::uint8_t>(catalog.migration.base_happiness[static_cast<std::size_t>(st.difficulty)]);
        }
        h.population += n;
        people -= n;
        people = h.elite ? 0 : people;  // elite do not take common houses
    }
    st.city.homeless += people;  // the house shrank or is gone meanwhile
    w.job = WalkerJob::None;
    w.phase = WalkerPhase::Done;
}

void settle_failed(WorldState& st, const Walker& w) {
    const auto it = st.city.buildings.items.find(w.target);
    if (it != st.city.buildings.items.end() && it->second.house) {
        it->second.house->incoming -= std::min(it->second.house->incoming, w.want_loads);
        if (it->second.house->elite) {
            return;  // elite do not take common houses
        }
    }
    st.city.homeless += w.want_loads;
}

}  // namespace bo::core
