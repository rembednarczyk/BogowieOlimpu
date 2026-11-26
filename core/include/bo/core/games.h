// Panhellenic games (docs/gameplay/09-kultura.md, Z:110-112; data/games.json). Model after eZeus (S-008,
// eboardcity.cpp winningChance) [L-123]: every year one of the four games by the year mod 4; in August the city
// wins with chance multiplier(population) x coverage^2, coverage = share of residents whose houses the
// discipline's performers visit; nothing below the invitation score; Apollo raises the chance (Z:112).
#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "bo/core/data.h"
#include "bo/core/state.h"

namespace bo::core {

struct GamesResult {
    std::string game;
    std::int32_t chance = 0;  // percent
    bool invited = false;
    bool won = false;
};

// Percent chance of the city in `game` now (0 when not invited).
std::int32_t games_chance(const WorldState& state, const DataCatalog& catalog, const GameParams& game);

// Daily: on the first day of the closing month the year's games are decided.
std::optional<GamesResult> advance_games(WorldState& state, const DataCatalog& catalog);

}  // namespace bo::core
