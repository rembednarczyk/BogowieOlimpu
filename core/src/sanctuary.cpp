// Module D1 stage 1: sanctuary construction and sacrifices (docs/gameplay/10, 21; Z:116-118, Z:124; L-052, L-111).
#include "bo/core/gods.h"
#include "bo/core/blessings.h"
#include "bo/core/sanctuary.h"

#include <algorithm>
#include <optional>

#include "bo/core/logistics.h"
#include "bo/core/path.h"
#include "bo/core/state.h"
#include "bo/core/storage.h"
#include "bo/core/walker.h"

namespace bo::core {
namespace {

const char* const kMaterials[] = {"marble", "wood", "sculpture", "orichalc"};  // [ZAŁOŻENIE, L-111]; orichalc: monuments (L-128)

// Walkers of a job heading for or standing at `target`.
std::vector<WalkerId> walkers_for(const WorldState& st, WalkerJob job, BuildingId target) {
    std::vector<WalkerId> out;
    for (const auto& [id, w] : st.walkers.items) {
        if (w.job == job && w.target == target && w.phase != WalkerPhase::Done) {
            out.push_back(id);
        }
    }
    return out;
}

// Nearest building by road from `from` that passes `ok`.
template <class Pred>
std::optional<BuildingId> nearest_by_road(const CityState& city, TilePos from, Pred ok) {
    const auto dist = road_distances(city, {from}, PathMode::Road);
    std::optional<std::pair<std::uint32_t, BuildingId>> best;
    for (const auto& [id, b] : city.buildings.items) {
        if (!b.road_access || !ok(b)) {
            continue;
        }
        const std::uint32_t d = dist[city.map.index(*b.road_access)];
        if (d != kUnreachable && (!best || d < best->first)) {
            best = std::make_pair(d, id);
        }
    }
    if (!best) {
        return std::nullopt;
    }
    return best->second;
}

int phase_of(const DataCatalog& catalog, const Building& site);

// The materials an artisan works with in the site's phase (S-005): tiles of marble, then marble and wood, then
// the sculptures, with the orichalc of the Citadel of Poseidon (P:26, B-025) [ZAŁOŻENIE, L-134]; a monument
// takes any (L-128).
std::vector<const char*> phase_goods(const DataCatalog& catalog, const Building& site) {
    if (catalog.sanctuaries.at(site.type).monument) {
        return {"marble", "orichalc", "wood", "sculpture"};
    }
    const int phase = phase_of(catalog, site);
    return phase == 1 ? std::vector<const char*>{"marble"}
           : phase == 2 ? std::vector<const char*>{"marble", "wood"}
                        : std::vector<const char*>{"sculpture", "orichalc"};
}

// B-022: something to build on the site now (its phase's material delivered).
bool has_work(const DataCatalog& catalog, const Building& site) {
    if (!site.sanctuary || site.sanctuary->complete) {
        return false;
    }
    const SanctuaryState& s = *site.sanctuary;
    for (const char* good : phase_goods(catalog, site)) {
        const auto need = s.needed.find(good);
        const auto have = s.on_site.find(good);
        if (need != s.needed.end() && need->second > 0 && have != s.on_site.end() && have->second > 0) {
            return true;
        }
    }
    return false;
}

bool work_elsewhere(const WorldState& st, const DataCatalog& catalog, BuildingId except) {
    return std::any_of(st.city.buildings.items.begin(), st.city.buildings.items.end(), [&](const auto& kv) {
        return kv.first != except && kv.second.road_access && has_work(catalog, kv.second);
    });
}

void dispatch_artisans(WorldState& st, const DataCatalog& catalog, MythEvents& ev) {
    for (auto& [gid, g] : st.city.buildings.items) {
        if (g.type != "artisans_guild" || !g.road_access || g.staffing_percent == 0) {
            continue;
        }
        // One artisan out at a time; with Atlas's blessing a guild sends two (P:22) [ZAŁOŻENIE, L-134].
        const auto out = std::count_if(st.walkers.items.begin(), st.walkers.items.end(), [gid = gid](const auto& kv) {
            return kv.second.home == gid && kv.second.job == WalkerJob::Build;
        });
        const bool busy = out >= (god_blesses(st.city, catalog, "atlas") ? catalog.blessings.atlas_artisans : 1);
        // B-022: the nearest site with its stone delivered; only when none has any, the nearest site to wait at.
        auto site = nearest_by_road(st.city, *g.road_access, [&catalog](const Building& b) {
            return has_work(catalog, b) && !b.sanctuary->paused;
        });
        if (!site) {
            site = nearest_by_road(st.city, *g.road_access, [](const Building& b) {
                return b.sanctuary && !b.sanctuary->complete && !b.sanctuary->paused;
            });
        }
        if (busy || !site) {
            continue;
        }
        const WalkerId w = spawn_destination_walker(st.city, st.walkers, catalog, "artisan", gid,
                                                    *st.city.buildings.items.at(*site).road_access);
        if (w != kNoWalker) {
            st.walkers.items.at(w).job = WalkerJob::Build;
            st.walkers.items.at(w).target = *site;
            ev.spawned.push_back(w);
        }
    }
}

// One cart per material and site a day from the nearest Storehouse holding a full delivery (Z:117).
void dispatch_supplies(WorldState& st, const DataCatalog& catalog, BuildingId site_id, MythEvents& ev) {
    Building& site = st.city.buildings.items.at(site_id);
    std::map<std::string, std::uint32_t> coming;
    for (const WalkerId id : walkers_for(st, WalkerJob::Supply, site_id)) {
        for (const auto& [good, loads] : st.walkers.items.at(id).cargo) {
            coming[good] += loads;
        }
    }
    const bool monument = catalog.sanctuaries.at(site.type).monument;
    for (const char* good : kMaterials) {
        if (st.stockpiled.count(good) != 0 && !(monument && std::string(good) == "marble")) {
            continue;  // stockpiled: not for the sanctuary sites (Z:80); a monument may take black marble
        }
        const std::uint32_t needed = site.sanctuary->needed[good];
        const std::uint32_t have = site.sanctuary->on_site[good] + coming[good];
        if (needed <= have) {
            continue;
        }
        const std::uint32_t batch = std::string(good) == "sculpture" ? 1U : std::min(kBulkLoads, needed - have);
        const auto holding = [&](const std::string& g) {
            return [&, g](const Building& b) {
                if (!b.storage || b.staffing_percent == 0) {
                    return false;
                }
                if (b.trade) {  // a trade building importing the material (Z:117); export goods stay (Z:79)
                    const auto order = b.trade->orders.find(g);
                    if (order == b.trade->orders.end() || !order->second.import_on || order->second.export_on) {
                        return false;
                    }
                }
                const auto it = b.storage->loads.find(g);
                return it != b.storage->loads.end() && it->second >= batch;
            };
        };
        std::string load = good;
        std::optional<BuildingId> from;
        if (st.stockpiled.count(load) == 0) {
            from = nearest_by_road(st.city, *site.road_access, holding(load));
        }
        if (!from && load == "marble" && monument && st.stockpiled.count("black_marble") == 0) {
            load = "black_marble";  // a monument takes white or black marble (P:26-28) [ZAŁOŻENIE, L-128]
            from = nearest_by_road(st.city, *site.road_access, holding(load));
        }
        if (!from) {
            continue;
        }
        const WalkerId w = spawn_destination_walker(st.city, st.walkers, catalog, "ox_cart", *from, *site.road_access);
        if (w == kNoWalker) {
            continue;
        }
        st.city.buildings.items.at(*from).storage->loads[load] -= batch;  // built in as marble
        Walker& cart = st.walkers.items.at(w);
        cart.job = WalkerJob::Supply;
        cart.target = site_id;
        cart.cargo[good] = batch;
        ev.spawned.push_back(w);
    }
}

// Phases (S-005 Vriesea, doc 21): 1 tiles, marble only; 2 construction, god statues and altar: 2 marble a
// wood, 4 marble for the altar, one artisan at a time; 3 sculptures. The data fit: Artemis 63 marble - 2 x 20
// - 4 = 19 slabs = 38 tiles, Zeus 145 - 56 - 4 = 85 slabs = 170 tiles.
int phase_of(const DataCatalog& catalog, const Building& site) {
    const SanctuaryState& s = *site.sanctuary;
    const auto wood = static_cast<std::uint32_t>(catalog.sanctuaries.at(site.type).materials.at("wood"));
    const auto left = [&s](const char* g) {
        const auto it = s.needed.find(g);
        return it == s.needed.end() ? 0U : it->second;
    };
    if (left("marble") > 2 * wood + kAltarMarble) {
        return 1;
    }
    return left("marble") + left("wood") > 0 ? 2 : 3;
}

void build(WorldState& st, const DataCatalog& catalog, BuildingId site_id, MythEvents& ev) {
    Building& site = st.city.buildings.items.at(site_id);
    SanctuaryState& s = *site.sanctuary;
    for (const WalkerId id : walkers_for(st, WalkerJob::Supply, site_id)) {
        Walker& cart = st.walkers.items.at(id);
        if (cart.phase == WalkerPhase::AtTarget) {
            for (const auto& [good, loads] : cart.cargo) {
                s.on_site[good] += loads;
            }
            cart.cargo.clear();
            send_home(st.city, cart);
        }
    }
    const auto artisans = walkers_for(st, WalkerJob::Build, site_id);
    bool one_at_work = false;
    // B-022: with nothing to build here and stone waiting on another site, the idle artisans go back to their
    // guild, which sends them where the work is.
    const bool leave = !has_work(catalog, site) && work_elsewhere(st, catalog, site_id);
    for (const WalkerId id : artisans) {
        Walker& w = st.walkers.items.at(id);
        const bool monument = catalog.sanctuaries.at(site.type).monument;
        const int phase = monument ? 0 : phase_of(catalog, site);
        if (w.phase == WalkerPhase::AtTarget && (leave || (phase == 2 && one_at_work && work_elsewhere(st, catalog, site_id)))) {
            w.job = WalkerJob::None;
            send_home(st.city, w);
            continue;
        }
        if (w.phase != WalkerPhase::AtTarget || (phase == 2 && one_at_work)) {
            continue;  // phase 2: one artisan only (S-005)
        }
        one_at_work = true;
        const std::vector<const char*> goods = phase_goods(catalog, site);  // a monument: any, a load a day [L-128]
        for (const char* good : goods) {
            if (s.needed[good] > 0 && s.on_site[good] > 0) {
                --s.needed[good];
                --s.on_site[good];
                if (phase == 1 && ++w.work_ticks >= kSlabsPerShift) {
                    w.job = WalkerJob::None;  // after 8 tiles back to the guild; it sends him again (S-005)
                    send_home(st.city, w);
                }
                break;
            }
        }
    }
    const bool done = std::all_of(s.needed.begin(), s.needed.end(), [](const auto& kv) { return kv.second == 0; });
    if (!done) {
        return;
    }
    s.complete = true;
    ev.completed.push_back(site_id);
    for (const BuildingId id : plant_grove(st.city, catalog, st.difficulty, site_id)) {
        ev.planted.push_back(id);  // Athena's olives, Dionysus's vines (L-131)
    }
    for (const TilePos t : bless_land(st.city, catalog, site_id)) {
        ev.land.push_back(t);  // Hades's silver, Hephaestus's copper, Demeter's meadow (L-131)
        ev.land_of = site_id;
    }
    for (const WalkerId id : artisans) {
        Walker& w = st.walkers.items.at(id);
        w.job = WalkerJob::None;
        send_home(st.city, w);
    }
}

// Priests take a sheep or a goat for the altar, without animals food from a Granary (Z:124).
void sacrifice(WorldState& st, const DataCatalog& catalog, BuildingId id, MythEvents& ev) {
    Building& b = st.city.buildings.items.at(id);
    SanctuaryState& s = *b.sanctuary;
    s.days_since_sacrifice = static_cast<std::uint16_t>(std::min(s.days_since_sacrifice + 1, 0xFFFF));
    const auto priest = st.walkers.items.find(s.priest);
    if (priest != st.walkers.items.end() && priest->second.job == WalkerJob::Sacrifice &&
        priest->second.phase == WalkerPhase::AtTarget) {
        Walker& w = priest->second;
        const auto t = st.city.buildings.items.find(w.target);
        if (t != st.city.buildings.items.end() && t->second.animal) {
            remove_building(st.city, w.target, ev.removed);
            s.days_since_sacrifice = 0;
        } else if (t != st.city.buildings.items.end() && t->second.storage) {
            for (auto& [good, loads] : t->second.storage->loads) {
                if (loads > 0 && catalog.goods.count(good) != 0 && catalog.goods.at(good) == "food") {
                    --loads;
                    s.days_since_sacrifice = 0;
                    break;
                }
            }
        }
        w.job = WalkerJob::None;
        send_home(st.city, w);
    }
    s.favor_lost = s.days_since_sacrifice > kFavorLostAfterDays;
    if (st.walkers.items.count(s.priest) != 0 || s.days_since_sacrifice < kSacrificeEveryDays ||
        b.staffing_percent == 0 || !b.road_access) {
        return;
    }
    std::optional<std::pair<std::int32_t, BuildingId>> animal;
    for (const auto& [aid, a] : st.city.buildings.items) {
        const std::int32_t d = std::max(std::abs(a.origin.x - b.origin.x), std::abs(a.origin.y - b.origin.y));
        if (a.animal && (!animal || d < animal->first)) {
            animal = std::make_pair(d, aid);
        }
    }
    WalkerId w = kNoWalker;
    BuildingId target = kNoBuilding;
    if (animal) {
        target = animal->second;
        w = spawn_destination_walker(st.city, st.walkers, catalog, "priest", id,
                                     st.city.buildings.items.at(target).origin, PathMode::Walkable, target);
    }
    if (w == kNoWalker) {
        const auto granary = nearest_by_road(st.city, *b.road_access, [&](const Building& g) {
            if (g.type != "granary" || !g.storage) {
                return false;
            }
            return std::any_of(g.storage->loads.begin(), g.storage->loads.end(), [&](const auto& kv) {
                return kv.second > 0 && catalog.goods.count(kv.first) != 0 && catalog.goods.at(kv.first) == "food";
            });
        });
        if (!granary) {
            return;
        }
        target = *granary;
        w = spawn_destination_walker(st.city, st.walkers, catalog, "priest", id,
                                     *st.city.buildings.items.at(target).road_access);
    }
    if (w != kNoWalker) {
        st.walkers.items.at(w).job = WalkerJob::Sacrifice;
        st.walkers.items.at(w).target = target;
        s.priest = w;
        ev.spawned.push_back(w);
    }
}

}  // namespace

std::string sanctuary_site_check(const CityState& city, const DataCatalog& catalog, const std::string& type) {
    const auto p = catalog.sanctuaries.find(type);
    if (p == catalog.sanctuaries.end() || p->second.monument) {
        return {};  // a monument: no limit, no marble before (S-008)
    }
    const auto count = std::count_if(city.buildings.items.begin(), city.buildings.items.end(), [&](const auto& kv) {
        const auto q = catalog.sanctuaries.find(kv.second.type);
        return kv.second.sanctuary.has_value() && (q == catalog.sanctuaries.end() || !q->second.monument);
    });
    if (count >= catalog.max_sanctuaries) {
        return "build.too_many_sanctuaries";
    }
    if (city_stock(city, catalog, "marble") < static_cast<std::uint32_t>(p->second.initial_marble)) {
        return "build.sanctuary_needs_marble";
    }
    return {};
}

void lay_foundation(CityState& city, const DataCatalog& catalog, Building& b) {
    const SanctuaryParams& p = catalog.sanctuaries.at(b.type);
    auto left = static_cast<std::uint32_t>(p.initial_marble);
    for (auto& [id, s] : city.buildings.items) {
        if (left == 0) {
            break;
        }
        if (!s.storage || s.trade) {
            continue;
        }
        auto& loads = s.storage->loads["marble"];
        const std::uint32_t n = std::min(loads, left);
        loads -= n;
        left -= n;
    }
    SanctuaryState state;
    for (const auto& [good, loads] : p.materials) {
        state.needed[good] = static_cast<std::uint32_t>(loads);  // the full column comes after the foundation [L-111]
    }
    b.sanctuary = state;
}

MythEvents advance_sanctuaries(WorldState& st, const DataCatalog& catalog) {
    MythEvents ev;
    for (auto& [id, w] : st.walkers.items) {  // the site or the animal is gone: back home
        const bool myth = w.job == WalkerJob::Supply || w.job == WalkerJob::Build || w.job == WalkerJob::Sacrifice;
        if (myth && w.phase == WalkerPhase::AtTarget && st.city.buildings.items.count(w.target) == 0) {
            send_home(st.city, w);
        }
    }
    dispatch_artisans(st, catalog, ev);
    std::vector<BuildingId> sites;
    for (const auto& [id, b] : st.city.buildings.items) {
        if (b.sanctuary) {
            sites.push_back(id);
        }
    }
    for (const BuildingId id : sites) {
        if (st.city.buildings.items.count(id) == 0) {
            continue;
        }
        const Building& b = st.city.buildings.items.at(id);
        // A monument starts when its plank reaches a road (P:26-28) and waits while held [ZAŁOŻENIE, L-128].
        if (!b.sanctuary->complete && b.road_access && !b.sanctuary->paused) {
            build(st, catalog, id, ev);
            dispatch_supplies(st, catalog, id, ev);
        } else if (b.sanctuary->complete && !catalog.sanctuaries.at(b.type).monument) {
            sacrifice(st, catalog, id, ev);  // a god's sanctuary; a monument has no priests (L-128)
        }
    }
    return ev;
}

std::string pause_construction(WorldState& st, const DataCatalog& catalog, BuildingId site, bool on) {
    const auto it = st.city.buildings.items.find(site);
    if (it == st.city.buildings.items.end() || !it->second.sanctuary || it->second.sanctuary->complete) {
        return "myth.no_site";
    }
    if (!catalog.sanctuaries.at(it->second.type).monument) {
        return "myth.not_a_monument";  // a god's sanctuary goes on (P:26-28 speaks of the monuments)
    }
    it->second.sanctuary->paused = on;
    if (on) {
        for (auto& [id, w] : st.walkers.items) {
            if (w.target == site && (w.job == WalkerJob::Build) && w.phase == WalkerPhase::AtTarget) {
                w.job = WalkerJob::None;
                send_home(st.city, w);  // the artisans go back to their guild
            }
        }
    }
    return {};
}

}  // namespace bo::core
