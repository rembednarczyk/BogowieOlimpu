#include "bo/core/serialize.h"

#include <type_traits>
#include <variant>

namespace bo::core {

bool ByteReader::u32(std::uint32_t& v) {
    v = 0;
    for (int i = 0; i < 4; ++i) {
        std::uint8_t b = 0;
        if (!u8(b)) {
            return false;
        }
        v |= static_cast<std::uint32_t>(b) << (8 * i);
    }
    return true;
}

bool ByteReader::u64(std::uint64_t& v) {
    v = 0;
    for (int i = 0; i < 8; ++i) {
        std::uint8_t b = 0;
        if (!u8(b)) {
            return false;
        }
        v |= static_cast<std::uint64_t>(b) << (8 * i);
    }
    return true;
}

bool ByteReader::i64(std::int64_t& v) {
    std::uint64_t u = 0;
    const bool ok = u64(u);
    v = static_cast<std::int64_t>(u);
    return ok;
}

bool ByteReader::i32(std::int32_t& v) {
    std::uint32_t u = 0;
    const bool ok = u32(u);
    v = static_cast<std::int32_t>(u);
    return ok;
}

bool ByteReader::str(std::string& s) {
    std::uint32_t n = 0;
    if (!u32(n) || n > bytes_.size() - pos_) {
        return false;
    }
    s.assign(reinterpret_cast<const char*>(bytes_.data() + pos_), n);
    pos_ += n;
    return true;
}

namespace {

// --- commands -------------------------------------------------------------------------------------------
void write_command(ByteWriter& w, const Command& c) {
    w.u32(static_cast<std::uint32_t>(c.index()));
    std::visit(
        [&w](const auto& cmd) {
            using T = std::decay_t<decltype(cmd)>;
            if constexpr (std::is_same_v<T, SetTaxRate> || std::is_same_v<T, SetWageRate>) {
                w.u8(static_cast<std::uint8_t>(cmd.rate));
            } else if constexpr (std::is_same_v<T, PlaceBuilding>) {
                w.str(cmd.type);
                w.i32(cmd.x);
                w.i32(cmd.y);
            } else {
                w.i32(cmd.x);
                w.i32(cmd.y);
            }
        },
        c);
}

template <class T>
bool read_xy(ByteReader& r, Command& out) {
    T t;
    if (!r.i32(t.x) || !r.i32(t.y)) {
        return false;
    }
    out = t;
    return true;
}

bool read_command(ByteReader& r, Command& out) {
    std::uint32_t index = 0;
    if (!r.u32(index)) {
        return false;
    }
    std::uint8_t v = 0;
    switch (index) {
        case 0:
            if (!r.u8(v) || v >= static_cast<std::uint8_t>(TaxRate::Count)) {
                return false;
            }
            out = SetTaxRate{static_cast<TaxRate>(v)};
            return true;
        case 1:
            if (!r.u8(v) || v >= static_cast<std::uint8_t>(WageRate::Count)) {
                return false;
            }
            out = SetWageRate{static_cast<WageRate>(v)};
            return true;
        case 2: {
            PlaceBuilding pb;
            if (!r.str(pb.type) || !r.i32(pb.x) || !r.i32(pb.y)) {
                return false;
            }
            out = pb;
            return true;
        }
        case 3:
            return read_xy<PlaceRoad>(r, out);
        case 4:
            return read_xy<PlaceRoadblock>(r, out);
        case 5:
            return read_xy<ClearLand>(r, out);
        case 6:
            return read_xy<Demolish>(r, out);
        default:
            return false;
    }
}

// --- city -----------------------------------------------------------------------------------------------
void write_pos(ByteWriter& w, TilePos p) {
    w.i32(p.x);
    w.i32(p.y);
}

bool read_pos(ByteReader& r, TilePos& p) { return r.i32(p.x) && r.i32(p.y); }

void write_city(ByteWriter& w, const CityState& c) {
    w.i32(c.map.width());
    w.i32(c.map.height());
    for (const Tile& t : c.map.tiles()) {
        w.u8(static_cast<std::uint8_t>(t.terrain));
        w.u8(static_cast<std::uint8_t>((t.road ? 1U : 0U) | (t.roadblock ? 2U : 0U)));
        w.u32(t.building);
        w.u8(t.random_byte);
    }
    w.u32(c.buildings.next_id);
    w.u32(static_cast<std::uint32_t>(c.buildings.items.size()));
    for (const auto& [id, b] : c.buildings.items) {
        w.u32(id);
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
    }
}

bool read_city(ByteReader& r, CityState& c) {
    std::int32_t width = 0;
    std::int32_t height = 0;
    if (!r.i32(width) || !r.i32(height) || width < 0 || height < 0 || width > 4096 || height > 4096) {
        return false;
    }
    c.map = TileMap(width, height);
    for (Tile& t : c.map.tiles()) {
        std::uint8_t terrain = 0;
        std::uint8_t flags = 0;
        if (!r.u8(terrain) || terrain >= static_cast<std::uint8_t>(Terrain::Count) || !r.u8(flags) ||
            !r.u32(t.building) || !r.u8(t.random_byte)) {
            return false;
        }
        t.terrain = static_cast<Terrain>(terrain);
        t.road = (flags & 1U) != 0;
        t.roadblock = (flags & 2U) != 0;
    }
    std::uint32_t count = 0;
    if (!r.u32(c.buildings.next_id) || !r.u32(count)) {
        return false;
    }
    for (std::uint32_t i = 0; i < count; ++i) {
        Building b;
        std::uint8_t dir = 0;
        std::uint32_t services = 0;
        if (!r.u32(b.id) || !r.str(b.type) || !read_pos(r, b.origin) || !r.i32(b.width) || !r.i32(b.height) ||
            !r.u8(b.staffing_percent) || !r.u32(b.days_since_spawn) || !r.u8(dir) || dir > 3 ||
            !r.u32(b.roaming_walker) || !r.u32(services)) {
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
        c.buildings.items.emplace(b.id, std::move(b));
    }
    update_roads(c);  // derived data: networks and road access
    return true;
}

// --- walkers --------------------------------------------------------------------------------------------
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
        w.u32(static_cast<std::uint32_t>(k.path.size()));
        for (const TilePos p : k.path) {
            write_pos(w, p);
        }
        w.u32(k.path_pos);
        w.u32(k.roam_ticks);
        w.u32(k.max_roam_ticks);
        write_pos(w, k.roam_target);
        w.u8(static_cast<std::uint8_t>(k.turn));
        w.i32(k.ticks_until_next_turn);
        w.u32(k.random_counter);
    }
}

bool read_walkers(ByteReader& r, WalkerStore& s) {
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
        std::uint32_t path_len = 0;
        bool ok = r.u32(k.id) && r.str(k.type) && r.u8(kind) && kind <= 2 && r.u32(k.home) && r.u8(phase) &&
                  phase <= 4 && read_pos(r, k.tile) && r.u8(dir) && dir <= 3 && r.u8(came) && r.i64(k.speed_raw) &&
                  r.i64(k.step_accum_raw) && r.u32(path_len) && path_len <= 1U << 20;
        for (std::uint32_t p = 0; ok && p < path_len; ++p) {
            TilePos t;
            ok = read_pos(r, t);
            k.path.push_back(t);
        }
        ok = ok && r.u32(k.path_pos) && r.u32(k.roam_ticks) && r.u32(k.max_roam_ticks) &&
             read_pos(r, k.roam_target) && r.u8(turn) && r.i32(k.ticks_until_next_turn) && r.u32(k.random_counter);
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
        s.items.emplace(k.id, std::move(k));
    }
    return true;
}

}  // namespace

std::vector<std::uint8_t> serialize_state(const WorldState& s, std::uint64_t data_version) {
    ByteWriter w;
    w.u32(kSaveMagic);
    w.u32(kSaveFormatVersion);
    w.u64(data_version);
    w.u64(s.time.tick());
    w.u64(s.rng.seed());
    w.u32(static_cast<std::uint32_t>(RngSet::size()));
    for (std::size_t i = 0; i < RngSet::size(); ++i) {
        const auto& p = s.rng.stream(static_cast<RngStream>(i));
        w.u64(p.state());
        w.u64(p.increment());
    }
    w.u8(static_cast<std::uint8_t>(s.difficulty));
    w.i64(s.finance.treasury);
    w.u8(static_cast<std::uint8_t>(s.finance.tax_rate));
    w.u8(static_cast<std::uint8_t>(s.finance.wage_rate));
    w.u64(s.next_command_sequence);
    w.u64(s.pending_commands.size());
    for (const auto& sc : s.pending_commands) {
        w.u64(sc.tick);
        w.u64(sc.sequence);
        write_command(w, sc.command);
    }
    write_city(w, s.city);       // since v2
    write_walkers(w, s.walkers);  // since v2
    return w.bytes();
}

std::optional<LoadedState> deserialize_state(const std::vector<std::uint8_t>& bytes, std::string& error) {
    ByteReader r(bytes);
    LoadedState out;
    std::uint32_t magic = 0;
    if (!r.u32(magic) || magic != kSaveMagic) {
        error = "save.bad_magic";
        return std::nullopt;
    }
    if (!r.u32(out.format_version) || out.format_version == 0 || out.format_version > kSaveFormatVersion) {
        error = "save.unsupported_version";
        return std::nullopt;
    }
    std::uint64_t tick = 0;
    std::uint64_t seed = 0;
    std::uint32_t streams = 0;
    bool ok = r.u64(out.data_version) && r.u64(tick) && r.u64(seed) && r.u32(streams);
    if (!ok || streams > RngSet::size()) {
        error = "save.truncated";
        return std::nullopt;
    }
    WorldState& s = out.state;
    s.time = GameTime(tick);
    s.rng.reseed(seed);
    for (std::uint32_t i = 0; i < streams && ok; ++i) {
        std::uint64_t st = 0;
        std::uint64_t inc = 0;
        ok = r.u64(st) && r.u64(inc);
        s.rng.stream(static_cast<RngStream>(i)).restore(st, inc);
    }
    std::uint8_t diff = 0;
    std::uint8_t tax = 0;
    std::uint8_t wage = 0;
    std::uint64_t pending = 0;
    ok = ok && r.u8(diff) && r.i64(s.finance.treasury) && r.u8(tax) && r.u8(wage) && r.u64(s.next_command_sequence) &&
         r.u64(pending);
    if (!ok || diff >= static_cast<std::uint8_t>(Difficulty::Count) || tax >= static_cast<std::uint8_t>(TaxRate::Count) ||
        wage >= static_cast<std::uint8_t>(WageRate::Count)) {
        error = "save.corrupt";
        return std::nullopt;
    }
    s.difficulty = static_cast<Difficulty>(diff);
    s.finance.tax_rate = static_cast<TaxRate>(tax);
    s.finance.wage_rate = static_cast<WageRate>(wage);
    for (std::uint64_t i = 0; i < pending; ++i) {
        ScheduledCommand sc;
        if (!r.u64(sc.tick) || !r.u64(sc.sequence) || !read_command(r, sc.command)) {
            error = "save.corrupt";
            return std::nullopt;
        }
        s.pending_commands.push_back(sc);
    }
    // Migration v1 -> v2: v1 saves have no city and no walkers (empty map).
    if (out.format_version >= 2 && (!read_city(r, s.city) || !read_walkers(r, s.walkers))) {
        error = "save.corrupt";
        return std::nullopt;
    }
    if (!r.at_end()) {
        error = "save.trailing_bytes";
        return std::nullopt;
    }
    return out;
}

}  // namespace bo::core
