// Module C4 stage 3: extraction and the mint (docs/gameplay/05, 19; S-004, S-005; assumptions L-102).
#include <algorithm>
#include <cstdlib>
#include <optional>
#include <map>
#include <set>
#include <vector>

#include "bo/core/logistics.h"
#include "bo/core/walker.h"

namespace bo::core {
namespace {

constexpr std::uint32_t kUnitsPerLoad = 100;

bool alive(const WorldState& st, WalkerId id) { return st.walkers.items.count(id) != 0; }

std::optional<Terrain> terrain_of(const std::string& id) {
    for (std::uint8_t i = 0; i < static_cast<std::uint8_t>(Terrain::Count); ++i) {
        if (terrain_id(static_cast<Terrain>(i)) == id) {
            return static_cast<Terrain>(i);
        }
    }
    return std::nullopt;
}

std::string worker_type_of(const DataCatalog& catalog, const std::string& building_type) {
    for (const auto& [id, w] : catalog.walkers) {
        if (std::find(w.from.begin(), w.from.end(), building_type) != w.from.end()) {
            return id;
        }
    }
    return "miner";  // [ZAŁOŻENIE] Foundry/Mint workers share the miner type in walkers.json
}

// The raw good the building piles up in units: silver for the mint, its output for the others.
std::string raw_good(const BuildingType& type) { return type.outputs.front() == "drachmas" ? "silver" : type.outputs.front(); }

bool resource_in_reach(const CityState& city, const Building& b, Terrain t, std::int32_t reach) {
    for (std::int32_t y = b.origin.y - reach; y <= b.origin.y + b.height + reach; ++y) {
        for (std::int32_t x = b.origin.x - reach; x <= b.origin.x + b.width + reach; ++x) {
            if (city.map.in_bounds({x, y}) && city.map.at({x, y}).terrain == t) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace

void dispatch_extractors(WorldState& st, const DataCatalog& catalog, LogisticsEvents& ev) {
    CityState& city = st.city;
    std::set<std::size_t> worked;  // one worker per resource tile at a time (doc 19)
    for (const auto& [wid, w] : st.walkers.items) {
        if (w.job == WalkerJob::Extract) {
            worked.insert(city.map.index(w.resource));
        }
    }
    for (auto& [id, b] : city.buildings.items) {
        const auto params = catalog.production.find(b.type);
        if (!b.producer || !b.road_access || b.staffing_percent == 0 || params == catalog.production.end() ||
            params->second.field_workers == 0) {
            continue;
        }
        const ProductionParams& pp = params->second;
        const BuildingType& type = catalog.buildings.at(b.type);
        const auto terrain = terrain_of(type.resource);
        auto& out = b.producer->field_workers;
        out.erase(std::remove_if(out.begin(), out.end(), [&st](WalkerId w) { return !alive(st, w); }), out.end());
        const auto allowed = static_cast<std::size_t>((pp.field_workers * b.staffing_percent + 99) / 100);
        const std::string raw = raw_good(type);
        const bool full = pp.input_capacity_units > 0
                              ? b.producer->units[raw] >= static_cast<std::uint32_t>(pp.input_capacity_units)
                              : b.producer->output_loads >= kExtractionStock;
        if (!terrain || full) {
            continue;
        }
        while (out.size() < allowed) {
            std::optional<std::pair<std::int32_t, std::size_t>> best;  // nearest free resource tile in reach
            for (std::size_t i = 0; i < city.map.size(); ++i) {
                const TilePos p = city.map.pos(i);
                const std::int32_t d = std::max(std::abs(p.x - b.origin.x), std::abs(p.y - b.origin.y));
                if (city.map.tiles()[i].terrain == *terrain && d <= pp.source_distance && worked.count(i) == 0 &&
                    (!best || d < best->first)) {
                    best = std::make_pair(d, i);
                }
            }
            if (!best) {
                break;
            }
            worked.insert(best->second);
            const TilePos res = city.map.pos(best->second);
            WalkerId w = kNoWalker;
            std::vector<Dir> sides(kDirs.begin(), kDirs.end());  // the side facing the building first
            const TilePos home = b.origin;
            std::stable_sort(sides.begin(), sides.end(), [res, home](Dir a, Dir c) {
                const TilePos pa = step(res, a);
                const TilePos pc = step(res, c);
                return std::abs(pa.x - home.x) + std::abs(pa.y - home.y) < std::abs(pc.x - home.x) + std::abs(pc.y - home.y);
            });
            for (const Dir d : sides) {  // workers stand next to the rock or tree
                const TilePos stand = step(res, d);
                if (city.map.in_bounds(stand) && terrain_walkable(city.map.at(stand).terrain) &&
                    city.map.at(stand).building == kNoBuilding) {
                    w = spawn_destination_walker(city, st.walkers, catalog, worker_type_of(catalog, b.type), id,
                                                 stand, PathMode::Walkable);
                    if (w != kNoWalker) {
                        break;
                    }
                }
            }
            if (w == kNoWalker) {
                continue;  // unreachable tile: try the next one
            }
            Walker& walker = st.walkers.items.at(w);
            walker.job = WalkerJob::Extract;
            walker.resource = res;
            walker.work_ticks = static_cast<std::uint32_t>(pp.work_days_per_trip) * kTicksPerDay;
            out.push_back(w);
            ev.spawned.push_back(w);
        }
    }
}

bool extraction_at_target(WorldState& st, const DataCatalog& catalog, Walker& w) {
    if (w.work_ticks > 1) {
        --w.work_ticks;
        return true;
    }
    const auto home = st.city.buildings.items.find(w.home);
    if (home != st.city.buildings.items.end()) {
        const auto& pp = catalog.production.at(home->second.type);
        w.cargo[raw_good(catalog.buildings.at(home->second.type))] = static_cast<std::uint32_t>(pp.units_per_trip);
    }
    return false;
}

void extraction_unload(WorldState&, const DataCatalog& catalog, const Walker& w, Building& home) {
    const BuildingType& type = catalog.buildings.at(home.type);
    for (const auto& [good, units] : w.cargo) {
        home.producer->units[good] += units;  // carried in units for extraction trips
    }
    if (type.outputs.front() != "drachmas") {
        auto& units = home.producer->units[type.outputs.front()];
        home.producer->output_loads += units / kUnitsPerLoad;  // a load at every 100 units (doc 19)
        units %= kUnitsPerLoad;
    }
}

std::int64_t advance_extraction(WorldState& st, const DataCatalog& catalog) {
    std::int64_t minted = 0;
    for (auto& [id, b] : st.city.buildings.items) {
        const auto params = catalog.production.find(b.type);
        if (!b.producer || params == catalog.production.end()) {
            continue;
        }
        const ProductionParams& pp = params->second;
        ProducerState& p = *b.producer;
        if (pp.silver_per_batch > 0) {
            auto& silver = p.units["silver"];
            if (silver >= static_cast<std::uint32_t>(pp.silver_per_batch)) {
                p.progress += b.staffing_percent;
                if (p.progress >= static_cast<std::uint32_t>(pp.batch_days) * 100) {
                    p.progress = 0;
                    silver -= static_cast<std::uint32_t>(pp.silver_per_batch);
                    minted += pp.drachmas_per_batch;  // straight to the treasury [ZAŁOŻENIE, L-102]
                }
            }
        } else if (pp.fixed_rate_near_resource) {
            const auto terrain = terrain_of(catalog.buildings.at(b.type).resource);
            if (terrain && resource_in_reach(st.city, b, *terrain, pp.source_distance) &&
                p.output_loads < kExtractionStock) {
                p.progress += static_cast<std::uint32_t>(pp.loads_per_year) * b.staffing_percent;
                if (p.progress >= 100 * 192) {
                    p.progress -= 100 * 192;
                    ++p.output_loads;
                }
            }
        }
    }
    st.finance.treasury += minted;
    return minted;
}

std::vector<BuildingId> herd_births(WorldState& st, const DataCatalog& catalog) {
    std::vector<BuildingId> born;
    std::map<BuildingId, std::vector<BuildingId>> herds;
    for (const auto& [id, b] : st.city.buildings.items) {
        if (b.animal) {
            herds[b.animal->shed].push_back(id);
        }
    }
    for (auto& [shed_id, herd] : herds) {
        const auto shed = st.city.buildings.items.find(shed_id);
        const auto params = shed == st.city.buildings.items.end() ? catalog.production.end()
                                                                  : catalog.production.find(shed->second.type);
        if (params == catalog.production.end() || !shed->second.producer || herd.size() < 2 ||
            params->second.slaughter_loads > 0 ||  // a corral breeds only in place of the slaughtered (P:7)
            herd.size() >= static_cast<std::size_t>(params->second.animals_max)) {
            continue;
        }
        auto& timer = shed->second.producer->progress;  // sheds have no other use for progress
        if (++timer < kBirthDays) {
            continue;
        }
        timer = 0;
        Building young = st.city.buildings.items.at(herd.front());
        young.coverage.clear();
        young.animal->days_to_ready = 192;  // the young give their first load after 12 months (doc 19)
        young.animal->worker_coming = false;
        born.push_back(add_building(st.city, std::move(young)));
    }
    return born;
}

}  // namespace bo::core
