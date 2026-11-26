// Module A5: save format. Explicit little-endian byte layout (no memcpy of structs), so saves and the golden
// hash are identical on every platform. Pattern: versioned binary format with migrations (docs/PRAKTYKI.md ch. 3).
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "bo/core/state.h"

namespace bo::core {

inline constexpr std::uint32_t kSaveMagic = 0x564F5342;  // "BSOV" little-endian bytes: B S O V
// v2: city map, buildings, walkers. v3: rotation, building tiles, agora slots, undo record, Undo command.
// v4: house state, homeless count. v5: labour, finance details, building workers, clerk counter.
// v6: house mood, migration state. v7: storage, producers, stalls, walker jobs and cargo.
// v8: terrain height and ramps per tile. v9: herds, orchards, processors, storage deliverymen.
// v10: extraction trips, Get orders (walker fields). v11: map features (spawn points, fishing grounds), herds.
// v12: risk counters, fires, quarantine, wind, walker risk resets and outlaw variants.
// v13: city hygiene, crime escalation per house, walkers ignoring roadblocks.
// v14: trade partners, trade buildings and orders. v15: immigrants on the way (house reservations).
inline constexpr std::uint32_t kSaveFormatVersion = 36;

class ByteWriter {
public:
    void u8(std::uint8_t v) { bytes_.push_back(v); }
    void u32(std::uint32_t v) {
        for (int i = 0; i < 4; ++i) {
            u8(static_cast<std::uint8_t>(v >> (8 * i)));
        }
    }
    void u64(std::uint64_t v) {
        for (int i = 0; i < 8; ++i) {
            u8(static_cast<std::uint8_t>(v >> (8 * i)));
        }
    }
    void i64(std::int64_t v) { u64(static_cast<std::uint64_t>(v)); }
    void i32(std::int32_t v) { u32(static_cast<std::uint32_t>(v)); }
    void str(const std::string& s) {
        u32(static_cast<std::uint32_t>(s.size()));
        for (const char c : s) {
            u8(static_cast<std::uint8_t>(c));
        }
    }
    [[nodiscard]] const std::vector<std::uint8_t>& bytes() const { return bytes_; }

private:
    std::vector<std::uint8_t> bytes_;
};

class ByteReader {
public:
    explicit ByteReader(const std::vector<std::uint8_t>& bytes) : bytes_(bytes) {}
    bool u8(std::uint8_t& v) {
        if (pos_ >= bytes_.size()) {
            return false;
        }
        v = bytes_[pos_++];
        return true;
    }
    bool u32(std::uint32_t& v);
    bool u64(std::uint64_t& v);
    bool i64(std::int64_t& v);
    bool i32(std::int32_t& v);
    bool str(std::string& s);
    [[nodiscard]] bool at_end() const { return pos_ == bytes_.size(); }

private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t pos_ = 0;
};

// Canonical serialization of the state; also the input of the state hash.
std::vector<std::uint8_t> serialize_state(const WorldState& state, std::uint64_t data_version);

struct LoadedState {
    WorldState state;
    std::uint64_t data_version = 0;
    std::uint32_t format_version = 0;
};

// Returns nullopt with `error` filled when the bytes are not a valid save. Older format versions are migrated.
std::optional<LoadedState> deserialize_state(const std::vector<std::uint8_t>& bytes, std::string& error);

}  // namespace bo::core
