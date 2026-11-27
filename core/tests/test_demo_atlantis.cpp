// The Atlantis demo gate (tools/make_demo_atlantis.py; docs/PRAKTYKI.md ch. 4): the autopilot builds the
// Atlantean city and wins it; science lifts its houses (P:10-11) and the hippodrome races (P:13-15).
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "bo/core/build_order.h"
#include "bo/core/hippodrome.h"
#include "bo/core/migration.h"
#include "bo/core/scenario.h"
#include "bo/core/simulation.h"
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

}  // namespace

TEST_CASE("demo gate: Atlantis, the autopilot builds the Atlantean demo and wins it") {
    std::string error;
    auto cfg = parse_scenario(read_text(std::string(BO_DATA_DIR) + "/../scenarios/demo_atlantis.json"), error);
    REQUIRE_MESSAGE(cfg, error);
    auto order = parse_build_order(read_text(std::string(BO_DATA_DIR) + "/../scenarios/demo_atlantis_build.json"), error);
    REQUIRE_MESSAGE(order, error);
    Simulation sim(bo::test::catalog(), *cfg);
    REQUIRE(sim.setup_error().empty());
    CHECK(sim.state().city.atlantean);
    const bool log = std::getenv("BO_DEMO_LOG") != nullptr;
    bool science = false;
    bool racing = false;
    std::vector<std::string> refused;
    std::uint64_t day = 0;
    const char* years = std::getenv("BO_DEMO_YEARS");
    const std::uint64_t cap = (years != nullptr ? std::strtoull(years, nullptr, 10) : 4) * kDaysPerMonth * kMonthsPerYear;
    for (; day < cap && !(sim.state().scenario.won && science && racing); ++day) {
        for (const auto& c : due_commands(*order, sim, day)) {
            sim.issue(c);
        }
        sim.run_days(1);
        for (const auto& e : sim.drain_events()) {
            if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
                refused.push_back(std::to_string(day) + ": " + r->reason_key);
            }
        }
        for (const auto& [id, b] : sim.state().city.buildings.items) {
            const auto s = b.coverage.find("scholar");
            science = science || (b.house && s != b.coverage.end() && s->second > 0);
        }
        racing = racing || working_hippodrome(sim.state(), sim.catalog()) > 0;
        if (log && day % 96 == 95) {
            const Track t = hippodrome_track(sim.state().city);
            std::printf("day %llu: pop %llu treasury %lld track %zu closed %d horses %u goals",
                        static_cast<unsigned long long>(day), static_cast<unsigned long long>(city_population(sim.state().city)),
                        static_cast<long long>(sim.state().finance.treasury), t.segments.size(), t.closed ? 1 : 0,
                        sim.state().hippodrome_horses);
            for (const auto& g : sim.state().scenario.goals) {
                std::printf(" %s%c", g.type.c_str(), g.met ? '+' : '-');
            }
            std::printf(" | workers %u/%u |", sim.state().labour.employed, sim.state().labour.workforce);
            for (const auto& [id, b] : sim.state().city.buildings.items) {
                if (b.house && id % 4 == 0) {
                    std::printf(" h%u:%u f%u", b.house->level, b.house->population, b.house->goods[0]);
                }
                if (b.storage && !b.storage->loads.empty()) {
                    std::printf(" %s", b.type.c_str());
                    for (const auto& [g, n] : b.storage->loads) {
                        std::printf(" %s:%u", g.c_str(), n);
                    }
                }
                if (b.producer && (b.type == "horse_ranch" || b.type == "wheat_farm")) {
                    std::printf(" %s staff %u out %u wheat %u", b.type.c_str(), b.staffing_percent, b.producer->output_loads,
                                b.producer->units.count("wheat") ? b.producer->units.at("wheat") : 0U);
                }
            }
            std::printf("\n");
        }
    }
    for (const auto& r : refused) {
        MESSAGE("refused: " << r);
    }
    CHECK(sim.state().scenario.won);
    CHECK(science);  // scholars in the houses (P:10-11)
    CHECK(racing);   // the hippodrome with its horses (P:13-15)
}
