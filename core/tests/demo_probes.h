// Probes of the demo gate (test_demo.cpp): what the demo showed, and the reports of BO_DEMO_LOG, BO_DEMO_INV.
#pragma once

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "bo/core/appeal.h"
#include "bo/core/culture.h"
#include "bo/core/housing.h"
#include "bo/core/migration.h"
#include "bo/core/monsters.h"
#include "bo/core/naval.h"
#include "bo/core/simulation.h"
#include "bo/core/storage.h"

namespace bo::test::demo {

using namespace bo::core;

inline std::size_t houses_at(const WorldState& st, const DataCatalog& cat, bool elite, std::size_t level) {
    std::size_t n = 0;
    for (const auto& [id, b] : st.city.buildings.items) {
        n += b.house && b.house->elite == elite && b.house->population > 0 && b.house->level >= level ? 1 : 0;
    }
    (void)cat;
    return n;
}

inline void report(const Simulation& sim, std::uint64_t year) {
    const WorldState& st = sim.state();
    std::printf("year %llu: pop %u, workforce %u/%u, popularity %u, treasury %lld, tenement+ %zu, elite %zu",
                static_cast<unsigned long long>(year), city_population(st.city), st.labour.employed,
                st.labour.workforce, st.migration.popularity, static_cast<long long>(st.finance.treasury),
                houses_at(st, sim.catalog(), false, 4), houses_at(st, sim.catalog(), true, 0));
    std::printf(", last year taxes %lld wages %lld building %lld", static_cast<long long>(st.finance.last_year.taxes),
                static_cast<long long>(st.finance.last_year.wages), static_cast<long long>(st.finance.last_year.construction));
    std::printf(", stock carrots %u onions %u fleece %u oil %u marble %u, goals",
                city_stock(st.city, sim.catalog(), "carrots"), city_stock(st.city, sim.catalog(), "onions"),
                city_stock(st.city, sim.catalog(), "fleece"), city_stock(st.city, sim.catalog(), "olive_oil"),
                city_stock(st.city, sim.catalog(), "marble"));
    for (const auto& g : st.scenario.goals) {
        std::printf(" %s%s", g.type.c_str(), g.met ? "+" : "-");
    }
    for (const auto& [id, b] : st.city.buildings.items) {
        if (b.type == "heros_hall") {
            std::printf(", Perseus lacks:");
            for (const auto& r : unmet_requirements(st, sim.catalog(), "perseus", id)) {
                std::printf(" %s;", r.c_str());
            }
        }
        if (b.sanctuary) {
            std::printf(", %s complete %d favour lost %d needs", b.type.c_str(), b.sanctuary->complete ? 1 : 0,
                        b.sanctuary->favor_lost ? 1 : 0);
            for (const auto& [g, n] : b.sanctuary->needed) {
                std::printf(" %s %d", g.c_str(), static_cast<int>(n));
            }
        }
    }
    std::printf(", wood %u sculpture %u bronze %u", city_stock(st.city, sim.catalog(), "wood"),
                city_stock(st.city, sim.catalog(), "sculpture"), city_stock(st.city, sim.catalog(), "bronze"));
    for (const auto& q : st.scenario.quests) {
        std::printf(", quest %s %s status %d days %u", q.god.c_str(), q.hero.c_str(), static_cast<int>(q.status), q.days_left);
    }
    for (const auto& h : st.heroes) {
        std::printf(", hero %s walker %u", h.id.c_str(), h.walker);
    }
    for (const auto& m : st.monsters) {
        std::printf(", monster %s slain %d", m.id.c_str(), m.slain ? 1 : 0);
    }
    std::printf("\n");
}

inline const Building* w_levels(const Simulation& sim, BuildingId id) {
    const auto it = sim.state().city.buildings.items.find(id);
    return it == sim.state().city.buildings.items.end() || !it->second.house ? nullptr : &it->second;
}

// BO_DEMO_LOG=2: per building type, the count, mean staffing and stock; houses by level with their goods.
inline void dump(const Simulation& sim) {
    std::map<std::string, std::array<std::uint64_t, 4>> types;  // count, staffing, output, stall units
    std::map<std::size_t, std::array<std::uint64_t, 5>> levels;  // count, food, fleece, oil, water-covered
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        auto& t = types[b.type];
        ++t[0];
        t[1] += b.staffing_percent;
        t[2] += b.producer ? b.producer->output_loads : 0;
        if (b.stall) {
            for (const auto& [g, n] : b.stall->units) {
                t[3] += n;
            }
        }
        if (b.storage) {
            for (const auto& [g, n] : b.storage->loads) {
                t[3] += n;
            }
        }
        if (b.house && b.house->population > 0) {
            auto& l = levels[b.house->level + (b.house->elite ? 10U : 0U)];
            ++l[0];
            l[1] += b.house->goods[0];
            l[2] += b.house->goods[1];
            l[3] += b.house->goods[2];
            l[4] += b.coverage.count("water_bearer") && b.coverage.at("water_bearer") > 0 ? 1 : 0;
        }
    }
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        if (b.storage) {
            std::printf("  %s %d,%d:", b.type.c_str(), b.origin.x, b.origin.y);
            for (const auto& [g, n] : b.storage->loads) {
                std::printf(" %s %u", g.c_str(), n);
            }
            std::printf("\n");
        }
    }
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        if (b.house && b.house->elite) {
            std::printf("  elite %d,%d pop %u incoming %u level %u road %d abandoned %d quarantine %u appeal %d culture %d"
                        " goods", b.origin.x, b.origin.y, b.house->population, b.house->incoming, b.house->level,
                        b.road_access ? 1 : 0, b.house->abandoned ? 1 : 0, b.house->quarantine_days,
                        building_appeal(sim.state().city, b),
                        house_culture(b, sim.catalog(), city_culture_bonus(sim.state().city, sim.catalog())));
            for (const auto g : b.house->goods) {
                std::printf(" %u", static_cast<unsigned>(g));
            }
            for (const auto& [walker, days] : b.coverage) {
                std::printf(" %s:%u", walker.c_str(), static_cast<unsigned>(days));
            }
            std::printf("\n");
        }
        if (b.type == "growers_lodge" || b.type == "olive_press" || b.type == "foundry" || b.type == "sculpture_studio" || b.type == "trireme_wharf" ||
            b.type == "timber_mill" || b.type == "fishery" || b.type == "urchin_quay" || b.type == "boar_hunters_lodge" ||
            b.type == "horse_ranch") {
            std::printf("  %s %d,%d road %d staff %u in %u out %u progress %u", b.type.c_str(), b.origin.x, b.origin.y,
                        b.road_access ? 1 : 0, b.staffing_percent, b.producer->input_loads, b.producer->output_loads, b.producer->progress);
            for (const auto& [g, n] : b.producer->units) {
                std::printf(" %s %u", g.c_str(), n);
            }
            std::printf("\n");
        }
    }
    std::uint32_t trees = 0, ripe = 0, untended = 0;
    for (const auto& [id, b] : sim.state().city.buildings.items) {
        if (b.type == "olive_tree") {
            ++trees;
            ripe += b.orchard->growth / 192;
            untended = std::max<std::uint32_t>(untended, b.orchard->days_since_tended);
        }
    }
    std::printf("  olive trees %u mean ripeness %u max untended %u\n", trees, trees ? ripe / trees : 0, untended);
    for (const auto& [t, v] : types) {
        std::printf("  %-22s n %3llu staff %3llu%% out %4llu stock %llu\n", t.c_str(), static_cast<unsigned long long>(v[0]),
                    static_cast<unsigned long long>(v[1] / v[0]), static_cast<unsigned long long>(v[2]),
                    static_cast<unsigned long long>(v[3]));
    }
    for (const auto& [lv, v] : levels) {
        std::printf("  house level %zu: n %llu food %llu fleece %llu oil %llu water %llu\n", lv,
                    static_cast<unsigned long long>(v[0]), static_cast<unsigned long long>(v[1]),
                    static_cast<unsigned long long>(v[2]), static_cast<unsigned long long>(v[3]),
                    static_cast<unsigned long long>(v[4]));
    }
}


// The goods every chain of the demo must bring to storage (Z:39-58, Z:147): farms, herds, the sea, the hunt,
// orchards and vines, woods and quarries, foundries, workshops and the ranch.
constexpr const char* kChains[] = {"wheat", "carrots", "onions", "cheese", "fish", "urchins", "pork", "olives",
                                   "grapes", "fleece", "olive_oil", "wine", "wood", "marble", "bronze", "sculpture",
                                   "armor"};  // horses go to the estates, not to storage: "horsemen"

// Mechanics shown by events beyond count_events: hazards by kind, trade both ways, the world's answers.
inline void note_events(const std::vector<TimedEvent>& events, std::map<std::string, std::uint32_t>& seen) {
    for (const auto& e : events) {
        if (const auto* d = std::get_if<Disaster>(&e.event)) {
            constexpr const char* kKinds[] = {"fire", "burned_down", "collapse", "plague", "crime"};  // Z:84-90
            ++seen[kKinds[static_cast<std::size_t>(d->kind)]];
        } else if (const auto* t = std::get_if<TradeDone>(&e.event)) {
            seen["import"] += t->loads_bought > 0 ? 1 : 0;  // Z:74-79
            seen["export"] += t->loads_sold > 0 ? 1 : 0;
        } else if (const auto* r = std::get_if<RequestClosed>(&e.event)) {
            seen["request_fulfilled"] += r->fulfilled ? 1 : 0;  // Z:161-164
        } else if (const auto* tr = std::get_if<TributePaid>(&e.event)) {
            seen["tribute"] += tr->amount > 0 ? 1 : 0;  // Z:172
            seen["tribute_paid"] += tr->amount < 0 ? 1 : 0;  // to the rival who won (Z:172)
        } else if (const auto* end = std::get_if<InvasionEnded>(&e.event)) {
            seen["bribed"] += end->result == 1 ? 1 : 0;  // Z:137
            seen["surrendered"] += end->result == 2 ? 1 : 0;
        } else if (const auto* gh = std::get_if<GodHelped>(&e.event)) {
            ++seen["god_helped"];  // Z:119-122
            ++seen["god_helped_" + gh->god];
        } else if (std::holds_alternative<UndoApplied>(e.event)) {
            ++seen["undo"];  // Z:20
        } else if (const auto* x = std::get_if<ExpeditionReturned>(&e.event)) {
            seen["army_won_abroad"] += x->won ? 1 : 0;  // Z:177-178
        } else if (const auto* g = std::get_if<GodAttacked>(&e.event)) {
            seen["god_repelled"] += g->repelled ? 1 : 0;  // Z:119, Z:122
        } else if (const auto* m = std::get_if<MonsterSlain>(&e.event)) {
            seen["hero_slew"] += m->by_hero ? 1 : 0;  // Z:129-131
        } else if (const auto* w = std::get_if<WalkerSpawned>(&e.event)) {
            seen["wolves"] += w->type == "wolf" ? 1 : 0;  // Z:45
        } else if (const auto* pm = std::get_if<PeopleMoved>(&e.event)) {
            seen["emigration"] += pm->left > 0 ? 1 : 0;  // Z:159-160
        } else if (const auto* h = std::get_if<HouseChanged>(&e.event)) {
            auto& top = seen[h->elite ? "top_elite_level" : "top_common_level"];
            top = std::max<std::uint32_t>(top, h->level);
        }
    }
}

// Mechanics shown by the city's state: what the chains made, culture and blessings in the houses (Z:105-109).
inline void note_state(const Simulation& sim, std::map<std::string, std::uint32_t>& seen) {
    const WorldState& st = sim.state();
    for (const char* g : kChains) {
        const auto a = st.produced_this_year.find(g);
        const auto b = st.produced_best_year.find(g);
        seen[std::string("made_") + g] += (a != st.produced_this_year.end() && a->second > 0) ||
                                                  (b != st.produced_best_year.end() && b->second > 0)
                                              ? 1 : 0;
    }
    seen["taxes"] += st.finance.last_year.taxes > 0 ? 1 : 0;  // Z:95-97
    seen["wages_set"] += st.finance.wage_rate != WageRate::Normal ? 1 : 0;  // Z:59-60
    seen["industry_off"] += st.labour.disabled_types.empty() ? 0 : 1;  // Z:62-63
    for (const TradePartner& p : st.trade.partners) {
        seen["gift_sent"] += p.diplomacy.gift_units_this_year.empty() ? 0 : 1;  // Z:176
    }
    for (const PendingDelivery& d : st.deliveries) {
        seen["goods_asked"] += d.good != "drachmas" ? 1 : 0;  // Z:174-175
    }
    for (const Ship& sh : st.military.ships) {
        seen["ship_attack_all"] += sh.order == ShipOrder::AttackAll ? 1 : 0;  // Z:142
        seen["ship_repair"] += sh.order == ShipOrder::Repair ? 1 : 0;
    }
    for (const Company& c : st.military.companies) {
        seen["sparti"] += c.nation == "divine" ? 1 : 0;  // Ares's Sparti (Z:121, Z:140)
    }
    for (const Company& c : st.military.companies) {  // the generals mustered the city's own (Z:145)
        seen["auto_defend"] += st.auto_defend && st.military.invasion.active && c.lender < 0 && c.nation.empty() &&
                                       c.status != CompanyStatus::AtHome && c.status != CompanyStatus::ServingAbroad
                                   ? 1 : 0;
    }
    for (const auto& [id, b] : st.city.buildings.items) {
        if (b.house) {
            for (const char* w : {"philosopher", "athlete", "actor", "competitor"}) {
                const auto c = b.coverage.find(w);
                seen[std::string("culture_") + w] += c != b.coverage.end() && c->second > 0 ? 1 : 0;
            }
        }
        seen["sanctified"] += b.sanctified ? 1 : 0;  // doc 21
        for (const char* t : {"grand_agora", "park", "bench", "fish_pond", "boulevard", "avenue", "water_crossing"}) {
            seen[std::string("built_") + t] += b.type == t ? 1 : 0;  // Z:69-70, Z:101-103, Z:154-155
        }
        if (b.type == "grand_agora") {
            for (const BuildingId s : b.slots) {
                const auto stall = st.city.buildings.items.find(s);
                seen["grand_agora_sells"] +=
                    s != kNoBuilding && stall != st.city.buildings.items.end() && stall->second.stall &&
                            !stall->second.stall->units.empty()
                        ? 1 : 0;
            }
        }
        if (b.storage) {
            for (const auto& [good, o] : b.storage->orders) {
                seen["storage_empty"] += o.kind == StorageOrderKind::Empty ? 1 : 0;  // Z:67
            }
        }
    }
}

inline void trace_invasion(const Simulation& sim, std::uint64_t day) {  // BO_DEMO_INV=1
    if (std::getenv("BO_DEMO_INV") == nullptr) {
        return;
    }
    static bool was = false;
    const Invasion& inv = sim.state().military.invasion;
    if (inv.active != was) {
        std::printf("day %llu invasion %d nation %s city %d sea %d invaders %zu\n", static_cast<unsigned long long>(day),
                    inv.active ? 1 : 0, inv.nation.c_str(), inv.city, inv.by_sea ? 1 : 0, inv.invaders.size());
        was = inv.active;
    }
    if (inv.active && day % 64 == 0) {
        std::printf("  day %llu invaders %zu:", static_cast<unsigned long long>(day), inv.invaders.size());
        for (const WalkerId id : inv.invaders) {
            const Walker& w = sim.state().walkers.items.at(id);
            std::printf(" %s@%d,%d ph%d", w.type.c_str(), w.tile.x, w.tile.y, static_cast<int>(w.phase));
        }
        std::printf("\n   companies:");
        for (const Company& c : sim.state().military.companies) {
            std::printf(" %s/%s st%d fl%d m%u n%zu @%d,%d t%d;", c.unit.c_str(), c.nation.c_str(),
                        static_cast<int>(c.status), c.fled ? 1 : 0, static_cast<unsigned>(c.morale),
                        c.soldiers.size(), c.standard.x, c.standard.y, static_cast<int>(c.tactic));
        }
        std::printf("\n");
    }
}

}  // namespace bo::test::demo
