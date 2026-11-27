// Module D3 stage 1: companies, standards, muster and send home (docs/gameplay/11, 22; Z:135-140; L-109).
#include "bo/core/military.h"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <tuple>

#include "bo/core/gods.h"
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

}  // namespace

TilePos company_slot(const CityState& city, TilePos standard, std::size_t k) { return slot(city, standard, k); }

namespace {

std::optional<TilePos> land_entry(const CityState& city) {
    for (std::size_t i = 0; i < city.map.size(); ++i) {
        if (city.map.tiles()[i].feature == static_cast<std::uint8_t>(TileFeature::LandEntry)) {
            return city.map.pos(i);
        }
    }
    return std::nullopt;
}

}  // namespace

namespace {

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
    if (m.invasion.active) {
        return;  // no new companies and no new soldiers during a battle (Z:140)
    }
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
        // The gods' companies (Z:121, Z:140): two of Sparti from Ares's sanctuary, two of Amazons from Artemis's;
        // they leave from the sanctuary, full strength every time, and come first within the 20 (Z:136)
        // [ZAŁOŻENIE, L-109].
        std::vector<Company> divine_companies;
        for (const auto& [god, nation, unit] : {std::tuple{"ares", "divine", "ares_warriors"},
                                                 std::tuple{"artemis", "amazon", "warrior"}}) {
            for (const auto& [id, b] : st.city.buildings.items) {
                const auto sp = catalog.sanctuaries.find(b.type);
                if (!b.sanctuary || sp == catalog.sanctuaries.end() || sp->second.god != god ||
                    !god_blesses(st.city, catalog, god)) {
                    continue;
                }
                for (int k = 0; k < 2; ++k) {
                    Company divine;
                    divine.unit = unit;
                    divine.nation = nation;
                    divine.members.push_back(CompanyMember{id, static_cast<std::uint8_t>(kCompanySize)});
                    divine_companies.push_back(divine);
                }
            }
        }
        formed.insert(formed.begin(), divine_companies.begin(), divine_companies.end());
        formed.resize(std::min(formed.size(), kMaxCompanies));
        const auto sites = standard_sites(st.city, st.city.buildings.items.at(*palace), formed.size());
        for (std::size_t i = 0; i < formed.size(); ++i) {
            const bool kept = i < m.companies.size() && free_land(st.city, m.companies[i].standard);
            formed[i].standard = kept ? m.companies[i].standard : (i < sites.size() ? sites[i] : st.city.buildings.items.at(*palace).origin);
            formed[i].morale = i < m.companies.size() ? m.companies[i].morale : 100;
            // The player's order stays with the company (Z:138); a new one takes the order of its kind (B-017).
            const auto same = [&f = formed[i]](const Company& o) { return o.unit == f.unit && o.nation == f.nation; };
            const auto old = i < m.companies.size() && same(m.companies[i])
                                 ? m.companies.begin() + static_cast<std::ptrdiff_t>(i)
                                 : std::find_if(m.companies.begin(), m.companies.end(), same);
            formed[i].tactic = old != m.companies.end() ? old->tactic : Tactic::Defensive;
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
        if (c.status == CompanyStatus::Fighting && !st.military.invasion.active) {
            c.status = CompanyStatus::Marching;
        }
        if ((c.status == CompanyStatus::GoingHome || c.status == CompanyStatus::Fleeing) && s.empty()) {
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
    if (c.fled && st.military.invasion.active) {
        return "military.company_fled";  // not again in this battle (Z:140)
    }
    const auto type = catalog.walkers.find(c.unit);
    const auto stats = catalog.combat.nations.find(c.nation.empty() ? "player_greek" : c.nation);
    if ((type == catalog.walkers.end() && c.nation.empty()) || stats == catalog.combat.nations.end() ||
        stats->second.units.count(c.unit) == 0) {
        return "military.bad_company";
    }
    const UnitStats& unit = stats->second.units.at(c.unit);
    std::vector<std::tuple<BuildingId, TilePos, std::uint8_t>> sources;  // home, where they set out, how many
    for (const CompanyMember& member : c.members) {
        const auto house = st.city.buildings.items.find(member.house);
        if (house != st.city.buildings.items.end()) {
            sources.emplace_back(member.house, house->second.road_access.value_or(house->second.origin), member.soldiers);
        }
    }
    const auto entry = land_entry(st.city);
    if (c.lender >= 0 && entry) {
        sources.emplace_back(kNoBuilding, *entry, static_cast<std::uint8_t>(kCompanySize));  // lent: from abroad
    }
    std::size_t k = 0;
    for (const auto& [home, from, count] : sources) {
        for (std::uint8_t i = 0; i < count; ++i, ++k) {
            Walker w;
            w.id = st.walkers.next_id;
            w.type = c.unit;
            w.kind = WalkerKind::Destination;
            w.home = home;
            w.tile = from;
            w.speed_raw = c.nation.empty() ? type->second.speed_raw : unit.speed_raw;
            w.variant = c.nation;  // a god's soldier: stats of that nation (combat.cpp stats_of)
            w.job = WalkerJob::Soldier;
            w.target = index;
            w.want_loads = static_cast<std::uint32_t>(k);  // place in the formation
            w.hp = unit.hp;
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

std::string lend_companies(WorldState& st, const DataCatalog& catalog, std::int32_t city, std::uint32_t count,
                           std::vector<WalkerId>& spawned) {
    const auto palace = palace_of(st.city);
    const auto nation = catalog.combat.nations.find(catalog.world.rival_attack_nation);
    if (!palace || !land_entry(st.city) || nation == catalog.combat.nations.end() ||
        nation->second.composition_units.empty()) {
        return "military.no_way_for_help";
    }
    auto& companies = st.military.companies;
    // Lent companies are not the city's: outside its 20 (Z:136), at most 20 of them [ZAŁOŻENIE, L-125].
    const auto already = static_cast<std::size_t>(
        std::count_if(companies.begin(), companies.end(), [](const Company& c) { return c.lender >= 0; }));
    count = std::min<std::uint32_t>(count, static_cast<std::uint32_t>(kMaxCompanies - std::min(kMaxCompanies, already)));
    if (count == 0) {
        return "military.too_many_companies";
    }
    const auto sites = standard_sites(st.city, st.city.buildings.items.at(*palace), companies.size() + count);
    for (std::uint32_t i = 0; i < count; ++i) {
        Company c;
        c.unit = nation->second.composition_units.front();  // heavy infantry first (doc 22)
        c.nation = nation->first;  // stats of that army; its morale never drops (Z:140)
        c.lender = city;
        const std::size_t at = companies.size();
        c.standard = at < sites.size() ? sites[at] : st.city.buildings.items.at(*palace).origin;
        companies.push_back(c);
        muster_company(st, catalog, static_cast<std::uint32_t>(at), spawned);
    }
    return {};
}

void release_borrowed(WorldState& st) {
    const auto entry = land_entry(st.city);
    auto& companies = st.military.companies;
    for (const Company& c : companies) {
        for (const WalkerId id : c.lender >= 0 ? c.soldiers : std::vector<WalkerId>{}) {
            const auto it = st.walkers.items.find(id);
            if (it == st.walkers.items.end()) {
                continue;
            }
            Walker& w = it->second;
            w.job = WalkerJob::None;  // removed when they reach the land entry
            if (!entry || !route(st.city, w, *entry)) {
                w.phase = WalkerPhase::Done;
            }
        }
    }
    companies.erase(std::remove_if(companies.begin(), companies.end(), [](const Company& c) { return c.lender >= 0; }),
                    companies.end());
}

std::string move_company(WorldState& st, std::uint32_t index, TilePos to) {
    if (index >= st.military.companies.size() || !st.city.map.in_bounds(to) ||
        !terrain_walkable(st.city.map.at(to).terrain)) {
        return "military.bad_move";
    }
    Company& c = st.military.companies[index];
    c.standard = to;
    if (c.status == CompanyStatus::Marching || c.status == CompanyStatus::Still || c.status == CompanyStatus::Fighting) {
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
         st.military.companies[index].status != CompanyStatus::Still &&
         st.military.companies[index].status != CompanyStatus::Fighting)) {
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

std::uint32_t workers_in_the_field(const WorldState& st, const DataCatalog& catalog) {
    std::uint32_t n = 0;
    for (const Company& c : st.military.companies) {
        if (c.unit == "rabble" && c.status != CompanyStatus::AtHome) {
            n += static_cast<std::uint32_t>(c.soldiers.size());
        }
    }
    n *= static_cast<std::uint32_t>(catalog.combat.rabble_workers_per_soldier);  // 6 each (S-004, doc 22)
    for (const Ship& s : st.military.ships) {
        n += s.hands;  // the crew aboard, home or abroad (Z:141)
    }
    return n;
}

}  // namespace bo::core
