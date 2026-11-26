// Module D5 stage 1 (docs/gameplay/13, 23; Z:161-164; assumptions L-093b, L-114): our adventure format,
// dated events, city requests with deadlines, episode goals.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "bo/core/data.h"

namespace bo::core {

struct WorldState;
struct SimulationConfig;

// A goal of the episode (doc 23): population, treasury, sanctuary (target = god), houses (target = level id,
// value = count at that level or higher), vassal (target = city id).
struct ScenarioGoal {
    std::string type;
    std::string target;
    std::int64_t value = 0;
    bool met = false;
    friend bool operator==(const ScenarioGoal& a, const ScenarioGoal& b) {
        return a.type == b.type && a.target == b.target && a.value == b.value && a.met == b.met;
    }
};

// A dated event (doc 23): invasion, request, gift, relation, god_attack (`god`), disaster (kind in `nation`:
// earthquake, tidal_wave, landslide, lava; centre x, y; radius in `amount`). Year counts from 0 (the first year), month 0-11.
// `repeat`: every year in that month from `year` on (invasions, requests and gifts; doc 23).
struct ScenarioEvent {
    std::string type;
    std::uint32_t year = 0;
    std::uint32_t month = 0;
    bool repeat = false;
    std::int32_t city = -1;
    std::string good;      // request, gift ("drachmas": money)
    std::string nation;    // invasion
    std::string relation;  // relation
    std::uint32_t amount = 0;  // loads, drachmas, soldiers
    std::uint32_t deadline_months = 0;  // request
    std::int32_t last_year = -1;        // year it last fired
    std::string god;                    // god_attack (since v23)
    std::int32_t x = 0;                 // disaster: centre and radius (amount), kind in `nation` (since v23)
    std::int32_t y = 0;
    friend bool operator==(const ScenarioEvent& a, const ScenarioEvent& b) {
        return a.type == b.type && a.year == b.year && a.month == b.month && a.repeat == b.repeat && a.city == b.city &&
               a.good == b.good && a.nation == b.nation && a.relation == b.relation && a.amount == b.amount &&
               a.deadline_months == b.deadline_months && a.last_year == b.last_year && a.god == b.god && a.x == b.x &&
               a.y == b.y;
    }
};

// A request from a city (Z:161-164): due by a day; after it "overdue", later withdrawn with an opinion loss.
struct CityRequest {
    std::uint32_t city = 0;
    std::string good;
    std::uint32_t amount = 0;
    std::uint64_t due_day = 0;
    friend bool operator==(const CityRequest& a, const CityRequest& b) {
        return a.city == b.city && a.good == b.good && a.amount == b.amount && a.due_day == b.due_day;
    }
};

// A god's quest (Z:124, P:24, doc 21): only a hero carries it out, at an enchanted place; no deadline.
enum class QuestStatus : std::uint8_t { Open = 0, Underway, Done };

struct Quest {
    std::string god;
    std::uint32_t index = 0;   // data/quests.json
    std::string hero;          // the scenario's choice, else the traditional hero
    QuestStatus status = QuestStatus::Open;
    std::uint32_t days_left = 0;
    friend bool operator==(const Quest& a, const Quest& b) {
        return a.god == b.god && a.index == b.index && a.hero == b.hero && a.status == b.status && a.days_left == b.days_left;
    }
};

struct ScenarioState {
    std::string id;
    std::vector<ScenarioGoal> goals;
    std::vector<ScenarioEvent> events;
    std::vector<CityRequest> requests;
    bool won = false;
    std::vector<Quest> quests;                         // since v26
    std::map<std::string, std::uint32_t> monuments;    // earned and not yet built: god_statue, hero, conquest
    friend bool operator==(const ScenarioState& a, const ScenarioState& b) {
        return a.id == b.id && a.goals == b.goals && a.events == b.events && a.requests == b.requests && a.won == b.won &&
               a.quests == b.quests && a.monuments == b.monuments;
    }
};

// Parses an adventure file (JSON, docs/gameplay/23 and data/scenarios/README in our format) into a config:
// map, heights, treasury, difficulty, partner cities with diplomacy, goals and events. Empty error = success.
std::optional<SimulationConfig> parse_scenario(const std::string& json_text, std::string& error);

struct ScenarioEvents {
    std::vector<std::size_t> new_requests;  // indices into ScenarioState::requests
    std::vector<std::pair<std::uint32_t, bool>> closed;  // city, fulfilled (false: withdrawn)
    std::vector<std::pair<std::string, std::uint32_t>> invasions;  // nation, soldiers (city in `invasion_cities`)
    std::vector<std::int32_t> invasion_cities;
    std::vector<const ScenarioEvent*> god_attacks;  // fired this day (pointers into ScenarioState::events)
    std::vector<const ScenarioEvent*> disasters;
    std::vector<const ScenarioEvent*> monsters;  // monster id in `nation`, strength in `amount`, target in `good`
    std::vector<std::string> quests_given;       // god
    std::vector<std::string> quests_done;        // god
    bool won = false;
};

// Daily: events of the month fire on its first day; requests turn overdue and are withdrawn 96 days after the
// deadline (-15 opinion, doc 24) [ZAŁOŻENIE, L-114]; goals are checked at the start of each month.
ScenarioEvents advance_scenario(WorldState& state, const DataCatalog& catalog);
// Sends what a request asks for (Z:162): on time +10 opinion (doc 24), late +5 [ZAŁOŻENIE, L-114].
std::string fulfill_request(WorldState& state, const DataCatalog& catalog, std::uint32_t index);
// Sends the hero of an open quest from his Hall to the enchanted place (Z:124); he comes back with the quest
// done after quest_days [ZAŁOŻENIE, L-118] and the god grants a statue (Z:156).
std::string send_hero_on_quest(WorldState& state, const DataCatalog& catalog, std::uint32_t quest);
inline constexpr std::uint32_t kQuestDays = 200;  // there and back [ZAŁOŻENIE, L-118: eZeus march, doc 24]

inline constexpr std::uint32_t kWithdrawAfterDays = 96;

}  // namespace bo::core
