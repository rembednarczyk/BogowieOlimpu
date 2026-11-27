// Gate of the Atlantean elite demo (tools/make_demo_atlantis_elite.py; docs/PRAKTYKI.md ch. 4): the autopilot
// raises Atlantean estates whose people serve as spearmen and charioteers (P:15, P:18-19).
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

// The elite's mechanics the demo must show (the owner's rule).
constexpr const char* kEliteMechanics[] = {
    "spearmen",       // a company of spearmen from a mansion or manor (P:15)
    "charioteers",    // a company of charioteers from an estate, a chariot each (P:15, P:18)
    "chariot_stall",  // the chariot stall stocked from the factory (P:19)
    "estate",         // an estate (Z:32-34)
};

void note_elite(const Simulation& sim, std::map<std::string, std::uint32_t>& seen) {
    const WorldState& st = sim.state();
    for (const Company& c : st.military.companies) {
        seen["spearmen"] += c.unit == "spearman" && !c.members.empty() ? 1 : 0;
        seen["charioteers"] += c.unit == "charioteer" && !c.members.empty() ? 1 : 0;
    }
    for (const auto& [id, b] : st.city.buildings.items) {
        if (b.stall) {
            const auto ch = b.stall->units.find("chariot");
            seen["chariot_stall"] += ch != b.stall->units.end() && ch->second > 0 ? 1 : 0;
        }
        seen["estate"] += b.house && b.house->elite && b.house->population > 0 &&
                                  b.house->level + 1U == sim.catalog().elite_levels.size()
                              ? 1
                              : 0;
    }
}

}  // namespace

TEST_CASE("demo gate: Atlantean elite, the autopilot raises estates of spearmen and charioteers") {
    std::string error;
    auto cfg = parse_scenario(read_text(std::string(BO_DATA_DIR) + "/../scenarios/demo_atlantis_elite.json"), error);
    REQUIRE_MESSAGE(cfg, error);
    auto order = parse_build_order(read_text(std::string(BO_DATA_DIR) + "/../scenarios/demo_atlantis_elite_build.json"), error);
    REQUIRE_MESSAGE(order, error);
    Simulation sim(bo::test::catalog(), *cfg);
    REQUIRE(sim.setup_error().empty());
    CHECK(sim.state().city.atlantean);
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
            if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
                refused.push_back(std::to_string(day) + ": " + r->reason_key);
            }

        }
        note_elite(sim, seen);
        if (log && day % 96 == 50) {
            for (const auto& [id, b] : sim.state().city.buildings.items) {
                if (b.house && b.house->elite) {
                    std::printf("E %llu %d,%d l%u p%u ab%d app%d cult%d g[%u %u %u %u %u %u]\n", (unsigned long long)day,
                                b.origin.x, b.origin.y, b.house->level, b.house->population, b.house->abandoned ? 1 : 0,
                                building_appeal(sim.state().city, b),
                                house_culture(b, sim.catalog(), city_culture_bonus(sim.state().city, sim.catalog())),
                                b.house->goods[0], b.house->goods[1], b.house->goods[2], b.house->goods[3], b.house->goods[4],
                                b.house->goods[5]);
                }
            }
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
                if (b.producer && (b.type == "chariot_factory" || b.type == "horse_ranch")) {
                    std::printf(" %s staff %u out %u", b.type.c_str(), b.staffing_percent, b.producer->output_loads);
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
            for (const char* m : kEliteMechanics) {
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
    for (const char* m : kEliteMechanics) {
        CHECK_MESSAGE((seen.count(m) > 0 && seen.at(m) > 0), "the Atlantean elite demo never showed: " << std::string(m));
    }
    for (const auto& c : bo::test::demo::cut_off(sim)) {
        MESSAGE("cut off from the land entry: " << c);  // B-021
    }
    CHECK(bo::test::demo::cut_off(sim).empty());
}
