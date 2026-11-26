// Module C5: Granary and Storehouse. Rules: docs/gameplay/06-dystrybucja-handel.md (Z:65-69),
// 19-produkcja-i-logistyka-liczby.md (8 slots x 4 loads, sculpture one per slot).
#pragma once

#include <cstdint>
#include <string>

#include "bo/core/city.h"
#include "bo/core/data.h"

namespace bo::core {

// Whether this storage type keeps the good at all (by class; the Storehouse keeps wheat for horses).
bool storage_keeps(const BuildingType& type, const DataCatalog& catalog, const std::string& good);

// The order for a good: the stored one, or the default (Granary: Accept 32; Storehouse: Don't Accept wheat).
StorageOrder storage_order(const Building& b, const BuildingType& type, const std::string& good);

// Loads of `good` that still fit: free slots and the partly filled slot of the same good.
std::uint32_t storage_room(const Building& b, const DataCatalog& catalog, const std::string& good);

// Accepts a delivery now: keeps the good, order Accept below its limit, room, and staffed (Granary: fully,
// Z:66; Storehouse: any workers [ZAŁOŻENIE]).
bool storage_accepts(const Building& b, const DataCatalog& catalog, const std::string& good);

// Loads in all storages of the city, of one good or of a whole class ("food").
std::uint32_t city_stock(const CityState& city, const DataCatalog& catalog, const std::string& good_or_class);

// Whether a good belongs to a list that may name goods or classes (e.g. stall "sells": ["food"]).
bool good_matches(const DataCatalog& catalog, const std::string& good, const std::vector<std::string>& list);

// Puts up to `loads` of `good` into city storages that accept it, in id order (gifts of gods and cities);
// returns the loads placed. Trade buildings are left out.
std::uint32_t store_in_city(CityState& city, const DataCatalog& catalog, const std::string& good, std::uint32_t loads);
// Takes up to `loads` of `good` out of city storages in id order; returns the loads taken.
std::uint32_t take_from_city(CityState& city, const std::string& good, std::uint32_t loads);

}  // namespace bo::core
