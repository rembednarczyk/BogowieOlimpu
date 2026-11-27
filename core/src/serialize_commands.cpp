// Commands added since v39, read by index (serialize.cpp read_command).
#include "serialize_city.h"
#include "bo/core/command.h"

namespace bo::core {

// A trireme order (index 30, since v39, Z:142).
bool read_ship_order(ByteReader& r, Command& out) {
    SetShipOrder c;
    if (!r.u32(c.ship) || !r.u8(c.order) || !r.i32(c.x) || !r.i32(c.y)) {
        return false;
    }
    out = c;
    return true;
}

// Consult Oracle (index 32, since v41, Z:124).
bool read_oracle(ByteReader& r, Command& out) {
    ConsultOracle c;
    if (!r.u32(c.building)) {
        return false;
    }
    out = c;
    return true;
}

// A request for military help (index 31, since v40, Z:174).
bool read_help_request(ByteReader& r, Command& out) {
    RequestMilitaryHelp c;
    std::uint8_t strike = 0;
    if (!r.u32(c.city) || !r.u8(strike) || !r.u32(c.target)) {
        return false;
    }
    c.strike = strike != 0;
    out = c;
    return true;
}

// Auto-Defend (index 33, since v48, Z:145).
bool read_auto_defend(ByteReader& r, Command& out) {
    std::uint8_t on = 0;
    if (!r.u8(on)) {
        return false;
    }
    out = SetAutoDefend{on != 0};
    return true;
}

bool read_late_command(std::uint32_t index, ByteReader& r, Command& out) {
    return (index == 30 && read_ship_order(r, out)) || (index == 31 && read_help_request(r, out)) ||
           (index == 32 && read_oracle(r, out)) || (index == 33 && read_auto_defend(r, out));
}

}  // namespace bo::core
