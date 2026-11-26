// Module C5: what the agora peddler leaves in houses (doc 06; assumptions L-100).
#include <algorithm>
#include <optional>

#include <cstdlib>
#include <map>

#include "bo/core/housing.h"
#include "bo/core/logistics.h"
#include "bo/core/path.h"
#include "bo/core/storage.h"
#include "bo/core/trade.h"
#include "bo/core/walker.h"

namespace bo::core {
namespace {

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

}  // namespace

void dispatch_vendor(WorldState& st, const DataCatalog& catalog, BuildingId id, LogisticsEvents& ev) {
    CityState& city = st.city;
    Building& stall = city.buildings.items.at(id);
    const BuildingType& type = catalog.buildings.at(stall.type);
    const std::string walker_type = walker_type_from(catalog, stall.type);
    if (walker_type.empty() || !stall.road_access || stall.stall->not_buying) {
        return;  // Not Buying (Z:72): the peddler still sells what the stall has
    }
    const WalkerType& wt = catalog.walkers.at(walker_type);
    const std::int32_t reach = wt.max_source_distance.value_or(kVendorReach);
    const TilePos from = *stall.road_access;
    const auto dist = road_distances(city, {from}, PathMode::Road);
    std::optional<std::pair<std::uint32_t, BuildingId>> best;
    for (const auto& [sid, s] : city.buildings.items) {
        const bool listed = std::find(type.sources.begin(), type.sources.end(), s.type) != type.sources.end();
        const bool importing = s.trade.has_value() &&
                               std::find(type.sources.begin(), type.sources.end(), "trade_importing") != type.sources.end();
        const bool pickup = listed && s.producer && catalog.buildings.at(s.type).pickup_only;
        if ((!s.storage && !pickup) || !s.road_access || (!listed && !importing) || (s.trade && !importing)) {
            continue;
        }
        const TilePos p = *s.road_access;
        if (std::max(std::abs(p.x - from.x), std::abs(p.y - from.y)) > reach) {
            continue;  // too distant (doc 06 status, doc 24: 40 tiles, Zeus measured 37)
        }
        bool has_goods = pickup && s.producer->output_loads > 0;  // straight from the ranch or factory (doc 06)
        for (const auto& [good, loads] : pickup ? std::map<std::string, std::uint32_t>{} : s.storage->loads) {
            const bool usable = !s.trade || trade_offers(s, good);  // export goods are not for the city (Z:79)
            has_goods = has_goods || (loads > 0 && usable && good_matches(catalog, good, type.sells) &&
                                      st.stockpiled.count(good) == 0);  // stockpiled: not for vendors (Z:80)
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
        // 2 units a resident: about 8 months at 0.25 a month [ZAŁOŻENIE, L-100: eZeus, owner's decision].
        return std::max<std::uint32_t>(2 * h.population, 1);
    }
    if (g == HouseGood::Horses) {  // HS: an estate keeps up to 4 horses (S-004)
        const std::int32_t slots = std::max(cur.model[di].horse_slots, next ? next->model[di].horse_slots : 0);
        return static_cast<std::uint32_t>(std::max(slots, 1));
    }
    if (g == HouseGood::Armor) {  // one armor per hoplite or horseman: round(Pop x %S) of the level (doc 22)
        const auto soldiers = [di](const HousingLevel& lv) {
            return (lv.model[di].population * lv.model[di].soldiers_percent + 50) / 100;
        };
        return static_cast<std::uint32_t>(std::max({soldiers(cur), next ? soldiers(*next) : 0, 1}));
    }
    return 8U;  // fleece, oil, wine: 4 months at 2 units a month [ZAŁOŻENIE, L-100: eZeus, owner's decision]
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
