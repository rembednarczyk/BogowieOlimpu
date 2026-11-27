// Auto-Defend (docs/gameplay/11-wojsko.md, Z:22, Z:145; assumptions L-129): with it on, the generals and admirals
// command on their own while the city is attacked.
#pragma once

#include "bo/core/data.h"
#include "bo/core/military.h"
#include "bo/core/state.h"

namespace bo::core {

// Daily, with WorldState::auto_defend on and invaders on the land: every company of the player at home is
// mustered and every company in the field marches on the nearest invader; while any invasion is on, every
// trireme calls its crew and attacks all enemies [ZAŁOŻENIE, L-129].
void advance_auto_defend(WorldState& state, const DataCatalog& catalog, CombatEvents& ev);

// The fighting is over: with Auto-Defend on, the generals send the companies in the city home (L-129).
void auto_defend_stand_down(WorldState& state);

// The switch (Z:145): on only before the fighting starts; off at any time. Returns an error key or "".
std::string set_auto_defend(WorldState& state, bool on);

}  // namespace bo::core
