// Simulation facade: commands in, events out, fixed tick, explicit system order (docs/05-SYMULACJA.md).
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "bo/core/command.h"
#include "bo/core/data.h"
#include "bo/core/event.h"
#include "bo/core/state.h"

namespace bo::core {

struct SimulationConfig {
    std::uint64_t seed = 0;
    std::optional<Difficulty> difficulty;  // default: catalog default (R-013: Hero)
    std::int64_t initial_treasury = 0;     // from the scenario
    std::vector<std::string> text_map;     // optional map in the parse_text_map format (tests, golden scenario)
    bool full_staffing = false;            // development switch (LabourState::full_staffing)
    std::vector<std::string> elevation_map;  // optional heights for text_map (one hex digit per tile)
    std::vector<TradePartner> trade_partners;  // scenario partner cities (C9)
};

class Simulation;

// A daily system runs once per day at a fixed tick of the day (pattern from the engine family, S-006/S-007).
struct DailySystem {
    std::string_view name;
    std::uint32_t tick_in_day;
    void (*run)(Simulation&);
};

// The schedule is one explicit table (docs/PRAKTYKI.md ch. 3). Order inside the table is the execution order
// for systems that share a tick.
const std::vector<DailySystem>& daily_schedule();

class Simulation {
public:
    Simulation(const DataCatalog& catalog, const SimulationConfig& config);
    // Valid when the map in the config parsed correctly; otherwise holds the error key.
    [[nodiscard]] const std::string& setup_error() const { return setup_error_; }
    Simulation(const DataCatalog& catalog, WorldState state);

    // Queues a command for the start of the next tick. Returns its sequence number.
    std::uint64_t issue(const Command& command);
    // Queues a command for a specific future tick (scripts, replays). Past ticks are clamped to the next tick.
    std::uint64_t issue_at(std::uint64_t tick, const Command& command);

    void step(std::uint64_t ticks = 1);
    void run_days(std::uint64_t days) { step(days * kTicksPerDay); }

    std::vector<TimedEvent> drain_events();
    void emit(Event event);
    void emit_combat(const CombatEvents& ev);
    void apply_combat_command(const Command& command, std::uint32_t index);

    [[nodiscard]] const WorldState& state() const { return state_; }
    WorldState& mutable_state() { return state_; }
    [[nodiscard]] const DataCatalog& catalog() const { return catalog_; }
    // Every command that has been executed, in execution order (input for replays).
    [[nodiscard]] const CommandLog& executed_commands() const { return executed_; }

    [[nodiscard]] std::uint64_t state_hash() const;
    [[nodiscard]] std::vector<std::uint8_t> save() const;
    static std::optional<Simulation> load(const DataCatalog& catalog, const std::vector<std::uint8_t>& bytes,
                                          std::string& error);

private:
    void execute_due_commands();
    void apply(const Command& command, std::uint32_t index);
    void apply_build(const BuildResult& r, std::uint32_t index, TilePos tile, bool tile_changed);
    void step_walkers();
    void apply_undo_command(std::uint32_t index);

    const DataCatalog& catalog_;
    WorldState state_;
    EventQueue events_;
    CommandLog executed_;
    std::string setup_error_;
    std::int64_t tick_build_cost_ = 0;  // cost of this tick's build commands (undo refund)
};

}  // namespace bo::core
