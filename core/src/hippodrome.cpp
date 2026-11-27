// The hippodrome (P:13-15; doc 18; assumptions L-127). The pattern: the chariot factory's hostler for the horses
// (horses.cpp), doc 18's table for the horses and the income by length.
#include "bo/core/hippodrome.h"

#include <algorithm>
#include <cstdlib>
#include <optional>

#include "bo/core/path.h"
#include "bo/core/walker.h"

namespace bo::core {
namespace {

constexpr std::int32_t kSegment = 4;  // a stade is a 4x4 piece (building_stats.json)

bool joined(const Building& a, const Building& b) {  // edge to edge, aligned
    const std::int32_t dx = std::abs(a.origin.x - b.origin.x);
    const std::int32_t dy = std::abs(a.origin.y - b.origin.y);
    return (dx == 0 && dy == kSegment) || (dy == 0 && dx == kSegment);
}

const HippodromeRow* row_for(const DataCatalog& catalog, std::size_t stades) {
    const HippodromeRow* best = nullptr;
    for (const HippodromeRow& r : catalog.hippodrome) {
        best = stades >= r.stades ? &r : best;
    }
    return best;
}

}  // namespace

Track hippodrome_track(const CityState& city) {
    Track t;
    for (const auto& [id, b] : city.buildings.items) {
        if (b.type == "hippodrome") {
            t.segments.push_back(id);
        }
    }
    if (t.segments.size() < 4) {
        return t;  // the smallest loop: a square of four
    }
    const auto& items = city.buildings.items;
    for (const BuildingId a : t.segments) {
        const auto n = std::count_if(t.segments.begin(), t.segments.end(),
                                     [&](BuildingId b) { return b != a && joined(items.at(a), items.at(b)); });
        if (n != 2) {
            return t;
        }
    }
    // One loop, not two: walk from the first segment along the track.
    std::vector<BuildingId> seen{t.segments.front()};
    for (std::size_t i = 0; i < seen.size(); ++i) {
        for (const BuildingId b : t.segments) {
            if (std::find(seen.begin(), seen.end(), b) == seen.end() && joined(items.at(seen[i]), items.at(b))) {
                seen.push_back(b);
            }
        }
    }
    t.closed = seen.size() == t.segments.size();
    return t;
}

std::uint32_t working_hippodrome(const WorldState& st, const DataCatalog& catalog) {
    const Track t = hippodrome_track(st.city);
    const HippodromeRow* r = t.closed ? row_for(catalog, t.segments.size()) : nullptr;
    return r && st.hippodrome_horses >= r->horses ? static_cast<std::uint32_t>(t.segments.size()) : 0;
}

std::int32_t hippodrome_mood(const WorldState& st, const DataCatalog& catalog) {
    const std::uint32_t stades = working_hippodrome(st, catalog);
    const auto& b = catalog.hippodrome_bonus;
    return stades > b.unrest_above ? b.unrest_mood : stades >= b.mood_from ? b.mood : 0;
}

void advance_hippodrome(WorldState& st, const DataCatalog& catalog, LogisticsEvents& ev) {
    const Track t = hippodrome_track(st.city);
    const HippodromeRow* r = t.closed ? row_for(catalog, t.segments.size()) : nullptr;
    if (!r) {
        return;
    }
    if (st.time.day_in_month() == 0 && st.hippodrome_horses >= r->horses) {
        st.finance.treasury += r->income;  // bets and snacks (P:14, doc 18)
        if (t.segments.size() >= catalog.hippodrome_bonus.opinion_from) {  // visitors from all over the world (P:14)
            for (TradePartner& p : st.trade.partners) {
                p.diplomacy.opinion = std::min(100, p.diplomacy.opinion + catalog.hippodrome_bonus.opinion);
            }
        }
    }
    Building& office = st.city.buildings.items.at(t.segments.front());
    const bool out = std::any_of(st.walkers.items.begin(), st.walkers.items.end(), [&](const auto& kv) {
        return kv.second.type == "hippodrome_handler" && kv.second.home == t.segments.front();
    });
    if (out || st.hippodrome_horses >= r->horses) {
        return;
    }
    if (!office.road_access) {
        return;  // the handler needs a road (P:14)
    }
    const auto dist = road_distances(st.city, {*office.road_access}, PathMode::Road);
    std::optional<std::pair<std::uint32_t, BuildingId>> best;  // the whole map: no reach (S-005)
    for (const auto& [rid, ranch] : st.city.buildings.items) {
        if (ranch.type != "horse_ranch" || !ranch.producer || !ranch.road_access || ranch.producer->output_loads == 0) {
            continue;
        }
        const std::uint32_t d = dist[st.city.map.index(*ranch.road_access)];
        if (d != kUnreachable && (!best || d < best->first)) {
            best = std::make_pair(d, rid);
        }
    }
    if (!best) {
        return;
    }
    const WalkerId w = spawn_destination_walker(st.city, st.walkers, catalog, "hippodrome_handler", t.segments.front(),
                                                *st.city.buildings.items.at(best->second).road_access);
    if (w == kNoWalker) {
        return;
    }
    Walker& walker = st.walkers.items.at(w);
    walker.job = WalkerJob::Fetch;
    walker.target = best->second;
    walker.want_good = "horses";
    walker.want_loads = 1;
    ev.spawned.push_back(w);
}

}  // namespace bo::core
