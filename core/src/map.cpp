#include "bo/core/map.h"

namespace bo::core {
namespace {

struct TerrainInfo {
    std::string_view id;
    bool buildable;
    bool walkable;
    char symbol;
};

constexpr std::array<TerrainInfo, static_cast<std::size_t>(Terrain::Count)> kTerrain = {{
    {"grassland", true, true, '.'},
    {"meadow", true, true, ','},
    {"forest", false, false, 'T'},
    {"marsh", false, true, 'm'},
    {"beach", false, true, 'b'},
    {"rock", false, false, '#'},
    {"copper_ore_rock", false, false, 'c'},
    {"silver_ore_rock", false, false, 's'},
    {"marble_quarry", false, false, 'q'},
    {"water", false, false, '~'},
    {"orichalc_ore_rock", false, false, 'o'},
    {"black_marble_quarry", false, false, 'k'},
}};

}  // namespace

std::string_view terrain_id(Terrain t) { return kTerrain[static_cast<std::size_t>(t)].id; }
bool terrain_buildable(Terrain t) { return kTerrain[static_cast<std::size_t>(t)].buildable; }
bool terrain_walkable(Terrain t) { return kTerrain[static_cast<std::size_t>(t)].walkable; }

void TileMap::generate_random_bytes(Pcg32& rng) {
    for (auto& t : tiles_) {
        t.random_byte = static_cast<std::uint8_t>(rng.next_u32() & 0xFFU);
    }
}

std::optional<TileMap> parse_text_map(const std::vector<std::string>& rows, std::string& error) {
    if (rows.empty() || rows.front().empty()) {
        error = "map.empty";
        return std::nullopt;
    }
    const auto width = static_cast<std::int32_t>(rows.front().size());
    TileMap map(width, static_cast<std::int32_t>(rows.size()));
    for (std::size_t y = 0; y < rows.size(); ++y) {
        if (static_cast<std::int32_t>(rows[y].size()) != width) {
            error = "map.ragged_row";
            return std::nullopt;
        }
        for (std::size_t x = 0; x < rows[y].size(); ++x) {
            Tile& tile = map.at({static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)});
            const char c = rows[y][x];
            if (c == '=' || c == 'x') {
                tile.terrain = Terrain::Grassland;
                tile.road = true;
                tile.roadblock = c == 'x';
                continue;
            }
            bool found = false;
            for (std::size_t i = 0; i < kTerrain.size(); ++i) {
                if (kTerrain[i].symbol == c) {
                    tile.terrain = static_cast<Terrain>(i);
                    found = true;
                    break;
                }
            }
            if (!found) {
                error = std::string("map.unknown_symbol:") + c;
                return std::nullopt;
            }
        }
    }
    return map;
}

}  // namespace bo::core
