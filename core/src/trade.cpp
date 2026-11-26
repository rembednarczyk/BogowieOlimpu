#include "bo/core/trade.h"

#include <algorithm>
#include <cstdlib>
#include <optional>

#include "bo/core/path.h"
#include "bo/core/walker.h"

namespace bo::core {
namespace {

bool on_terms(const DataCatalog& catalog, const TradePartner& p) {
    const auto& ok = catalog.trade.trading_relations;
    return std::find(ok.begin(), ok.end(), p.relation) != ok.end();  // rivals do not trade (Z:74)
}

std::uint32_t stock_of(const Building& b, const std::string& good) {
    const auto it = b.storage->loads.find(good);
    return it == b.storage->loads.end() ? 0 : it->second;
}

std::uint32_t total_stock(const Building& b) {
    std::uint32_t n = 0;
    for (const auto& [g, l] : b.storage->loads) {
        n += l;
    }
    return n;
}

// The water tile where ships moor: the pier's first tile on water.
std::optional<TilePos> dock_of(const CityState& city, const Building& b) {
    for (const TilePos p : b.tiles) {
        if (city.map.at(p).terrain == Terrain::Water) {
            return p;
        }
    }
    return std::nullopt;
}

std::optional<TilePos> entry_of(const CityState& city, TileFeature f, TilePos near) {
    std::optional<std::pair<std::int32_t, TilePos>> best;
    for (std::size_t i = 0; i < city.map.size(); ++i) {
        if (city.map.tiles()[i].feature == static_cast<std::uint8_t>(f)) {
            const TilePos p = city.map.pos(i);
            const std::int32_t d = std::abs(p.x - near.x) + std::abs(p.y - near.y);
            if (!best || d < best->first) {
                best = std::make_pair(d, p);
            }
        }
    }
    return best ? std::optional<TilePos>(best->second) : std::nullopt;
}

WalkerId spawn_trader(WorldState& st, const DataCatalog& catalog, BuildingId id, bool sea) {
    CityState& city = st.city;
    const Building& b = city.buildings.items.at(id);
    const auto target = sea ? dock_of(city, b) : b.road_access;
    if (!target) {
        return kNoWalker;
    }
    const auto entry = entry_of(city, sea ? TileFeature::SeaEntry : TileFeature::LandEntry, *target);
    const TilePos from = entry.value_or(*target);  // no entry on the map: the trader appears at the building
    PathRequest req{from, *target, sea ? PathMode::Water : PathMode::Road};
    req.goal_building = id;  // ships moor at the pier's dock
    auto path = find_path(city, req);
    const std::string type = sea ? "trade_ship" : "trade_caravan";
    const auto t = catalog.walkers.find(type);
    if (!path || t == catalog.walkers.end()) {
        return kNoWalker;
    }
    Walker w;
    w.id = st.walkers.next_id++;
    w.type = type;
    w.home = id;
    w.tile = from;
    w.speed_raw = t->second.speed_raw;
    w.path = std::move(*path);
    w.phase = WalkerPhase::Routed;
    w.job = WalkerJob::Trade;
    w.target = id;
    w.roam_target = from;  // leaves the way it came
    const WalkerId wid = w.id;
    st.walkers.items.emplace(wid, std::move(w));
    return wid;
}

}  // namespace

bool trade_accepts(const Building& b, const DataCatalog& catalog, const std::string& good) {
    if (!b.trade || !b.storage || b.staffing_percent == 0) {
        return false;
    }
    const auto o = b.trade->orders.find(good);
    return o != b.trade->orders.end() && o->second.export_on && stock_of(b, good) < o->second.export_keep &&
           total_stock(b) < catalog.trade.building_capacity_loads;
}

bool trade_offers(const Building& b, const std::string& good) {
    if (!b.trade || !b.storage) {
        return false;
    }
    const auto o = b.trade->orders.find(good);
    return o != b.trade->orders.end() && o->second.import_on && stock_of(b, good) > 0;
}

TradeEvents dispatch_traders(WorldState& st, const DataCatalog& catalog) {
    TradeEvents ev;
    TradeWorld& tw = st.trade;
    if (st.time.month() == 0 && st.time.day_in_month() == 0) {
        for (auto& p : tw.partners) {  // yearly limits start over (Z:75)
            for (auto& [g, o] : p.sells) {
                o.traded = 0;
            }
            for (auto& [g, o] : p.buys) {
                o.traded = 0;
            }
        }
    }
    const std::uint64_t today = st.time.total_days();
    if (tw.last_trader_day != today) {
        tw.last_trader_day = today;
        tw.traders_today = 0;
    }
    for (auto& [id, b] : st.city.buildings.items) {
        if (!b.trade || b.trade->partner < 0 || b.staffing_percent == 0 || !b.road_access) {
            continue;
        }
        TradePostState& t = *b.trade;
        const TradePartner& partner = tw.partners.at(static_cast<std::size_t>(t.partner));
        const bool sea = b.type == "pier";
        if (!on_terms(catalog, partner) || partner.by_sea != sea) {
            continue;  // a land partner sends caravans to a Trading Post, a sea partner ships to a Pier
        }
        if ((sea ? st.ship_curse_days : st.caravan_curse_days) > 0) {
            continue;  // a god's curse: no merchants come (Z:125-127)
        }
        ++t.days_since_trader;
        const std::uint32_t interval = sea ? catalog.trade.ship_interval_days : catalog.trade.caravan_interval_days;
        if (t.days_since_trader < interval || st.walkers.items.count(t.trader) != 0 ||
            tw.traders_today >= catalog.trade.max_new_traders_per_day) {
            continue;
        }
        const WalkerId w = spawn_trader(st, catalog, id, sea);
        if (w != kNoWalker) {
            t.trader = w;
            t.days_since_trader = 0;
            ++tw.traders_today;
            ev.spawned.push_back(w);
        }
    }
    return ev;
}

void trade_at_target(WorldState& st, const DataCatalog& catalog, Walker& w, TradeEvents& ev) {
    const auto it = st.city.buildings.items.find(w.target);
    if (it != st.city.buildings.items.end() && it->second.trade && it->second.trade->partner >= 0) {
        Building& b = it->second;
        TradePartner& partner = st.trade.partners.at(static_cast<std::size_t>(b.trade->partner));
        const std::uint32_t cap = partner.by_sea ? catalog.trade.ship_loads : catalog.trade.caravan_loads;
        TradeDone done{b.id, 0, 0, 0};
        std::uint32_t carried_in = 0;
        for (auto& [good, offer] : partner.sells) {  // import: the trader sells to us (Z:77-78)
            const auto o = b.trade->orders.find(good);
            if (o == b.trade->orders.end() || !o->second.import_on) {
                continue;
            }
            const std::uint32_t have = stock_of(b, good);
            const std::uint32_t room = catalog.trade.building_capacity_loads - std::min(total_stock(b), catalog.trade.building_capacity_loads);
            const std::uint32_t n = std::min({o->second.import_limit > have ? o->second.import_limit - have : 0U,
                                              cap - carried_in, offer.yearly_limit - std::min(offer.traded, offer.yearly_limit),
                                              room});
            b.storage->loads[good] += n;
            offer.traded += n;
            carried_in += n;
            done.loads_bought += n;
            done.money -= static_cast<std::int64_t>(n) * offer.price;
        }
        std::uint32_t carried_out = 0;
        for (auto& [good, offer] : partner.buys) {  // export: the trader buys from us (Z:78-79)
            const auto o = b.trade->orders.find(good);
            if (o == b.trade->orders.end() || !o->second.export_on) {
                continue;
            }
            const std::uint32_t n = std::min({stock_of(b, good), cap - carried_out,
                                              offer.yearly_limit - std::min(offer.traded, offer.yearly_limit)});
            b.storage->loads[good] -= n;
            offer.traded += n;
            carried_out += n;
            done.loads_sold += n;
            done.money += static_cast<std::int64_t>(n) * offer.price;
        }
        st.finance.treasury += done.money;
        if (done.loads_bought > 0 || done.loads_sold > 0) {
            ev.done.push_back(done);
        }
    }
    // Leave the way it came (road or sea), then disappear.
    const bool sea = w.type == "trade_ship";
    PathRequest back{w.tile, w.roam_target, sea ? PathMode::Water : PathMode::Road};
    back.goal_building = w.target;
    auto path = find_path(st.city, back);
    w.job = WalkerJob::None;
    w.path = path ? std::move(*path) : std::vector<TilePos>{};
    w.path_pos = 0;
    w.phase = path ? WalkerPhase::Returning : WalkerPhase::Done;
}

}  // namespace bo::core
