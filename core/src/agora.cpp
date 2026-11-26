// Module C5: what the agora peddler leaves in houses (doc 06; assumptions L-100).
#include <algorithm>
#include <optional>

#include "bo/core/housing.h"
#include "bo/core/logistics.h"
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
