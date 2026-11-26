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
// serialize_goods.cpp
void write_counts(ByteWriter& w, const std::map<std::string, std::uint32_t>& m);
bool read_counts(ByteReader& r, std::map<std::string, std::uint32_t>& m);
void write_goods_state(ByteWriter& w, const Building& b);
bool read_goods_state(ByteReader& r, Building& b, std::uint32_t version);
bool read_economy(ByteReader& r, WorldState& s, std::uint32_t version);
void write_military(ByteWriter& w, const MilitaryState& m);
bool read_military(ByteReader& r, MilitaryState& m, std::uint32_t version);
void write_world(ByteWriter& w, const WorldState& s);
bool read_world(ByteReader& r, WorldState& s);
void write_scenario(ByteWriter& w, const ScenarioState& s);
bool read_scenario(ByteReader& r, ScenarioState& s, std::uint32_t version);

}  // namespace bo::core
