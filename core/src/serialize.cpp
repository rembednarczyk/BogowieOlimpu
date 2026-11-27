#include "bo/core/serialize.h"

#include "serialize_city.h"

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
                w.u8(cmd.rotation);  // since v3
            } else if constexpr (std::is_same_v<T, Undo> || std::is_same_v<T, BribeInvaders> ||
                                 std::is_same_v<T, Surrender>) {
                // no payload
            } else if constexpr (std::is_same_v<T, SetSectorPriority>) {
                w.u8(cmd.sector);
                w.u8(cmd.priority);
            } else if constexpr (std::is_same_v<T, SetIndustryEnabled>) {
                w.str(cmd.type);
                w.u8(cmd.enabled ? 1 : 0);
            } else if constexpr (std::is_same_v<T, SetTradePartner>) {
                w.u32(cmd.building);
                w.i32(cmd.partner);
            } else if constexpr (std::is_same_v<T, SetTradeOrder>) {
                w.u32(cmd.building);
                w.str(cmd.good);
                w.u8(static_cast<std::uint8_t>((cmd.import_on ? 1U : 0U) | (cmd.export_on ? 2U : 0U)));
                w.u32(cmd.import_limit);
                w.u32(cmd.export_keep);
            } else if constexpr (std::is_same_v<T, StartInvasion>) {
                w.str(cmd.nation);
                w.u32(cmd.soldiers);
                w.i32(cmd.city);  // since v21
                w.u8(cmd.by_sea ? 1 : 0);  // since v39
            } else if constexpr (std::is_same_v<T, RequestMilitaryHelp>) {
                w.u32(cmd.city);
                w.u8(cmd.strike ? 1 : 0);
                w.u32(cmd.target);
            } else if constexpr (std::is_same_v<T, SetShipOrder>) {
                w.u32(cmd.ship);
                w.u8(cmd.order);
                w.i32(cmd.x);
                w.i32(cmd.y);
            } else if constexpr (std::is_same_v<T, GiftToCity>) {
                w.u32(cmd.city);
                w.str(cmd.good);
                w.u32(cmd.amount);
            } else if constexpr (std::is_same_v<T, RequestFromCity>) {
                w.u32(cmd.city);
                w.str(cmd.good);
            } else if constexpr (std::is_same_v<T, DispatchArmy>) {
                w.u32(cmd.city);
                w.u8(static_cast<std::uint8_t>((cmd.raid ? 1U : 0U) | (cmd.ships ? 2U : 0U)));  // bit 1: triremes
                w.str(cmd.good);
                w.u32(static_cast<std::uint32_t>(cmd.companies.size()));
                for (const std::uint32_t c : cmd.companies) {
                    w.u32(c);
                }
            } else if constexpr (std::is_same_v<T, Pray> || std::is_same_v<T, ConsultOracle>) {
                w.u32(cmd.building);
            } else if constexpr (std::is_same_v<T, FulfillRequest>) {
                w.u32(cmd.request);
            } else if constexpr (std::is_same_v<T, SummonHero>) {
                w.u32(cmd.hall);
            } else if constexpr (std::is_same_v<T, SendHeroOnQuest>) {
                w.u32(cmd.quest);
            } else if constexpr (std::is_same_v<T, SetStockpile>) {
                w.str(cmd.good);
                w.u8(cmd.on ? 1 : 0);
            } else if constexpr (std::is_same_v<T, SetNotBuying> || std::is_same_v<T, SetConstructionPaused>) {
                w.u32(cmd.building);
                w.u8(cmd.on ? 1 : 0);
            } else if constexpr (std::is_same_v<T, ManTowers> || std::is_same_v<T, SetAutoDefend>) {
                w.u8(cmd.on ? 1 : 0);
            } else if constexpr (std::is_same_v<T, SetCompanyTactic>) {
                w.u32(cmd.company);
                w.u8(cmd.tactic);
            } else if constexpr (std::is_same_v<T, MusterCompany> || std::is_same_v<T, SendCompanyHome>) {
                w.u32(cmd.company);
            } else if constexpr (std::is_same_v<T, MoveCompany>) {
                w.u32(cmd.company);
                w.i32(cmd.x);
                w.i32(cmd.y);
            } else if constexpr (std::is_same_v<T, SetStorageOrder>) {
                w.u32(cmd.building);
                w.str(cmd.good);
                w.u8(cmd.kind);
                w.u32(cmd.limit);
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

bool read_dispatch(ByteReader& r, Command& out) {
    DispatchArmy c;
    std::uint8_t raid = 0;
    std::uint32_t n = 0;
    if (!r.u32(c.city) || !r.u8(raid) || !r.str(c.good) || !r.u32(n) || n > kMaxCompanies) {
        return false;
    }
    c.raid = (raid & 1U) != 0;
    c.ships = (raid & 2U) != 0;
    c.companies.resize(n);
    for (std::uint32_t& id : c.companies) {
        if (!r.u32(id)) {
            return false;
        }
    }
    out = c;
    return true;
}

// Commands of modules D (army, invasions, gods, world), indices 13 and up.
bool read_module_d_command(ByteReader& r, std::uint32_t index, Command& out, std::uint32_t version) {
    switch (index) {
        case 13:
        case 15: {
            std::uint32_t company = 0;
            if (!r.u32(company)) {
                return false;
            }
            out = index == 13 ? Command{MusterCompany{company}} : Command{SendCompanyHome{company}};
            return true;
        }
        case 14: {
            MoveCompany c;
            if (!r.u32(c.company) || !r.i32(c.x) || !r.i32(c.y)) {
                return false;
            }
            out = c;
            return true;
        }
        case 16: {
            StartInvasion c;
            std::uint8_t sea = 0;
            if (!r.str(c.nation) || !r.u32(c.soldiers) || (version >= 21 && !r.i32(c.city)) || (version >= 39 && !r.u8(sea))) {
                return false;
            }
            c.by_sea = sea != 0;
            out = c;
            return true;
        }
        case 17:
            out = BribeInvaders{};
            return true;
        case 18:
            out = Surrender{};
            return true;
        case 19: {
            Pray c;
            if (!r.u32(c.building)) {
                return false;
            }
            out = c;
            return true;
        }
        case 20: {
            GiftToCity c;
            if (!r.u32(c.city) || !r.str(c.good) || !r.u32(c.amount)) {
                return false;
            }
            out = c;
            return true;
        }
        case 21: {
            RequestFromCity c;
            if (!r.u32(c.city) || !r.str(c.good)) {
                return false;
            }
            out = c;
            return true;
        }
        case 22:
            return read_dispatch(r, out);
        case 23:
        case 24:
        case 25: {
            std::uint32_t v = 0;
            if (!r.u32(v)) {
                return false;
            }
            out = index == 23   ? Command{FulfillRequest{v}}
                  : index == 24 ? Command{SummonHero{v}}
                                : Command{SendHeroOnQuest{v}};
            return true;
        }
        case 26: {  // since v35 (Z:80)
            SetStockpile c;
            std::uint8_t on = 0;
            if (!r.str(c.good) || !r.u8(on)) {
                return false;
            }
            c.on = on != 0;
            out = c;
            return true;
        }
        case 29: {  // since v38 (Z:138-139)
            SetCompanyTactic c;
            if (!r.u32(c.company) || !r.u8(c.tactic)) {
                return false;
            }
            out = c;
            return true;
        }
        case 28: {  // since v36 (Z:146)
            std::uint8_t on = 0;
            if (!r.u8(on)) {
                return false;
            }
            out = ManTowers{on != 0};
            return true;
        }
        case 27: {  // since v35 (Z:72)
            SetNotBuying c;
            std::uint8_t on = 0;
            if (!r.u32(c.building) || !r.u8(on)) {
                return false;
            }
            c.on = on != 0;
            out = c;
            return true;
        }
        default:
            return read_late_command(index, r, out);  // serialize_commands.cpp
    }
}

bool read_command(ByteReader& r, Command& out, std::uint32_t version) {
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
            if (!r.str(pb.type) || !r.i32(pb.x) || !r.i32(pb.y) || (version >= 3 && !r.u8(pb.rotation))) {
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
        case 7:
            out = Undo{};
            return true;
        case 8: {
            SetSectorPriority c;
            if (!r.u8(c.sector) || !r.u8(c.priority)) {
                return false;
            }
            out = c;
            return true;
        }
        case 9: {
            SetIndustryEnabled c;
            std::uint8_t on = 0;
            if (!r.str(c.type) || !r.u8(on)) {
                return false;
            }
            c.enabled = on != 0;
            out = c;
            return true;
        }
        case 11: {
            SetTradePartner c;
            if (!r.u32(c.building) || !r.i32(c.partner)) {
                return false;
            }
            out = c;
            return true;
        }
        case 12: {
            SetTradeOrder c;
            std::uint8_t flags = 0;
            if (!r.u32(c.building) || !r.str(c.good) || !r.u8(flags) || !r.u32(c.import_limit) || !r.u32(c.export_keep)) {
                return false;
            }
            c.import_on = (flags & 1U) != 0;
            c.export_on = (flags & 2U) != 0;
            out = c;
            return true;
        }
        case 10: {
            SetStorageOrder c;
            if (!r.u32(c.building) || !r.str(c.good) || !r.u8(c.kind) || !r.u32(c.limit)) {
                return false;
            }
            out = c;
            return true;
        }
        default:
            return read_module_d_command(r, index, out, version);
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
    write_city(w, s.city);        // since v2 (v3: building geometry, undo record)
    write_economy(w, s);          // since v5: labour and finance details
    write_walkers(w, s.walkers);  // since v2 (v3: return_north)
    write_military(w, s.military);  // since v17
    write_world(w, s);              // since v21
    write_scenario(w, s.scenario);  // since v22
    write_monsters(w, s);           // since v24
    write_atlas(w, s);              // since v50: map places, histories (L-135, Z:171)
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
        if (!r.u64(sc.tick) || !r.u64(sc.sequence) || !read_command(r, sc.command, out.format_version)) {
            error = "save.corrupt";
            return std::nullopt;
        }
        s.pending_commands.push_back(sc);
    }
    // Migration v1 -> v2: v1 saves have no city and no walkers (empty map).
    if (out.format_version >= 2 &&
        (!read_city(r, s.city, out.format_version) || (out.format_version >= 5 && !read_economy(r, s, out.format_version)) ||
         !read_walkers(r, s.walkers, out.format_version) ||
         (out.format_version >= 17 && !read_military(r, s.military, out.format_version)) ||
         (out.format_version >= 21 && !read_world(r, s, out.format_version)) ||
         (out.format_version >= 22 && !read_scenario(r, s.scenario, out.format_version)) ||
         (out.format_version >= 24 && !read_monsters(r, s)) || (out.format_version >= 50 && !read_atlas(r, s)))) {
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
