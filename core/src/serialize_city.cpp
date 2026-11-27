// City and walker blocks of the save (format v2, extended in v3). Little-endian, field by field.
#include "serialize_city.h"

namespace bo::core {
namespace {

void write_pos(ByteWriter& w, TilePos p) {
    w.i32(p.x);
    w.i32(p.y);
}

bool read_pos(ByteReader& r, TilePos& p) { return r.i32(p.x) && r.i32(p.y); }

void write_positions(ByteWriter& w, const std::vector<TilePos>& v) {
    w.u32(static_cast<std::uint32_t>(v.size()));
    for (const TilePos p : v) {
        write_pos(w, p);
    }
}

bool read_positions(ByteReader& r, std::vector<TilePos>& v) {
    std::uint32_t n = 0;
    if (!r.u32(n) || n > (1U << 24)) {
        return false;
    }
    v.resize(n);
    for (auto& p : v) {
        if (!read_pos(r, p)) {
            return false;
        }
    }
    return true;
}

void write_tile(ByteWriter& w, const Tile& t) {
    w.u8(static_cast<std::uint8_t>(t.terrain));
    w.u8(static_cast<std::uint8_t>((t.road ? 1U : 0U) | (t.roadblock ? 2U : 0U)));
    w.u32(t.building);
    w.u8(t.random_byte);
    w.u8(static_cast<std::uint8_t>(t.elevation | (t.ramp << 4U)));  // since v8
    w.u8(t.feature);                                                  // since v11
}

bool read_tile(ByteReader& r, Tile& t, std::uint32_t version) {
    std::uint8_t terrain = 0;
    std::uint8_t flags = 0;
    if (!r.u8(terrain) || terrain >= static_cast<std::uint8_t>(Terrain::Count) || !r.u8(flags) || !r.u32(t.building) ||
        !r.u8(t.random_byte)) {
        return false;
    }
    t.terrain = static_cast<Terrain>(terrain);
    t.road = (flags & 1U) != 0;
    t.roadblock = (flags & 2U) != 0;
    std::uint8_t height = 0;
    if (version >= 8) {
        if (!r.u8(height) || (height >> 4U) > 4) {
            return false;
        }
        t.elevation = height & 0x0FU;
        t.ramp = static_cast<std::uint8_t>(height >> 4U);
    }
    if (version >= 11 && (!r.u8(t.feature) || t.feature > 9)) {
        return false;
    }
    return true;
}

void write_building(ByteWriter& w, const Building& b) {
    w.u32(b.id);
    w.str(b.type);
    write_pos(w, b.origin);
    w.i32(b.width);
    w.i32(b.height);
    w.u8(b.staffing_percent);
    w.u32(b.days_since_spawn);
    w.u8(static_cast<std::uint8_t>(b.next_roam_dir));
    w.u32(b.roaming_walker);
    w.u32(static_cast<std::uint32_t>(b.coverage.size()));
    for (const auto& [service, days] : b.coverage) {
        w.str(service);
        w.u8(days);
    }
    // since v3
    w.u8(b.rotation);
    write_positions(w, b.tiles);
    w.u8(static_cast<std::uint8_t>(b.road_mode));
    w.u8(b.road_tile ? 1 : 0);
    write_pos(w, b.road_tile.value_or(TilePos{}));
    w.u32(b.parent);
    write_positions(w, b.slot_origins);
    w.u32(static_cast<std::uint32_t>(b.slots.size()));
    for (const BuildingId id : b.slots) {
        w.u32(id);
    }
    // since v4
    w.u8(b.house ? 1 : 0);
    if (b.house) {
        const HouseState& h = *b.house;
        w.u8(static_cast<std::uint8_t>((h.elite ? 1U : 0U) | (h.abandoned ? 2U : 0U) | (h.established ? 4U : 0U)));
        w.u8(h.level);
        w.u32(h.population);
        for (const std::uint32_t g : h.goods) {
            w.u32(g);
        }
        w.u32(h.food_remainder);
        w.u8(h.failed_checks);
        w.u8(h.happiness);          // since v6
        w.u8(h.hungry_updates);     // since v6
        w.u8(h.days_without_road);  // since v6
    }
    w.u32(b.workers);  // since v5
    write_goods_state(w, b);  // since v7
    w.u32(b.fire_counter);    // since v12
    w.u32(b.damage_counter);
    w.u32(b.disease_counter);
    w.u32(b.crime_counter);
    w.u8(b.burning_days);
    w.u32(b.house ? b.house->quarantine_days : 0U);
    w.u8(b.house ? b.house->crime_level : 0U);  // since v13
    w.u32(b.house ? b.house->incoming : 0U);     // since v15
    w.u8(b.sanctified ? 1 : 0);                  // since v20
    w.u32(b.sanctified_base);
    w.u32(b.cursed_days);                        // since v23
    w.i32(b.hits);                               // since v25
    w.u32(b.cooldown);
    write_culture(w, b);                         // since v28
}

bool read_risks(ByteReader& r, Building& b, std::uint32_t version) {
    // In the order write_risks writes them: four counters, the burning days, then the quarantine (B-001).
    std::uint32_t v[5] = {};
    for (std::size_t i = 0; i < 4; ++i) {
        if (!r.u32(v[i]) || v[i] > 0xFFFF) {
            return false;
        }
    }
    if (!r.u8(b.burning_days) || !r.u32(v[4]) || v[4] > 0xFFFF) {
        return false;
    }
    b.fire_counter = static_cast<std::uint16_t>(v[0]);
    b.damage_counter = static_cast<std::uint16_t>(v[1]);
    b.disease_counter = static_cast<std::uint16_t>(v[2]);
    b.crime_counter = static_cast<std::uint16_t>(v[3]);
    if (b.house) {
        b.house->quarantine_days = static_cast<std::uint16_t>(v[4]);
    }
    std::uint8_t level = 0;
    if (version >= 13 && !r.u8(level)) {
        return false;
    }
    if (b.house) {
        b.house->crime_level = level;
    }
    std::uint32_t incoming = 0;
    if (version >= 15 && !r.u32(incoming)) {
        return false;
    }
    if (b.house) {
        b.house->incoming = incoming;
    }
    std::uint8_t sanctified = 0;
    if (version >= 20 && (!r.u8(sanctified) || !r.u32(b.sanctified_base))) {
        return false;
    }
    b.sanctified = sanctified != 0;
    std::uint32_t cursed = 0;
    if (version >= 23 && (!r.u32(cursed) || cursed > 0xFFFF)) {
        return false;
    }
    b.cursed_days = static_cast<std::uint16_t>(cursed);
    std::uint32_t cooldown = 0;
    if (version >= 25 && (!r.i32(b.hits) || !r.u32(cooldown) || cooldown > 0xFFFF)) {
        return false;
    }
    b.cooldown = static_cast<std::uint16_t>(cooldown);
    return version < 28 || read_culture(r, b);
}

bool read_house(ByteReader& r, Building& b, std::uint32_t version) {
    std::uint8_t has = 0;
    if (!r.u8(has)) {
        return false;
    }
    if (has == 0) {
        return true;
    }
    HouseState h;
    std::uint8_t flags = 0;
    if (!r.u8(flags) || !r.u8(h.level) || !r.u32(h.population)) {
        return false;
    }
    for (auto& g : h.goods) {
        if (!r.u32(g)) {
            return false;
        }
    }
    if (!r.u32(h.food_remainder) || !r.u8(h.failed_checks)) {
        return false;
    }
    if (version >= 6 && (!r.u8(h.happiness) || !r.u8(h.hungry_updates) || !r.u8(h.days_without_road))) {
        return false;
    }
    h.elite = (flags & 1U) != 0;
    h.abandoned = (flags & 2U) != 0;
    h.established = (flags & 4U) != 0;  // bit 2 since B-013; older saves: not yet established
    b.house = h;
    return true;
}

bool read_building_v3(ByteReader& r, Building& b) {
    std::uint8_t mode = 0;
    std::uint8_t has_road_tile = 0;
    TilePos road_tile;
    std::uint32_t slots = 0;
    if (!r.u8(b.rotation) || b.rotation > 3 || !read_positions(r, b.tiles) || !r.u8(mode) || mode > 2 ||
        !r.u8(has_road_tile) || !read_pos(r, road_tile) || !r.u32(b.parent) || !read_positions(r, b.slot_origins) ||
        !r.u32(slots) || slots != b.slot_origins.size()) {
        return false;
    }
    b.road_mode = static_cast<RoadMode>(mode);
    if (has_road_tile != 0) {
        b.road_tile = road_tile;
    }
    b.slots.resize(slots);
    for (auto& id : b.slots) {
        if (!r.u32(id)) {
            return false;
        }
    }
    return true;
}

bool read_building(ByteReader& r, Building& b, std::uint32_t version) {
    std::uint8_t dir = 0;
    std::uint32_t services = 0;
    if (!r.u32(b.id) || !r.str(b.type) || !read_pos(r, b.origin) || !r.i32(b.width) || !r.i32(b.height) ||
        !r.u8(b.staffing_percent) || !r.u32(b.days_since_spawn) || !r.u8(dir) || dir > 3 || !r.u32(b.roaming_walker) ||
        !r.u32(services)) {
        return false;
    }
    b.next_roam_dir = static_cast<Dir>(dir);
    for (std::uint32_t k = 0; k < services; ++k) {
        std::string service;
        std::uint8_t days = 0;
        if (!r.str(service) || !r.u8(days)) {
            return false;
        }
        b.coverage[service] = days;
    }
    if (version >= 3) {
        return read_building_v3(r, b) && (version < 4 || read_house(r, b, version)) && (version < 5 || r.u32(b.workers)) &&
               (version < 7 || read_goods_state(r, b, version)) && (version < 12 || read_risks(r, b, version));
    }
    // Migration v2 -> v3: every v2 building is an ordinary rectangle.
    for (std::int32_t dy = 0; dy < b.height; ++dy) {
        for (std::int32_t dx = 0; dx < b.width; ++dx) {
            b.tiles.push_back({b.origin.x + dx, b.origin.y + dy});
        }
    }
    return true;
}

void write_undo(ByteWriter& w, const UndoRecord& u) {
    w.u8(u.active ? 1 : 0);
    w.u64(u.tick);
    w.i64(u.cost);
    w.u32(static_cast<std::uint32_t>(u.tiles_before.size()));
    for (const auto& [pos, tile] : u.tiles_before) {
        write_pos(w, pos);
        write_tile(w, tile);
    }
    w.u32(static_cast<std::uint32_t>(u.buildings_before.size()));
    for (const Building& b : u.buildings_before) {
        write_building(w, b);
    }
    w.u32(static_cast<std::uint32_t>(u.added.size()));
    for (const BuildingId id : u.added) {
        w.u32(id);
    }
}

bool read_undo(ByteReader& r, UndoRecord& u, std::uint32_t version) {
    std::uint8_t active = 0;
    std::uint32_t n = 0;
    if (!r.u8(active) || !r.u64(u.tick) || !r.i64(u.cost) || !r.u32(n) || n > (1U << 24)) {
        return false;
    }
    u.active = active != 0;
    u.tiles_before.resize(n);
    for (auto& [pos, tile] : u.tiles_before) {
        if (!read_pos(r, pos) || !read_tile(r, tile, version)) {
            return false;
        }
    }
    if (!r.u32(n) || n > (1U << 20)) {
        return false;
    }
    u.buildings_before.resize(n);
    for (Building& b : u.buildings_before) {
        if (!read_building(r, b, version)) {
            return false;
        }
    }
    if (!r.u32(n) || n > (1U << 20)) {
        return false;
    }
    u.added.resize(n);
    for (auto& id : u.added) {
        if (!r.u32(id)) {
            return false;
        }
    }
    return true;
}

}  // namespace

void write_city(ByteWriter& w, const CityState& c) {
    w.i32(c.map.width());
    w.i32(c.map.height());
    for (const Tile& t : c.map.tiles()) {
        write_tile(w, t);
    }
    w.u32(c.buildings.next_id);
    w.u32(static_cast<std::uint32_t>(c.buildings.items.size()));
    for (const auto& [id, b] : c.buildings.items) {
        write_building(w, b);
    }
    write_undo(w, c.undo);  // since v3
    w.u32(c.homeless);      // since v4
    w.u8(c.wind);                                        // since v12
    w.u8(c.hygiene);                                     // since v13
    w.u32(static_cast<std::uint32_t>(c.spawns.size()));  // since v11
    for (const auto& [tile, s] : c.spawns) {
        w.u32(tile);
        w.u8(s.animals);
        w.u32(s.timer);
        w.u32(s.hunter);  // since v16
    }
}

bool read_city(ByteReader& r, CityState& c, std::uint32_t version) {
    std::int32_t width = 0;
    std::int32_t height = 0;
    if (!r.i32(width) || !r.i32(height) || width < 0 || height < 0 || width > 4096 || height > 4096) {
        return false;
    }
    c.map = TileMap(width, height);
    for (Tile& t : c.map.tiles()) {
        if (!read_tile(r, t, version)) {
            return false;
        }
    }
    std::uint32_t count = 0;
    if (!r.u32(c.buildings.next_id) || !r.u32(count)) {
        return false;
    }
    for (std::uint32_t i = 0; i < count; ++i) {
        Building b;
        if (!read_building(r, b, version)) {
            return false;
        }
        c.buildings.items.emplace(b.id, std::move(b));
    }
    if (version >= 3 && !read_undo(r, c.undo, version)) {
        return false;
    }
    if (version >= 4 && !r.u32(c.homeless)) {
        return false;
    }
    if (version >= 12 && (!r.u8(c.wind) || c.wind > 3)) {
        return false;
    }
    if (version >= 13 && (!r.u8(c.hygiene) || c.hygiene > 100)) {
        return false;
    }
    std::uint32_t spawns = 0;
    if (version >= 11 && (!r.u32(spawns) || spawns > c.map.size())) {
        return false;
    }
    for (std::uint32_t i = 0; i < spawns; ++i) {
        std::uint32_t tile = 0;
        std::uint32_t timer = 0;
        SpawnState s;
        if (!r.u32(tile) || !r.u8(s.animals) || !r.u32(timer) || timer > 0xFFFF ||
            (version >= 16 && !r.u32(s.hunter))) {
            return false;
        }
        s.timer = static_cast<std::uint16_t>(timer);
        c.spawns[tile] = s;
    }
    update_roads(c);  // derived data: networks and road access
    return true;
}

void write_walkers(ByteWriter& w, const WalkerStore& s) {
    w.u32(s.next_id);
    w.u32(static_cast<std::uint32_t>(s.items.size()));
    for (const auto& [id, k] : s.items) {
        w.u32(id);
        w.str(k.type);
        w.u8(static_cast<std::uint8_t>(k.kind));
        w.u32(k.home);
        w.u8(static_cast<std::uint8_t>(k.phase));
        write_pos(w, k.tile);
        w.u8(static_cast<std::uint8_t>(k.dir));
        w.u8(k.came_from ? static_cast<std::uint8_t>(*k.came_from) : 0xFF);
        w.i64(k.speed_raw);
        w.i64(k.step_accum_raw);
        write_positions(w, k.path);
        w.u32(k.path_pos);
        w.u32(k.roam_ticks);
        w.u32(k.max_roam_ticks);
        write_pos(w, k.roam_target);
        w.u8(static_cast<std::uint8_t>(k.turn));
        w.i32(k.ticks_until_next_turn);
        w.u32(k.random_counter);
        w.u8(k.return_north ? 1 : 0);  // since v3
        w.u8(k.coverage_days);         // since v5
        w.u8(static_cast<std::uint8_t>(k.job));  // since v7
        w.u32(k.target);
        write_counts(w, k.cargo);
        w.str(k.want_good);  // since v10
        w.u32(k.want_loads);
        write_pos(w, k.resource);
        w.u32(k.work_ticks);
        w.u8(k.resets);  // since v12
        w.str(k.variant);
        w.u8(k.ignores_roadblocks ? 1 : 0);  // since v13
        w.i32(k.hp);                         // since v18
        w.u32(k.cooldown);
        w.u32(k.aboard);  // since v39
    }
}

bool read_walkers(ByteReader& r, WalkerStore& s, std::uint32_t version) {
    std::uint32_t count = 0;
    if (!r.u32(s.next_id) || !r.u32(count)) {
        return false;
    }
    for (std::uint32_t i = 0; i < count; ++i) {
        Walker k;
        std::uint8_t kind = 0;
        std::uint8_t phase = 0;
        std::uint8_t dir = 0;
        std::uint8_t came = 0;
        std::uint8_t turn = 0;
        std::uint8_t north = 0;
        const bool ok = r.u32(k.id) && r.str(k.type) && r.u8(kind) && kind <= 2 && r.u32(k.home) && r.u8(phase) &&
                        phase <= 5 && read_pos(r, k.tile) && r.u8(dir) && dir <= 3 && r.u8(came) &&
                        r.i64(k.speed_raw) && r.i64(k.step_accum_raw) && read_positions(r, k.path) &&
                        r.u32(k.path_pos) && r.u32(k.roam_ticks) && r.u32(k.max_roam_ticks) &&
                        read_pos(r, k.roam_target) && r.u8(turn) && r.i32(k.ticks_until_next_turn) &&
                        r.u32(k.random_counter) && (version < 3 || r.u8(north)) &&
                        (version < 5 || r.u8(k.coverage_days));
        std::uint8_t job = 0;
        if (ok && version >= 7 && (!r.u8(job) || job > static_cast<std::uint8_t>(WalkerJob::Ship) || !r.u32(k.target) || !read_counts(r, k.cargo))) {
            return false;
        }
        if (ok && version >= 10 &&
            (!r.str(k.want_good) || !r.u32(k.want_loads) || !read_pos(r, k.resource) || !r.u32(k.work_ticks))) {
            return false;
        }
        if (ok && version >= 12 && (!r.u8(k.resets) || !r.str(k.variant))) {
            return false;
        }
        std::uint8_t ignores = 0;
        if (ok && version >= 13 && !r.u8(ignores)) {
            return false;
        }
        k.ignores_roadblocks = ignores != 0;
        std::uint32_t cooldown = 0;
        std::uint32_t aboard = 0;
        if (ok && ((version >= 18 && (!r.i32(k.hp) || !r.u32(cooldown) || cooldown > 0xFFFF)) ||
                   (version >= 39 && (!r.u32(aboard) || aboard > 0xFFFF)))) {
            return false;
        }
        k.cooldown = static_cast<std::uint16_t>(cooldown);
        k.aboard = static_cast<std::uint16_t>(aboard);
        k.job = static_cast<WalkerJob>(job);
        if (!ok) {
            return false;
        }
        k.kind = static_cast<WalkerKind>(kind);
        k.phase = static_cast<WalkerPhase>(phase);
        k.dir = static_cast<Dir>(dir);
        if (came <= 3) {
            k.came_from = static_cast<Dir>(came);
        }
        k.turn = static_cast<std::int8_t>(turn);
        k.return_north = north != 0;
        s.items.emplace(k.id, std::move(k));
    }
    return true;
}

}  // namespace bo::core

