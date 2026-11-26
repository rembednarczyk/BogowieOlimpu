#include "bo/core/serialize.h"

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
    if (!u64(u)) {
        return false;
    }
    v = static_cast<std::int64_t>(u);
    return true;
}

namespace {

void write_command(ByteWriter& w, const Command& c) {
    w.u32(static_cast<std::uint32_t>(c.index()));
    std::visit(
        [&w](const auto& cmd) {
            using T = std::decay_t<decltype(cmd)>;
            if constexpr (std::is_same_v<T, SetTaxRate> || std::is_same_v<T, SetWageRate>) {
                w.u8(static_cast<std::uint8_t>(cmd.rate));
            }
        },
        c);
}

bool read_command(ByteReader& r, Command& out) {
    std::uint32_t index = 0;
    std::uint8_t value = 0;
    if (!r.u32(index) || !r.u8(value)) {
        return false;
    }
    switch (index) {
        case 0:
            if (value >= static_cast<std::uint8_t>(TaxRate::Count)) {
                return false;
            }
            out = SetTaxRate{static_cast<TaxRate>(value)};
            return true;
        case 1:
            if (value >= static_cast<std::uint8_t>(WageRate::Count)) {
                return false;
            }
            out = SetWageRate{static_cast<WageRate>(value)};
            return true;
        default:
            return false;
    }
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
    // Migrations: when kSaveFormatVersion grows, branch on out.format_version here and upgrade old fields.
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
    if (!r.at_end()) {
        error = "save.trailing_bytes";
        return std::nullopt;
    }
    return out;
}

}  // namespace bo::core
