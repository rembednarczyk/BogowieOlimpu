// Module D3 stage 1: companies, standards, muster and send home (docs/gameplay/11, 22; Z:135-140; L-109).
#include "bo/core/military.h"

#include <algorithm>
#include <cstdlib>
#include <optional>

#include "bo/core/housing.h"
#include "bo/core/path.h"
#include "bo/core/state.h"
#include "bo/core/walker.h"

namespace bo::core {
namespace {

std::optional<BuildingId> palace_of(const CityState& city) {
    for (const auto& [id, b] : city.buildings.items) {
        if (b.type == "palace") {
            return id;
        }
    }
    return std::nullopt;
}

bool free_land(const CityState& city, TilePos p) {
    if (!city.map.in_bounds(p)) {
        return false;
    }
    const Tile& t = city.map.at(p);
    return terrain_walkable(t.terrain) && t.building == kNoBuilding && !t.road;
}

// Standards stand on free land in rings around the Palace, at least 3 tiles apart [ZAŁOŻENIE, L-109].
std::vector<TilePos> standard_sites(const CityState& city, const Building& palace, std::size_t count) {
    std::vector<TilePos> out;
    for (std::int32_t r = 2; r < 40 && out.size() < count; ++r) {
        const std::int32_t x0 = palace.origin.x - r;
        const std::int32_t y0 = palace.origin.y - r;
        const std::int32_t x1 = palace.origin.x + palace.width - 1 + r;
        const std::int32_t y1 = palace.origin.y + palace.height - 1 + r;
        for (std::int32_t y = y0; y <= y1 && out.size() < count; ++y) {
            for (std::int32_t x = x0; x <= x1 && out.size() < count; ++x) {
                if (x != x0 && x != x1 && y != y0 && y != y1) {
                    continue;  // ring only
                }
                const TilePos p{x, y};
                const bool spaced = std::none_of(out.begin(), out.end(), [p](TilePos q) {
                    return std::max(std::abs(p.x - q.x), std::abs(p.y - q.y)) < 3;
                });
                if (spaced && free_land(city, p)) {
                    out.push_back(p);
                }
            }
        }
    }
    return out;
}

// Soldier k of a company stands in a 4x2 block at the standard [ZAŁOŻENIE, L-109].
TilePos slot(const CityState& city, TilePos standard, std::size_t k) {
    const TilePos p{standard.x + static_cast<std::int32_t>(k % 4) - 1, standard.y + static_cast<std::int32_t>(k / 4)};
    return city.map.in_bounds(p) && terrain_walkable(city.map.at(p).terrain) ? p : standard;
}

bool route(CityState& city, Walker& w, TilePos to) {
    auto path = find_path(city, {w.tile, to, PathMode::Walkable});
    if (!path) {
        return false;
    }
    w.path = std::move(*path);
    w.path_pos = 0;
    w.phase = WalkerPhase::Routed;
    return true;
}

}  // namespace

std::uint32_t house_soldiers(const DataCatalog& catalog, const HouseState& h, Difficulty d) {
    const auto& levels = levels_of(catalog, h);
    if (h.abandoned || h.level >= levels.size() || levels[h.level].military.empty()) {
        return 0;
    }
    const HousingLevel& lv = levels[h.level];
    const auto pct = static_cast<std::uint32_t>(lv.model[static_cast<std::size_t>(d)].soldiers_percent);
    std::uint32_t n = (h.population * pct + 50) / 100;  // Hovel 24 x 20 % = 5, Estate 20 x 20 % = 4 (doc 22)
    if (lv.military != "rabble") {
        n = std::min(n, h.goods[static_cast<std::size_t>(HouseGood::Armor)]);
    }
    if (lv.military == "horseman") {
        n = std::min(n, h.goods[static_cast<std::size_t>(HouseGood::Horses)]);
    }
    return n;
}

void form_companies(WorldState& st, const DataCatalog& catalog) {
    MilitaryState& m = st.military;
    if (std::any_of(m.companies.begin(), m.companies.end(),
                    [](const Company& c) { return c.status != CompanyStatus::AtHome; })) {
        return;  // companies in the field keep their soldiers (Z:140)
    }
    const auto palace = palace_of(st.city);
    std::vector<Company> formed;
    if (palace) {
        for (const char* unit : {"horseman", "hoplite", "rabble"}) {
            Company current;
            current.unit = unit;
            std::size_t size = 0;
            for (const auto& [id, b] : st.city.buildings.items) {
                if (!b.house || formed.size() == kMaxCompanies) {
                    continue;
                }
                const auto& levels = levels_of(catalog, *b.house);
                if (b.house->level >= levels.size() || levels[b.house->level].military != unit) {
                    continue;
                }
                std::uint32_t left = house_soldiers(catalog, *b.house, st.difficulty);
                while (left > 0 && formed.size() < kMaxCompanies) {
                    const auto n = static_cast<std::uint8_t>(std::min<std::size_t>(left, kCompanySize - size));
                    current.members.push_back(CompanyMember{id, n});
                    size += n;
                    left -= n;
                    if (size == kCompanySize) {
                        formed.push_back(current);
                        current.members.clear();
                        size = 0;
                    }
                }
            }
            if (size > 0 && formed.size() < kMaxCompanies) {
                formed.push_back(current);  // a company that is not yet full [ZAŁOŻENIE, L-109]
            }
        }
        const auto sites = standard_sites(st.city, st.city.buildings.items.at(*palace), formed.size());
        for (std::size_t i = 0; i < formed.size(); ++i) {
            const bool kept = i < m.companies.size() && free_land(st.city, m.companies[i].standard);
            formed[i].standard = kept ? m.companies[i].standard : (i < sites.size() ? sites[i] : st.city.buildings.items.at(*palace).origin);
            formed[i].morale = i < m.companies.size() ? m.companies[i].morale : 100;
        }
    }
    m.companies = std::move(formed);
}

void update_companies(WorldState& st) {
    for (Company& c : st.military.companies) {
        auto& s = c.soldiers;
        s.erase(std::remove_if(s.begin(), s.end(), [&st](WalkerId w) { return st.walkers.items.count(w) == 0; }), s.end());
        if (c.status == CompanyStatus::Marching && !s.empty() &&
            std::all_of(s.begin(), s.end(), [&st](WalkerId w) { return st.walkers.items.at(w).phase == WalkerPhase::AtTarget; })) {
            c.status = CompanyStatus::Still;
        }
        if (c.status == CompanyStatus::GoingHome && s.empty()) {
            c.status = CompanyStatus::AtHome;
        }
    }
}

std::string muster_company(WorldState& st, const DataCatalog& catalog, std::uint32_t index,
                           std::vector<WalkerId>& spawned) {
    if (!palace_of(st.city)) {
        return "military.no_palace";  // Z:135
    }
    if (index >= st.military.companies.size() || st.military.companies[index].status != CompanyStatus::AtHome) {
        return "military.bad_company";
    }
    Company& c = st.military.companies[index];
    const auto type = catalog.walkers.find(c.unit);
    if (type == catalog.walkers.end()) {
        return "military.bad_company";
    }
    std::size_t k = 0;
    for (const CompanyMember& member : c.members) {
        const auto house = st.city.buildings.items.find(member.house);
        if (house == st.city.buildings.items.end()) {
            continue;
        }
        const TilePos from = house->second.road_access ? *house->second.road_access : house->second.origin;
        for (std::uint8_t i = 0; i < member.soldiers; ++i, ++k) {
            Walker w;
            w.id = st.walkers.next_id;
            w.type = c.unit;
            w.kind = WalkerKind::Destination;
            w.home = member.house;
            w.tile = from;
            w.speed_raw = type->second.speed_raw;
            w.job = WalkerJob::Soldier;
            w.target = index;
            w.want_loads = static_cast<std::uint32_t>(k);  // place in the formation
            if (!route(st.city, w, slot(st.city, c.standard, k))) {
                continue;
            }
            ++st.walkers.next_id;
            c.soldiers.push_back(w.id);
            spawned.push_back(w.id);
            st.walkers.items.emplace(w.id, std::move(w));
        }
    }
    if (c.soldiers.empty()) {
        return "military.no_soldiers";
    }
    c.status = CompanyStatus::Marching;
    return {};
}

std::string move_company(WorldState& st, std::uint32_t index, TilePos to) {
    if (index >= st.military.companies.size() || !st.city.map.in_bounds(to) ||
        !terrain_walkable(st.city.map.at(to).terrain)) {
        return "military.bad_move";
    }
    Company& c = st.military.companies[index];
    c.standard = to;
    if (c.status == CompanyStatus::Marching || c.status == CompanyStatus::Still) {
        for (const WalkerId id : c.soldiers) {
            Walker& w = st.walkers.items.at(id);
            route(st.city, w, slot(st.city, to, w.want_loads));
        }
        c.status = CompanyStatus::Marching;
    }
    return {};
}

std::string send_company_home(WorldState& st, std::uint32_t index) {
    if (index >= st.military.companies.size() ||
        (st.military.companies[index].status != CompanyStatus::Marching &&
         st.military.companies[index].status != CompanyStatus::Still)) {
        return "military.bad_company";
    }
    Company& c = st.military.companies[index];
    for (const WalkerId id : c.soldiers) {
        Walker& w = st.walkers.items.at(id);
        w.job = WalkerJob::None;  // a routed walker without a job is removed on arrival
        const auto house = st.city.buildings.items.find(w.home);
        const bool home = house != st.city.buildings.items.end() && house->second.road_access &&
                          route(st.city, w, *house->second.road_access);
        if (!home) {
            w.phase = WalkerPhase::Done;
        }
    }
    c.status = CompanyStatus::GoingHome;
    return {};
}

std::uint32_t soldiers_in_field_from_common_houses(const WorldState& st) {
    std::uint32_t n = 0;
    for (const Company& c : st.military.companies) {
        if (c.unit == "rabble" && c.status != CompanyStatus::AtHome) {
            n += static_cast<std::uint32_t>(c.soldiers.size());
        }
    }
    return n;
}

}  // namespace bo::core
