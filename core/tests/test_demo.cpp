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
            std::printf("  elite %d,%d pop %u incoming %u level %u road %d abandoned %d quarantine %u\n", b.origin.x,
                        b.origin.y, b.house->population, b.house->incoming, b.house->level, b.road_access ? 1 : 0,
                        b.house->abandoned ? 1 : 0, b.house->quarantine_days);
        }
        if (b.type == "growers_lodge" || b.type == "olive_press" || b.type == "foundry" || b.type == "sculpture_studio" ||
            b.type == "timber_mill") {
            std::printf("  %s %d,%d staff %u in %u out %u progress %u", b.type.c_str(), b.origin.x, b.origin.y,
                        b.staffing_percent, b.producer->input_loads, b.producer->output_loads, b.producer->progress);
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
                                      "aid_sent", "aid_returned", "raid_sent", "attack_sent", "games_invited", "stockpile", "not_buying", "towers_manned"};

bool all_seen(const std::map<std::string, std::uint32_t>& seen) {
    return std::all_of(std::begin(kMechanics), std::end(kMechanics), [&seen](const char* m) { return seen.count(m) != 0; });
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
        for (const auto& e : events) {
            if (const auto* g = std::get_if<GamesHeld>(&e.event)) {
                run.seen[g->invited ? "games_invited" : "games_not_invited"] += 1;  // Z:110-112
                run.seen["games_won"] += g->won ? 1 : 0;
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
        run.seen["stockpile"] += sim.state().stockpiled.empty() ? 0 : 1;  // Z:80
        run.seen["towers_manned"] += sim.state().towers_manned ? 1 : 0;  // Z:146
        for (const auto& [id, b] : sim.state().city.buildings.items) {
            run.seen["not_buying"] += b.stall && b.stall->not_buying ? 1 : 0;  // Z:72
        }
        for (const Expedition& e : sim.state().expeditions) {
            run.seen["aid_sent"] += e.aid ? 1 : 0;  // companies sent where a city asked for them (Z:161)
            run.seen["raid_sent"] += e.raid ? 1 : 0;  // Z:178
            run.seen[e.raid || e.aid ? "aid_sent" : "attack_sent"] += 1;  // Z:177
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
        CHECK_MESSAGE(run.seen.count(m) > 0, "the demo never showed: " << std::string(m));
    }
}
