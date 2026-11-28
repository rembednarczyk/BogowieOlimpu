// Wolves (Z:45: "wolves are sneaky and occasionally make off with an innocent victim"; P:7: bulls guard
// cattle). Numbers are assumptions L-107 after the defaults of S-008 (eZeus), kept in data/walkers.json.
#include <algorithm>
#include <cstdlib>
#include <optional>

#include "bo/core/logistics.h"
#include "bo/core/path.h"
#include "bo/core/walker.h"

namespace bo::core {
namespace {

bool guarded(const WorldState& st, const WolfParams& p, const Building& animal) {
    for (const auto& [id, w] : st.walkers.items) {
        if (w.job == WalkerJob::Shear && w.home == animal.animal->shed &&
            std::max(std::abs(w.tile.x - animal.origin.x), std::abs(w.tile.y - animal.origin.y)) <= p.guard_tiles) {
            return true;  // shepherds and goatherds usually keep an eye on their flocks (Z:45)
        }
    }
    return false;
}

std::optional<BuildingId> nearest_prey(const WorldState& st, const WolfParams& p, TilePos den) {
    std::optional<std::pair<std::int32_t, BuildingId>> best;
    for (const auto& [id, b] : st.city.buildings.items) {
        if (!b.animal || std::find(p.prey.begin(), p.prey.end(), b.type) == p.prey.end()) {
            continue;  // cattle are guarded by their bulls (P:7)
        }
        const std::int32_t d = std::max(std::abs(b.origin.x - den.x), std::abs(b.origin.y - den.y));
        if (d <= p.reach_tiles && (!best || d < best->first)) {
            best = std::make_pair(d, id);
        }
    }
    if (!best) {
        return std::nullopt;
    }
    return best->second;
}

// The wolf at its prey: takes it unless a shepherd is near, then trots back to the den.
void attack(WorldState& st, const DataCatalog& catalog, Walker& w, TilePos den, WolfEvents& ev) {
    const auto prey = st.city.buildings.items.find(w.target);
    if (prey != st.city.buildings.items.end() && prey->second.animal && !guarded(st, catalog.wolves, prey->second)) {
        remove_building(st.city, w.target, ev.removed);
    }
    w.job = WalkerJob::None;
    auto path = find_path(st.city, {w.tile, den, PathMode::Walkable});
    if (!path) {
        w.phase = WalkerPhase::Done;
        return;
    }
    w.path = std::move(*path);
    w.path_pos = 0;
    w.phase = WalkerPhase::Routed;
}

WalkerId send_wolf(WorldState& st, const DataCatalog& catalog, TilePos den, BuildingId prey) {
    const Building& b = st.city.buildings.items.at(prey);
    auto path = find_path(st.city, {den, b.origin, PathMode::Walkable});
    const auto t = catalog.walkers.find("wolf");
    if (!path || t == catalog.walkers.end()) {
        return kNoWalker;
    }
    Walker w;
    w.id = st.walkers.next_id++;
    w.type = "wolf";
    w.kind = WalkerKind::Destination;
    w.home = kNoBuilding;
    w.tile = den;
    w.speed_raw = t->second.speed_raw;
    w.path = std::move(*path);
    w.phase = WalkerPhase::Routed;
    w.job = WalkerJob::Prowl;
    w.target = prey;
    const WalkerId id = w.id;
    st.walkers.items.emplace(id, std::move(w));
    return id;
}

}  // namespace

WolfEvents advance_wolves(WorldState& st, const DataCatalog& catalog) {
    WolfEvents ev;
    CityState& city = st.city;
    const WolfParams& p = catalog.wolves;
    for (std::size_t i = 0; i < city.map.size(); ++i) {
        if (city.map.tiles()[i].feature != static_cast<std::uint8_t>(TileFeature::WolfSpawn)) {
            continue;
        }
        const auto key = static_cast<std::uint32_t>(i);
        const TilePos den = city.map.pos(i);
        auto it = city.spawns.find(key);
        if (it == city.spawns.end()) {  // a new map starts with a full pack [ZAŁOŻENIE, L-107]
            it = city.spawns.emplace(key, SpawnState{static_cast<std::uint8_t>(p.pack), 0, kNoWalker}).first;
        }
        SpawnState& s = it->second;
        if (s.animals < p.pack && ++s.timer >= p.refill_days) {
            s.timer = 0;
            ++s.animals;
        }
        const auto out = st.walkers.items.find(s.hunter);
        if (out != st.walkers.items.end()) {
            if (out->second.phase == WalkerPhase::AtTarget && out->second.job == WalkerJob::Prowl) {
                attack(st, catalog, out->second, den, ev);
            }
            continue;  // one wolf of a pack hunts at a time [ZAŁOŻENIE, L-107]
        }
        s.hunter = kNoWalker;
        auto& rng = st.rng.stream(RngStream::Walkers);
        if (s.animals == 0 || rng.next_u32() % static_cast<std::uint32_t>(p.hunt_one_in_days) != 0) {
            continue;
        }
        const auto prey = nearest_prey(st, p, den);
        if (!prey) {
            continue;
        }
        s.hunter = send_wolf(st, catalog, den, *prey);
        if (s.hunter != kNoWalker) {
            ev.spawned.push_back(s.hunter);
        }
    }
    return ev;
}

}  // namespace bo::core
