// Module D3 stage 1 (docs/gameplay/11, 22; Z:135-140): companies formed from houses, standards, muster,
// march and send home. Combat, morale losses and invaders come with stage 2 (assumptions L-109).
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "bo/core/city.h"
#include "bo/core/data.h"
#include "bo/core/map.h"

namespace bo::core {

struct WorldState;

// Stored in saves: append only.
enum class CompanyStatus : std::uint8_t { AtHome = 0, Marching, Still, GoingHome };

struct CompanyMember {
    BuildingId house = kNoBuilding;
    std::uint8_t soldiers = 0;
    friend bool operator==(const CompanyMember& a, const CompanyMember& b) {
        return a.house == b.house && a.soldiers == b.soldiers;
    }
};

struct Company {
    std::string unit;  // rabble, hoplite, horseman (walker type of its soldiers)
    std::vector<CompanyMember> members;
    TilePos standard{0, 0};
    CompanyStatus status = CompanyStatus::AtHome;
    std::uint8_t morale = 100;
    std::vector<WalkerId> soldiers;  // soldiers in the field
    friend bool operator==(const Company& a, const Company& b) {
        return a.unit == b.unit && a.members == b.members && a.standard == b.standard && a.status == b.status &&
               a.morale == b.morale && a.soldiers == b.soldiers;
    }
};

struct MilitaryState {
    std::vector<Company> companies;
    friend bool operator==(const MilitaryState& a, const MilitaryState& b) { return a.companies == b.companies; }
};

// Soldiers a house gives (doc 22): round(population x %S / 100) of the unit of its level; hoplites need one
// armor each, horsemen armor and a horse each (Z:136-137).
std::uint32_t house_soldiers(const DataCatalog& catalog, const HouseState& h, Difficulty d);

// Daily, while every company is at home: companies of up to 8 soldiers (horsemen, hoplites, rabble; houses in
// id order), at most 20, standards around the Palace; no Palace, no army (Z:135-136).
void form_companies(WorldState& state, const DataCatalog& catalog);
// Daily: a marching company whose soldiers all stand at the standard is Still; a company going home whose
// soldiers are all back is At Home.
void update_companies(WorldState& state);

// Commands. Return an error code for CommandRejected, or an empty string.
std::string muster_company(WorldState& state, const DataCatalog& catalog, std::uint32_t company,
                           std::vector<WalkerId>& spawned);
std::string move_company(WorldState& state, std::uint32_t company, TilePos to);
std::string send_company_home(WorldState& state, std::uint32_t company);

// Common-house soldiers in the field per house: they do not work meanwhile [ZAŁOŻENIE, L-109].
std::uint32_t soldiers_in_field_from_common_houses(const WorldState& state);

inline constexpr std::size_t kCompanySize = 8;
inline constexpr std::size_t kMaxCompanies = 20;

}  // namespace bo::core
