// Module B2/B3 placement rules: rotation, coast, agoras and stalls, boulevards and avenues, crossings.
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

BuildResult put(CityState& c, const std::string& type, TilePos p, std::uint8_t rot = 0) {
    return place_building(c, bo::test::catalog(), Difficulty::Hero, type, p, rot);
}

}  // namespace

TEST_CASE("rotation: rotatable buildings swap their footprint, others refuse (Z:24-25)") {
    auto c = city_from(std::vector<std::string>(12, std::string(12, '.')));
    const auto r = put(c, "palace", {0, 0}, 1);  // 9x6 -> 6x9
    REQUIRE(r.ok());
    const Building& b = c.buildings.items.at(r.building);
    CHECK(b.width == 6);
    CHECK(b.height == 9);
    CHECK(b.rotation == 1);
    CHECK(put(c, "fountain", {8, 0}, 1).reason == "build.not_rotatable");
    CHECK(put(c, "fountain", {8, 0}, 4).reason == "build.bad_rotation");
}

TEST_CASE("coast buildings need one whole side on a straight shore and face the water (Z:46-47)") {
    auto c = city_from({"......", "......", "......", "~~~~~~", "~~~~~~"});
    CHECK(put(c, "fishery", {0, 0}).reason == "build.needs_straight_coast");
    const auto r = put(c, "fishery", {2, 1});
    REQUIRE(r.ok());
    CHECK(c.buildings.items.at(r.building).rotation == static_cast<std::uint8_t>(Dir::South));
    auto bay = city_from({"......", "......", "......", ".~~...", "~~~~~~"});
    CHECK(put(bay, "fishery", {1, 1}).reason == "build.needs_straight_coast");  // shore not straight past corner
}

TEST_CASE("agoras: common over a road on its long side, grand with the road in the middle (Z:69-70)") {
    std::vector<std::string> rows(8, std::string(10, '.'));
    for (auto& row : rows) {
        row[4] = '=';
    }
    auto c = city_from(rows);
    CHECK(put(c, "common_agora", {0, 0}).reason == "build.needs_road_under");
    const auto common = put(c, "common_agora", {4, 0});  // road on the west lane
    REQUIRE(common.ok());
    const Building& a = c.buildings.items.at(common.building);
    CHECK(a.slot_origins == std::vector<TilePos>{{5, 0}, {5, 2}, {5, 4}});
    CHECK(c.map.is_road({4, 3}));
    CHECK(a.road_access == TilePos{4, 3});  // middle of its road [eZeus]
    auto g = city_from(rows);
    const auto grand = put(g, "grand_agora", {2, 1});
    REQUIRE(grand.ok());
    CHECK(g.buildings.items.at(grand.building).slots.size() == 6);
}

TEST_CASE("stalls fill free agora slots; demolishing the slot removes the stall, then the agora") {
    std::vector<std::string> rows(8, std::string(10, '.'));
    for (auto& row : rows) {
        row[4] = '=';
    }
    auto c = city_from(rows);
    const auto agora = put(c, "common_agora", {4, 0}).building;
    CHECK(put(c, "food_vendor_stall", {6, 0}).reason == "build.needs_agora_slot");
    const auto stall = put(c, "food_vendor_stall", {5, 2});
    REQUIRE(stall.ok());
    CHECK(put(c, "oil_vendor_stall", {5, 2}).reason == "build.occupied");
    CHECK(c.buildings.items.at(stall.building).parent == agora);
    CHECK(c.buildings.items.at(stall.building).road_access == c.buildings.items.at(agora).road_access);
    CHECK(demolish(c, {6, 3}).removed == std::vector<BuildingId>{stall.building});
    CHECK(c.buildings.items.at(agora).slots[1] == kNoBuilding);
    put(c, "wine_vendor_stall", {5, 4});
    CHECK(demolish(c, {6, 1}).removed.size() == 2);  // free slot: the agora goes with its stalls
    CHECK(c.map.is_road({4, 2}));                     // the road under it stays
}

TEST_CASE("boulevard and avenue decorate a road; buildings behind them reach the road (Z:154)") {
    auto c = city_from({".....", ".....", "=====", ".....", "....."});
    CHECK(put(c, "boulevard", {0, 0}, 1).reason == "build.needs_road");
    const auto bl = put(c, "boulevard", {1, 2}, 1);  // road runs east-west: ornaments north and south
    REQUIRE(bl.ok());
    CHECK(c.map.at({1, 1}).building == bl.building);
    CHECK(c.map.at({1, 3}).building == bl.building);
    CHECK(c.map.is_road({1, 2}));
    const auto av = put(c, "avenue", {3, 2}, static_cast<std::uint8_t>(Dir::North));
    REQUIRE(av.ok());
    CHECK(*find_road_access(c, {1, 0}, 1, 1) == TilePos{1, 2});  // through the boulevard ornament
    CHECK(demolish(c, {1, 2}).removed == std::vector<BuildingId>{bl.building});
}

TEST_CASE("water crossing: straight shores, at most kMaxCrossingSpan, no crossing side by side (Z:101-103)") {
    auto c = city_from({"......", "......", "~~~~~~", "~~~~~~", "......", "......"});
    const auto r = put(c, "water_crossing", {2, 2}, static_cast<std::uint8_t>(Dir::South));
    REQUIRE(r.ok());
    CHECK(c.map.is_road({2, 2}));
    CHECK(c.map.is_road({2, 3}));
    CHECK(put(c, "water_crossing", {3, 2}, static_cast<std::uint8_t>(Dir::South)).reason == "build.crossing_adjacent");
    CHECK(put(c, "bridge", {4, 2}, static_cast<std::uint8_t>(Dir::South)).ok());
    CHECK(put(c, "water_crossing", {2, 1}, static_cast<std::uint8_t>(Dir::South)).reason == "build.needs_coast");
    demolish(c, {2, 3});
    CHECK_FALSE(c.map.is_road({2, 2}));  // its own road goes with it
    auto wide = city_from({"..", "~~", "~~", "~~", "~~", "~~", "~~", "~~", "~~", "~~", ".."});
    CHECK(put(wide, "water_crossing", {0, 1}, static_cast<std::uint8_t>(Dir::South)).reason == "build.crossing_too_long");
}
