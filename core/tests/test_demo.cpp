// Whole-game gate (docs/PRAKTYKI.md ch. 4): the autopilot builds the demo city from scenarios/demo_build.json on
// scenarios/demo.json and must win it. Every placement of the build order must be accepted, and the city must
// reach common houses that need culture (B-008) and elite houses. BO_DEMO_LOG=1 prints a yearly report.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

#include "bo/core/appeal.h"
#include "bo/core/build_order.h"
#include "bo/core/culture.h"
#include "bo/core/housing.h"
#include "bo/core/city.h"
#include "bo/core/migration.h"
#include "bo/core/monsters.h"
#include "bo/core/scenario.h"
#include "bo/core/naval.h"
#include "bo/core/simulation.h"
#include "bo/core/storage.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

std::string read_file(const std::string& path) {
    std::ifstream f(path);
    std::stringstream s;
    s << f.rdbuf();
    return s.str();
}

struct DemoRun {
    std::optional<Simulation> sim;
    BuildOrder order;
    std::vector<std::string> refused;  // placements the city refused, "day type x,y: reason"
    std::uint64_t won_day = 0;
    std::map<std::string, std::uint32_t> seen;  // mechanics that happened: event kind -> count
};

// The events that show a mechanic of the demo at work (the owner's rule: the demo shows every mechanic).
void count_events(const std::vector<TimedEvent>& events, std::map<std::string, std::uint32_t>& seen) {
    for (const auto& e : events) {
        std::visit([&seen](const auto& ev) {
            using T = std::decay_t<decltype(ev)>;
            const char* name = std::is_same_v<T, InvasionStarted>      ? "invasion"
                               : std::is_same_v<T, GodAttacked>        ? "god_attack"
                               : std::is_same_v<T, Disaster>           ? "fire_or_collapse"
                               : std::is_same_v<T, MonsterAppeared>    ? "monster"
                               : std::is_same_v<T, MonsterSlain>       ? "monster_slain"
                               : std::is_same_v<T, HeroArrived>        ? "hero"
                               : std::is_same_v<T, QuestCompleted>     ? "quest_done"
                               : std::is_same_v<T, SanctuaryCompleted> ? "sanctuary"
                               : std::is_same_v<T, CityRequestReceived> ? "request"
                               : std::is_same_v<T, TradeDone>          ? "trade"
                               : std::is_same_v<T, EpisodeWon>         ? "won"
                               : std::is_same_v<T, CityRebelled>       ? "rebellion"
                               : std::is_same_v<T, CityGift>           ? "city_gift"
                               : std::is_same_v<T, ExpeditionReturned> ? "aid_returned"
                               : std::is_same_v<T, PrayerAnswered>     ? "prayer"
                                                                       : nullptr;
            if (name != nullptr) {
                ++seen[name];
            }
        }, e.event);
    }
}

// Trial of the day's placements on a copy, in order (stalls need the agora placed just before them).
void note_refusals(const Simulation& sim, const std::vector<Command>& cmds, std::uint64_t day,
                   std::vector<std::string>& refused) {
    CityState copy = sim.state().city;
    for (const Command& c : cmds) {
        if (const auto* p = std::get_if<PlaceBuilding>(&c)) {
            const auto r = place_building(copy, sim.catalog(), sim.state().difficulty, p->type, {p->x, p->y}, p->rotation);
            const bool waits = r.reason == "build.elite_needs_goods" || r.reason.rfind("myth.", 0) == 0 ||
                               r.reason.rfind("build.sanctuary", 0) == 0;  // retry steps wait for their goods
            const std::string key = p->type + " " + std::to_string(p->x) + "," + std::to_string(p->y);
            const bool seen = std::any_of(refused.begin(), refused.end(),
                                          [&key](const std::string& s) { return s.find(key) != std::string::npos; });
            const char* trace = std::getenv("BO_DEMO_TRACE");  // e.g. BO_DEMO_TRACE=elite_housing
            if (!r.ok() && trace != nullptr && p->type == trace && day % 32 == 0) {
                std::printf("trace day %llu %s: %s\n", static_cast<unsigned long long>(day), key.c_str(), r.reason.c_str());
            }
            if (!r.ok() && !waits && !seen) {
                refused.push_back(std::to_string(day) + " " + p->type + " " + std::to_string(p->x) + "," +
                                  std::to_string(p->y) + ": " + r.reason);
            }
        } else if (const auto* road = std::get_if<PlaceRoad>(&c)) {
            place_road(copy, sim.catalog(), sim.state().difficulty, {road->x, road->y});
        }
    }
}

std::size_t houses_at(const WorldState& st, const DataCatalog& cat, bool elite, std::size_t level) {
    std::size_t n = 0;
    for (const auto& [id, b] : st.city.buildings.items) {
        n += b.house && b.house->elite == elite && b.house->population > 0 && b.house->level >= level ? 1 : 0;
    }
    (void)cat;
    return n;
}

void report(const Simulation& sim, std::uint64_t year) {
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

const Building* w_levels(const Simulation& sim, BuildingId id) {
    const auto it = sim.state().city.buildings.items.find(id);
    return it == sim.state().city.buildings.items.end() || !it->second.house ? nullptr : &it->second;
}

// BO_DEMO_LOG=2: per building type, the count, mean staffing and stock; houses by level with their goods.
void dump(const Simulation& sim) {
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

// Every mechanic the demo must show (the owner's rule); the run goes on until all are seen.
constexpr const char* kMechanics[] = {"invasion", "god_attack", "monster", "monster_slain", "hero", "quest_done",
                                      "sanctuary", "request", "trade", "won", "rebellion", "city_gift", "prayer",
                                      "aid_sent", "aid_returned", "raid_sent", "attack_sent", "games_invited", "games_hosted", "tactics_set", "hoplites", "hoplite_wedge", "trireme", "sea_invasion", "naval_hit", "lent_troops", "strike", "monument_built", "oracle", "horsemen", "crew_called", "ship_patrol", "ships_abroad", "stockpile", "not_buying", "towers_manned"};

// The goods every chain of the demo must bring to storage (Z:39-58, Z:147): farms, herds, the sea, the hunt,
// orchards and vines, woods and quarries, foundries, workshops and the ranch.
constexpr const char* kChains[] = {"wheat", "carrots", "onions", "cheese", "fish", "urchins", "pork", "olives",
                                   "grapes", "fleece", "olive_oil", "wine", "wood", "marble", "bronze", "sculpture",
                                   "armor"};  // horses go to the estates, not to storage: "horsemen"

// Mechanics shown by events beyond count_events: hazards by kind, trade both ways, the world's answers.
void note_events(const std::vector<TimedEvent>& events, std::map<std::string, std::uint32_t>& seen) {
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
void note_state(const Simulation& sim, std::map<std::string, std::uint32_t>& seen) {
    const WorldState& st = sim.state();
    for (const char* g : kChains) {
        const auto a = st.produced_this_year.find(g);
        const auto b = st.produced_best_year.find(g);
        seen[std::string("made_") + g] += (a != st.produced_this_year.end() && a->second > 0) ||
                                                  (b != st.produced_best_year.end() && b->second > 0)
                                              ? 1 : 0;
    }
    seen["taxes"] += st.finance.last_year.taxes > 0 ? 1 : 0;  // Z:95-97
    for (const auto& [id, b] : st.city.buildings.items) {
        if (b.house) {
            for (const char* w : {"philosopher", "athlete", "actor", "competitor"}) {
                const auto c = b.coverage.find(w);
                seen[std::string("culture_") + w] += c != b.coverage.end() && c->second > 0 ? 1 : 0;
            }
        }
        seen["sanctified"] += b.sanctified ? 1 : 0;  // doc 21
    }
}

bool all_seen(const std::map<std::string, std::uint32_t>& seen) {
    return std::all_of(std::begin(kMechanics), std::end(kMechanics), [&seen](const char* m) {
        const auto it = seen.find(m);  // a tally can hold a zero: seen means at least once
        return it != seen.end() && it->second > 0;
    });
}

DemoRun run_demo(std::uint64_t years) {
    DemoRun run;
    std::string error;
    auto cfg = parse_scenario(read_file(std::string(BO_DATA_DIR) + "/../scenarios/demo.json"), error);
    REQUIRE_MESSAGE(cfg, error);
    auto order = parse_build_order(read_file(std::string(BO_DATA_DIR) + "/../scenarios/demo_build.json"), error);
    REQUIRE_MESSAGE(order, error);
    run.order = std::move(*order);
    run.sim.emplace(bo::test::catalog(), *cfg);
    REQUIRE(run.sim->setup_error().empty());
    const bool log = std::getenv("BO_DEMO_LOG") != nullptr;
    std::map<BuildingId, bool> watched;  // elite houses: their needs on the first days with a family
    Simulation& sim = *run.sim;
    for (std::uint64_t day = 0; day < years * kDaysPerMonth * kMonthsPerYear; ++day) {
        const auto cmds = due_commands(run.order, sim, day);
        note_refusals(sim, cmds, day, run.refused);
        for (const auto& c : cmds) {
            sim.issue(c);
        }
        sim.run_days(1);
        const auto events = sim.drain_events();
        count_events(events, run.seen);
        note_events(events, run.seen);
        note_state(sim, run.seen);
        for (const auto& e : events) {
            if (const auto* g = std::get_if<GamesHeld>(&e.event)) {
                run.seen[g->invited ? "games_invited" : "games_not_invited"] += 1;  // Z:110-112
                run.seen["games_won"] += g->won ? 1 : 0;
                run.seen["games_hosted"] += g->hosted ? 1 : 0;  // Z:111
            }
            run.seen["oracle"] += std::holds_alternative<OracleSpoke>(e.event) ? 1 : 0;  // Z:124
            if (const auto* h = std::get_if<MilitaryHelpGiven>(&e.event)) {
                run.seen[h->strike ? "strike" : "lent_troops"] += 1;  // Z:174
            }
        }
        if (const char* m = std::getenv("BO_DEMO_MONTHLY")) {  // e.g. BO_DEMO_MONTHLY=7: a line a month from year 7
            static std::map<std::string, std::uint32_t> tally;
            for (const auto& e : events) {
                if (const auto* d = std::get_if<Disaster>(&e.event)) {
                    ++tally["disaster" + std::to_string(static_cast<int>(d->kind))];
                }
                if (const auto* h = std::get_if<HouseChanged>(&e.event)) {
                    static std::map<BuildingId, std::uint8_t> last_level;
                    const auto it = w_levels(sim, h->id);
                    if (last_level.count(h->id) && h->level < last_level[h->id] && it) {
                        // What the level above lacks now: water, food, fleece, oil, culture, appeal.
                        const Building& b = *it;
                        const auto& lv = levels_of(sim.catalog(), *b.house)[h->level + 1U];
                        const HouseModel& mo = lv.model[static_cast<std::size_t>(sim.state().difficulty)];
                        const auto wb = b.coverage.find("water_bearer");
                        if (mo.water && (wb == b.coverage.end() || wb->second == 0)) ++tally["no_water"];
                        for (std::size_t g = 0; g < 3; ++g) {
                            if (mo.needs[g] && b.house->goods[g] == 0) ++tally[g == 0 ? "no_food" : g == 1 ? "no_fleece" : "no_oil"];
                        }
                        if (house_culture(b, sim.catalog(), city_culture_bonus(sim.state().city, sim.catalog())) < mo.culture) ++tally["no_culture"];
                        if (building_appeal(sim.state().city, b) < levels_of(sim.catalog(), *b.house)[h->level + 1U].model[static_cast<std::size_t>(sim.state().difficulty)].appeal_devolve) ++tally["low_appeal"];
                        ++tally["drops"];
                    }
                    last_level[h->id] = h->level;
                }
            }
            const WorldState& w = sim.state();
            if (w.time.day_in_month() == kDaysPerMonth - 1 && day / (kDaysPerMonth * kMonthsPerYear) >= std::strtoull(m, nullptr, 10)) {
                std::size_t lv[8] = {};
                for (const auto& [id, b] : w.city.buildings.items) {
                    if (b.house && !b.house->elite && b.house->population > 0) {
                        ++lv[std::min<std::size_t>(b.house->level, 7)];
                    }
                }
                std::printf("y%llu m%u pop %u in %u out %u homeless %u pop%% %u food %u fleece %u | lv %zu %zu %zu %zu %zu %zu %zu |",
                            static_cast<unsigned long long>(day / 192), w.time.month(), city_population(w.city),
                            w.migration.arrived_this_month, w.migration.left_this_month, w.city.homeless,
                            w.migration.popularity, city_stock(w.city, sim.catalog(), "food"),
                            city_stock(w.city, sim.catalog(), "fleece"), lv[0], lv[1], lv[2], lv[3], lv[4], lv[5], lv[6]);
                for (const auto& [k, v] : tally) {
                    std::printf(" %s %u", k.c_str(), v);
                }
                std::printf("\n");
                tally.clear();
            }
        }
        if (std::getenv("BO_DEMO_VENUES") != nullptr && sim.state().time.day_in_month() == 0) {  // venues, schools
            std::printf("d%llu", static_cast<unsigned long long>(day));
            for (const auto& [id, b] : sim.state().city.buildings.items) {
                if (b.type == "stadium" || b.type == "gymnasium") {
                    std::printf(" | %s %d,%d perf %u road %d", b.type.c_str(), b.origin.x, b.origin.y,
                                static_cast<unsigned>(b.performing_days), b.road_access ? 1 : 0);
                    for (const auto& [w, st] : b.schools) {
                        std::printf(" %s:%u>%u", w.c_str(), static_cast<unsigned>(st.first), static_cast<unsigned>(st.second));
                    }
                }
            }
            for (const auto& [id, w] : sim.state().walkers.items) {
                if (w.type == "competitor") {
                    std::printf(" | comp %d,%d ph %d job %d", w.tile.x, w.tile.y, static_cast<int>(w.phase), static_cast<int>(w.job));
                }
            }
            std::printf("\n");
        }
        if (std::getenv("BO_DEMO_ELITE") != nullptr && sim.state().time.day_in_month() == 0) {  // a line a month
            std::printf("d%llu", static_cast<unsigned long long>(day));
            for (const auto& [id, b] : sim.state().city.buildings.items) {
                if (b.house && b.house->elite) {
                    std::printf(" | %d,%d p%u l%u a%d g", b.origin.x, b.origin.y, b.house->population, b.house->level,
                                b.house->abandoned ? 1 : 0);
                    for (const auto g : b.house->goods) {
                        std::printf(" %u", static_cast<unsigned>(g));
                    }
                }
            }
            std::printf(" | oil %u fleece %u\n", city_stock(sim.state().city, sim.catalog(), "olive_oil"),
                        city_stock(sim.state().city, sim.catalog(), "fleece"));
        }
        run.seen["stockpile"] += sim.state().stockpiled.empty() ? 0 : 1;  // Z:80
        run.seen["towers_manned"] += sim.state().towers_manned ? 1 : 0;  // Z:146
        run.seen["trireme"] += sim.state().military.ships.empty() ? 0 : 1;  // launched from a wharf (Z:141)
        for (const Ship& sh : sim.state().military.ships) {
            run.seen["crew_called"] += sh.crew == Crew::Aboard ? 1 : 0;  // Z:141
            run.seen["crew_short"] +=
                sh.crew == Crew::Aboard && sh.hands < sim.catalog().naval.crew_workers ? 1 : 0;  // Z:141
            run.seen["ship_patrol"] += sh.order == ShipOrder::Patrol ? 1 : 0;  // Z:142
        }
        run.seen["monument_built"] += std::any_of(sim.state().city.buildings.items.begin(), sim.state().city.buildings.items.end(),
                                                  [](const auto& kv) { return kv.second.type == "commemorative_monument"; })
                                          ? 1 : 0;  // earned and built (Z:155)
        run.seen["sea_invasion"] += sim.state().military.invasion.by_sea ? 1 : 0;  // Z:144
        for (const WalkerId id : sim.state().military.invasion.invaders) {
            const Walker& w = sim.state().walkers.items.at(id);
            const auto& units = sim.catalog().combat.nations.at(w.variant).units;
            run.seen["naval_hit"] += is_ship(w) && units.count(w.type) != 0 && w.hp < units.at(w.type).hp ? 1 : 0;
        }
        for (const Company& c : sim.state().military.companies) {
            run.seen["tactics_set"] += c.tactic != Tactic::Defensive ? 1 : 0;  // Z:138-139
            run.seen["hoplites"] += c.unit == "hoplite" && !c.members.empty() ? 1 : 0;  // from the mansions (Z:136)
            run.seen["hoplite_wedge"] += c.unit == "hoplite" && c.tactic == Tactic::Special ? 1 : 0;  // Z:139
            run.seen["horsemen"] += c.unit == "horseman" && !c.members.empty() ? 1 : 0;  // from the estates
        }
        for (const auto& [id, b] : sim.state().city.buildings.items) {
            run.seen["not_buying"] += b.stall && b.stall->not_buying ? 1 : 0;  // Z:72
        }
        for (const Expedition& e : sim.state().expeditions) {
            run.seen["aid_sent"] += e.aid ? 1 : 0;  // companies sent where a city asked for them (Z:161)
            run.seen["raid_sent"] += e.raid ? 1 : 0;  // Z:178
            run.seen["attack_sent"] += e.raid || e.aid ? 0 : 1;  // Z:177
            run.seen["ships_abroad"] += e.ships.empty() ? 0 : 1;  // Z:177
        }
        for (const auto& [id, b] : sim.state().city.buildings.items) {
            if (log && b.house && b.house->elite && b.house->population > 0 && !watched[id]) {
                watched[id] = true;
                std::printf("elite %d,%d moved in day %llu: appeal %d culture %d goods %u %u %u\n", b.origin.x, b.origin.y,
                            static_cast<unsigned long long>(day), building_appeal(sim.state().city, b),
                            house_culture(b, sim.catalog(), city_culture_bonus(sim.state().city, sim.catalog())),
                            b.house->goods[0], b.house->goods[1], b.house->goods[2]);
            }
        }
        if (sim.state().scenario.won && run.won_day == 0) {
            run.won_day = day;
            if (log) {
                std::printf("won on day %llu\n", static_cast<unsigned long long>(day));
            }
        }
        if (run.won_day != 0 && day > run.won_day + kDaysPerMonth * kMonthsPerYear && all_seen(run.seen) && !log) {
            break;  // a year after the victory, every mechanic seen: enough for the gate
        }
        if (log && (day + 1) % (kDaysPerMonth * kMonthsPerYear) == 0) {
            report(sim, (day + 1) / (kDaysPerMonth * kMonthsPerYear));
            if (std::string(std::getenv("BO_DEMO_LOG")) == "2") {
                dump(sim);
            }
        }
    }
    return run;
}

}  // namespace

TEST_CASE("demo gate: the autopilot builds the demo city and wins it") {
    const char* years = std::getenv("BO_DEMO_YEARS");  // shorter runs while tuning the demo
    const auto run = run_demo(years != nullptr ? std::strtoull(years, nullptr, 10) : 10);
    if (std::getenv("BO_DEMO_LOG") != nullptr) {
        for (const BuildStep& step : run.order.steps) {
            const auto* p = std::get_if<PlaceBuilding>(&step.command);
            const auto& map = run.sim->state().city.map;
            if (p != nullptr && p->type != "wall" && p->type != "common_housing" &&
                (map.at({p->x, p->y}).building == kNoBuilding ||
                 run.sim->state().city.buildings.items.at(map.at({p->x, p->y}).building).type != p->type)) {
                const BuildingId b = map.at({p->x, p->y}).building;
                std::printf("missing %s %d,%d (now %s)\n", p->type.c_str(), p->x, p->y,
                            b == kNoBuilding ? "empty" : run.sim->state().city.buildings.items.at(b).type.c_str());
            }
        }
    }
    if (std::getenv("BO_DEMO_MAP") != nullptr) {  // the city at the end: a letter per building, # road
        const auto& city = run.sim->state().city;
        for (std::int32_t y = 0; y < city.map.height(); ++y) {
            std::printf("%2d ", y);
            for (std::int32_t x = 0; x < city.map.width(); ++x) {
                const Tile& t = city.map.at({x, y});
                const char c = t.building != kNoBuilding ? city.buildings.items.at(t.building).type[0]
                               : t.road                  ? '#'
                               : t.terrain == Terrain::Grassland ? '.'
                                                                 : '~';
                std::printf("%c", c);
            }
            std::printf("\n");
        }
    }
    if (const char* a = std::getenv("BO_DEMO_APPEAL")) {  // x0,y0,x1,y1: the appeal of the tiles at the end
        int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        if (std::sscanf(a, "%d,%d,%d,%d", &x0, &y0, &x1, &y1) == 4) {
            const auto& city = run.sim->state().city;
            for (int y = y0; y <= y1; ++y) {
                std::printf("%2d ", y);
                for (int x = x0; x <= x1; ++x) {
                    std::printf("%4d", city.appeal.empty() ? 0 : city.appeal[city.map.index({x, y})]);
                }
                std::printf("\n");
            }
        }
    }
    if (std::getenv("BO_DEMO_LOG") != nullptr) {
        for (const auto& [k, v] : run.seen) {
            std::printf("seen %s %u\n", k.c_str(), v);
        }
    }
    for (const auto& r : run.refused) {
        MESSAGE("refused: " << r);
    }
    CHECK(run.refused.empty());
    const WorldState& st = run.sim->state();
    CHECK(houses_at(st, run.sim->catalog(), false, 4) >= 10);  // Tenement needs 35 culture points (B-008)
    CHECK(houses_at(st, run.sim->catalog(), true, 0) >= 2);
    CHECK(st.scenario.won);
    CHECK(run.won_day < 7 * kDaysPerMonth * kMonthsPerYear);
    CHECK(std::any_of(st.military.companies.begin(), st.military.companies.end(),
                      [](const Company& c) { return c.nation == "divine"; }));  // Ares's Sparti (Z:140)
    CHECK(st.city.map.at({69, 48}).terrain == Terrain::Rock);  // the earthquake of year 3 (demo.json) cracked the land
    // A healthy city at the end: work for nine in ten workers, people content (Z:159-160).
    CHECK(st.labour.employed * 10 >= st.labour.workforce * 9);
    CHECK(st.migration.popularity >= 70);
    for (const char* m : kMechanics) {
        CHECK_MESSAGE((run.seen.count(m) > 0 && run.seen.at(m) > 0), "the demo never showed: " << std::string(m));
    }
}
