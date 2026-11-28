// Stable 64-bit hash for the golden hash and the data version.
// Pattern: FNV-1a 64 (Fowler, Noll, Vo). Byte order is fixed by the serializer, so the value is platform independent.
#pragma once

#include <cstdint>
#include <string_view>

namespace bo::core {

class Fnv1a64 {
public:
    void add_byte(std::uint8_t b) {
        value_ ^= b;
        value_ *= 0x100000001b3ULL;
    }
    void add_bytes(const std::uint8_t* data, std::size_t size) {
        for (std::size_t i = 0; i < size; ++i) {
            add_byte(data[i]);
        }
    }
    void add_string(std::string_view s) {
        add_u64(s.size());
        for (const char c : s) {
            add_byte(static_cast<std::uint8_t>(c));
        }
    }
    void add_u64(std::uint64_t v) {
        for (int i = 0; i < 8; ++i) {
            add_byte(static_cast<std::uint8_t>(v >> (8 * i)));
        }
    }
    [[nodiscard]] std::uint64_t value() const { return value_; }

private:
    std::uint64_t value_ = 0xcbf29ce484222325ULL;
};

}  // namespace bo::core
