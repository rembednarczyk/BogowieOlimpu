#include "bo/core/logistics.h"

#include <algorithm>
#include <cstdlib>
#include <optional>

#include "bo/core/housing.h"
#include "bo/core/path.h"
#include "bo/core/storage.h"
#include "bo/core/trade.h"
#include "bo/core/walker.h"

namespace bo::core {
namespace {

constexpr std::uint32_t kDaysPerYear = 192;

bool alive(const WorldState& st, WalkerId id) { return id != kNoWalker && st.walkers.items.count(id) != 0; }

// "granary" matches Granary; "storehouse_accepting_wheat" matches Storehouse (acceptance is checked apart).
bool category_matches(const std::string& category, const std::string& type) {
    return category == type || category.rfind(type + "_", 0) == 0;
}

}  // namespace

std::string walker_type_from(const DataCatalog& catalog, const std::string& building_type) {
    for (const auto& [id, w] : catalog.walkers) {
        if (std::find(w.from.begin(), w.from.end(), building_type) != w.from.end()) {
            return id;
        }
    }
    return {};
}

namespace {

std::uint32_t stall_units(const DataCatalog& catalog, const Building& stall) {
    const auto& sells = catalog.buildings.at(stall.type).sells;
    std::uint32_t total = 0;
    for (const auto& [good, units] : stall.stall->units) {
        total += good_matches(catalog, good, sells) ? units : 0;
    }
    return total;
}

bool receiver_accepts(const Building& r, const DataCatalog& catalog, const std::string& good) {
    return storage_accepts(r, catalog, good) || processor_accepts(r, catalog, good) || trade_accepts(r, catalog, good);
}

// The good a producer ships next: its single output, or a lodge fruit with a full load (100 units).
std::optional<std::string> next_shipment(const Building& b, const BuildingType& type) {
    if (type.pickup_only) {
        return std::nullopt;  // horses and chariots wait for the hostler or the vendor (doc 06)
    }
    if (b.producer->output_loads > 0 && type.outputs.size() == 1 && type.tends.empty()) {
        return type.outputs.front();
    }
    for (const auto& [fruit, units] : b.producer->units) {
        if (units >= 100) {
            return fruit;
        }
    }
    return std::nullopt;
}

void dispatch_deliveryman(WorldState& st, const DataCatalog& catalog, BuildingId id, LogisticsEvents& ev) {
    CityState& city = st.city;
    Building& b = city.buildings.items.at(id);
    const BuildingType& type = catalog.buildings.at(b.type);
    const auto shipment = next_shipment(b, type);
    if (!shipment) {
        return;
    }
    const std::string good = *shipment;
    const auto by_good = type.delivery_priority_by_good.find(good);
    const auto& priority = by_good != type.delivery_priority_by_good.end() ? by_good->second : type.delivery_priority;
    const auto params = catalog.production.find(b.type);
    const auto reach = static_cast<std::uint32_t>(params == catalog.production.end() ? 300 : params->second.max_storage_distance);
    const auto dist = road_distances(city, {*b.road_access}, PathMode::Road);
    for (const auto& category : priority) {
        std::optional<std::pair<std::uint32_t, BuildingId>> best;
        for (const auto& [rid, r] : city.buildings.items) {
            const bool kind = category == "trade_exporting" ? r.trade.has_value() : category_matches(category, r.type);
            if (!r.road_access || !kind || !receiver_accepts(r, catalog, good)) {
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
        if (type.outputs.size() == 1 && type.tends.empty()) {
            b.producer->output_loads -= 1;
        } else {
            b.producer->units[good] -= 100;
        }
        b.producer->deliveryman = w;
        ev.spawned.push_back(w);
        return;
    }
}

}  // namespace

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

// Storage deliveryman (up to 4 loads, doc 19) takes goods a processor needs to the nearest one in reach
// [ZAŁOŻENIE, L-101: the processor's delivery reach].
WalkerId send_storage_walker(WorldState& st, const DataCatalog& catalog, BuildingId from, BuildingId to,
                             WalkerJob job, LogisticsEvents& ev) {
    CityState& city = st.city;
    const WalkerId w =
        spawn_destination_walker(city, st.walkers, catalog, "deliveryman", from, *city.buildings.items.at(to).road_access);
    if (w != kNoWalker) {
        Walker& walker = st.walkers.items.at(w);
        walker.job = job;
        walker.target = to;
        city.buildings.items.at(from).storage->deliveryman = w;
        ev.spawned.push_back(w);
    }
    return w;
}

// Nearest other storage by road for which `ok` holds (ties: lower id).
template <class Pred>
std::optional<BuildingId> nearest_storage(const CityState& city, BuildingId self, const std::vector<std::uint32_t>& dist,
                                          Pred ok) {
    std::optional<std::pair<std::uint32_t, BuildingId>> best;
    for (const auto& [rid, r] : city.buildings.items) {
        if (rid == self || !r.storage || !r.road_access || !ok(r)) {
            continue;
        }
        const std::uint32_t d = dist[city.map.index(*r.road_access)];
        if (d != kUnreachable && (!best || d < best->first)) {
            best = std::make_pair(d, rid);
        }
    }
    return best ? std::optional<BuildingId>(best->second) : std::nullopt;
}

// Empty and Get orders (Z:66-68): only at full staffing; Empty first, then processors, then Get.
bool dispatch_orders(WorldState& st, const DataCatalog& catalog, BuildingId id, LogisticsEvents& ev,
                     const std::vector<std::uint32_t>& dist, bool empty_pass) {
    CityState& city = st.city;
    Building& s = city.buildings.items.at(id);
    const BuildingType& type = catalog.buildings.at(s.type);
    if (s.staffing_percent < 100) {
        return false;
    }
    for (const auto& [good_key, order] : s.storage->orders) {
        const std::string& good = good_key;  // named copy: lambdas cannot capture structured bindings in C++17
        const auto have_it = s.storage->loads.find(good);
        const std::uint32_t have = have_it == s.storage->loads.end() ? 0 : have_it->second;
        if (empty_pass && order.kind == StorageOrderKind::Empty && have > 0) {
            const auto to = nearest_storage(city, id, dist, [&](const Building& r) {
                return storage_accepts(r, catalog, good) &&
                       storage_order(r, catalog.buildings.at(r.type), good).kind != StorageOrderKind::Empty;
            });
            if (!to) {
                continue;
            }
            const std::uint32_t n = std::min(have, kVendorLoads);
            if (send_storage_walker(st, catalog, id, *to, WalkerJob::Deliver, ev) != kNoWalker) {
                st.walkers.items.at(s.storage->deliveryman).cargo[good] = n;
                s.storage->loads[good] -= n;
                return true;
            }
        }
        if (!empty_pass && order.kind == StorageOrderKind::Get && have < order.limit &&
            storage_room(s, catalog, good) > 0) {
            const auto from = nearest_storage(city, id, dist, [&](const Building& r) {
                const auto it = r.storage->loads.find(good);
                return !r.trade && it != r.storage->loads.end() && it->second > 0 &&
                       storage_order(r, catalog.buildings.at(r.type), good).kind != StorageOrderKind::Get;
            });
            if (!from) {
                continue;
            }
            if (send_storage_walker(st, catalog, id, *from, WalkerJob::Fetch, ev) != kNoWalker) {
                Walker& w = st.walkers.items.at(s.storage->deliveryman);
                w.want_good = good;
                w.want_loads = std::min({kVendorLoads, order.limit - have, storage_room(s, catalog, good)});
                return true;
            }
        }
    }
    (void)type;
    return false;
}

// Trade building deliverymen (Z:78-79, 4 loads, doc 19): imported goods go to a processor that needs them or to
// a storage that takes them; goods for export are fetched from storages up to the amount kept for sale.
void dispatch_trade_deliveryman(WorldState& st, const DataCatalog& catalog, BuildingId id, LogisticsEvents& ev,
                                const std::vector<std::uint32_t>& dist) {
    CityState& city = st.city;
    Building& s = city.buildings.items.at(id);
    for (const auto& [good_key, order] : s.trade->orders) {
        const std::string& good = good_key;
        const auto have_it = s.storage->loads.find(good);
        const std::uint32_t have = have_it == s.storage->loads.end() ? 0 : have_it->second;
        if (order.import_on && have > 0) {
            std::optional<std::pair<std::uint32_t, BuildingId>> best;
            for (const auto& [rid, r] : city.buildings.items) {
                if (rid == id || r.trade || !r.road_access ||
                    !(processor_accepts(r, catalog, good) || storage_accepts(r, catalog, good))) {
                    continue;
                }
                const std::uint32_t d = dist[city.map.index(*r.road_access)];
                if (d != kUnreachable && (!best || d < best->first)) {
                    best = std::make_pair(d, rid);
                }
            }
            if (best && send_storage_walker(st, catalog, id, best->second, WalkerJob::Deliver, ev) != kNoWalker) {
                const std::uint32_t n = std::min(have, catalog.trade.deliveryman_loads);
                st.walkers.items.at(s.storage->deliveryman).cargo[good] = n;
                s.storage->loads[good] -= n;
                return;
            }
        }
        if (order.export_on && have < order.export_keep) {
            const auto from = nearest_storage(city, id, dist, [&](const Building& r) {
                const auto it = r.storage->loads.find(good);
                return !r.trade && it != r.storage->loads.end() && it->second > 0;
            });
            if (from && send_storage_walker(st, catalog, id, *from, WalkerJob::Fetch, ev) != kNoWalker) {
                Walker& w = st.walkers.items.at(s.storage->deliveryman);
                w.want_good = good;
                w.want_loads = std::min(catalog.trade.deliveryman_loads, order.export_keep - have);
                return;
            }
        }
    }
}

void dispatch_storage_push(WorldState& st, const DataCatalog& catalog, BuildingId id, LogisticsEvents& ev) {
    CityState& city = st.city;
    Building& s = city.buildings.items.at(id);
    const auto dist = road_distances(city, {*s.road_access}, PathMode::Road);
    if (s.trade) {
        dispatch_trade_deliveryman(st, catalog, id, ev, dist);
        return;
    }
    if (dispatch_orders(st, catalog, id, ev, dist, true)) {
        return;
    }
    for (const auto& [good, loads] : s.storage->loads) {
        if (loads == 0) {
            continue;
        }
        std::optional<std::pair<std::uint32_t, BuildingId>> best;
        for (const auto& [pid, p] : city.buildings.items) {
            const auto params = catalog.production.find(p.type);
            const auto& never = catalog.buildings.at(p.type).never_from;
            if (!p.road_access || !processor_accepts(p, catalog, good) || params == catalog.production.end() ||
                std::find(never.begin(), never.end(), s.type) != never.end()) {
                continue;  // e.g. a horse ranch never takes wheat from a Granary (Z:148)
            }
            const std::uint32_t d = dist[city.map.index(*p.road_access)];
            if (d <= static_cast<std::uint32_t>(params->second.max_storage_distance) && (!best || d < best->first)) {
                best = std::make_pair(d, pid);
            }
        }
        if (!best) {
            continue;
        }
        const Building& p = city.buildings.items.at(best->second);
        const std::uint32_t n = std::min({loads, kVendorLoads, processor_room_loads(p, catalog, good)});
        const WalkerId w = spawn_destination_walker(city, st.walkers, catalog, "deliveryman", id, *p.road_access);
        if (w == kNoWalker) {
            return;
        }
        Walker& walker = st.walkers.items.at(w);
        walker.job = WalkerJob::Deliver;
        walker.target = best->second;
        walker.cargo[good] = n;
        s.storage->loads[good] -= n;
        s.storage->deliveryman = w;
        ev.spawned.push_back(w);
        return;
    }
    dispatch_orders(st, catalog, id, ev, dist, false);
}

LogisticsEvents dispatch_workers(WorldState& st, const DataCatalog& catalog) {
    LogisticsEvents ev;
    dispatch_field_workers(st, catalog, ev);
    dispatch_extractors(st, catalog, ev);
    dispatch_hostlers(st, catalog, ev);
    dispatch_hunters_and_boats(st, catalog, ev);
    std::vector<BuildingId> producers;
    std::vector<BuildingId> stalls;
    std::vector<BuildingId> storages;
    for (auto& [id, b] : st.city.buildings.items) {
        if (b.producer && b.road_access && !alive(st, b.producer->deliveryman) &&
            next_shipment(b, catalog.buildings.at(b.type))) {
            producers.push_back(id);
        }
        if (b.storage && b.road_access && b.staffing_percent > 0 && !alive(st, b.storage->deliveryman)) {
            storages.push_back(id);
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
    for (const BuildingId id : storages) {
        dispatch_storage_push(st, catalog, id, ev);
    }
    return ev;
}

void handle_arrivals(WorldState& st, const DataCatalog& catalog) {
    for (auto& [wid, w] : st.walkers.items) {
        if (w.phase != WalkerPhase::AtTarget || w.job == WalkerJob::Crime || w.job == WalkerJob::Extinguish ||
            w.job == WalkerJob::Trade || w.job == WalkerJob::Settle || w.job == WalkerJob::Prowl) {
            continue;
        }
        const auto target = st.city.buildings.items.find(w.target);
        if (w.job == WalkerJob::Extract) {
            if (extraction_at_target(st, catalog, w)) {
                continue;  // still working at the rock or tree
            }
        } else if (w.job == WalkerJob::Hunt || w.job == WalkerJob::Fish) {
            if (wildlife_at_target(st, catalog, w)) {
                continue;  // still hunting or fishing
            }
        } else if (w.job == WalkerJob::Shear || w.job == WalkerJob::Tend || w.job == WalkerJob::Harvest) {
            field_work_at_target(st, catalog, w);
        } else if (w.job == WalkerJob::Deliver && target != st.city.buildings.items.end() && target->second.producer) {
            Building& p = target->second;
            for (auto it = w.cargo.begin(); it != w.cargo.end();) {
                const std::uint32_t n = std::min(it->second, processor_room_loads(p, catalog, it->first));
                if (n > 0) {
                    processor_add(p, catalog, it->first, n);
                    it->second -= n;
                }
                it = it->second == 0 ? w.cargo.erase(it) : std::next(it);
            }
        } else if (w.job == WalkerJob::Fetch && target != st.city.buildings.items.end() && target->second.producer) {
            fetch_from_producer(st, catalog, w, target->second);
        } else if (target != st.city.buildings.items.end() && target->second.storage) {
            Building& s = target->second;
            if (w.job == WalkerJob::Deliver) {
                for (auto it = w.cargo.begin(); it != w.cargo.end();) {
                    if (storage_accepts(s, catalog, it->first) || trade_accepts(s, catalog, it->first)) {
                        s.storage->loads[it->first] += it->second;
                        it = w.cargo.erase(it);
                    } else {
                        ++it;  // no longer accepted: the load goes back home
                    }
                }
            } else if (w.job == WalkerJob::Fetch && !w.want_good.empty()) {
                auto& loads = s.storage->loads[w.want_good];  // Get order: take what was asked for
                const std::uint32_t n = std::min(loads, w.want_loads);
                loads -= n;
                if (n > 0) {
                    w.cargo[w.want_good] += n;
                }
            } else if (w.job == WalkerJob::Fetch) {
                const auto home = st.city.buildings.items.find(w.home);
                const auto& sells = home == st.city.buildings.items.end()
                                        ? std::vector<std::string>{}
                                        : catalog.buildings.at(home->second.type).sells;
                std::uint32_t taken = 0;
                for (auto& [good, loads] : s.storage->loads) {
                    if (!good_matches(catalog, good, sells) || (s.trade && !trade_offers(s, good))) {
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
    if ((w.job == WalkerJob::Hunt || w.job == WalkerJob::Fish) && b.producer) {
        wildlife_unload(st, catalog, w, b);
    } else if (w.job == WalkerJob::Extract && b.producer) {
        extraction_unload(st, catalog, w, b);
    } else if ((w.job == WalkerJob::Shear || w.job == WalkerJob::Harvest || w.job == WalkerJob::Tend) && b.producer) {
        field_work_unload(st, catalog, w, b);
    } else if ((w.job == WalkerJob::Deliver || w.job == WalkerJob::Fetch) && b.storage) {
        for (const auto& [good, loads] : w.cargo) {
            b.storage->loads[good] += loads;  // goods fetched for a Get order, or not delivered
        }
        b.storage->deliveryman = kNoWalker;
    } else if (w.job == WalkerJob::Deliver && b.producer) {
        for (const auto& [good, loads] : w.cargo) {
            b.producer->output_loads += loads;  // undelivered load back to the producer
        }
        const auto& bt = catalog.buildings.at(b.type);
        if (!w.cargo.empty() && (!bt.tends.empty() || bt.outputs.size() > 1)) {
            for (const auto& [good, loads] : w.cargo) {
                b.producer->output_loads -= std::min(b.producer->output_loads, loads);
                b.producer->units[good] += loads * 100;  // a lodge keeps fruit in units
            }
        }
        b.producer->deliveryman = kNoWalker;
    } else if (w.job == WalkerJob::Fetch && b.producer) {
        hostler_unload(catalog, w, b);
    } else if (w.job == WalkerJob::Fetch && b.stall) {
        for (const auto& [good, loads] : w.cargo) {
            b.stall->units[good] += loads * units_per_load_of(catalog, good);  // armor, horses: one per load
        }
        b.stall->vendor = kNoWalker;
    }
}

}  // namespace bo::core
