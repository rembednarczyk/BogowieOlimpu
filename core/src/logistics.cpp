#include "bo/core/logistics.h"

#include <algorithm>
#include <cstdlib>
#include <optional>

#include "bo/core/housing.h"
#include "bo/core/path.h"
#include "bo/core/storage.h"
#include "bo/core/walker.h"

namespace bo::core {
namespace {

constexpr std::uint32_t kDaysPerYear = 192;

std::optional<HouseGood> house_good_of(const DataCatalog& catalog, const std::string& good) {
    const auto cls = catalog.goods.find(good);
    if (cls != catalog.goods.end() && cls->second == "food") {
        return HouseGood::Food;
    }
    if (good == "fleece") {
        return HouseGood::Fleece;
    }
    if (good == "olive_oil") {
        return HouseGood::OliveOil;
    }
    if (good == "wine") {
        return HouseGood::Wine;
    }
    if (good == "armor") {
        return HouseGood::Armor;
    }
    if (good == "horses") {
        return HouseGood::Horses;
    }
    return std::nullopt;
}

bool alive(const WorldState& st, WalkerId id) { return id != kNoWalker && st.walkers.items.count(id) != 0; }

// "granary" matches Granary; "storehouse_accepting_wheat" matches Storehouse (acceptance is checked apart).
bool category_matches(const std::string& category, const std::string& type) {
    return category == type || category.rfind(type + "_", 0) == 0;
}

std::string walker_type_from(const DataCatalog& catalog, const std::string& building_type) {
    for (const auto& [id, w] : catalog.walkers) {
        if (std::find(w.from.begin(), w.from.end(), building_type) != w.from.end()) {
            return id;
        }
    }
    return {};
}

std::uint32_t stall_units(const DataCatalog& catalog, const Building& stall) {
    const auto& sells = catalog.buildings.at(stall.type).sells;
    std::uint32_t total = 0;
    for (const auto& [good, units] : stall.stall->units) {
        total += good_matches(catalog, good, sells) ? units : 0;
    }
    return total;
}

void dispatch_deliveryman(WorldState& st, const DataCatalog& catalog, BuildingId id, LogisticsEvents& ev) {
    CityState& city = st.city;
    Building& b = city.buildings.items.at(id);
    const BuildingType& type = catalog.buildings.at(b.type);
    const std::string& good = type.outputs.front();
    const auto params = catalog.production.find(b.type);
    const auto reach = static_cast<std::uint32_t>(params == catalog.production.end() ? 300 : params->second.max_storage_distance);
    const auto dist = road_distances(city, {*b.road_access}, PathMode::Road);
    for (const auto& category : type.delivery_priority) {
        std::optional<std::pair<std::uint32_t, BuildingId>> best;
        for (const auto& [rid, r] : city.buildings.items) {
            if (!r.road_access || !category_matches(category, r.type) || !storage_accepts(r, catalog, good)) {
                continue;
            }
            const std::uint32_t d = dist[city.map.index(*r.road_access)];
            if (d <= reach && (!best || d < best->first)) {
                best = std::make_pair(d, rid);
            }
        }
        if (!best) {
            continue;  // trade, processors and horse ranches join in stage 2
        }
        const TilePos to = *city.buildings.items.at(best->second).road_access;
        const WalkerId w = spawn_destination_walker(city, st.walkers, catalog, "deliveryman", id, to);
        if (w == kNoWalker) {
            return;
        }
        Walker& walker = st.walkers.items.at(w);
        walker.job = WalkerJob::Deliver;
        walker.target = best->second;
        walker.cargo[good] = 1;  // a production deliveryman carries one load (doc 19)
        b.producer->output_loads -= 1;
        b.producer->deliveryman = w;
        ev.spawned.push_back(w);
        return;
    }
}

void dispatch_vendor(WorldState& st, const DataCatalog& catalog, BuildingId id, LogisticsEvents& ev) {
    CityState& city = st.city;
    Building& stall = city.buildings.items.at(id);
    const BuildingType& type = catalog.buildings.at(stall.type);
    const std::string walker_type = walker_type_from(catalog, stall.type);
    if (walker_type.empty() || !stall.road_access) {
        return;
    }
    const WalkerType& wt = catalog.walkers.at(walker_type);
    const std::int32_t reach = wt.max_source_distance.value_or(kVendorReach);
    const TilePos from = *stall.road_access;
    const auto dist = road_distances(city, {from}, PathMode::Road);
    std::optional<std::pair<std::uint32_t, BuildingId>> best;
    for (const auto& [sid, s] : city.buildings.items) {
        if (!s.storage || !s.road_access ||
            std::find(type.sources.begin(), type.sources.end(), s.type) == type.sources.end()) {
            continue;
        }
        const TilePos p = *s.road_access;
        if (std::max(std::abs(p.x - from.x), std::abs(p.y - from.y)) > reach) {
            continue;  // too distant (doc 06 status, doc 24: 40 tiles, Zeus measured 37)
        }
        bool has_goods = false;
        for (const auto& [good, loads] : s.storage->loads) {
            has_goods = has_goods || (loads > 0 && good_matches(catalog, good, type.sells));
        }
        const std::uint32_t d = dist[city.map.index(p)];
        if (has_goods && d != kUnreachable && (!best || d < best->first)) {
            best = std::make_pair(d, sid);
        }
    }
    if (!best) {
        return;
    }
    const WalkerId w = spawn_destination_walker(city, st.walkers, catalog, walker_type, id,
                                                *city.buildings.items.at(best->second).road_access);
    if (w == kNoWalker) {
        return;
    }
    Walker& walker = st.walkers.items.at(w);
    walker.job = WalkerJob::Fetch;
    walker.target = best->second;
    stall.stall->vendor = w;
    ev.spawned.push_back(w);
}

}  // namespace

std::uint32_t house_target(const DataCatalog& catalog, const HouseState& h, HouseGood g, Difficulty d) {
    const auto& levels = levels_of(catalog, h);
    const auto di = static_cast<std::size_t>(d);
    const auto gi = static_cast<std::size_t>(g);
    const HousingLevel& cur = levels[h.level];
    const HousingLevel* next = h.level + 1U < levels.size() ? &levels[h.level + 1U] : nullptr;
    const bool needed = cur.model[di].needs[gi] || (next && next->model[di].needs[gi]);
    if (!needed) {
        return 0;
    }
    if (g == HouseGood::Food) {
        const std::int32_t monthly = std::max(cur.consumption[0], next ? next->consumption[0] : 0);
        return static_cast<std::uint32_t>(2 * std::max(monthly, 1));
    }
    return g == HouseGood::Armor || g == HouseGood::Horses ? 1U : 4U;
}

void advance_production(WorldState& st, const DataCatalog& catalog) {
    const std::uint32_t month = st.time.month();
    const bool month_start = st.time.day_in_month() == 0;
    for (auto& [id, b] : st.city.buildings.items) {
        const auto params = catalog.production.find(b.type);
        if (!b.producer || params == catalog.production.end() || !params->second.harvest_month) {
            continue;
        }
        const auto lpy = static_cast<std::uint32_t>(params->second.loads_per_year);
        ProducerState& p = *b.producer;
        if (month_start && month == static_cast<std::uint32_t>(*params->second.harvest_month)) {
            p.output_loads += p.progress / (100 * kDaysPerYear);  // the yearly harvest (Z:39-40)
            p.progress = 0;
        }
        p.progress = std::min(p.progress + lpy * b.staffing_percent, lpy * 100 * kDaysPerYear);
    }
}

LogisticsEvents dispatch_workers(WorldState& st, const DataCatalog& catalog) {
    LogisticsEvents ev;
    std::vector<BuildingId> producers;
    std::vector<BuildingId> stalls;
    for (auto& [id, b] : st.city.buildings.items) {
        if (b.producer && b.producer->output_loads > 0 && b.road_access && !alive(st, b.producer->deliveryman)) {
            producers.push_back(id);
        }
        if (b.stall && b.staffing_percent > 0 && !alive(st, b.stall->vendor) &&
            stall_units(catalog, b) < kStallRefillUnits) {
            stalls.push_back(id);
        }
    }
    for (const BuildingId id : producers) {
        dispatch_deliveryman(st, catalog, id, ev);
    }
    for (const BuildingId id : stalls) {
        dispatch_vendor(st, catalog, id, ev);
    }
    return ev;
}

void handle_arrivals(WorldState& st, const DataCatalog& catalog) {
    for (auto& [wid, w] : st.walkers.items) {
        if (w.phase != WalkerPhase::AtTarget) {
            continue;
        }
        const auto target = st.city.buildings.items.find(w.target);
        if (target != st.city.buildings.items.end() && target->second.storage) {
            Building& s = target->second;
            if (w.job == WalkerJob::Deliver) {
                for (auto it = w.cargo.begin(); it != w.cargo.end();) {
                    if (storage_accepts(s, catalog, it->first)) {
                        s.storage->loads[it->first] += it->second;
                        it = w.cargo.erase(it);
                    } else {
                        ++it;  // no longer accepted: the load goes back home
                    }
                }
            } else if (w.job == WalkerJob::Fetch) {
                const auto home = st.city.buildings.items.find(w.home);
                const auto& sells = home == st.city.buildings.items.end()
                                        ? std::vector<std::string>{}
                                        : catalog.buildings.at(home->second.type).sells;
                std::uint32_t taken = 0;
                for (auto& [good, loads] : s.storage->loads) {
                    if (!good_matches(catalog, good, sells)) {
                        continue;
                    }
                    const std::uint32_t n = std::min(loads, kVendorLoads - taken);
                    loads -= n;
                    taken += n;
                    if (n > 0) {
                        w.cargo[good] += n;
                    }
                }
                for (auto it = s.storage->loads.begin(); it != s.storage->loads.end();) {
                    it = it->second == 0 ? s.storage->loads.erase(it) : std::next(it);
                }
            }
        }
        send_home(st.city, w);
    }
}

void unload_at_home(WorldState& st, const DataCatalog& catalog, const Walker& w) {
    const auto home = st.city.buildings.items.find(w.home);
    if (w.job == WalkerJob::None || home == st.city.buildings.items.end()) {
        return;
    }
    Building& b = home->second;
    if (w.job == WalkerJob::Deliver && b.producer) {
        for (const auto& [good, loads] : w.cargo) {
            b.producer->output_loads += loads;  // undelivered load back to the producer
        }
        b.producer->deliveryman = kNoWalker;
    } else if (w.job == WalkerJob::Fetch && b.stall) {
        for (const auto& [good, loads] : w.cargo) {
            b.stall->units[good] += loads * static_cast<std::uint32_t>(catalog.units_per_load);
        }
        b.stall->vendor = kNoWalker;
    }
}

void peddler_sells(WorldState& st, const DataCatalog& catalog, const Walker& w) {
    auto& items = st.city.buildings.items;
    const auto agora = items.find(w.home);
    if (agora == items.end() || agora->second.slots.empty()) {
        return;
    }
    const std::vector<BuildingId> stalls = agora->second.slots;
    for (const BuildingId hid : buildings_in_square(st.city, w.tile, kServiceRadius)) {
        Building& hb = items.at(hid);
        if (!hb.house || hb.house->population == 0 || hb.house->abandoned) {
            continue;
        }
        HouseState& h = *hb.house;
        for (std::size_t gi = 0; gi < kHouseGoods; ++gi) {
            const auto g = static_cast<HouseGood>(gi);
            const std::uint32_t target = house_target(catalog, h, g, st.difficulty);
            for (const BuildingId sid : stalls) {
                if (sid == kNoBuilding || h.goods[gi] >= target || !items.at(sid).stall) {
                    continue;
                }
                for (auto& [good, units] : items.at(sid).stall->units) {
                    if (house_good_of(catalog, good) != g || h.goods[gi] >= target) {
                        continue;
                    }
                    const std::uint32_t n = std::min(units, target - h.goods[gi]);
                    units -= n;
                    h.goods[gi] += n;
                }
            }
        }
    }
}

}  // namespace bo::core
