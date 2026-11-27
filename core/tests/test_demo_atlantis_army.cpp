// Gate of the Atlantean army demo (tools/make_demo_atlantis_army.py; docs/PRAKTYKI.md ch. 4): the autopilot
// builds an Atlantean city by the sea and holds it against an army from the sea and the Oceanids (P:15-19).
// BO_DEMO_LOG=1 prints a line every half year.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

#include "bo/core/build_order.h"
#include "bo/core/migration.h"
#include "bo/core/military.h"
#include "bo/core/naval.h"
#include "bo/core/scenario.h"
#include "bo/core/simulation.h"
#include "bo/core/storage.h"
#include "demo_probes.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;
using bo::test::read_text;

namespace {


// The expansion's army, as the demo must show it (the owner's rule).
constexpr const char* kArmyMechanics[] = {
    "archers",       // a company of archers in the field (P:15)
    "frigate",       // a frigate on the water (P:15-18)
    "frigate_fire",  // with Atlantean Fire (P:15-18)
    "tower_fire",    // a manned tower with orichalc in store while enemies are near (P:18)
    "oceanids",      // the Oceanids' invasion (P:19)
    "invasion_won",  // an army beaten (Z:144)
};

void note_army(const Simulation& sim, std::map<std::string, std::uint32_t>& seen) {
    const WorldState& st = sim.state();
    for (const Company& c : st.military.companies) {
        seen["archers"] += c.unit == "archer" && c.status != CompanyStatus::AtHome && !c.soldiers.empty() ? 1 : 0;
    }
    for (const Ship& sh : st.military.ships) {
        const auto w = st.walkers.items.find(sh.walker);
        if (w != st.walkers.items.end() && w->second.type == "frigate") {
            ++seen["frigate"];
            seen["frigate_fire"] += with_atlantean_fire(st, sim.catalog(), w->second, 100) > 100 ? 1 : 0;
        }
    }
    seen["oceanids"] += st.military.invasion.active && st.military.invasion.nation == "oceanid" ? 1 : 0;
    const bool orichalc = city_stock(st.city, sim.catalog(), "orichalc") > 0;
    for (const auto& [id, b] : st.city.buildings.items) {
        seen["tower_fire"] +=
            b.type == "tower" && st.towers_manned && b.staffing_percent > 0 && orichalc && st.military.invasion.active ? 1 : 0;
    }
}

}  // namespace

TEST_CASE("demo gate: Atlantean army, the autopilot builds the city by the sea and holds it") {
    std::string error;
    auto cfg = parse_scenario(read_text(std::string(BO_DATA_DIR) + "/../scenarios/demo_atlantis_army.json"), error);
    REQUIRE_MESSAGE(cfg, error);
    auto order = parse_build_order(read_text(std::string(BO_DATA_DIR) + "/../scenarios/demo_atlantis_army_build.json"), error);
    REQUIRE_MESSAGE(order, error);
    Simulation sim(bo::test::catalog(), *cfg);
    REQUIRE(sim.setup_error().empty());
    CHECK(sim.state().city.atlantean);
    const bool log = std::getenv("BO_DEMO_LOG") != nullptr;
    std::map<std::string, std::uint32_t> seen;
    std::vector<std::string> refused;
    const char* years = std::getenv("BO_DEMO_YEARS");
    const std::uint64_t cap = (years != nullptr ? std::strtoull(years, nullptr, 10) : 4) * kDaysPerMonth * kMonthsPerYear;
    for (std::uint64_t day = 0; day < cap; ++day) {
        for (const auto& c : due_commands(*order, sim, day)) {
            sim.issue(c);
        }
        sim.run_days(1);
        for (const auto& e : sim.drain_events()) {
            if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
                refused.push_back(std::to_string(day) + ": " + r->reason_key);
            }
            if (const auto* i = std::get_if<InvasionEnded>(&e.event)) {
                seen["invasion_won"] += i->result == 0 ? 1 : 0;
                if (log) {
                    std::printf("day %llu: invasion ended %u\n", static_cast<unsigned long long>(day), i->result);
                }
            }
        }
        note_army(sim, seen);
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
                if (b.producer && (b.type == "frigate_wharf" || b.type == "timber_mill")) {
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
            for (const char* m : kArmyMechanics) {
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
    for (const char* m : kArmyMechanics) {
        CHECK_MESSAGE((seen.count(m) > 0 && seen.at(m) > 0), "the Atlantean army demo never showed: " << std::string(m));
    }
    for (const auto& c : bo::test::demo::cut_off(sim)) {
        MESSAGE("cut off from the land entry: " << c);  // B-021
    }
    CHECK(bo::test::demo::cut_off(sim).empty());
}
