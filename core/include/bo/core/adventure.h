// Module D5: adventures of several episodes (docs/gameplay/23; S-004 scendesign, S-005; assumptions L-119).
// An adventure starts in the parent city and ends with victory there; parent episodes continue the same city
// with new goals and events; after an episode marked "colony" the player picks one of the colonies not yet
// founded, plays it on its own map, and returns to the parent city.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "bo/core/scenario.h"
#include "bo/core/simulation.h"

namespace bo::core {

struct AdventureStep {
    std::string kind;                      // parent, colony, victory
    ScenarioState episode;                 // parent: the goals and events of this episode
    std::vector<SimulationConfig> colonies;  // colony: the choices, each with `scenario.id` = the colony city id
};

struct AdventureData {
    std::string id;
    SimulationConfig parent;  // map, cities, treasury of the parent city
    std::vector<AdventureStep> steps;
};

// Where the player is (kept by the shell next to the save of the city being played).
struct AdventureRun {
    std::size_t step = 0;
    bool in_colony = false;
    std::vector<std::string> founded;          // colonies played
    std::vector<std::uint8_t> parent_save;     // the parent city while a colony is played
};

// Parses an adventure file: an episode file of parse_scenario, plus "steps" (scenarios/README.md).
std::optional<AdventureData> parse_adventure(const std::string& json_text, std::string& error);

// The first parent episode.
Simulation begin_adventure(const DataCatalog& catalog, const AdventureData& adventure, AdventureRun& run);

// After the current episode is won: the next parent episode goes on in the same simulation; before a colony the
// shell asks the player to choose (colony_choices); after a colony the parent city comes back from its save.
enum class NextEpisode : std::uint8_t { ParentEpisode = 0, ChooseColony, BackToParent, Victory };
NextEpisode advance_adventure(const AdventureData& adventure, AdventureRun& run, std::optional<Simulation>& sim);
// Colonies of the current step not founded yet (ids).
std::vector<std::string> colony_choices(const AdventureData& adventure, const AdventureRun& run);
// Starts the chosen colony; the parent city is kept in run.parent_save. Empty string: success.
std::string found_colony(const DataCatalog& catalog, const AdventureData& adventure, AdventureRun& run,
                         const std::string& colony, std::optional<Simulation>& sim);

}  // namespace bo::core
