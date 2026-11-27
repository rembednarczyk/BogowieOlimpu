// Horses and chariots (doc 05, 06, 19; P:18-19): the horse ranch and the chariot factory keep their output for
// a pickup; the factory's hostler fetches horses from the nearest ranch (assumptions L-106).
#include <algorithm>
#include <cstdlib>
#include <optional>

#include "bo/core/logistics.h"
#include "bo/core/path.h"
#include "bo/core/storage.h"
#include "bo/core/walker.h"

namespace bo::core {

void dispatch_hostlers(WorldState& st, const DataCatalog& catalog, LogisticsEvents& ev) {
    CityState& city = st.city;
    const auto hostler = catalog.walkers.find("hostler");
    if (hostler == catalog.walkers.end()) {
        return;
    }
    const std::int32_t reach = hostler->second.max_source_distance.value_or(kVendorReach);
    for (auto& [id, b] : city.buildings.items) {
        const auto params = catalog.production.find(b.type);
        if (!b.producer || !b.road_access || params == catalog.production.end() ||
            params->second.inputs_multi.count("horses") == 0) {
            continue;
        }
        auto& out = b.producer->field_workers;
        out.erase(std::remove_if(out.begin(), out.end(),
                                 [&st](WalkerId w) { return st.walkers.items.count(w) == 0; }),
                  out.end());
        if (!out.empty() || processor_room_loads(b, catalog, "horses") == 0) {
            continue;
        }
        const TilePos from = *b.road_access;
        const auto dist = road_distances(city, {from}, PathMode::Road);
        std::optional<std::pair<std::uint32_t, BuildingId>> best;
        for (const auto& [rid, r] : city.buildings.items) {
            const auto& outputs = catalog.buildings.at(r.type).outputs;
            if (!r.producer || !r.road_access || r.producer->output_loads == 0 ||
                std::find(outputs.begin(), outputs.end(), "horses") == outputs.end()) {
                continue;
            }
            const TilePos p = *r.road_access;
            const std::uint32_t d = dist[city.map.index(p)];
            if (std::max(std::abs(p.x - from.x), std::abs(p.y - from.y)) <= reach && d != kUnreachable &&
                (!best || d < best->first)) {
                best = std::make_pair(d, rid);
            }
        }
        if (!best) {
            continue;
        }
        const WalkerId w = spawn_destination_walker(city, st.walkers, catalog, "hostler", id,
                                                    *city.buildings.items.at(best->second).road_access);
        if (w == kNoWalker) {
            continue;
        }
        Walker& walker = st.walkers.items.at(w);
        walker.job = WalkerJob::Fetch;
        walker.target = best->second;
        walker.want_good = "horses";
        walker.want_loads = 1;
        out.push_back(w);
        ev.spawned.push_back(w);
    }
}

void fetch_from_producer(WorldState& st, const DataCatalog& catalog, Walker& w, Building& producer) {
    const auto& outputs = catalog.buildings.at(producer.type).outputs;
    if (outputs.size() != 1 || producer.producer->output_loads == 0) {
        return;
    }
    const std::string& good = outputs.front();
    std::uint32_t want = kVendorLoads;
    if (!w.want_good.empty()) {
        want = w.want_good == good ? w.want_loads : 0;
    } else {
        const auto home = st.city.buildings.items.find(w.home);
        const auto& items = st.city.buildings.items;
        if (home == items.end() || !good_matches(catalog, good, catalog.buildings.at(home->second.type).sells)) {
            want = 0;
        }
    }
    const std::uint32_t n = std::min(producer.producer->output_loads, want);
    producer.producer->output_loads -= n;
    if (n > 0) {
        w.cargo[good] += n;
    }
}

void hostler_unload(WorldState& st, const DataCatalog& catalog, const Walker& w, Building& home) {
    if (!home.producer) {  // the hippodrome's handler (P:14)
        const auto it = w.cargo.find("horses");
        st.hippodrome_horses += it != w.cargo.end() ? it->second : 0;
        return;
    }
    for (const auto& [good, loads] : w.cargo) {
        processor_add(home, catalog, good, loads);
    }
    auto& out = home.producer->field_workers;
    out.erase(std::remove(out.begin(), out.end(), w.id), out.end());
}

}  // namespace bo::core
