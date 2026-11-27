// Terrain height (L-099, S-009, S-005): levels, ramps, cliffs in movement, roads and construction.
#include "bo/core/city.h"
#include "bo/core/path.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

// Lower level on the left (x < 5), one level higher on the right; a ramp at (4, 2) leads up to the east.
CityState plateau(bool roads) {
    std::string err;
    CityState c;
    std::vector<std::string> rows(5, std::string(10, '.'));
    rows[2] = roads ? "====>=====" : "....>.....";
    const std::vector<std::string> heights(5, "0000011111");
    c.map = *parse_text_map(rows, err, heights);
    c.map.at({4, 2}).road = roads;  // a road dragged over a ramp becomes stairs (S-005)
    update_roads(c);
    return c;
}

}  // namespace

TEST_CASE("steps: same level, up and down a ramp; cliffs and the ramp's sides block") {
    const auto c = plateau(false);
    const TileMap& m = c.map;
    CHECK(m.can_step({0, 0}, Dir::East));
    CHECK_FALSE(m.can_step({4, 0}, Dir::East));  // cliff
    CHECK(m.can_step({4, 2}, Dir::East));        // up the ramp
    CHECK(m.can_step({5, 2}, Dir::West));        // down the ramp
    CHECK(m.can_step({3, 2}, Dir::East));        // onto the ramp from its own level
    CHECK_FALSE(m.can_step({5, 1}, Dir::West));  // the upper level does not reach the ramp's side
}

TEST_CASE("roads over a ramp become one network; without the ramp the cliff splits them (S-005)") {
    auto c = plateau(true);
    CHECK(c.map.at({0, 2}).road_network == c.map.at({9, 2}).road_network);
    const auto p = find_path(c, {{0, 2}, {9, 2}, PathMode::Road});
    REQUIRE(p);
    CHECK(p->size() == 9);
    std::string err;
    CityState flat_cliff;
    flat_cliff.map = *parse_text_map({"=========="}, err, {"0000011111"});
    update_roads(flat_cliff);
    CHECK(flat_cliff.map.at({0, 0}).road_network != flat_cliff.map.at({9, 0}).road_network);
    CHECK_FALSE(find_path(flat_cliff, {{0, 0}, {9, 0}, PathMode::Road}));
}

TEST_CASE("buildings stand on one level and not on ramps (assumption, L-099)") {
    const auto& cat = bo::test::catalog();
    auto c = plateau(false);
    CHECK(place_building(c, cat, Difficulty::Hero, "fountain", {4, 0}).reason == "build.uneven_ground");
    CHECK(place_building(c, cat, Difficulty::Hero, "fountain", {3, 2}).reason == "build.uneven_ground");
    CHECK(place_building(c, cat, Difficulty::Hero, "fountain", {6, 0}).ok());
    CHECK(place_road(c, cat, Difficulty::Hero, {4, 2}).ok());  // roads may run over ramps (stairs)
}

TEST_CASE("road access does not reach across a cliff") {
    std::string err;
    CityState c;
    c.map = *parse_text_map({"..=", "..=", "..."}, err, {"001", "001", "000"});
    update_roads(c);
    CHECK_FALSE(find_road_access(c, {0, 0}, 2, 2));
    c.map.at({2, 2}).road = true;  // same level, south-east corner is not on the perimeter edges
    c.map.at({1, 2}).road = true;
    update_roads(c);
    CHECK(*find_road_access(c, {0, 0}, 2, 2) == TilePos{1, 2});
}
