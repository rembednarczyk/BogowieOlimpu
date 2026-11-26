// Module B4: A* path finding.
#include "bo/core/path.h"
#include "doctest/doctest.h"

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

TEST_CASE("path: shortest along roads, empty when start is goal, nullopt when unreachable") {
    auto c = city_from({"=====", "=...=", "=====", "....=", "=...."});
    const auto p = find_path(c, {{0, 0}, {4, 2}, PathMode::Road});
    REQUIRE(p);
    CHECK(p->size() == 6);
    CHECK(p->back() == TilePos{4, 2});
    CHECK(find_path(c, {{0, 0}, {0, 0}, PathMode::Road})->empty());
    CHECK_FALSE(find_path(c, {{0, 0}, {0, 4}, PathMode::Road}));
}

TEST_CASE("path: equal-length routes resolve the same way every time (tile-index tie-break)") {
    auto c = city_from({"=====", "=...=", "====="});
    const auto a = find_path(c, {{0, 1}, {4, 1}, PathMode::Road});
    const auto b = find_path(c, {{0, 1}, {4, 1}, PathMode::Road});
    REQUIRE(a);
    CHECK(a->size() == 6);
    CHECK(*a == *b);
}

TEST_CASE("path: roadblocks stop roaming paths and let destination paths through (Z:19-20)") {
    auto c = city_from({"==x=="});
    CHECK(find_path(c, {{0, 0}, {4, 0}, PathMode::Road}));
    CHECK_FALSE(find_path(c, {{0, 0}, {4, 0}, PathMode::RoadNoRoadblock}));
}

TEST_CASE("path: walkable mode crosses grass and marsh, not forest or water") {
    auto c = city_from({".m.", "T~T", "..."});
    CHECK(find_path(c, {{0, 0}, {2, 0}, PathMode::Walkable})->size() == 2);
    CHECK_FALSE(find_path(c, {{0, 0}, {0, 2}, PathMode::Walkable}));
}

TEST_CASE("closest road: Chebyshev distance, then tile index") {
    auto c = city_from({"=....", ".....", "....=", "....."});
    CHECK(*closest_road_within(c.map, {2, 1}, 6, false) == TilePos{0, 0});
    CHECK(*closest_road_within(c.map, {3, 2}, 6, false) == TilePos{4, 2});
    CHECK_FALSE(closest_road_within(c.map, {2, 1}, 1, false));
}

TEST_CASE("distance map: steps along roads from the nearest source, unreachable elsewhere") {
    auto c = city_from({"=====", "....=", "=====", "....."});
    const auto d = road_distances(c, {{0, 0}, {0, 2}}, PathMode::Road);
    CHECK(d[c.map.index({0, 0})] == 0);
    CHECK(d[c.map.index({4, 0})] == 4);
    CHECK(d[c.map.index({4, 1})] == 5);
    CHECK(d[c.map.index({2, 2})] == 2);
    CHECK(d[c.map.index({2, 3})] == kUnreachable);
}
