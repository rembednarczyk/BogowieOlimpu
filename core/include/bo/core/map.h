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

// Resource points set by the map: animal spawn points and fishing grounds. Stored in saves: append only.
enum class TileFeature : std::uint8_t {
    None = 0, BoarSpawn, DeerSpawn, FishGround, UrchinGround,
    LandEntry,  // where the road to the rest of Greece meets the map edge (Z:17): caravans, immigrants
    SeaEntry    // where ships enter the map
};

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
    // Terrain height (L-099, S-009): integer level 0-15 above flat land. A ramp tile lies at its own level and
    // leads up one level in direction ramp - 1 (0 = no ramp). Any other change of level is a cliff.
    std::uint8_t elevation = 0;
    std::uint8_t ramp = 0;
    std::uint8_t feature = 0;  // map resource point: TileFeature (scenario data, Z:46-48, P:8)

    friend bool operator==(const Tile& a, const Tile& b) {
        return a.terrain == b.terrain && a.road == b.road && a.roadblock == b.roadblock && a.building == b.building &&
               a.random_byte == b.random_byte && a.elevation == b.elevation &&
               a.ramp == b.ramp && a.feature == b.feature;  // road_network is derived
    }
    friend bool operator!=(const Tile& a, const Tile& b) { return !(a == b); }
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

    // Whether a walker may step from `a` to its neighbour in direction `d`: same level, or along a ramp
    // (up its direction or down against it). Cliffs block (S-005, S-009).
    [[nodiscard]] bool can_step(TilePos a, Dir d) const {
        const TilePos b = step(a, d);
        if (!in_bounds(a) || !in_bounds(b)) {
            return false;
        }
        const Tile& ta = at(a);
        const Tile& tb = at(b);
        if (ta.elevation == tb.elevation) {
            return true;
        }
        const auto up = static_cast<std::uint8_t>(static_cast<std::uint8_t>(d) + 1);
        const auto down = static_cast<std::uint8_t>(static_cast<std::uint8_t>(opposite(d)) + 1);
        return (ta.ramp == up && tb.elevation == ta.elevation + 1) || (tb.ramp == down && ta.elevation == tb.elevation + 1);
    }

    // Fills random_byte for every tile from the map-generation stream.
    void generate_random_bytes(Pcg32& rng);

private:
    std::int32_t width_ = 0;
    std::int32_t height_ = 0;
    std::vector<Tile> tiles_;
};

// Parses a text map used by tests, the golden scenario and debug tools. One character per tile:
// '.' grassland, ',' meadow, 'T' forest, 'm' marsh, 'b' beach, '#' rock, '~' water, 'c' copper, 's' silver,
// 'q' marble quarry, 'o' orichalc, 'k' black marble, '=' road on grassland, 'x' roadblock on grassland,
// '^' '>' 'v' '<' grassland ramp leading up to the north, east, south, west,
// 'h' boar spawn and 'd' deer spawn on grassland, 'f' fishing ground and 'u' urchins on water,
// 'E' land entry (a road tile), 'S' sea entry (a water tile).
// Optional `elevation` rows: one hex digit (0-f) per tile. All rows must have the same length.
// Returns nullopt with `error` on bad input.
std::optional<TileMap> parse_text_map(const std::vector<std::string>& rows, std::string& error,
                                      const std::vector<std::string>& elevation = {});

}  // namespace bo::core
