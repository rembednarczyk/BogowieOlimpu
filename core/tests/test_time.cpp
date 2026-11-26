#include "bo/core/time.h"
#include "doctest/doctest.h"

using namespace bo::core;

TEST_CASE("time constants match the reference model (doc 17, 24)") {
    CHECK(kTicksPerDay == 51);
    CHECK(kTicksPerMonth == 816);
    CHECK(kTicksPerYear == 9792);  // S-007: 51 * 16 * 12
}

TEST_CASE("calendar fields") {
    GameTime t(0);
    CHECK(t.is_year_start());
    t = GameTime(kTicksPerYear + 3 * kTicksPerMonth + 5 * kTicksPerDay + 7);
    CHECK(t.year() == 1);
    CHECK(t.month() == 3);
    CHECK(t.day_in_month() == 5);
    CHECK(t.tick_in_day() == 7);
    CHECK_FALSE(t.is_day_start());
}

TEST_CASE("standard walker covers 54.4 tiles per month") {
    // 1 step per tick, 15 steps per tile: 816 / 15 = 54.4
    CHECK(kTicksPerMonth * 10 / kStepsPerTile == 544);
}
