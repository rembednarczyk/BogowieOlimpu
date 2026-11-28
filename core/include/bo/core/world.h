// Module D4 stage 1 (docs/gameplay/14, 24; Z:167-178; assumptions L-063, L-064, L-065, L-113): leaders'
// opinions, gifts, requests, tribute, armies sent abroad, and what an invasion by a rival leaves behind.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "bo/core/data.h"
#include "bo/core/state.h"

namespace bo::core {

struct WorldState;

struct WorldEvents {
    std::vector<std::pair<std::uint32_t, std::int64_t>> tribute;  // city, + received / - paid
    std::vector<Expedition> returned;                             // expeditions back today
    std::vector<std::uint32_t> delivered;                         // requested goods arrived (city)
    bool episode_lost = false;
    std::vector<std::uint32_t> rebellions;                              // vassals or colonies that rose (L-121)
    std::vector<std::uint32_t> rival_attacks;                           // rivals that march on the city
    std::vector<std::pair<std::uint32_t, std::int64_t>> gifts;          // goodwill or debt aid, drachmas
};

// Changes a leader's opinion (clamped to 0-100) and notes it in the city's history (Z:171): a change of the
// same kind as the newest note joins it; the history keeps the newest kHistoryLength notes.
void change_opinion(CityDiplomacy& d, std::int32_t delta, const std::string& what, std::uint64_t day);

// A city's call for troops (Z:161): `companies` companies at home, no rabble, march to fight its enemy.
std::string dispatch_aid(WorldState& state, const DataCatalog& catalog, std::uint32_t city, std::uint32_t companies);

// Monthly (L-121; Z:169, Z:172, Z:178, Z:100, Z:179): a vassal or colony with a bad opinion for a year rises
// (no trade, no tribute until put down); a rival that hates the player for a year attacks; a friendly city
// gives a gift after two good years, and helps a city long in debt once a year.
void advance_relations(WorldState& state, const DataCatalog& catalog, WorldEvents& ev);

// Gift (Z:176-177; doc 24): loads of a good from storage, or drachmas from the treasury (500 dr a unit,
// [ZAŁOŻENIE, L-113]); +3 opinion a unit of a good the city needs (buys), +1.5 otherwise, at most 3 units of
// a good a year; other cities of the same state (not rivals) gain half.
std::string give_gift(WorldState& state, const DataCatalog& catalog, std::uint32_t city, const std::string& good,
                      std::uint32_t amount);
// Request (Z:173-175; doc 24): food, goods or money by state and opinion; a rival only gives to a stronger
// player. The goods arrive after request_delay_days (90, eZeus; L-113); every city's opinion -10.
// Military help (Z:174; [ZAŁOŻENIE, L-125] numbers in diplomacy.json military_help): defensive aid only while an
// invasion is on (allies when they think very highly of the player, vassals, colonies and the parent city more
// readily; rivals never): 1 + military / 2 companies come; a strike on `target` (allies who adore the player,
// the others above average): their armies meet, the weaker loses a shield, the target thinks less of the player.
std::string request_military_help(WorldState& state, const DataCatalog& catalog, std::uint32_t city, bool strike,
                                  std::uint32_t target, std::vector<WalkerId>& spawned);

std::string request_from_city(WorldState& state, const DataCatalog& catalog, std::uint32_t city,
                              const std::string& good);
// Attack or raid (Z:177-178): companies at home, no rabble; 200 days there, a battle by strength (L-063),
// 200 days back.
std::string dispatch_army(WorldState& state, const DataCatalog& catalog, std::uint32_t city, bool raid,
                          const std::string& good, const std::vector<std::uint32_t>& companies, bool ships = false);
// Daily: expeditions march, fight and come back; request cooldowns run down. Yearly: tribute both ways.
WorldEvents advance_world(WorldState& state, const DataCatalog& catalog);
// An invasion sent by `city` ended (InvasionEnded::result): taken or surrendered, the rival demands tribute
// and a second time the episode is lost; beaten off, the tribute ends (Z:172).
void invasion_outcome(WorldState& state, const DataCatalog& catalog, std::int32_t city, std::uint8_t result,
                      WorldEvents& ev);
// Army strength abroad (doc 24, L-063): soldiers, horsemen count 1.5.
std::int64_t army_strength_x2(const WorldState& state, const std::vector<std::uint32_t>& companies);

}  // namespace bo::core
