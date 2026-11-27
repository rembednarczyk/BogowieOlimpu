// The late blocks read by the presentation. The world map block (since v50, L-135, Z:170-171): each city's
// place on the map and its history of interactions, one record per trade partner in the partners' order.
// The buildings' placing days (since v51, O-005): the count, then one day per building in id order.
#include "serialize_city.h"

namespace bo::core {

void write_atlas(ByteWriter& w, const WorldState& s) {
    for (const TradePartner& p : s.trade.partners) {
        w.i32(p.map_x);
        w.i32(p.map_y);
        w.u32(static_cast<std::uint32_t>(p.diplomacy.history.size()));
        for (const Interaction& h : p.diplomacy.history) {
            w.u32(h.day);
            w.str(h.what);
            w.i32(h.delta);
        }
    }
}

bool read_atlas(ByteReader& r, WorldState& s) {
    for (TradePartner& p : s.trade.partners) {
        std::uint32_t n = 0;
        if (!r.i32(p.map_x) || !r.i32(p.map_y) || !r.u32(n) || n > kHistoryLength) {
            return false;
        }
        p.diplomacy.history.resize(n);
        for (Interaction& h : p.diplomacy.history) {
            if (!r.u32(h.day) || !r.str(h.what) || !r.i32(h.delta)) {
                return false;
            }
        }
    }
    return true;
}

void write_placed_days(ByteWriter& w, const CityState& c) {
    w.u32(static_cast<std::uint32_t>(c.buildings.items.size()));
    for (const auto& [id, b] : c.buildings.items) {
        w.u32(b.placed_day);
    }
}

bool read_placed_days(ByteReader& r, CityState& c) {
    std::uint32_t n = 0;
    if (!r.u32(n) || n != c.buildings.items.size()) {
        return false;
    }
    for (auto& [id, b] : c.buildings.items) {
        if (!r.u32(b.placed_day)) {
            return false;
        }
    }
    return true;
}

}  // namespace bo::core
