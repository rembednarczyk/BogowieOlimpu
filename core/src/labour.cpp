#include "bo/core/labour.h"

#include <algorithm>
#include <map>
#include <optional>

#include "bo/core/path.h"
#include <vector>

namespace bo::core {
namespace {

// Splits `total` over `needs` proportionally; the remainder goes one by one to the largest fractional parts
// (ties: lower index). Never gives more than a need.
std::vector<std::uint32_t> split(std::uint32_t total, const std::vector<std::uint32_t>& needs) {
    std::uint64_t sum = 0;
    for (const auto n : needs) {
        sum += n;
    }
    std::vector<std::uint32_t> out(needs.size(), 0);
    if (sum == 0) {
        return out;
    }
    if (total >= sum) {
        return needs;
    }
    std::vector<std::pair<std::uint64_t, std::size_t>> fractions;
    std::uint64_t given = 0;
    for (std::size_t i = 0; i < needs.size(); ++i) {
        const std::uint64_t num = static_cast<std::uint64_t>(total) * needs[i];
        out[i] = static_cast<std::uint32_t>(num / sum);
        given += out[i];
        fractions.emplace_back(num % sum, i);
    }
    std::stable_sort(fractions.begin(), fractions.end(),
                     [](const auto& a, const auto& b) { return a.first > b.first; });
    for (std::size_t k = 0; given < total && k < fractions.size(); ++k) {
        const std::size_t i = fractions[k].second;
        if (out[i] < needs[i]) {
            ++out[i];
            ++given;
        }
    }
    return out;
}

}  // namespace

std::optional<Sector> sector_of_tab(const std::string& tab) {
    static const std::map<std::string, Sector> kTabs = {
        {"husbandry", Sector::Husbandry},   {"industry", Sector::Industry},
        {"distribution", Sector::Distribution}, {"hygiene_safety", Sector::HygieneSafety},
        {"administration", Sector::Administration}, {"culture", Sector::Culture},
        {"science", Sector::Culture},       {"mythology", Sector::Mythology},
        {"military", Sector::Military},
    };
    const auto it = kTabs.find(tab);
    return it == kTabs.end() ? std::nullopt : std::optional<Sector>(it->second);
}

std::uint32_t workers_needed(const Building& b, const DataCatalog& catalog, const LabourState& labour) {
    const auto type = catalog.buildings.find(b.type);
    if (type == catalog.buildings.end() || labour.disabled_types.count(b.type) != 0) {
        return 0;
    }
    if (b.cursed_days > 0) {
        return 0;  // cursed by a god: nobody works there until the curse ends (Z:125-127)
    }
    if (b.sanctuary && !b.sanctuary->complete) {
        return 0;  // a building site: priests and staff come when it is finished [ZAŁOŻENIE, L-111]
    }
    const auto stats = catalog.building_stats.find(type->second.stats_id);
    if (stats == catalog.building_stats.end() || !stats->second.workers || *stats->second.workers <= 0) {
        return 0;
    }
    return static_cast<std::uint32_t>(*stats->second.workers);
}

void allocate_labour(CityState& city, const DataCatalog& catalog, Difficulty d, WageRate wage, LabourState& labour,
                     std::uint32_t absent) {
    std::uint64_t people = 0;
    for (const auto& [id, b] : city.buildings.items) {
        if (b.house && !b.house->elite) {
            people += b.house->population;  // only common houses work (Z:29)
        }
    }
    const auto pct = static_cast<std::uint64_t>(
        catalog.worker_percent[static_cast<std::size_t>(d)][static_cast<std::size_t>(wage)]);
    const auto pool = static_cast<std::uint32_t>(people * pct / 100);
    labour.workforce = pool - std::min(pool, absent);

    // Needs per sector, buildings per sector in id order. A building gets workers when they can walk to it from
    // the map entry (S-005: a walking connection, not necessarily a road); maps without an entry fall back to
    // road access only.
    std::optional<std::vector<std::uint32_t>> from_entry;
    for (std::size_t i = 0; i < city.map.size(); ++i) {
        if (city.map.tiles()[i].feature == static_cast<std::uint8_t>(TileFeature::LandEntry)) {
            from_entry = road_distances(city, {city.map.pos(i)}, PathMode::Walkable);
            break;
        }
    }
    std::array<std::vector<BuildingId>, kSectors> members;
    std::array<std::vector<std::uint32_t>, kSectors> needs;
    labour.sector_need.fill(0);
    for (auto& [id, b] : city.buildings.items) {
        const std::uint32_t need = workers_needed(b, catalog, labour);
        b.workers = 0;
        const auto sector = sector_of_tab(catalog.buildings.at(b.type).tab);
        const bool reachable =
            b.road_access && (!from_entry || (*from_entry)[city.map.index(*b.road_access)] != kUnreachable);
        if (need == 0 || !sector || !reachable) {
            b.staffing_percent = need == 0 && labour.disabled_types.count(b.type) == 0 ? 100 : 0;
            continue;
        }
        const auto s = static_cast<std::size_t>(*sector);
        members[s].push_back(id);
        needs[s].push_back(need);
        labour.sector_need[s] += need;
    }

    std::uint32_t available = labour.full_staffing ? 0xFFFFFFFFU : labour.workforce;
    labour.sector_employed.fill(0);
    for (int p = kPriorityLevels - 1; p >= 0; --p) {
        std::vector<std::size_t> group;
        std::vector<std::uint32_t> group_need;
        for (std::size_t s = 0; s < kSectors; ++s) {
            if (labour.priority[s] == p) {
                group.push_back(s);
                group_need.push_back(labour.sector_need[s]);
            }
        }
        // At equal priority Hygiene & Safety fills first (S-010: "will automatically fill at a much quicker
        // rate than the others"), then the rest of the group splits what is left.
        const auto hs = std::find(group.begin(), group.end(), static_cast<std::size_t>(Sector::HygieneSafety));
        if (hs != group.end()) {
            const auto k = static_cast<std::size_t>(hs - group.begin());
            const std::uint32_t n = std::min(available, group_need[k]);
            labour.sector_employed[group[k]] = n;
            available -= n;
            group.erase(group.begin() + static_cast<std::ptrdiff_t>(k));
            group_need.erase(group_need.begin() + static_cast<std::ptrdiff_t>(k));
        }
        const auto given = split(available, group_need);
        for (std::size_t k = 0; k < group.size(); ++k) {
            labour.sector_employed[group[k]] = given[k];
            available -= given[k];
        }
    }
    labour.employed = 0;
    for (std::size_t s = 0; s < kSectors; ++s) {
        const auto per_building = split(labour.sector_employed[s], needs[s]);
        for (std::size_t k = 0; k < members[s].size(); ++k) {
            Building& b = city.buildings.items.at(members[s][k]);
            b.workers = per_building[k];
            b.staffing_percent = static_cast<std::uint8_t>(per_building[k] * 100 / needs[s][k]);
        }
        labour.employed += labour.full_staffing ? 0 : labour.sector_employed[s];
    }
}

}  // namespace bo::core
