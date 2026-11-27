// Gate of the gods' demo (docs/PRAKTYKI.md ch. 4; tools/make_demo_gods.py): the autopilot plays the adventure
// scenarios/demo_gods.json, the parent city with Zeus, Demeter, Aphrodite and Dionysus, then the colony with
// Poseidon, Hades, Artemis and Hephaestus (doc 23, L-119), and every god's gift shows (Z:116-122, L-130, L-131).
// BO_DEMO_LOG=1 prints the mechanics seen and the refusals.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

#include "bo/core/adventure.h"
#include "bo/core/blessings.h"
#include "bo/core/build_order.h"
#include "bo/core/gods.h"
#include "bo/core/simulation.h"
#include "demo_probes.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;
using namespace bo::test::demo;

namespace {

std::string read_text(const std::string& path) {
    std::ifstream f(path);
    std::stringstream s;
    s << f.rdbuf();
    return s.str();
}

BuildOrder order_of(const std::string& file) {
    std::string error;
    auto order = parse_build_order(read_text(std::string(BO_DATA_DIR) + "/../scenarios/" + file), error);
    REQUIRE_MESSAGE(order, error);
    return *order;
}

// One episode with its autopilot until won (at most `years`); the day it was won, 0 if never.
std::uint64_t play(Simulation& sim, const BuildOrder& order, std::uint64_t years, std::map<std::string, std::uint32_t>& seen,
                   std::vector<std::string>& refused) {
    const std::uint64_t start = sim.state().time.total_days();
    for (std::uint64_t day = 0; day < years * kDaysPerMonth * kMonthsPerYear; ++day) {
        const auto cmds = due_commands(order, sim, day);
        note_refusals(sim, cmds, day, refused);
        for (const auto& c : cmds) {
            sim.issue(c);
        }
        sim.step(1);
        for (const auto& c : due_commands(order, sim, day, true)) {
            sim.issue(c);
        }
        sim.step(kTicksPerDay - 1);
        const auto events = sim.drain_events();
        note_events(events, seen);
        probe_tile(sim, day);
        trace_invasion(sim, day);
        const std::string save_path = std::string(BO_DATA_DIR) + "/../build/demo_day.sav";
        if (const char* at = std::getenv("BO_DEMO_SAVE_AT");
            at != nullptr && std::strtoull(at, nullptr, 10) == day && !std::ifstream(save_path)) {  // the first episode
            const auto bytes = sim.save();  // the state after that day, for a probe
            std::ofstream out(std::string(BO_DATA_DIR) + "/../build/demo_day.sav", std::ios::binary);
            out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        }
        for (const auto& e : events) {
            seen["sanctuary"] += std::holds_alternative<SanctuaryCompleted>(e.event) ? 1 : 0;
            seen["god_attack"] += std::holds_alternative<GodAttacked>(e.event) ? 1 : 0;
        }
        const WorldState& st = sim.state();
        seen["aphrodite_appeal"] += blessed_appeal(st.city, sim.catalog()) > 0 ? 1 : 0;  // Z:122
        for (const Ship& sh : st.military.ships) {
            seen["crew_short"] += sh.crew == Crew::Aboard && sh.hands < sim.catalog().naval.crew_workers ? 1 : 0;
        }
        for (const Company& c : st.military.companies) {
            seen["amazons"] += c.nation == "amazon" ? 1 : 0;  // Artemis (Z:120)
        }
        for (const auto& [id, w] : st.walkers.items) {
            seen["poseidon_boats"] +=
                w.job == WalkerJob::Fish && god_blesses(st.city, sim.catalog(), "poseidon") ? 1 : 0;  // Z:119
        }
        if (std::getenv("BO_DEMO_LOG") != nullptr && (day + 1) % (kDaysPerMonth * kMonthsPerYear / 2) == 0) {
            report(sim, (day + 1) / (kDaysPerMonth * kMonthsPerYear / 2));  // half-years
        }
        if (st.scenario.won) {
            return st.time.total_days() - start;
        }
    }
    return 0;
}

// Every gift of the eight gods (the owner's rule: the demo shows every mechanic).
constexpr const char* kGodsMechanics[] = {
    "god_repelled", "zeus_opinion", "demeter_land", "dionysus_grove", "aphrodite_appeal",       // the parent city
    "poseidon_kraken", "poseidon_boats", "hades_cerberus", "hades_land", "hephaestus_land",   // the colony
    "artemis_hunt", "amazons", "crew_short"};

}  // namespace

TEST_CASE("demo gate: gods, the autopilot plays both episodes of the gods' adventure and wins it") {
    std::string error;
    const std::string text = read_text(std::string(BO_DATA_DIR) + "/../scenarios/demo_gods.json");
    const auto adventure = parse_adventure(text, error);
    REQUIRE_MESSAGE(adventure, error);
    AdventureRun run;
    std::optional<Simulation> sim;
    sim.emplace(begin_adventure(bo::test::catalog(), *adventure, run));
    std::map<std::string, std::uint32_t> seen;
    std::vector<std::string> refused;
    const std::uint64_t parent_days = play(*sim, order_of("demo_gods_build.json"), 8, seen, refused);
    std::vector<std::string> cut = cut_off(*sim);  // B-021: every building's road leads to the land entry
    CHECK(parent_days > 0);
    REQUIRE(advance_adventure(*adventure, run, sim) == NextEpisode::ChooseColony);
    const auto choices = colony_choices(*adventure, run);
    REQUIRE(choices.size() == 1);
    REQUIRE(found_colony(bo::test::catalog(), *adventure, run, choices.front(), sim).empty());
    const std::uint64_t colony_days = play(*sim, order_of("demo_gods_colony_build.json"), 8, seen, refused);
    for (const auto& c : cut_off(*sim)) {
        cut.push_back("colony " + c);
    }
    CHECK(colony_days > 0);
    NextEpisode next = advance_adventure(*adventure, run, sim);
    if (next == NextEpisode::BackToParent) {
        next = advance_adventure(*adventure, run, sim);
    }
    CHECK(next == NextEpisode::Victory);
    if (std::getenv("BO_DEMO_LOG") != nullptr) {
        std::printf("parent won after %llu days, colony after %llu\n", static_cast<unsigned long long>(parent_days),
                    static_cast<unsigned long long>(colony_days));
        for (const auto& [k, v] : seen) {
            std::printf("seen %s %u\n", k.c_str(), v);
        }
    }
    for (const auto& r : refused) {
        MESSAGE("refused: " << r);
    }
    CHECK(refused.empty());
    for (const auto& c : cut) {
        MESSAGE("cut off from the land entry: " << c);
    }
    CHECK(cut.empty());
    CHECK(seen["sanctuary"] == 8);
    for (const char* m : kGodsMechanics) {
        CHECK_MESSAGE((seen.count(m) > 0 && seen.at(m) > 0), "the gods' demo never showed: " << std::string(m));
    }
}
