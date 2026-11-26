// Module D5: adventures of several episodes (docs/gameplay/23; L-119).
#include "bo/core/adventure.h"

#include <algorithm>

#include "bo/core/serialize.h"
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

namespace {

std::string as_string(const std::vector<std::uint8_t>& b) { return std::string(b.begin(), b.end()); }
std::vector<std::uint8_t> as_bytes(const std::string& s) { return std::vector<std::uint8_t>(s.begin(), s.end()); }

}  // namespace

std::vector<std::uint8_t> save_adventure(const std::string& text, const AdventureRun& run, const Simulation& sim) {
    ByteWriter w;
    w.u32(kAdventureMagic);
    w.u32(1);  // container version
    w.str(text);
    w.u32(static_cast<std::uint32_t>(run.step));
    w.u8(run.in_colony ? 1 : 0);
    w.u32(static_cast<std::uint32_t>(run.founded.size()));
    for (const std::string& c : run.founded) {
        w.str(c);
    }
    w.str(as_string(run.parent_save));
    w.str(as_string(sim.save()));
    return w.bytes();
}

bool is_adventure_save(const std::vector<std::uint8_t>& bytes) {
    ByteReader r(bytes);
    std::uint32_t magic = 0;
    return r.u32(magic) && magic == kAdventureMagic;
}

std::optional<LoadedAdventure> load_adventure(const DataCatalog& catalog, const std::vector<std::uint8_t>& bytes,
                                              std::string& error) {
    ByteReader r(bytes);
    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t step = 0;
    std::uint8_t colony = 0;
    std::uint32_t founded = 0;
    LoadedAdventure out;
    if (!r.u32(magic) || magic != kAdventureMagic || !r.u32(version) || version != 1 || !r.str(out.text) ||
        !r.u32(step) || !r.u8(colony) || !r.u32(founded) || founded > 64) {
        error = "adventure.bad_save";
        return std::nullopt;
    }
    out.run.step = step;
    out.run.in_colony = colony != 0;
    out.run.founded.resize(founded);
    std::string parent;
    std::string current;
    for (std::string& c : out.run.founded) {
        if (!r.str(c)) {
            error = "adventure.bad_save";
            return std::nullopt;
        }
    }
    if (!r.str(parent) || !r.str(current) || !r.at_end()) {
        error = "adventure.bad_save";
        return std::nullopt;
    }
    out.run.parent_save = as_bytes(parent);
    auto data = parse_adventure(out.text, error);
    if (!data || step >= data->steps.size()) {
        error = error.empty() ? "adventure.bad_save" : error;
        return std::nullopt;
    }
    out.data = std::move(*data);
    auto sim = Simulation::load(catalog, as_bytes(current), error);
    if (!sim) {
        return std::nullopt;
    }
    out.sim.emplace(std::move(*sim));
    return out;
}

}  // namespace bo::core
