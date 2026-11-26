// Save format of the city and the walkers (internal to serialize*.cpp). Layout notes in serialize_city.cpp.
#pragma once

#include <cstdint>

#include "bo/core/serialize.h"

namespace bo::core {

void write_city(ByteWriter& w, const CityState& c);
bool read_city(ByteReader& r, CityState& c, std::uint32_t version);
void write_walkers(ByteWriter& w, const WalkerStore& s);
bool read_walkers(ByteReader& r, WalkerStore& s, std::uint32_t version);
void write_economy(ByteWriter& w, const WorldState& s);
bool read_economy(ByteReader& r, WorldState& s, std::uint32_t version);

}  // namespace bo::core
