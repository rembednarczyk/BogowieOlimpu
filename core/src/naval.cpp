// Module D3 stage 3: fleets (docs/gameplay/11-wojsko.md, Z:141-144; assumptions L-124). The pattern: eZeus
// (S-008) for the landing (transports of 32 soldiers; a sunk transport takes its soldiers down; one trireme a
// wharf, built again when sunk); the manual for the orders and the target priorities (Z:142-144).
#include "bo/core/naval.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <optional>
#include <tuple>

#include "bo/core/labour.h"
#include "bo/core/logistics.h"
#include "combat_internal.h"

namespace bo::core {
using namespace combat_detail;

namespace {

bool open_water(const CityState& city, TilePos p) {
    return city.map.in_bounds(p) && city.map.at(p).terrain == Terrain::Water && city.map.at(p).building == kNoBuilding;
}

// A free land tile next to a water tile: where the soldiers of a transport step ashore.
std::optional<TilePos> shore_of(const CityState& city, TilePos p) {
    for (const Dir d : kDirs) {
        const TilePos q = step(p, d);
        if (city.map.in_bounds(q) && city.map.at(q).building == kNoBuilding &&
            (city.map.at(q).road || terrain_walkable(city.map.at(q).terrain))) {
            return q;
        }
    }
    return std::nullopt;
}

// The water by the shore nearest the goal that ships from the sea entry can reach [ZAŁOŻENIE, L-124].
std::optional<TilePos> landing_for(const CityState& city, TilePos entry, TilePos goal) {
    std::vector<bool> seen(city.map.size(), false);
    std::deque<TilePos> open{entry};
    seen[city.map.index(entry)] = true;
    std::optional<std::pair<std::int32_t, TilePos>> best;
    while (!open.empty()) {
        const TilePos p = open.front();
        open.pop_front();
        const std::int32_t d = chebyshev(p, goal) * 1000 + std::abs(p.x - goal.x) + std::abs(p.y - goal.y);
        if (shore_of(city, p) && (!best || d < best->first)) {
            best = std::make_pair(d, p);
        }
        for (const Dir dir : kDirs) {
            const TilePos q = step(p, dir);
            if (open_water(city, q) && !seen[city.map.index(q)]) {
                seen[city.map.index(q)] = true;
                open.push_back(q);
            }
        }
    }
    if (!best) {
        return std::nullopt;
    }
    return best->second;
}

std::optional<TilePos> sea_entry(const CityState& city) {
    for (std::size_t i = 0; i < city.map.size(); ++i) {
        if (city.map.tiles()[i].feature == static_cast<std::uint8_t>(TileFeature::SeaEntry)) {
            return city.map.pos(i);
        }
    }
    return std::nullopt;
}

Ship* ship_of(WorldState& st, WalkerId id) {
    for (Ship& s : st.military.ships) {
        if (s.walker == id) {
            return &s;
        }
    }
    return nullptr;
}

// The wharf's mooring: where a trireme is launched and repaired; its post when the wharf is gone.
TilePos mooring_of(const WorldState& st, const Ship& s) {
    const auto b = st.city.buildings.items.find(s.wharf);
    const auto m = b != st.city.buildings.items.end() ? mooring(st.city, b->second) : std::nullopt;
    return m.value_or(s.station);
}

void launch(WorldState& st, const DataCatalog& catalog, CombatEvents& ev) {
    const auto& player = catalog.combat.nations.find("player_greek");
    if (player == catalog.combat.nations.end() || player->second.units.count("trireme") == 0) {
        return;
    }
    const UnitStats& s = player->second.units.at("trireme");
    for (auto& [id, b] : st.city.buildings.items) {
        const bool has_ship = std::any_of(st.military.ships.begin(), st.military.ships.end(),
                                          [id = id](const Ship& sh) { return sh.wharf == id; });
        const auto moor = b.type == "trireme_wharf" && b.producer ? mooring(st.city, b) : std::nullopt;
        if (has_ship || !moor || b.producer->output_loads == 0) {
            continue;
        }
        Walker w;
        w.id = st.walkers.next_id++;
        w.type = "trireme";
        w.job = WalkerJob::Ship;
        w.home = id;
        w.tile = *moor;
        w.phase = WalkerPhase::AtTarget;
        w.speed_raw = s.speed_raw;
        w.hp = s.hp;
        st.military.ships.push_back(Ship{w.id, id, ShipOrder::Defend, *moor});
        ev.spawned.push_back(w.id);
        st.walkers.items.emplace(w.id, std::move(w));
    }
}

// Z:141 with L-124: a crew under orders_min_crew_percent of its places cannot sail or fight.
bool short_handed(const DataCatalog& catalog, const Ship& sh) {
    return sh.hands * 100 < catalog.naval.crew_workers * catalog.naval.orders_min_crew_percent;
}

// Every crewed ship fills its places, in ship order, from the workers of the houses not away as rabble or with a
// trireme abroad (they leave their jobs, military.cpp workers_in_the_field). A ship sails at its speed times its
// share of the crew (Z:141, L-124).
void man_ships(WorldState& st, const DataCatalog& catalog) {
    std::uint32_t held = 0;
    for (const Ship& sh : st.military.ships) {
        held += sh.hands;
    }
    const std::uint32_t pool = labour_pool(st.city, catalog, st.difficulty, st.finance.wage_rate);
    const std::uint32_t rabble = workers_in_the_field(st, catalog) - held;
    std::uint32_t free = st.labour.full_staffing ? 0xFFFFFFFFU : pool - std::min(pool, rabble);  // dev switch: full
    for (const Ship& sh : st.military.ships) {
        free = sh.crew == Crew::Abroad ? free - std::min(free, std::uint32_t{sh.hands}) : free;  // away with the army
    }
    const auto places = static_cast<std::uint32_t>(std::max(0, catalog.naval.crew_workers));
    for (Ship& sh : st.military.ships) {
        if (sh.crew == Crew::None) {
            sh.hands = 0;
        }
        if (sh.crew != Crew::Aboard && sh.crew != Crew::GoingHome) {
            continue;
        }
        sh.hands = static_cast<std::uint8_t>(std::min({places, free, 255U}));
        free -= sh.hands;
        Walker& w = st.walkers.items.at(sh.walker);
        if (const UnitStats* s = stats_of(catalog, w); s && places > 0) {
            w.speed_raw = std::max<std::int64_t>(1, s->speed_raw * sh.hands / places);
        }
    }
}

}  // namespace

bool is_ship(const Walker& w) {
    return w.job == WalkerJob::Ship ||
           (w.job == WalkerJob::Invade && (w.type == "transport" || w.type == "warship" || w.type == "frigate"));
}

bool is_transport(const Walker& w) { return w.job == WalkerJob::Invade && w.type == "transport"; }

namespace combat_detail {

std::int32_t ship_engage(const WorldState& st, const DataCatalog& catalog, const Walker& w, const UnitStats& s) {
    if (w.job != WalkerJob::Ship) {
        return catalog.naval.attack_all_engage;  // enemy warships hunt [ZAŁOŻENIE, L-124]
    }
    for (const Ship& sh : st.military.ships) {
        if (sh.walker == w.id) {
            if (sh.crew != Crew::Aboard || short_handed(catalog, sh)) {
                return -1;  // unmanned, short-handed or the crew going home: no fighting (Z:141)
            }
            return sh.order == ShipOrder::AttackAll                                   ? catalog.naval.attack_all_engage
                   : sh.order == ShipOrder::Defend || sh.order == ShipOrder::Patrol ? s.missile_range
                                                                                    : -1;  // under repair
        }
    }
    return s.missile_range;
}

// Targets of a ship within `limit` tiles, by priority (Z:144): a trireme takes transports with soldiers, then
// warships, then soldiers within its missiles' reach of the shore, then empty transports. A warship takes
// triremes, then soldiers on the shore [ZAŁOŻENIE, L-124].
std::optional<std::pair<std::int32_t, WalkerId>> naval_foe(const WorldState& st, const DataCatalog& catalog,
                                                           const Walker& w, const std::vector<WalkerId>& fighters,
                                                           std::int32_t limit) {
    const UnitStats* s = stats_of(catalog, w);
    const std::int32_t reach = s ? std::max(1, s->missile_range) : 1;
    std::optional<std::tuple<int, std::int32_t, WalkerId>> best;
    for (const WalkerId id : fighters) {
        const Walker& o = st.walkers.items.at(id);
        const std::int32_t d = chebyshev(w.tile, o.tile);
        if (!fighter(o) || defends(o) == defends(w) || d > limit) {
            continue;
        }
        int rank = 0;
        if (w.job == WalkerJob::Ship) {
            rank = is_transport(o) ? (o.aboard > 0 ? 1 : 4) : is_ship(o) ? 2 : d <= reach ? 3 : 0;
        } else {
            rank = o.job == WalkerJob::Ship ? 1 : d <= reach ? 2 : 0;
        }
        const auto key = std::make_tuple(rank, d, id);
        if (rank > 0 && (!best || key < *best)) {
            best = key;
        }
    }
    if (!best) {
        return std::nullopt;
    }
    return std::make_pair(std::get<1>(*best), std::get<2>(*best));
}

// A trireme with nothing to fight sails back to its post (Defend Location, Z:142); on patrol it sails between
// the post and the wharf [ZAŁOŻENIE, L-124]. Without a crew, or short-handed, it does not move (Z:141).
void ship_idle(WorldState& st, const DataCatalog& catalog, Walker& w) {
    const Ship* sh = ship_of(st, w.id);
    if (!sh || sh->crew != Crew::Aboard || short_handed(catalog, *sh) || sh->order == ShipOrder::Repair || w.phase != WalkerPhase::AtTarget) {
        return;
    }
    const TilePos moor = mooring_of(st, *sh);
    if (sh->order == ShipOrder::Patrol) {
        route(st.city, w, w.tile == sh->station ? moor : sh->station);
    } else if (w.tile != sh->station) {
        route(st.city, w, sh->station);
    }
}

// Transports at the landing put their soldiers ashore, then sail away (Z:144); an empty one leaves the map.
void land_transports(WorldState& st, const DataCatalog& catalog, CombatEvents& ev) {
    Invasion& inv = st.military.invasion;
    const auto n = catalog.combat.nations.find(inv.nation);
    if (!inv.by_sea || n == catalog.combat.nations.end()) {
        return;
    }
    std::vector<WalkerId> landed;
    for (const WalkerId id : inv.invaders) {
        Walker& t = st.walkers.items.at(id);
        if (!is_transport(t) || t.hp <= 0) {
            continue;
        }
        if (t.aboard == 0) {
            t.phase = t.phase == WalkerPhase::AtTarget && t.tile == inv.origin ? WalkerPhase::Done : t.phase;
            continue;
        }
        const auto shore = chebyshev(t.tile, inv.landing) <= 1 ? shore_of(st.city, t.tile) : std::nullopt;
        if (!shore) {
            continue;
        }
        // The nation's mix, rounded down, the rest heavy infantry (doc 22) [ZAŁOŻENIE, L-124].
        const NationParams& np = n->second;
        std::uint32_t left = t.aboard;
        for (std::size_t k = np.composition_units.size(); k-- > 0;) {
            const auto pct = k < np.composition.size() ? static_cast<std::uint32_t>(np.composition[k]) : 0U;
            const std::uint32_t count = k == 0 ? left : std::min(left, t.aboard * pct / 100);
            left -= count;
            for (std::uint32_t i = 0; i < count; ++i) {
                landed.push_back(spawn_invader(st, catalog, np.composition_units[k], *shore, ev));
            }
        }
        t.aboard = 0;
        if (!route(st.city, t, inv.origin)) {
            t.phase = WalkerPhase::Done;
        }
    }
    inv.invaders.insert(inv.invaders.end(), landed.begin(), landed.end());
}

}  // namespace combat_detail

std::string start_sea_invasion(WorldState& st, const DataCatalog& catalog, std::uint32_t soldiers, BuildingId goal,
                               CombatEvents& ev) {
    Invasion& inv = st.military.invasion;
    const auto n = catalog.combat.nations.find(inv.nation);
    const auto entry = sea_entry(st.city);
    if (n == catalog.combat.nations.end() || n->second.units.count("transport") == 0 || !entry) {
        return "military.no_sea_route";
    }
    const auto land = landing_for(st.city, *entry, st.city.buildings.items.at(goal).origin);
    if (!land) {
        return "military.no_sea_route";
    }
    inv.by_sea = true;
    inv.origin = *entry;
    inv.landing = *land;
    const NavalParams& p = catalog.naval;
    const std::uint32_t per = std::max(1U, p.transport_soldiers);
    const std::uint32_t transports = (soldiers + per - 1) / per;
    const std::uint32_t warships =
        n->second.units.count("warship") != 0 ? (transports + std::max(1U, p.transports_per_warship) - 1) /
                                                     std::max(1U, p.transports_per_warship)
                                               : 0;
    for (std::uint32_t i = 0; i < warships + transports; ++i) {
        const bool transport = i >= warships;
        const WalkerId id = spawn_invader(st, catalog, transport ? "transport" : "warship", *entry, ev);
        Walker& w = st.walkers.items.at(id);
        w.aboard = transport ? static_cast<std::uint16_t>(std::min(per, soldiers - (i - warships) * per)) : 0;
        if (!route(st.city, w, *land)) {
            stand(w);
        }
        inv.invaders.push_back(id);
    }
    return {};
}

std::vector<BuildingId> sail_abroad(WorldState& st) {
    std::vector<BuildingId> out;
    for (Ship& sh : st.military.ships) {
        const auto w = st.walkers.items.find(sh.walker);
        if (sh.crew != Crew::Aboard || w == st.walkers.items.end()) {
            continue;
        }
        w->second.phase = WalkerPhase::Done;  // off the map until it comes back
        sh.walker = kNoWalker;
        sh.crew = Crew::Abroad;
        out.push_back(sh.wharf);
    }
    return out;
}

void sail_home(WorldState& st, const DataCatalog& catalog, BuildingId wharf) {
    for (Ship& sh : st.military.ships) {
        if (sh.wharf != wharf || sh.crew != Crew::Abroad) {
            continue;
        }
        const UnitStats& s = catalog.combat.nations.at("player_greek").units.at("trireme");
        Walker w;
        w.id = st.walkers.next_id++;
        w.type = "trireme";
        w.job = WalkerJob::Ship;
        w.home = wharf;
        w.tile = mooring_of(st, sh);
        w.phase = WalkerPhase::AtTarget;
        w.speed_raw = s.speed_raw;
        w.hp = s.hp;
        sh.walker = w.id;
        sh.crew = Crew::Aboard;
        sh.station = w.tile;
        st.walkers.items.emplace(w.id, std::move(w));
        return;
    }
}

void advance_fleet(WorldState& st, const DataCatalog& catalog, CombatEvents& ev) {
    auto& ships = st.military.ships;
    for (auto it = ships.begin(); it != ships.end();) {
        if (it->crew == Crew::Abroad || st.walkers.items.count(it->walker) != 0) {
            ++it;
            continue;
        }
        const auto wharf = st.city.buildings.items.find(it->wharf);
        if (wharf != st.city.buildings.items.end() && wharf->second.producer) {
            wharf->second.producer->output_loads = 0;  // sunk: the wharf builds another (S-008)
        }
        it = ships.erase(it);
    }
    if (std::none_of(st.city.buildings.items.begin(), st.city.buildings.items.end(),
                     [](const auto& kv) { return kv.second.type == "palace"; })) {
        return;  // no Palace, no crews (Z:135)
    }
    launch(st, catalog, ev);
    man_ships(st, catalog);
    for (Ship& sh : ships) {
        if (sh.crew == Crew::Abroad) {
            continue;  // with an army abroad (Z:177)
        }
        Walker& w = st.walkers.items.at(sh.walker);
        const UnitStats* s = stats_of(catalog, w);
        const std::int32_t full = s ? s->hp : w.hp;
        const TilePos moor = mooring_of(st, sh);
        if (sh.crew == Crew::GoingHome && w.tile == moor) {
            sh.crew = Crew::None;  // back at the wharf: the crew goes back to work (Z:141)
            sh.hands = 0;
        }
        if (sh.crew == Crew::None) {
            continue;  // unmanned: it waits at the wharf
        }
        if (sh.crew == Crew::GoingHome) {
            if (w.phase == WalkerPhase::AtTarget && !route(st.city, w, moor)) {
                stand(w);
            }
            continue;
        }
        if (sh.order != ShipOrder::Repair && w.hp * 100 < full * catalog.naval.repair_below_percent) {
            sh.order = ShipOrder::Repair;  // the captain turns back on his own (Z:142)
            route(st.city, w, moor);
        }
        if (sh.order != ShipOrder::Repair) {
            ship_idle(st, catalog, w);
        } else if (w.tile == moor) {
            w.hp = std::min(full, w.hp + catalog.naval.repair_hp_per_day);
            if (w.hp >= full) {
                sh.order = ShipOrder::Defend;
                route(st.city, w, sh.station);
            }
        } else if (w.phase == WalkerPhase::AtTarget && !route(st.city, w, moor)) {
            stand(w);
        }
    }
}

std::string set_ship_order(WorldState& st, const DataCatalog& catalog, std::uint32_t ship, std::uint8_t order,
                           TilePos at) {
    if (ship >= st.military.ships.size() || order > 5) {
        return "military.bad_ship";
    }
    Ship& sh = st.military.ships[ship];
    if (sh.crew == Crew::Abroad) {
        return "military.ship_abroad";
    }
    Walker& w = st.walkers.items.at(sh.walker);
    if (order == 4) {  // Call Crew to Ship (Z:141): as many free workers as there are
        sh.crew = Crew::Aboard;
        man_ships(st, catalog);
        return {};
    }
    if (order == 5) {  // Send Crew Home: to the wharf, then the crew goes back to work (Z:141)
        if (sh.crew == Crew::Aboard) {
            sh.crew = Crew::GoingHome;
            route(st.city, w, mooring_of(st, sh));
        }
        return {};
    }
    if (sh.crew == Crew::None) {
        return "military.no_crew";  // an unmanned ship takes no orders (Z:141)
    }
    if (short_handed(catalog, sh)) {
        return "military.crew_short";  // too few hands to carry out orders (Z:141)
    }
    const auto o = static_cast<ShipOrder>(order);
    if ((o == ShipOrder::Defend || o == ShipOrder::Patrol) && at.x >= 0 && !open_water(st.city, at)) {
        return "military.not_water";
    }
    sh.order = o;
    if (sh.order == ShipOrder::Repair) {
        route(st.city, w, mooring_of(st, sh));
    } else if ((sh.order == ShipOrder::Defend || sh.order == ShipOrder::Patrol) && at.x >= 0) {
        sh.station = at;
        route(st.city, w, at);
    } else if (sh.order == ShipOrder::Defend) {
        sh.station = w.tile;
    }
    return {};
}

}  // namespace bo::core
