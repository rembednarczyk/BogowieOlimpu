// Module B1: terrain table and text map parser.
#include <fstream>

#include "bo/core/map.h"
#include "doctest/doctest.h"
#include "nlohmann/json.hpp"

using namespace bo::core;

TEST_CASE("terrain enum order and ids match data/terrain.json") {
    std::ifstream in(std::string(BO_DATA_DIR) + "/terrain.json");
    REQUIRE(in.good());
    const auto j = nlohmann::json::parse(in);
    REQUIRE(j.size() == static_cast<std::size_t>(Terrain::Count));
    for (std::size_t i = 0; i < j.size(); ++i) {
        const auto t = static_cast<Terrain>(i);
        CHECK(j[i]["id"].get<std::string>() == terrain_id(t));
        CHECK(j[i]["buildable"].get<bool>() == terrain_buildable(t));
    }
}

TEST_CASE("text map: symbols, roads and roadblocks") {
    std::string err;
    const auto m = parse_text_map({".,T~", "=x#b"}, err);
    REQUIRE(m);
    CHECK(m->width() == 4);
    CHECK(m->height() == 2);
    CHECK(m->at({1, 0}).terrain == Terrain::Meadow);
    CHECK(m->at({2, 0}).terrain == Terrain::Forest);
    CHECK(m->at({3, 0}).terrain == Terrain::Water);
    CHECK(m->is_road({0, 1}));
    CHECK(m->is_roam_road({0, 1}));
    CHECK(m->is_road({1, 1}));
    CHECK_FALSE(m->is_roam_road({1, 1}));
    CHECK(m->at({2, 1}).terrain == Terrain::Rock);
    CHECK_FALSE(m->is_road({9, 9}));
}

TEST_CASE("text map: bad input is rejected with a key") {
    std::string err;
    CHECK_FALSE(parse_text_map({"..", "."}, err));
    CHECK(err == "map.ragged_row");
    CHECK_FALSE(parse_text_map({".?"}, err));
    CHECK_FALSE(err.empty());
    CHECK_FALSE(parse_text_map({}, err));
    CHECK(err == "map.empty");
}

TEST_CASE("directions: rotate and opposite") {
    CHECK(rotate(Dir::North, 1) == Dir::East);
    CHECK(rotate(Dir::North, -1) == Dir::West);
    CHECK(rotate(Dir::West, 2) == Dir::East);
    CHECK(opposite(Dir::South) == Dir::North);
    CHECK(step({5, 5}, Dir::North) == TilePos{5, 4});
}
