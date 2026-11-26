// Modules B2-B3: placement rules, costs, road networks and road access order.
#include "bo/core/city.h"
#include "doctest/doctest.h"
#include "helpers.h"

using namespace bo::core;

namespace {

CityState city_from(const std::vector<std::string>& rows) {
    std::string err;
    CityState c;
    c.map = *parse_text_map(rows, err);
    update_roads(c);
    return c;
}

}  // namespace

TEST_CASE("placement: footprint, occupancy, terrain and cost by difficulty (S-004 cost table)") {
    const auto& cat = bo::test::catalog();
    auto c = city_from({"......", "......", "..T...", "......"});
    const auto r = place_building(c, cat, Difficulty::Hero, "fountain", {0, 0});
    REQUIRE(r.ok());
    CHECK(r.cost == 35);  // fountain, Hero (S-004)
    CHECK(c.map.at({1, 1}).building == r.building);
    CHECK(place_building(c, cat, Difficulty::Hero, "fountain", {1, 1}).reason == "build.occupied");
    CHECK(place_building(c, cat, Difficulty::Hero, "fountain", {2, 2}).reason == "build.bad_terrain");
    CHECK(place_building(c, cat, Difficulty::Hero, "fountain", {5, 3}).reason == "build.out_of_bounds");
    CHECK(place_building(c, cat, Difficulty::Hero, "no_such", {4, 0}).reason == "build.unknown_type");
}

TEST_CASE("placement: farms need meadow (Z:39, size legend m)") {
    const auto& cat = bo::test::catalog();
    auto c = city_from({"......", "......", "......", ",....."});
    CHECK(place_building(c, cat, Difficulty::Hero, "wheat_farm", {3, 0}).reason == "build.needs_meadow");
    CHECK(place_building(c, cat, Difficulty::Hero, "wheat_farm", {0, 1}).ok());
}

TEST_CASE("roads: terrain, roadblocks, clearing forest and demolition") {
    const auto& cat = bo::test::catalog();
    auto c = city_from({".T~", "..."});
    CHECK(place_road(c, cat, Difficulty::Hero, {0, 0}).cost == 3);  // road, Hero (S-004)
    CHECK(place_road(c, cat, Difficulty::Hero, {0, 0}).reason == "build.already_road");
    CHECK(place_road(c, cat, Difficulty::Hero, {1, 0}).reason == "build.bad_terrain");
    CHECK(place_road(c, cat, Difficulty::Hero, {2, 0}).reason == "build.bad_terrain");
    CHECK(place_roadblock(c, cat, Difficulty::Hero, {0, 1}).reason == "build.needs_road");
    CHECK(place_roadblock(c, cat, Difficulty::Hero, {0, 0}).ok());
    CHECK(clear_land(c, cat, Difficulty::Hero, {1, 0}).ok());
    CHECK(c.map.at({1, 0}).terrain == Terrain::Grassland);  // forest never grows back (Z:18)
    CHECK(demolish(c, {0, 0}).ok());
    CHECK_FALSE(c.map.at({0, 0}).road);
    CHECK_FALSE(c.map.at({0, 0}).roadblock);
    CHECK(demolish(c, {0, 0}).reason == "build.nothing_to_demolish");
}

TEST_CASE("road networks are connected components numbered in tile-index order") {
    auto c = city_from({"==.=", "...=", "==.="});
    CHECK(c.map.at({0, 0}).road_network == 1);
    CHECK(c.map.at({1, 0}).road_network == 1);
    CHECK(c.map.at({3, 0}).road_network == 2);
    CHECK(c.map.at({3, 2}).road_network == 2);
    CHECK(c.map.at({0, 2}).road_network == 3);
    CHECK(c.map.at({2, 1}).road_network == 0);
}

TEST_CASE("road access: perimeter from north of the NW corner, clockwise (S-007)") {
    // 2x2 building at (1,1); roads on the west and south sides: south is found first (clockwise order
    // N, E, S, W).
    auto c = city_from({"....", "=...", "=...", "===."});
    const auto& cat = bo::test::catalog();
    const auto r = place_building(c, cat, Difficulty::Hero, "fountain", {1, 1});
    REQUIRE(r.ok());
    const auto& b = c.buildings.items.at(r.building);
    REQUIRE(b.road_access);
    CHECK(*b.road_access == TilePos{2, 3});  // south edge scanned east to west
    CHECK_FALSE(find_road_access(c.map, {1, 0}, 1, 1));
    CHECK(*find_road_access(c.map, {1, 2}, 1, 1) == TilePos{1, 3});  // south beats west
}

TEST_CASE("demolishing a building frees every tile; ids are never reused") {
    const auto& cat = bo::test::catalog();
    auto c = city_from({"....", "....", "....", "...."});
    const auto a = place_building(c, cat, Difficulty::Hero, "fountain", {0, 0}).building;
    CHECK(demolish(c, {1, 1}).building == a);
    CHECK(c.map.at({0, 0}).building == kNoBuilding);
    const auto b = place_building(c, cat, Difficulty::Hero, "fountain", {0, 0}).building;
    CHECK(b > a);
    CHECK(buildings_in_square(c, {3, 3}, 2) == std::vector<BuildingId>{b});
}
