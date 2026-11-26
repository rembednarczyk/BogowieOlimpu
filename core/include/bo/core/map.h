// Module B1: tile grid and terrain (docs/gameplay/01-podstawy.md, data/terrain.json).
// Coordinates: x grows east, y grows south; "north" is y - 1. Tiles are stored row-major, so iterating by
// tile index is a fixed, platform independent order (docs/PRAKTYKI.md ch. 3).
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "bo/core/rng.h"

namespace bo::core {

// Values are stored in saves: append only. Order and ids match data/terrain.json (checked by a test).
enum class Terrain : std::uint8_t {
    Grassland = 0,
    Meadow,
    Forest,
    Marsh,
    Beach,
    Rock,
    CopperOre,
    SilverOre,
    MarbleQuarry,
    Water,
    OrichalcOre,
    BlackMarbleQuarry,
    Count
};

std::string_view terrain_id(Terrain t);

// Building placement allowed (data/terrain.json "buildable").
bool terrain_buildable(Terrain t);
// Pedestrians may cross it off-road (Z:17-18: marsh passable, rock only at passes; beach and forest:
// [ZAŁOŻENIE] beach passable, forest not).
bool terrain_walkable(Terrain t);

enum class Dir : std::uint8_t { North = 0, East = 1, South = 2, West = 3 };
inline constexpr std::array<Dir, 4> kDirs = {Dir::North, Dir::East, Dir::South, Dir::West};
constexpr Dir opposite(Dir d) { return static_cast<Dir>((static_cast<std::uint8_t>(d) + 2) % 4); }
constexpr Dir rotate(Dir d, int quarter_turns) {
    return static_cast<Dir>(((static_cast<int>(d) + quarter_turns) % 4 + 4) % 4);
}

struct TilePos {
    std::int32_t x = 0;
    std::int32_t y = 0;
    friend constexpr bool operator==(TilePos a, TilePos b) { return a.x == b.x && a.y == b.y; }
    friend constexpr bool operator!=(TilePos a, TilePos b) { return !(a == b); }
};

constexpr TilePos step(TilePos p, Dir d) {
    constexpr std::int32_t kDx[] = {0, 1, 0, -1};
    constexpr std::int32_t kDy[] = {-1, 0, 1, 0};
    return {p.x + kDx[static_cast<int>(d)], p.y + kDy[static_cast<int>(d)]};
}

using BuildingId = std::uint32_t;  // 0 = none
inline constexpr BuildingId kNoBuilding = 0;

struct Tile {
    Terrain terrain = Terrain::Grassland;
    bool road = false;
    bool roadblock = false;     // a road tile that stops roaming walkers (Z:19-20)
    BuildingId building = kNoBuilding;
    std::uint8_t random_byte = 0;  // fixed per tile at map creation (engine family, doc 24)
    std::uint32_t road_network = 0;  // 0 = no road; recomputed by the road system
};

class TileMap {
public:
    TileMap() = default;
    TileMap(std::int32_t width, std::int32_t height) : width_(width), height_(height), tiles_(size()) {}

    [[nodiscard]] std::int32_t width() const { return width_; }
    [[nodiscard]] std::int32_t height() const { return height_; }
    [[nodiscard]] std::size_t size() const { return static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_); }
    [[nodiscard]] bool in_bounds(TilePos p) const { return p.x >= 0 && p.y >= 0 && p.x < width_ && p.y < height_; }
    [[nodiscard]] std::size_t index(TilePos p) const {
        return static_cast<std::size_t>(p.y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(p.x);
    }
    [[nodiscard]] TilePos pos(std::size_t index) const {
        return {static_cast<std::int32_t>(index % static_cast<std::size_t>(width_)),
                static_cast<std::int32_t>(index / static_cast<std::size_t>(width_))};
    }
    Tile& at(TilePos p) { return tiles_[index(p)]; }
    [[nodiscard]] const Tile& at(TilePos p) const { return tiles_[index(p)]; }
    [[nodiscard]] const std::vector<Tile>& tiles() const { return tiles_; }
    std::vector<Tile>& tiles() { return tiles_; }

    [[nodiscard]] bool is_road(TilePos p) const { return in_bounds(p) && at(p).road; }
    // Road usable by roaming walkers: road and not a roadblock (gatehouse handling arrives with military).
    [[nodiscard]] bool is_roam_road(TilePos p) const { return is_road(p) && !at(p).roadblock; }

    // Fills random_byte for every tile from the map-generation stream.
    void generate_random_bytes(Pcg32& rng);

private:
    std::int32_t width_ = 0;
    std::int32_t height_ = 0;
    std::vector<Tile> tiles_;
};

// Parses a text map used by tests, the golden scenario and debug tools. One character per tile:
// '.' grassland, ',' meadow, 'T' forest, 'm' marsh, 'b' beach, '#' rock, '~' water, 'c' copper, 's' silver,
// 'q' marble quarry, 'o' orichalc, 'k' black marble, '=' road on grassland, 'x' roadblock on grassland.
// All rows must have the same length. Returns nullopt with `error` on bad input.
std::optional<TileMap> parse_text_map(const std::vector<std::string>& rows, std::string& error);

}  // namespace bo::core
