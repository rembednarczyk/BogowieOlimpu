// Save block of the scenario (D5, since v22): goals, events with the year they last fired, open requests.
#include "serialize_city.h"

namespace bo::core {

void write_scenario(ByteWriter& w, const ScenarioState& s) {
    w.str(s.id);
    w.u32(static_cast<std::uint32_t>(s.goals.size()));
    for (const ScenarioGoal& g : s.goals) {
        w.str(g.type);
        w.str(g.target);
        w.i64(g.value);
        w.u8(g.met ? 1 : 0);
    }
    w.u32(static_cast<std::uint32_t>(s.events.size()));
    for (const ScenarioEvent& e : s.events) {
        w.str(e.type);
        w.u32(e.year);
        w.u32(e.month);
        w.u8(e.repeat ? 1 : 0);
        w.i32(e.city);
        w.str(e.good);
        w.str(e.nation);
        w.str(e.relation);
        w.u32(e.amount);
        w.u32(e.deadline_months);
        w.i32(e.last_year);
        w.str(e.god);  // since v23
        w.i32(e.x);
        w.i32(e.y);
    }
    w.u32(static_cast<std::uint32_t>(s.requests.size()));
    for (const CityRequest& r : s.requests) {
        w.u32(r.city);
        w.str(r.good);
        w.u32(r.amount);
        w.u64(r.due_day);
    }
    w.u8(s.won ? 1 : 0);
}

bool read_scenario(ByteReader& r, ScenarioState& s, std::uint32_t version) {
    std::uint32_t n = 0;
    if (!r.str(s.id) || !r.u32(n) || n > 4096) {
        return false;
    }
    s.goals.resize(n);
    for (ScenarioGoal& g : s.goals) {
        std::uint8_t met = 0;
        if (!r.str(g.type) || !r.str(g.target) || !r.i64(g.value) || !r.u8(met)) {
            return false;
        }
        g.met = met != 0;
    }
    if (!r.u32(n) || n > 65536) {
        return false;
    }
    s.events.resize(n);
    for (ScenarioEvent& e : s.events) {
        std::uint8_t repeat = 0;
        if (!r.str(e.type) || !r.u32(e.year) || !r.u32(e.month) || e.month > 11 || !r.u8(repeat) || !r.i32(e.city) ||
            !r.str(e.good) || !r.str(e.nation) || !r.str(e.relation) || !r.u32(e.amount) ||
            !r.u32(e.deadline_months) || !r.i32(e.last_year)) {
            return false;
        }
        e.repeat = repeat != 0;
        if (version >= 23 && (!r.str(e.god) || !r.i32(e.x) || !r.i32(e.y))) {
            return false;
        }
    }
    if (!r.u32(n) || n > 4096) {
        return false;
    }
    s.requests.resize(n);
    for (CityRequest& q : s.requests) {
        if (!r.u32(q.city) || !r.str(q.good) || !r.u32(q.amount) || !r.u64(q.due_day)) {
            return false;
        }
    }
    std::uint8_t won = 0;
    if (!r.u8(won)) {
        return false;
    }
    s.won = won != 0;
    return true;
}

// Monsters and heroes (D5 stage 3, since v24).
void write_monsters(ByteWriter& w, const WorldState& s) {
    w.u32(static_cast<std::uint32_t>(s.monsters.size()));
    for (const MonsterState& m : s.monsters) {
        w.str(m.id);
        w.u32(m.walker);
        w.i32(m.home.x);
        w.i32(m.home.y);
        w.str(m.target_kind);
        w.u32(m.target);
        w.u32(m.days);
        w.u8(static_cast<std::uint8_t>((m.resting ? 1U : 0U) | (m.slain ? 2U : 0U)));
    }
    w.u32(static_cast<std::uint32_t>(s.heroes.size()));
    for (const HeroState& h : s.heroes) {
        w.str(h.id);
        w.u32(h.hall);
        w.u32(h.walker);
    }
}

bool read_monsters(ByteReader& r, WorldState& s) {
    std::uint32_t n = 0;
    if (!r.u32(n) || n > 256) {
        return false;
    }
    s.monsters.resize(n);
    for (MonsterState& m : s.monsters) {
        std::uint32_t days = 0;
        std::uint8_t flags = 0;
        if (!r.str(m.id) || !r.u32(m.walker) || !r.i32(m.home.x) || !r.i32(m.home.y) || !r.str(m.target_kind) ||
            !r.u32(m.target) || !r.u32(days) || days > 0xFFFF || !r.u8(flags)) {
            return false;
        }
        m.days = static_cast<std::uint16_t>(days);
        m.resting = (flags & 1U) != 0;
        m.slain = (flags & 2U) != 0;
    }
    if (!r.u32(n) || n > 64) {
        return false;
    }
    s.heroes.resize(n);
    for (HeroState& h : s.heroes) {
        if (!r.str(h.id) || !r.u32(h.hall) || !r.u32(h.walker)) {
            return false;
        }
    }
    return true;
}

}  // namespace bo::core
