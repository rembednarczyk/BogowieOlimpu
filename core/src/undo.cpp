// Undo (Z:20, docs/gameplay/01-podstawy.md): the last build action can be reverted with a full refund for a
// short time. Pattern: Memento stored as a difference, so the record is small and part of the save.
#include <algorithm>

#include "bo/core/city.h"

namespace bo::core {

void record_undo(CityState& city, const CityState& before, std::uint64_t tick, std::int64_t cost) {
    UndoRecord rec;
    for (std::size_t i = 0; i < city.map.size(); ++i) {
        if (city.map.tiles()[i] != before.map.tiles()[i]) {
            rec.tiles_before.emplace_back(city.map.pos(i), before.map.tiles()[i]);
        }
    }
    for (const auto& [id, b] : before.buildings.items) {
        const auto now = city.buildings.items.find(id);
        if (now == city.buildings.items.end() || now->second != b) {
            rec.buildings_before.push_back(b);
        }
    }
    for (const auto& [id, b] : city.buildings.items) {
        if (before.buildings.items.count(id) == 0) {
            rec.added.push_back(id);
        }
    }
    if (rec.tiles_before.empty() && rec.buildings_before.empty() && rec.added.empty()) {
        return;  // nothing changed (every command was rejected): the previous record stays
    }
    rec.active = true;
    rec.tick = tick;
    rec.cost = cost;
    city.undo = std::move(rec);
}

std::optional<std::int64_t> apply_undo(CityState& city) {
    if (!city.undo.active) {
        return std::nullopt;
    }
    UndoRecord rec = std::move(city.undo);
    city.undo = UndoRecord{};
    for (const BuildingId id : rec.added) {
        city.buildings.items.erase(id);
    }
    for (Building& b : rec.buildings_before) {
        city.buildings.items[b.id] = std::move(b);
    }
    for (const auto& [pos, tile] : rec.tiles_before) {
        city.map.at(pos) = tile;
    }
    update_roads(city);
    return rec.cost;
}

}  // namespace bo::core
