// Module C4: hunting and fishing (docs/gameplay/04 Z:46-48, P:8; 19 S-005; assumptions L-103).
#include <algorithm>
#include <cstdlib>
#include <optional>

#include "bo/core/logistics.h"
#include "bo/core/walker.h"

namespace bo::core {
namespace {

bool alive(const WorldState& st, WalkerId id) { return st.walkers.items.count(id) != 0; }

std::string prey_good(const std::string& prey) { return prey == "deer" ? "meat" : "pork"; }

TileFeature feature_of(const std::string& prey) { return prey == "deer" ? TileFeature::DeerSpawn : TileFeature::BoarSpawn; }

// The water tile next to a coast building on its water side (rotation, set by place_on_coast).
std::optional<TilePos> mooring(const CityState& city, const Building& b) {
    const auto d = static_cast<Dir>(b.rotation);
    const std::int32_t len = d == Dir::North || d == Dir::South ? b.width : b.height;
    for (std::int32_t i = 0; i < len; ++i) {
        TilePos edge = b.origin;
        if (d == Dir::North || d == Dir::South) {
            edge.x += i;
            edge.y = d == Dir::North ? b.origin.y : b.origin.y + b.height - 1;
        } else {
            edge.y += i;
            edge.x = d == Dir::West ? b.origin.x : b.origin.x + b.width - 1;
        }
        const TilePos p = step(edge, d);
        if (city.map.in_bounds(p) && city.map.at(p).terrain == Terrain::Water) {
            return p;
        }
    }
    return std::nullopt;
}

WalkerId spawn_at(WorldState& st, const DataCatalog& catalog, const std::string& type, BuildingId home, TilePos from,
                  TilePos to, PathMode mode) {
    PathRequest req{from, to, mode};
    auto path = find_path(st.city, req);
    const auto t = catalog.walkers.find(type);
    if (!path || t == catalog.walkers.end()) {
        return kNoWalker;
    }
    Walker w;
    w.id = st.walkers.next_id++;
    w.type = type;
    w.kind = WalkerKind::Destination;
    w.home = home;
    w.tile = from;
    w.speed_raw = t->second.speed_raw;
    w.path = std::move(*path);
    w.phase = WalkerPhase::Routed;
    const WalkerId id = w.id;
    st.walkers.items.emplace(id, std::move(w));
    return id;
}

std::string worker_of(const DataCatalog& catalog, const std::string& building_type) {
    for (const auto& [id, w] : catalog.walkers) {
        if (std::find(w.from.begin(), w.from.end(), building_type) != w.from.end()) {
            return id;
        }
    }
    return {};
}

}  // namespace

void advance_wildlife(WorldState& st, const DataCatalog&) {
    CityState& city = st.city;
    const bool month_start = st.time.day_in_month() == 0;
    for (std::size_t i = 0; i < city.map.size(); ++i) {
        const auto f = static_cast<TileFeature>(city.map.tiles()[i].feature);
        if (f != TileFeature::BoarSpawn && f != TileFeature::DeerSpawn) {
            continue;
        }
        const auto key = static_cast<std::uint32_t>(i);
        auto it = city.spawns.find(key);
        if (it == city.spawns.end()) {  // a new map starts with a full boar herd [ZAŁOŻENIE, L-103]
            it = city.spawns.emplace(key, SpawnState{f == TileFeature::BoarSpawn ? kBoarHerd : std::uint8_t{0}, 0}).first;
        }
        SpawnState& s = it->second;
        if (f == TileFeature::BoarSpawn && s.animals < kBoarHerd && ++s.timer >= kBoarBirthDays) {
            s.timer = 0;
            ++s.animals;  // a young boar every 26 days below 7 (doc 19)
        } else if (f == TileFeature::DeerSpawn && month_start && s.animals < kBoarHerd) {
            ++s.animals;  // one deer a month (doc 19); capped like a boar herd [ZAŁOŻENIE, L-103]
        }
    }
}

void dispatch_hunters_and_boats(WorldState& st, const DataCatalog& catalog, LogisticsEvents& ev) {
    CityState& city = st.city;
    for (auto& [id, b] : city.buildings.items) {
        const BuildingType& type = catalog.buildings.at(b.type);
        const auto params = catalog.production.find(b.type);
        const bool hunter = !type.prey.empty();
        const bool boat = b.type == "fishery" || b.type == "urchin_quay";
        if (!b.producer || !b.road_access || b.staffing_percent == 0 || params == catalog.production.end() ||
            (!hunter && !boat)) {
            continue;
        }
        const ProductionParams& pp = params->second;
        auto& out = b.producer->field_workers;
        out.erase(std::remove_if(out.begin(), out.end(), [&st](WalkerId w) { return !alive(st, w); }), out.end());
        if (!out.empty() || b.producer->output_loads >= static_cast<std::uint32_t>(pp.storage_loads)) {
            continue;
        }
        if (pp.boat_build_days > 0 && b.producer->progress < static_cast<std::uint32_t>(pp.boat_build_days)) {
            ++b.producer->progress;  // building the boat first (Z:47) [ZAŁOŻENIE, L-103: 16 days]
            continue;
        }
        const std::string type_name = worker_of(catalog, b.type);
        std::optional<std::pair<std::int32_t, std::size_t>> best;
        for (std::size_t i = 0; i < city.map.size(); ++i) {
            const TilePos p = city.map.pos(i);
            const auto f = static_cast<TileFeature>(city.map.tiles()[i].feature);
            const std::int32_t d = std::max(std::abs(p.x - b.origin.x), std::abs(p.y - b.origin.y));
            bool wanted = false;
            if (hunter) {
                for (const auto& prey : type.prey) {
                    const auto s = city.spawns.find(static_cast<std::uint32_t>(i));
                    wanted = wanted || (f == feature_of(prey) && s != city.spawns.end() && s->second.animals > 0);
                }
            } else {
                wanted = f == (b.type == "fishery" ? TileFeature::FishGround : TileFeature::UrchinGround);
            }
            if (wanted && d <= pp.source_distance && (!best || d < best->first)) {
                best = std::make_pair(d, i);
            }
        }
        if (!best || type_name.empty()) {
            continue;
        }
        const TilePos target = city.map.pos(best->second);
        WalkerId w = kNoWalker;
        TilePos start = *b.road_access;
        if (hunter) {
            w = spawn_at(st, catalog, type_name, id, start, target, PathMode::Walkable);
        } else if (const auto moor = mooring(city, b)) {
            start = *moor;
            w = spawn_at(st, catalog, type_name, id, start, target, PathMode::Water);
        }
        if (w == kNoWalker) {
            continue;
        }
        Walker& walker = st.walkers.items.at(w);
        walker.job = hunter ? WalkerJob::Hunt : WalkerJob::Fish;
        walker.resource = target;
        walker.roam_target = start;  // boats sail back here
        walker.work_ticks = static_cast<std::uint32_t>(pp.work_days_per_trip) * kTicksPerDay;
        out.push_back(w);
        ev.spawned.push_back(w);
    }
}

bool wildlife_at_target(WorldState& st, const DataCatalog& catalog, Walker& w) {
    if (w.work_ticks > 1) {
        --w.work_ticks;
        return true;
    }
    const auto home = st.city.buildings.items.find(w.home);
    if (home == st.city.buildings.items.end()) {
        return false;
    }
    const BuildingType& type = catalog.buildings.at(home->second.type);
    if (w.job == WalkerJob::Fish) {
        w.cargo[type.outputs.front()] = 1;  // one load a trip (doc 19: 100 units)
        return false;
    }
    auto s = st.city.spawns.find(static_cast<std::uint32_t>(st.city.map.index(w.resource)));
    if (s == st.city.spawns.end() || s->second.animals == 0) {
        return false;  // the herd is gone: back empty-handed
    }
    --s->second.animals;
    const auto f = static_cast<TileFeature>(st.city.map.at(w.resource).feature);
    w.cargo[prey_good(f == TileFeature::DeerSpawn ? "deer" : "boar")] = 1;
    return false;
}

void wildlife_unload(WorldState&, const DataCatalog& catalog, const Walker& w, Building& home) {
    const auto& pp = catalog.production.at(home.type);
    const BuildingType& type = catalog.buildings.at(home.type);
    for (const auto& [good, loads] : w.cargo) {
        if (type.outputs.size() > 1) {
            home.producer->units[good] += loads * 100;  // a hunting lodge keeps meat and pork apart
        } else {
            // A fishery holds one load: the rest of a catch is lost (doc 19).
            home.producer->output_loads = std::min(home.producer->output_loads + loads,
                                                   static_cast<std::uint32_t>(pp.storage_loads));
        }
    }
}

}  // namespace bo::core
