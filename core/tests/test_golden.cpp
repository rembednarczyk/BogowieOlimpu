// Golden hash (docs/PRAKTYKI.md ch. 3): a small fixed scenario, hash updated ONLY by hand, and the commit that
// changes it must name the rule that changed it. Never regenerate it automatically.
#include <cstdio>

#include "bo/core/hash.h"
#include "bo/core/serialize.h"
#include "bo/core/simulation.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

Simulation golden_scenario() {
    Simulation sim(bo::test::catalog(), SimulationConfig{20261126, Difficulty::Hero, 10000});
    sim.issue_at(10, SetTaxRate{TaxRate::High});
    sim.issue_at(kTicksPerMonth * 3, SetWageRate{WageRate::VeryHigh});
    sim.issue_at(kTicksPerYear, SetTaxRate{TaxRate::Low});
    sim.step(kTicksPerYear * 2 + 17);
    return sim;
}

}  // namespace

TEST_CASE("replay: the same seed and command log give the same hash") {
    CHECK(golden_scenario().state_hash() == golden_scenario().state_hash());
}

TEST_CASE("golden hash") {
    // The data version is part of the hash, so this test covers the logic, not the data: it hashes a state
    // whose data_version field is fixed to 0.
    const auto sim = golden_scenario();
    const auto bytes = bo::core::serialize_state(sim.state(), 0);
    bo::core::Fnv1a64 h;
    h.add_bytes(bytes.data(), bytes.size());
    const std::uint64_t kGolden = 0x8e4d415da80bd045ULL;  // 2026-11-26: initial value (modules A1-A5)
    if (h.value() != kGolden) {
        std::printf("golden hash is 0x%016llx\n", static_cast<unsigned long long>(h.value()));
    }
    CHECK(h.value() == kGolden);
}
