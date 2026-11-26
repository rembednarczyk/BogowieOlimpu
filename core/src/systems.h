// Internal: shared between simulation.cpp and systems.cpp.
#pragma once

#include "bo/core/risks.h"
#include "bo/core/simulation.h"

namespace bo::core {

// Emits the events of a risk step (buildings lost and placed, walkers, disasters).
void emit_risks(Simulation& sim, const RiskEvents& ev);

}  // namespace bo::core
