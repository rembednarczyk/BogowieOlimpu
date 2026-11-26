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
    return true;
}

void write_goods_state(ByteWriter& w, const Building& b);

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
        w.u8(static_cast<std::uint8_t>((h.elite ? 1U : 0U) | (h.abandoned ? 2U : 0U)));
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
}

void write_counts(ByteWriter& w, const std::map<std::string, std::uint32_t>& m) {
    w.u32(static_cast<std::uint32_t>(m.size()));
    for (const auto& [k, v] : m) {
        w.str(k);
        w.u32(v);
    }
}

bool read_counts(ByteReader& r, std::map<std::string, std::uint32_t>& m) {
    std::uint32_t n = 0;
    if (!r.u32(n) || n > 4096) {
        return false;
    }
    for (std::uint32_t i = 0; i < n; ++i) {
        std::string k;
        std::uint32_t v = 0;
        if (!r.str(k) || !r.u32(v)) {
            return false;
        }
        m[k] = v;
    }
    return true;
}

// Storage, producer and stall state (v7): a flag byte, then each present block.
void write_goods_state(ByteWriter& w, const Building& b) {
    w.u8(static_cast<std::uint8_t>((b.storage ? 1U : 0U) | (b.producer ? 2U : 0U) | (b.stall ? 4U : 0U)));
    if (b.storage) {
        write_counts(w, b.storage->loads);
        w.u32(static_cast<std::uint32_t>(b.storage->orders.size()));
        for (const auto& [good, o] : b.storage->orders) {
            w.str(good);
            w.u8(static_cast<std::uint8_t>(o.kind));
            w.u32(o.limit);
        }
    }
    if (b.producer) {
        w.u32(b.producer->progress);
        w.u32(b.producer->output_loads);
        w.u32(b.producer->deliveryman);
    }
    if (b.stall) {
        write_counts(w, b.stall->units);
        w.u32(b.stall->vendor);
    }
}

bool read_goods_state(ByteReader& r, Building& b) {
    std::uint8_t flags = 0;
    if (!r.u8(flags) || flags > 7) {
        return false;
    }
    if ((flags & 1U) != 0) {
        StorageState s;
        std::uint32_t n = 0;
        if (!read_counts(r, s.loads) || !r.u32(n) || n > 4096) {
            return false;
        }
        for (std::uint32_t i = 0; i < n; ++i) {
            std::string good;
            std::uint8_t kind = 0;
            StorageOrder o;
            if (!r.str(good) || !r.u8(kind) || kind > 3 || !r.u32(o.limit)) {
                return false;
            }
            o.kind = static_cast<StorageOrderKind>(kind);
            s.orders[good] = o;
        }
        b.storage = std::move(s);
    }
    if ((flags & 2U) != 0) {
        ProducerState p;
        if (!r.u32(p.progress) || !r.u32(p.output_loads) || !r.u32(p.deliveryman)) {
            return false;
        }
        b.producer = p;
    }
    if ((flags & 4U) != 0) {
        StallState s;
        if (!read_counts(r, s.units) || !r.u32(s.vendor)) {
            return false;
        }
        b.stall = std::move(s);
    }
    return true;
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
               (version < 7 || read_goods_state(r, b));
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
        if (ok && version >= 7 && (!r.u8(job) || job > 2 || !r.u32(k.target) || !read_counts(r, k.cargo))) {
            return false;
        }
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

