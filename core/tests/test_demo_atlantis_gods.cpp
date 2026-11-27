// Gate of the demo of the expansion's gods (tools/make_demo_atlantis_gods.py; docs/PRAKTYKI.md ch. 4): the
// autopilot builds an Atlantean city with the sanctuaries of Hera and Atlas and the Citadel of Poseidon (P:20-26).
// BO_DEMO_LOG=1 prints a line every half year.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

#include "bo/core/build_order.h"
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

// The expansion's gods, as the demo must show them (the owner's rule).
constexpr const char* kGodsMechanics[] = {
    "hera_grove",        // Hera's two orange groves (P:21)
    "oranges",           // picked by the orange tenders (P:8)
    "atlas_artisans",    // a guild sends two artisans with Atlas (P:22)
    "atlas_prayer",      // Atlas answers: the stone on the sites at once (P:22)
    "citadel",           // the Citadel of Poseidon finished (P:26)
    "cattle_sacrifice",  // the priests take cattle (P:22, Z:124)
    "sphinx",            // Hera's monster (P:23)
    "plank_waits",       // a monument off the road waits for its plank to reach one (P:26-28)
    "monument_held",     // its building held (P:26-28)
    "black_marble",      // black marble taken for it (P:26-28)
    "pyramid",           // the pyramid finished (P:26-28)
    "aesthetics",        // the expansion's twelve aesthetic buildings stand (P:16-17)
};

void note_gods(const Simulation& sim, std::size_t& cattle, std::uint32_t& black_before,
               std::map<std::string, std::uint32_t>& seen) {
    const WorldState& st = sim.state();
    std::map<BuildingId, int> out;
    for (const auto& [id, w] : st.walkers.items) {
        out[w.home] += w.job == WalkerJob::Build ? 1 : 0;
    }
    for (const auto& [id, n] : out) {
        seen["atlas_artisans"] += n >= 2 ? 1 : 0;
    }
    std::size_t now = 0;
    for (const auto& [id, b] : st.city.buildings.items) {
        now += b.type == "cattle" ? 1 : 0;
        seen["citadel"] += b.type == "sanctuary_poseidon_citadel" && b.sanctuary && b.sanctuary->complete ? 1 : 0;
        if (b.storage) {
            const auto o = b.storage->loads.find("oranges");
            seen["oranges"] += o != b.storage->loads.end() && o->second > 0 ? 1 : 0;
        }
    }
    seen["cattle_sacrifice"] += cattle > 0 && now < cattle ? 1 : 0;
    cattle = now;
    std::uint32_t black = 0;
    std::size_t adorn = 0;
    for (const auto& [id, b] : st.city.buildings.items) {
        if (b.type == "pyramid_modest" && b.sanctuary) {
            seen["plank_waits"] += !b.road_access && !b.sanctuary->complete ? 1 : 0;
            seen["monument_held"] += b.sanctuary->paused ? 1 : 0;
            seen["pyramid"] += b.sanctuary->complete ? 1 : 0;
        }
        if (b.storage) {
            const auto k = b.storage->loads.find("black_marble");
            black += k != b.storage->loads.end() ? k->second : 0;
        }
        for (const char* t : {"water_park", "bird_bath", "short_obelisk", "tall_obelisk", "shell_garden", "sundial",
                              "dolphin_sculpture", "orrery", "spring", "topiary", "baths", "stone_circle"}) {
            adorn += b.type == t ? 1 : 0;
        }
    }
    seen["aesthetics"] += adorn == 12 ? 1 : 0;
    seen["black_marble"] += black_before > black && black_before > 0 ? 1 : 0;  // taken out of store
    black_before = black;
}

}  // namespace

TEST_CASE("demo gate: Atlantean gods, the autopilot builds Hera's, Atlas's and Poseidon's sanctuaries") {
    std::string error;
    auto cfg = parse_scenario(read_text(std::string(BO_DATA_DIR) + "/../scenarios/demo_atlantis_gods.json"), error);
    REQUIRE_MESSAGE(cfg, error);
    auto order = parse_build_order(read_text(std::string(BO_DATA_DIR) + "/../scenarios/demo_atlantis_gods_build.json"), error);
    REQUIRE_MESSAGE(order, error);
    Simulation sim(bo::test::catalog(), *cfg);
    REQUIRE(sim.setup_error().empty());
    CHECK(sim.state().city.atlantean);
    const bool log = std::getenv("BO_DEMO_LOG") != nullptr;
    std::map<std::string, std::uint32_t> seen;
    std::vector<std::string> refused;
    const char* years = std::getenv("BO_DEMO_YEARS");
    const std::uint64_t cap = (years != nullptr ? std::strtoull(years, nullptr, 10) : 8) * kDaysPerMonth * kMonthsPerYear;
    std::size_t cattle = 0;
    std::uint32_t black = 0;
    for (std::uint64_t day = 0; day < cap; ++day) {
        for (const auto& c : due_commands(*order, sim, day)) {
            sim.issue(c);
        }
        sim.run_days(1);
        for (const auto& e : sim.drain_events()) {
            if (const auto* r = std::get_if<CommandRejected>(&e.event)) {
                refused.push_back(std::to_string(day) + ": " + r->reason_key);
            }
            if (const auto* g = std::get_if<GodBlessing>(&e.event)) {
                seen["hera_grove"] += g->god == "hera" && g->kind == "grove" ? 1 : 0;
            }
            if (const auto* m = std::get_if<MonsterAppeared>(&e.event)) {
                seen["sphinx"] += m->monster == "sphinx" ? 1 : 0;
            }
            if (std::holds_alternative<PrayerAnswered>(e.event)) {
                ++seen["atlas_prayer"];  // the only prayer the autopilot says (Atlas's)
            }
            if (log && std::holds_alternative<SanctuaryCompleted>(e.event)) {
                std::printf("day %llu: sanctuary completed\n", static_cast<unsigned long long>(day));
            }
        }
        note_gods(sim, cattle, black, seen);
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
                if (b.sanctuary) {
                    std::printf(" %s%s", b.type.c_str(), b.sanctuary->complete ? "+" : "");
                    for (const auto& [g, n] : b.sanctuary->needed) {
                        std::printf(" %s:%u", g.c_str(), n);
                    }
                }
                if (b.producer && b.type == "artisans_guild") {
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
            for (const char* m : kGodsMechanics) {
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
    for (const char* m : kGodsMechanics) {
        CHECK_MESSAGE((seen.count(m) > 0 && seen.at(m) > 0), "the Atlantean gods demo never showed: " << std::string(m));
    }
    for (const auto& c : bo::test::demo::cut_off(sim)) {
        MESSAGE("cut off from the land entry: " << c);  // B-021
    }
    CHECK(bo::test::demo::cut_off(sim).empty());
}
