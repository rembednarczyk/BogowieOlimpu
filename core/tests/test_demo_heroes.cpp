// Gate of the heroes' demo (tools/make_demo_heroes.py; docs/PRAKTYKI.md ch. 4): the autopilot meets the demands
// of Atalanta and Bellerophon, who answer their halls and slay the Sphinx and Echidna (P:23-24, Z:130).
// BO_DEMO_LOG=1 prints a line every half year.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

#include "bo/core/appeal.h"
#include "bo/core/build_order.h"
#include "bo/core/culture.h"
#include "bo/core/housing.h"
#include "bo/core/migration.h"
#include "bo/core/military.h"
#include "bo/core/monsters.h"
#include "bo/core/scenario.h"
#include "bo/core/simulation.h"
#include "bo/core/storage.h"
#include "demo_probes.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

std::string read_text(const std::string& path) {
    std::ifstream f(path);
    std::stringstream s;
    s << f.rdbuf();
    return s.str();
}

// The heroes the demo must show at work (the owner's rule).
constexpr const char* kHeroMechanics[] = {
    "atalanta",     // Atalanta slays the Sphinx (P:23-24)
    "bellerophon",  // Bellerophon slays Echidna (P:23-24)
};

}  // namespace

TEST_CASE("demo gate: heroes, Atalanta and Bellerophon answer their halls and slay their monsters") {
    std::string error;
    auto cfg = parse_scenario(read_text(std::string(BO_DATA_DIR) + "/../scenarios/demo_heroes.json"), error);
    REQUIRE_MESSAGE(cfg, error);
    auto order = parse_build_order(read_text(std::string(BO_DATA_DIR) + "/../scenarios/demo_heroes_build.json"), error);
    REQUIRE_MESSAGE(order, error);
    Simulation sim(bo::test::catalog(), *cfg);
    REQUIRE(sim.setup_error().empty());
    const bool log = std::getenv("BO_DEMO_LOG") != nullptr;
    std::map<std::string, std::uint32_t> seen;
    std::vector<std::string> refused;
    const char* years = std::getenv("BO_DEMO_YEARS");
    const std::uint64_t cap = (years != nullptr ? std::strtoull(years, nullptr, 10) : 10) * kDaysPerMonth * kMonthsPerYear;
    for (std::uint64_t day = 0; day < cap; ++day) {
        for (const auto& c : due_commands(*order, sim, day)) {
            sim.issue(c);
        }
        sim.run_days(1);
        for (const auto& e : sim.drain_events()) {
            if (const auto* r = std::get_if<CommandRejected>(&e.event); r && r->reason_key.rfind("myth.", 0) != 0) {
                refused.push_back(std::to_string(day) + ": " + r->reason_key);  // a summons waits for the demands
            }
            if (const auto* m = std::get_if<MonsterSlain>(&e.event)) {
                seen["atalanta"] += m->monster == "sphinx" && m->by_hero ? 1 : 0;
                seen["bellerophon"] += m->monster == "echidna" && m->by_hero ? 1 : 0;
                if (log) {
                    std::printf("day %llu: %s slain, by a hero %d\n", static_cast<unsigned long long>(day), m->monster.c_str(),
                                m->by_hero ? 1 : 0);
                }
            }

        }
        if (log && day % 64 == 0) {
            std::printf("HERO %llu companies %zu treasury %lld:", static_cast<unsigned long long>(day),
                        sim.state().military.companies.size(), static_cast<long long>(sim.state().finance.treasury));
            for (const char* h : {"atalanta", "bellerophon"}) {
                for (const auto& u : unmet_requirements(sim.state(), sim.catalog(), h, kNoBuilding)) {
                    std::printf(" %s:%s", h, u.c_str());
                }
            }
            std::printf("\n");
        }
        if (log && day % 96 == 95) {
            const WorldState& st = sim.state();
            std::printf("day %llu: pop %u treasury %lld workers %u/%u companies %zu ships %zu goals",
                        static_cast<unsigned long long>(day), city_population(st.city), static_cast<long long>(st.finance.treasury),
                        st.labour.employed, st.labour.workforce, st.military.companies.size(), st.military.ships.size());
            for (const auto& g : st.scenario.goals) {
                std::printf(" %s%c", g.type.c_str(), g.met ? '+' : '-');
            }
            std::printf(" |");
            for (const auto& [id, b] : st.city.buildings.items) {
                if (b.producer && (b.type == "horse_ranch" || b.type == "hunting_lodge" || b.type == "wheat_farm")) {
                    std::printf(" %s@%d,%d staff %u out %u road %d", b.type.c_str(), b.origin.x, b.origin.y, b.staffing_percent, b.producer->output_loads, b.road_access ? 1 : 0);
                    for (const auto& [g, n] : b.producer->units) {
                        std::printf(" %s:%u", g.c_str(), n);
                    }
                }
                if (b.storage && !b.storage->loads.empty()) {
                    std::printf(" %s", b.type.c_str());
                    for (const auto& [g, n] : b.storage->loads) {
                        std::printf(" %s:%u", g.c_str(), n);
                    }
                }
            }
            std::map<int, int> levels;
            for (const auto& [id, b] : st.city.buildings.items) {
                levels[b.house ? b.house->level : -1] += b.house ? 1 : 0;
            }
            std::printf(" | houses");
            for (const auto& [l, n] : levels) {
                if (l >= 0) {
                    std::printf(" l%d:%d", l, n);
                }
            }
            std::printf("\n");
        }
        if (sim.state().scenario.won) {
            bool all = true;
            for (const char* m : kHeroMechanics) {
                all = all && seen.count(m) > 0 && seen.at(m) > 0;
            }
            if (all) {
                break;
            }
        }
    }
    for (const auto& r : refused) {
        MESSAGE("refused: " << r);
    }
    CHECK(refused.empty());
    CHECK(sim.state().scenario.won);
    for (const char* m : kHeroMechanics) {
        CHECK_MESSAGE((seen.count(m) > 0 && seen.at(m) > 0), "the heroes demo never showed: " << std::string(m));
    }
    for (const auto& c : bo::test::demo::cut_off(sim)) {
        MESSAGE("cut off from the land entry: " << c);  // B-021
    }
    CHECK(bo::test::demo::cut_off(sim).empty());
}
