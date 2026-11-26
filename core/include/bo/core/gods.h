// Module D1 stage 2 (docs/gameplay/10, 21; Z:119-124; assumptions L-053, L-112): what a finished sanctuary in
// the god's favor gives: lasting benefits, sanctification, prayers.
#pragma once

#include <cstdint>
#include <string>

#include "bo/core/city.h"
#include "bo/core/data.h"

namespace bo::core {

struct WorldState;

// A finished sanctuary of `god` whose god has not turned away (Z:124: no sacrifices, no benefits).
bool god_blesses(const CityState& city, const DataCatalog& catalog, const std::string& god);

// Daily: each blessing god sanctifies one building of its types every sanctify_every_days within reach
// (Zeus: any producer) [ZAŁOŻENIE, L-112]; prayer cooldowns run down.
void advance_gods(WorldState& state, const DataCatalog& catalog);

// Sanctified producers: the next production gives 4x (doc 21). Call after anything that adds output loads;
// it also notes the output that deliveries took.
void apply_sanctification(CityState& city);

// Prayer (Z:124, prayers.cpp): returns an error key, or empty when the god answered. Effects (doc 10, 21):
// Demeter food to Granaries, Artemis pork, Poseidon fish, Athena olives and oil, Dionysus wine (24 loads each,
// S-004 trivia / L-112), Hades money, Apollo cures the plague; Zeus beats the next army, Ares goes with the next
// expedition, Hephaestus sends Talos to guard the city, Aphrodite fills the houses, Hermes fulfils a request;
// expansion: Hera restocks the stalls, Atlas brings a site's materials (Z:119-122, P:20-21).
// The god may refuse (Z:132): see prayer_chance.
std::string pray(WorldState& state, const DataCatalog& catalog, BuildingId sanctuary);

// Percent chance that the god of a finished sanctuary grants a prayer now (Z:132) [ZAŁOŻENIE, L-053]: full soon
// after a sacrifice, then falling to prayer_chance_min when the god is about to turn away.
std::int32_t prayer_chance(const SanctuaryState& s, const DataCatalog& catalog);

}  // namespace bo::core
