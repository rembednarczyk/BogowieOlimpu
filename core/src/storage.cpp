#include "bo/core/storage.h"

#include <algorithm>

namespace bo::core {

bool good_matches(const DataCatalog& catalog, const std::string& good, const std::vector<std::string>& list) {
    const auto cls = catalog.goods.find(good);
    for (const auto& entry : list) {
        if (entry == good || (cls != catalog.goods.end() && entry == cls->second)) {
            return true;
        }
    }
    return false;
}

bool storage_keeps(const BuildingType& type, const DataCatalog& catalog, const std::string& good) {
    if (good == "wheat" && std::find(type.stores_class.begin(), type.stores_class.end(), "wheat_for_horses") !=
                               type.stores_class.end()) {
        return true;
    }
    return good_matches(catalog, good, type.stores_class);
}

StorageOrder storage_order(const Building& b, const BuildingType& type, const std::string& good) {
    const auto it = b.storage->orders.find(good);
    if (it != b.storage->orders.end()) {
        return it->second;
    }
    StorageOrder o;
    if (good == "wheat" && type.storage_wheat_only_for_horses) {
        o.kind = StorageOrderKind::DontAccept;  // a new Storehouse takes everything but wheat (Z:68)
    }
    return o;
}

std::uint32_t storage_room(const Building& b, const DataCatalog& catalog, const std::string& good) {
    const auto per_slot = static_cast<std::uint32_t>(catalog.loads_per_slot);
    auto slots_of = [per_slot](const std::string& g, std::uint32_t loads) {
        return g == "sculpture" ? loads : (loads + per_slot - 1) / per_slot;  // a sculpture fills a slot
    };
    std::uint32_t used = 0;
    for (const auto& [g, loads] : b.storage->loads) {
        used += slots_of(g, loads);
    }
    const auto total = static_cast<std::uint32_t>(catalog.storage_slots);
    const std::uint32_t free_slots = used >= total ? 0 : total - used;
    const auto mine = b.storage->loads.find(good);
    const std::uint32_t own = mine == b.storage->loads.end() ? 0 : mine->second;
    const std::uint32_t partial = good == "sculpture" || own % per_slot == 0 ? 0 : per_slot - own % per_slot;
    return free_slots * (good == "sculpture" ? 1 : per_slot) + partial;
}

bool storage_accepts(const Building& b, const DataCatalog& catalog, const std::string& good) {
    if (!b.storage) {
        return false;
    }
    const BuildingType& type = catalog.buildings.at(b.type);
    if (!storage_keeps(type, catalog, good)) {
        return false;
    }
    const StorageOrder o = storage_order(b, type, good);
    const auto it = b.storage->loads.find(good);
    const std::uint32_t have = it == b.storage->loads.end() ? 0 : it->second;
    const bool staffed = type.accepts_only_when_fully_staffed ? b.staffing_percent >= 100 : b.staffing_percent > 0;
    const bool open = o.kind == StorageOrderKind::Accept || o.kind == StorageOrderKind::Get;  // Get also receives
    return staffed && open && have < o.limit && storage_room(b, catalog, good) > 0;
}

std::uint32_t city_stock(const CityState& city, const DataCatalog& catalog, const std::string& good_or_class) {
    std::uint32_t total = 0;
    for (const auto& [id, b] : city.buildings.items) {
        if (!b.storage || b.trade) {
            continue;  // goods in trade buildings are not the city's stock (Z:79)
        }
        for (const auto& [good, loads] : b.storage->loads) {
            if (good_matches(catalog, good, {good_or_class})) {
                total += loads;
            }
        }
    }
    return total;
}

std::uint32_t store_in_city(CityState& city, const DataCatalog& catalog, const std::string& good, std::uint32_t loads) {
    std::uint32_t placed = 0;
    for (auto& [id, b] : city.buildings.items) {
        if (placed == loads || !b.storage || b.trade || !storage_accepts(b, catalog, good)) {
            continue;
        }
        const std::uint32_t n = std::min(loads - placed, storage_room(b, catalog, good));
        b.storage->loads[good] += n;
        placed += n;
    }
    return placed;
}

std::uint32_t take_from_city(CityState& city, const std::string& good, std::uint32_t loads) {
    std::uint32_t taken = 0;
    for (auto& [id, b] : city.buildings.items) {
        if (taken == loads || !b.storage || b.trade) {
            continue;
        }
        const auto it = b.storage->loads.find(good);
        if (it == b.storage->loads.end()) {
            continue;
        }
        const std::uint32_t n = std::min(loads - taken, it->second);
        it->second -= n;
        taken += n;
    }
    return taken;
}

}  // namespace bo::core
