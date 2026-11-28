// Module C9: trade. Rules: docs/gameplay/06-dystrybucja-handel.md (Z:74-80), 24 (family: 8 and 12 loads,
// a caravan every 4 days, a ship every 30, at most one new trader a day). Parameters: data/trade.json.
// Assumptions: L-105.
#pragma once

#include <cstdint>
#include <vector>

#include "bo/core/data.h"
#include "bo/core/event.h"
#include "bo/core/state.h"

namespace bo::core {

struct TradeEvents {
    std::vector<WalkerId> spawned;
    std::vector<TradeDone> done;
};

// Daily: yearly limits reset at the year start; each staffed trade building with a partner on trading terms
// gets a caravan (land) or a ship (sea) when its interval has passed and no trader of it is on the way.
// Traders come from the map entry (road 'E' or sea 'S'), or appear at the building when the map has none.
TradeEvents dispatch_traders(WorldState& state, const DataCatalog& catalog);

// A trader at its building: sells what the building imports (up to the import limit, the load capacity and
// the city's yearly limit; the treasury pays), then buys what it exports (the treasury earns). Then it leaves.
void trade_at_target(WorldState& state, const DataCatalog& catalog, Walker& w, TradeEvents& ev);

// Whether a trade building takes a delivery of `good` for export (Z:78-79: kept up to export_keep).
bool trade_accepts(const Building& b, const DataCatalog& catalog, const std::string& good);

// Whether a vendor may fetch `good` from this trade building (import, Z:78).
bool trade_offers(const Building& b, const std::string& good);

}  // namespace bo::core
