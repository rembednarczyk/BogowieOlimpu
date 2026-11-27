// Module D1: the oracle of Zeus's and Apollo's sanctuaries (Z:124; oracle.cpp; assumptions L-126).
#pragma once

#include <string>

#include "bo/core/data.h"
#include "bo/core/event.h"
#include "bo/core/state.h"

namespace bo::core {

// Consult Oracle (Z:124) at the complete sanctuary `id` of a god with an oracle: `out` gets the next threat on
// the scenario's calendar (or none). Returns an error key or "": "myth.oracle_silent" if it spoke not long ago.
std::string consult_oracle(WorldState& state, const DataCatalog& catalog, BuildingId id, OracleSpoke& out);

}  // namespace bo::core
