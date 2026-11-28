// Module C1: appeal field (docs/gameplay/20, S-004 tables).
#include "bo/core/appeal.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

CityState open_land(std::int32_t w, std::int32_t h) {
    std::string err;
    CityState c;
    c.map = *parse_text_map(std::vector<std::string>(static_cast<std::size_t>(h), std::string(static_cast<std::size_t>(w), '.')), err);
    return c;
}

}  // namespace

TEST_CASE("appeal by distance matches the S-004 examples (doc 20)") {
    CHECK(appeal_at_distance({8, 1, -1, 3}, 1) == 8);  // Flower Garden 8, 7, 6
    CHECK(appeal_at_distance({8, 1, -1, 3}, 3) == 6);
    CHECK(appeal_at_distance({8, 1, -1, 3}, 4) == 0);
    CHECK(appeal_at_distance({-10, 2, 2, 6}, 2) == -10);  // Mint -10, -10, -8, -8, -6, -6
    CHECK(appeal_at_distance({-10, 2, 2, 6}, 3) == -8);
    CHECK(appeal_at_distance({-10, 2, 2, 6}, 6) == -6);
    CHECK(appeal_at_distance({18, 1, -3, 6}, 6) == 3);  // Fish Pond
}

TEST_CASE("appeal field: rings around the footprint, contributions add up, nothing on the footprint itself") {
    const auto& cat = bo::test::catalog();
    auto c = open_land(12, 12);
    REQUIRE(place_building(c, cat, Difficulty::Hero, "flower_garden", {5, 5}).ok());  // 2x2, (8, 1, -1, 3)
    auto f = compute_appeal(c, cat);
    CHECK(f[c.map.index({5, 5})] == 0);
    CHECK(f[c.map.index({4, 4})] == 8);   // diagonal ring 1
    CHECK(f[c.map.index({7, 6})] == 8);
    CHECK(f[c.map.index({9, 6})] == 6);   // ring 3
    CHECK(f[c.map.index({10, 6})] == 0);  // out of range
    REQUIRE(place_building(c, cat, Difficulty::Hero, "flower_garden", {9, 5}).ok());
    f = compute_appeal(c, cat);
    CHECK(f[c.map.index({8, 5})] == 7 + 8);  // ring 2 of the first garden, ring 1 of the second
}

TEST_CASE("building appeal is the highest value over its tiles") {
    const auto& cat = bo::test::catalog();
    auto c = open_land(12, 12);
    place_building(c, cat, Difficulty::Hero, "flower_garden", {0, 0});
    std::string err;
    const auto r = place_building(c, cat, Difficulty::Hero, "fountain", {3, 0});
    REQUIRE(r.ok());
    c.appeal = compute_appeal(c, cat);
    CHECK(building_appeal(c, c.buildings.items.at(r.building)) == 7);  // nearest fountain tile is ring 2
}
