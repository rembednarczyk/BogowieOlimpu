#include "bo/core/culture.h"

#include <algorithm>
#include <vector>

namespace bo::core {

namespace {

bool working(const Building& b) { return b.staffing_percent > 0 && b.road_access.has_value() && b.cursed_days == 0; }

// Venues of `type` in id order, starting after `last` and wrapping around.
std::vector<BuildingId> venues_after(const CityState& city, const std::string& type, BuildingId last) {
    std::vector<BuildingId> after;
    std::vector<BuildingId> before;
    for (const auto& [id, b] : city.buildings.items) {
        if (b.type == type && working(b)) {
            (id > last ? after : before).push_back(id);
        }
    }
    after.insert(after.end(), before.begin(), before.end());
    return after;
}

void send_performer(CityState& city, WalkerStore& walkers, const DataCatalog& catalog, BuildingId school,
                    const WalkerType& wt) {
    auto& state = city.buildings.items.at(school).schools[wt.id];
    if (state.first < wt.school_send_days) {
        ++state.first;
        return;
    }
    for (const BuildingId venue : venues_after(city, wt.performs_at, state.second)) {
        const Building& v = city.buildings.items.at(venue);
        const WalkerId w = spawn_destination_walker(city, walkers, catalog, wt.id, school, *v.road_access);
        if (w == kNoWalker) {
            continue;  // no road from the school to this venue (doc 02: then nobody ever comes)
        }
        Walker& walker = walkers.items.at(w);
        walker.job = WalkerJob::Perform;
        walker.target = venue;
        auto& s = city.buildings.items.at(school).schools[wt.id];
        s.first = 0;
        s.second = venue;
        return;
    }
    // No venue to go to: the school waits with a performer ready (Z:108: no Stadium, no competitors).
}

}  // namespace

void run_schools(CityState& city, WalkerStore& walkers, const DataCatalog& catalog) {
    std::vector<BuildingId> schools;
    for (auto& [id, b] : city.buildings.items) {
        if (b.performing_days > 0) {
            --b.performing_days;
        }
        if (working(b)) {
            schools.push_back(id);
        }
    }
    for (const BuildingId id : schools) {
        for (const auto& [wid, wt] : catalog.walkers) {
            if (wt.kind == WalkerKind::TwoPhase && wt.trained_at == city.buildings.items.at(id).type) {
                send_performer(city, walkers, catalog, id, wt);
            }
        }
    }
}

void performer_arrived(CityState& city, const DataCatalog& catalog, Walker& w) {
    const auto venue = city.buildings.items.find(w.target);
    const auto wt = catalog.walkers.find(w.type);
    if (venue != city.buildings.items.end() && wt != catalog.walkers.end() && venue->second.type == wt->second.performs_at) {
        venue->second.performing_days = wt->second.performance_days;
    }
    w.phase = WalkerPhase::Done;
}

bool venue_performs(const Building& b, const WalkerType& wt) {
    return wt.kind == WalkerKind::TwoPhase && wt.performs_at == b.type && b.performing_days > 0;
}

std::int32_t city_culture_bonus(const CityState& city, const DataCatalog& catalog) {
    std::int32_t bonus = 0;
    std::vector<std::string> counted;
    for (const auto& [id, b] : city.buildings.items) {
        const auto t = catalog.buildings.find(b.type);
        if (t == catalog.buildings.end() || t->second.city_culture_bonus == 0 || b.performing_days == 0 || !working(b)) {
            continue;
        }
        if (std::find(counted.begin(), counted.end(), b.type) == counted.end()) {
            counted.push_back(b.type);
            bonus += t->second.city_culture_bonus;
        }
    }
    return bonus;
}

}  // namespace bo::core
