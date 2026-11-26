// Module D5: adventures of several episodes (docs/gameplay/23; L-119).
#include "bo/core/adventure.h"

#include <algorithm>

#include "nlohmann/json.hpp"

namespace bo::core {
namespace {

using nlohmann::json;

// Goals and events of a step, parsed with the episode parser (a tiny map keeps it valid).
std::optional<ScenarioState> parse_episode(const json& step, std::string& error) {
    json e = json::object();
    e["map"] = json::array({"."});
    e["goals"] = step.value("goals", json::array());
    e["events"] = step.value("events", json::array());
    auto cfg = parse_scenario(e.dump(), error);
    if (!cfg) {
        return std::nullopt;
    }
    return cfg->scenario;
}

// Next step index of the given kind is not needed: steps run in order.
void start_parent_episode(const AdventureData& adventure, const AdventureRun& run, WorldState& st) {
    const ScenarioState& next = adventure.steps[run.step].episode;
    st.scenario.goals = next.goals;
    st.scenario.events = next.events;
    st.scenario.won = false;  // quests, requests and monuments carry over
    st.scenario.id = adventure.id + "#" + std::to_string(run.step);  // B-006
}

}  // namespace

std::optional<AdventureData> parse_adventure(const std::string& text, std::string& error) {
    auto base = parse_scenario(text, error);
    if (!base) {
        return std::nullopt;
    }
    AdventureData a;
    a.parent = *base;
    try {
        const json j = json::parse(text);
        a.id = j.value("id", "");
        if (!j.contains("steps")) {  // a single episode
            AdventureStep only;
            only.kind = "parent";
            only.episode = base->scenario;
            a.steps = {only, AdventureStep{"victory", {}, {}}};
            return a;
        }
        for (const auto& s : j.at("steps")) {
            AdventureStep step;
            step.kind = s.value("type", "parent");
            if (step.kind == "parent") {
                auto ep = parse_episode(s, error);
                if (!ep) {
                    return std::nullopt;
                }
                step.episode = *ep;
            } else if (step.kind == "colony") {
                for (const auto& c : s.value("colonies", json::array())) {
                    auto cfg = parse_scenario(c.dump(), error);
                    if (!cfg) {
                        return std::nullopt;
                    }
                    step.colonies.push_back(*cfg);
                }
                if (step.colonies.empty()) {
                    error = "adventure.colony_step_without_colonies";
                    return std::nullopt;
                }
            } else if (step.kind != "victory") {
                error = "adventure.bad_step";
                return std::nullopt;
            }
            a.steps.push_back(step);
        }
    } catch (const std::exception& e) {
        error = std::string("adventure.invalid: ") + e.what();
        return std::nullopt;
    }
    if (a.steps.empty() || a.steps.front().kind != "parent" || a.steps.back().kind != "victory") {
        error = "adventure.must_start_in_parent_and_end_in_victory";  // doc 23
        return std::nullopt;
    }
    a.parent.scenario = a.steps.front().episode;
    return a;
}

Simulation begin_adventure(const DataCatalog& catalog, const AdventureData& adventure, AdventureRun& run) {
    run = AdventureRun{};
    return Simulation(catalog, adventure.parent);
}

std::vector<std::string> colony_choices(const AdventureData& adventure, const AdventureRun& run) {
    std::vector<std::string> out;
    if (run.step >= adventure.steps.size() || adventure.steps[run.step].kind != "colony") {
        return out;
    }
    for (const SimulationConfig& c : adventure.steps[run.step].colonies) {
        if (std::find(run.founded.begin(), run.founded.end(), c.scenario.id) == run.founded.end()) {
            out.push_back(c.scenario.id);  // the player picks among the unused ones (doc 23)
        }
    }
    return out;
}

NextEpisode advance_adventure(const AdventureData& adventure, AdventureRun& run, std::optional<Simulation>& sim) {
    if (run.in_colony) {  // back to the parent city; the colony joins its world map (doc 23)
        const std::string colony = sim ? sim->state().scenario.id : std::string();
        std::string error;
        auto parent = Simulation::load(sim->catalog(), run.parent_save, error);
        if (!parent) {
            return NextEpisode::Victory;  // cannot happen with a save made by found_colony
        }
        sim.reset();
        sim.emplace(std::move(*parent));
        run.parent_save.clear();
        run.in_colony = false;
        for (TradePartner& p : sim->mutable_state().trade.partners) {
            if (p.id == colony) {
                p.relation = "colony";
            }
        }
        ++sim->mutable_state().scenario.monuments["colony"];  // a Colony monument (Z:156, B-005)
    }
    ++run.step;
    if (run.step >= adventure.steps.size() || adventure.steps[run.step].kind == "victory") {
        return NextEpisode::Victory;
    }
    if (adventure.steps[run.step].kind == "colony") {
        return NextEpisode::ChooseColony;
    }
    start_parent_episode(adventure, run, sim->mutable_state());
    return NextEpisode::ParentEpisode;
}

std::string found_colony(const DataCatalog& catalog, const AdventureData& adventure, AdventureRun& run,
                         const std::string& colony, std::optional<Simulation>& sim) {
    const auto choices = colony_choices(adventure, run);
    if (std::find(choices.begin(), choices.end(), colony) == choices.end() || !sim) {
        return "adventure.bad_colony";
    }
    const auto& list = adventure.steps[run.step].colonies;
    const auto it = std::find_if(list.begin(), list.end(),
                                 [&colony](const SimulationConfig& c) { return c.scenario.id == colony; });
    run.parent_save = sim->save();
    run.in_colony = true;
    run.founded.push_back(colony);
    sim.emplace(catalog, *it);
    return {};
}

}  // namespace bo::core
