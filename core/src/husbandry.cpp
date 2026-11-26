// Modules C4 stage 2: herds, orchards, processors. Rules: docs/gameplay/04-zywnosc.md (Z:41-45),
// 19-produkcja-i-logistyka-liczby.md (S-004, S-005). Assumptions: L-101.
#include <algorithm>
#include <cstdlib>
#include <optional>

#include "bo/core/logistics.h"
#include "bo/core/walker.h"

namespace bo::core {
namespace {

constexpr std::uint32_t kDaysPerYear = 192;
constexpr std::uint32_t kFullGrowth = 100 * kDaysPerYear;  // ripeness 100 % (one load) x 192
constexpr std::uint16_t kHalfGrowthDays = 96;              // no visit for >= 6 months: half speed (doc 19)
constexpr std::uint16_t kNoGrowthDays = 160;               // >= 10 months: no growth
constexpr std::uint32_t kCarryOverPercent = 33;            // below ~33 % a plant keeps its ripeness (doc 19)

bool alive(const WorldState& st, WalkerId id) { return st.walkers.items.count(id) != 0; }

std::string worker_type_of(const DataCatalog& catalog, const std::string& building_type) {
    for (const auto& [id, w] : catalog.walkers) {
        if (std::find(w.from.begin(), w.from.end(), building_type) != w.from.end()) {
            return id;
        }
    }
    return {};
}

// The fruit of a tree type: lodge outputs are listed in the order of the types it tends.
std::optional<std::string> fruit_of(const DataCatalog& catalog, const std::string& tree, std::string* lodge_type) {
    for (const auto& [id, t] : catalog.buildings) {
        for (std::size_t i = 0; i < t.tends.size() && i < t.outputs.size(); ++i) {
            if (t.tends[i] == tree) {
                if (lodge_type) {
                    *lodge_type = id;
                }
                return t.outputs[i];
            }
        }
    }
    return std::nullopt;
}

bool in_months(const std::vector<std::int32_t>& months, std::uint32_t month) {
    return std::find(months.begin(), months.end(), static_cast<std::int32_t>(month)) != months.end();
}

std::int32_t chebyshev(TilePos a, TilePos b) { return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y)); }

WalkerId send(WorldState& st, const DataCatalog& catalog, BuildingId home, BuildingId target, WalkerJob job,
              LogisticsEvents& ev) {
    CityState& city = st.city;
    const Building& h = city.buildings.items.at(home);
    const Building& t = city.buildings.items.at(target);
    const std::string type = worker_type_of(catalog, h.type);
    if (type.empty()) {
        return kNoWalker;
    }
    const BuildingId goal = t.tiles.empty() ? kNoBuilding : target;
    const WalkerId w = spawn_destination_walker(city, st.walkers, catalog, type, home, t.origin, PathMode::Walkable, goal);
    if (w != kNoWalker) {
        Walker& walker = st.walkers.items.at(w);
        walker.job = job;
        walker.target = target;
        city.buildings.items.at(home).producer->field_workers.push_back(w);
        ev.spawned.push_back(w);
    }
    return w;
}

}  // namespace

std::uint32_t processor_room_loads(const Building& b, const DataCatalog& catalog, const std::string& good) {
    const auto it = catalog.production.find(b.type);
    if (!b.producer || it == catalog.production.end() || b.staffing_percent == 0) {
        return 0;
    }
    const ProductionParams& p = it->second;
    const auto units = [&b](const std::string& g) {
        const auto u = b.producer->units.find(g);
        return u == b.producer->units.end() ? 0U : u->second;
    };
    if (!p.input_units_good.empty()) {  // horse ranch: wheat kept in units (doc 19)
        const auto cap = static_cast<std::uint32_t>(p.input_units_capacity);
        return good == p.input_units_good && units(good) < cap ? (cap - units(good)) / 100 : 0;
    }
    if (!p.inputs_multi.empty()) {  // chariot factory: wood and horses in pieces
        const auto cap = static_cast<std::uint32_t>(p.input_capacity);
        return p.inputs_multi.count(good) != 0 && units(good) < cap ? cap - units(good) : 0;
    }
    const auto cap = static_cast<std::uint32_t>(p.input_capacity);
    return p.input_good == good && b.producer->input_loads < cap ? cap - b.producer->input_loads : 0;
}

bool processor_accepts(const Building& b, const DataCatalog& catalog, const std::string& good) {
    return processor_room_loads(b, catalog, good) > 0;
}

void processor_add(Building& b, const DataCatalog& catalog, const std::string& good, std::uint32_t loads) {
    const ProductionParams& p = catalog.production.at(b.type);
    if (!p.input_units_good.empty()) {
        b.producer->units[good] += loads * 100;
    } else if (!p.inputs_multi.empty()) {
        b.producer->units[good] += loads;
    } else {
        b.producer->input_loads += loads;
    }
}

void advance_husbandry(WorldState& st, const DataCatalog& catalog) {
    const std::uint32_t month = st.time.month();
    const bool month_start = st.time.day_in_month() == 0;
    for (auto& [id, b] : st.city.buildings.items) {
        if (b.animal && b.animal->days_to_ready > 0) {
            --b.animal->days_to_ready;
        }
        if (b.orchard) {
            OrchardState& o = *b.orchard;
            const std::uint32_t speed = o.days_since_tended >= kNoGrowthDays    ? 0
                                        : o.days_since_tended >= kHalfGrowthDays ? 50
                                                                                  : 100;
            o.growth = std::min(o.growth + speed, kFullGrowth);
            o.days_since_tended = static_cast<std::uint16_t>(std::min(o.days_since_tended + 1, 1000));
            std::string lodge;
            const auto fruit = fruit_of(catalog, b.type, &lodge);
            const auto params = catalog.production.find(lodge);
            if (month_start && fruit && params != catalog.production.end()) {
                const auto& months = params->second.harvest_months.find(*fruit);
                // First day after the season: unpicked fruit is lost (Z:42); weak plants carry over (doc 19).
                if (months != params->second.harvest_months.end() && !months->second.empty() &&
                    month == static_cast<std::uint32_t>((months->second.back() + 1) % 12) &&
                    o.growth / kDaysPerYear >= kCarryOverPercent) {
                    o.growth = 0;
                }
            }
        }
        const auto params = catalog.production.find(b.type);
        if (b.producer && params != catalog.production.end() && !params->second.input_units_good.empty()) {
            // Horse ranch (doc 19): 50 wheat a horse, 6 a year, at most 6 waiting, each eats 10 a month.
            const ProductionParams& pp = params->second;
            ProducerState& p = *b.producer;
            auto& wheat = p.units[pp.input_units_good];
            if (st.time.day_in_month() == 0) {
                wheat -= std::min(wheat, static_cast<std::uint32_t>(pp.upkeep_units_per_month) * p.output_loads);
            }
            if (wheat >= static_cast<std::uint32_t>(pp.input_units_per_output) &&
                p.output_loads < static_cast<std::uint32_t>(pp.storage_loads)) {
                p.progress += static_cast<std::uint32_t>(pp.loads_per_year) * b.staffing_percent;
                if (p.progress >= 100 * kDaysPerYear) {
                    p.progress -= 100 * kDaysPerYear;
                    wheat -= static_cast<std::uint32_t>(pp.input_units_per_output);
                    ++p.output_loads;
                }
            }
        } else if (b.producer && params != catalog.production.end() && !params->second.inputs_multi.empty()) {
            // Chariot factory (P:18): one wood and one horse per chariot, 6 a year.
            const ProductionParams& pp = params->second;
            ProducerState& p = *b.producer;
            const bool ready = std::all_of(pp.inputs_multi.begin(), pp.inputs_multi.end(), [&p](const auto& in) {
                return p.units[in.first] >= static_cast<std::uint32_t>(in.second);
            });
            if (ready) {
                p.progress += static_cast<std::uint32_t>(pp.loads_per_year) * b.staffing_percent;
                if (p.progress >= 100 * kDaysPerYear) {
                    p.progress -= 100 * kDaysPerYear;
                    for (const auto& [g, n] : pp.inputs_multi) {
                        p.units[g] -= static_cast<std::uint32_t>(n);
                    }
                    ++p.output_loads;
                }
            }
        } else if (b.producer && params != catalog.production.end() && !params->second.input_good.empty()) {
            ProducerState& p = *b.producer;
            const auto per_output = static_cast<std::uint32_t>(params->second.input_per_output);
            if (p.input_loads >= per_output) {  // a sculpture studio waits for 4 bronze (doc 19)
                p.progress += static_cast<std::uint32_t>(params->second.loads_per_year) * b.staffing_percent;
                if (p.progress >= 100 * kDaysPerYear) {  // 6 loads a year at full staffing (doc 19)
                    p.progress -= 100 * kDaysPerYear;
                    p.input_loads -= per_output;
                    ++p.output_loads;
                }
            }
        }
    }
}

void dispatch_field_workers(WorldState& st, const DataCatalog& catalog, LogisticsEvents& ev) {
    auto& items = st.city.buildings.items;
    const std::uint32_t month = st.time.month();
    std::vector<BuildingId> ids;
    for (const auto& [id, b] : items) {
        if (b.producer && b.road_access && b.staffing_percent > 0) {
            ids.push_back(id);
        }
    }
    for (const BuildingId id : ids) {
        Building& b = items.at(id);
        auto& out = b.producer->field_workers;
        out.erase(std::remove_if(out.begin(), out.end(), [&st](WalkerId w) { return !alive(st, w); }), out.end());
        const auto params = catalog.production.find(b.type);
        if (params == catalog.production.end()) {
            continue;
        }
        const ProductionParams& pp = params->second;
        const BuildingType& type = catalog.buildings.at(b.type);
        if (pp.animals_max > 0 && out.empty() && b.producer->output_loads < static_cast<std::uint32_t>(pp.storage_loads)) {
            std::optional<std::pair<std::int32_t, BuildingId>> best;  // nearest ready animal of this shed
            for (const auto& [aid, a] : items) {
                if (a.animal && a.animal->shed == id && a.animal->days_to_ready == 0 && !a.animal->worker_coming) {
                    const std::int32_t d = chebyshev(a.origin, b.origin);
                    if (!best || d < best->first) {
                        best = std::make_pair(d, aid);
                    }
                }
            }
            if (best && send(st, catalog, id, best->second, WalkerJob::Shear, ev) != kNoWalker) {
                items.at(best->second).animal->worker_coming = true;
            }
        }
        if (type.tends.empty()) {
            continue;
        }
        bool pruner_out = false;
        for (const WalkerId w : out) {
            pruner_out = pruner_out || st.walkers.items.at(w).job == WalkerJob::Tend;
        }
        if (st.time.day_in_month() == 0 && !pruner_out) {  // one pruner a month (buildings.json, Z:42)
            std::optional<std::pair<std::uint16_t, BuildingId>> oldest;
            for (const auto& [tid, t] : items) {
                if (t.orchard && std::find(type.tends.begin(), type.tends.end(), t.type) != type.tends.end() &&
                    (!oldest || t.orchard->days_since_tended > oldest->first)) {
                    oldest = std::make_pair(t.orchard->days_since_tended, tid);
                }
            }
            if (oldest) {
                send(st, catalog, id, oldest->second, WalkerJob::Tend, ev);
            }
        }
        for (std::size_t i = 0; i < type.tends.size() && i < type.outputs.size(); ++i) {
            const std::string& fruit = type.outputs[i];
            const auto months = pp.harvest_months.find(fruit);
            const std::uint32_t cap = static_cast<std::uint32_t>(pp.storage_loads) * 100;
            if (months == pp.harvest_months.end() || !in_months(months->second, month) || b.producer->units[fruit] >= cap) {
                continue;
            }
            while (std::count_if(out.begin(), out.end(), [&st](WalkerId w) {
                       return st.walkers.items.at(w).job == WalkerJob::Harvest;
                   }) < static_cast<std::ptrdiff_t>(kHarvesters)) {
                std::optional<std::pair<std::int32_t, BuildingId>> best;  // nearest ripe plant, any distance
                for (const auto& [tid, t] : items) {
                    // Pick only plants ripe enough to be lost after the season [ZAŁOŻENIE, L-101].
                    if (t.type == type.tends[i] && t.orchard && !t.orchard->worker_coming &&
                        t.orchard->growth / kDaysPerYear >= kCarryOverPercent) {
                        const std::int32_t d = chebyshev(t.origin, b.origin);
                        if (!best || d < best->first) {
                            best = std::make_pair(d, tid);
                        }
                    }
                }
                if (!best || send(st, catalog, id, best->second, WalkerJob::Harvest, ev) == kNoWalker) {
                    break;
                }
                items.at(best->second).orchard->worker_coming = true;
            }
        }
    }
}

void field_work_at_target(WorldState& st, const DataCatalog& catalog, Walker& w) {
    auto& items = st.city.buildings.items;
    const auto target = items.find(w.target);
    const auto home = items.find(w.home);
    if (target == items.end() || home == items.end()) {
        return;
    }
    Building& t = target->second;
    const BuildingType& home_type = catalog.buildings.at(home->second.type);
    if (w.job == WalkerJob::Shear && t.animal) {
        t.animal->worker_coming = false;
        t.animal->days_to_ready = kDaysPerYear;  // one load a year per animal (doc 19)
        w.cargo[home_type.outputs.front()] = 1;
    } else if (w.job == WalkerJob::Tend) {
        for (const BuildingId id : buildings_in_square(st.city, t.origin, kPrunerRadius)) {
            if (items.at(id).orchard) {
                items.at(id).orchard->days_since_tended = 0;
            }
        }
    } else if (w.job == WalkerJob::Harvest && t.orchard) {
        const auto fruit = fruit_of(catalog, t.type, nullptr);
        if (fruit) {
            w.cargo[*fruit] = t.orchard->growth / kDaysPerYear;  // units = ripeness percent (doc 19)
        }
        t.orchard->growth = 0;
        t.orchard->worker_coming = false;
    }
}

void field_work_unload(WorldState& st, const DataCatalog& catalog, const Walker& w, Building& home) {
    const auto params = catalog.production.find(home.type);
    const std::uint32_t cap =
        params == catalog.production.end() ? 0 : static_cast<std::uint32_t>(params->second.storage_loads);
    if (w.job == WalkerJob::Shear) {
        for (const auto& [good, loads] : w.cargo) {
            home.producer->output_loads = std::min(home.producer->output_loads + loads, cap);
        }
    } else if (w.job == WalkerJob::Harvest) {
        for (const auto& [fruit, units] : w.cargo) {
            home.producer->units[fruit] = std::min(home.producer->units[fruit] + units, cap * 100);
        }
    }
    (void)st;
}

}  // namespace bo::core
