// Goods blocks of a building and walker cargo in the save (v7, extended in v9).
#include "serialize_city.h"

namespace bo::core {

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
    w.u8(static_cast<std::uint8_t>((b.storage ? 1U : 0U) | (b.producer ? 2U : 0U) | (b.stall ? 4U : 0U) |
                                   (b.animal ? 8U : 0U) | (b.orchard ? 16U : 0U)));
    if (b.storage) {
        write_counts(w, b.storage->loads);
        w.u32(static_cast<std::uint32_t>(b.storage->orders.size()));
        for (const auto& [good, o] : b.storage->orders) {
            w.str(good);
            w.u8(static_cast<std::uint8_t>(o.kind));
            w.u32(o.limit);
        }
        w.u32(b.storage->deliveryman);  // since v9
    }
    if (b.producer) {
        w.u32(b.producer->progress);
        w.u32(b.producer->output_loads);
        w.u32(b.producer->deliveryman);
        w.u32(b.producer->input_loads);  // since v9
        write_counts(w, b.producer->units);
        w.u32(static_cast<std::uint32_t>(b.producer->field_workers.size()));
        for (const WalkerId id : b.producer->field_workers) {
            w.u32(id);
        }
    }
    if (b.stall) {
        write_counts(w, b.stall->units);
        w.u32(b.stall->vendor);
    }
    if (b.animal) {  // since v9
        w.u32(b.animal->shed);
        w.u32(b.animal->days_to_ready);
        w.u8(b.animal->worker_coming ? 1 : 0);
    }
    if (b.orchard) {  // since v9
        w.u32(b.orchard->growth);
        w.u32(b.orchard->days_since_tended);
        w.u8(b.orchard->worker_coming ? 1 : 0);
    }
}

bool read_goods_state(ByteReader& r, Building& b, std::uint32_t version) {
    std::uint8_t flags = 0;
    if (!r.u8(flags) || flags > 31) {
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
        if (version >= 9 && !r.u32(s.deliveryman)) {
            return false;
        }
        b.storage = std::move(s);
    }
    if ((flags & 2U) != 0) {
        ProducerState p;
        if (!r.u32(p.progress) || !r.u32(p.output_loads) || !r.u32(p.deliveryman)) {
            return false;
        }
        if (version >= 9) {
            std::uint32_t n = 0;
            if (!r.u32(p.input_loads) || !read_counts(r, p.units) || !r.u32(n) || n > 64) {
                return false;
            }
            p.field_workers.resize(n);
            for (auto& id : p.field_workers) {
                if (!r.u32(id)) {
                    return false;
                }
            }
        }
        b.producer = std::move(p);
    }
    if ((flags & 4U) != 0) {
        StallState s;
        if (!read_counts(r, s.units) || !r.u32(s.vendor)) {
            return false;
        }
        b.stall = std::move(s);
    }
    auto read_flag = [&r](bool& out) {
        std::uint8_t v = 0;
        const bool ok = r.u8(v);
        out = v != 0;
        return ok;
    };
    if ((flags & 8U) != 0) {
        AnimalState a;
        std::uint32_t days = 0;
        if (!r.u32(a.shed) || !r.u32(days) || days > 0xFFFF || !read_flag(a.worker_coming)) {
            return false;
        }
        a.days_to_ready = static_cast<std::uint16_t>(days);
        b.animal = a;
    }
    if ((flags & 16U) != 0) {
        OrchardState o;
        std::uint32_t days = 0;
        if (!r.u32(o.growth) || !r.u32(days) || days > 0xFFFF || !read_flag(o.worker_coming)) {
            return false;
        }
        o.days_since_tended = static_cast<std::uint16_t>(days);
        b.orchard = o;
    }
    return true;
}

}  // namespace bo::core
