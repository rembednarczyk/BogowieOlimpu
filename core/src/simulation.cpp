#include "bo/core/simulation.h"

#include <algorithm>
#include <type_traits>

#include "bo/core/hash.h"
#include "bo/core/serialize.h"

namespace bo::core {
namespace {

template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

// Calendar system: tick 0 of each day. Emits day/month/year events for presentation.
void run_calendar(Simulation& sim) {
    const GameTime t = sim.state().time;
    if (t.is_year_start()) {
        sim.emit(YearStarted{t.year()});
    }
    if (t.is_month_start()) {
        sim.emit(MonthStarted{t.month(), t.year()});
    }
    sim.emit(DayStarted{t.total_days()});
}

bool schedule_order(const ScheduledCommand& a, const ScheduledCommand& b) {
    return a.tick != b.tick ? a.tick < b.tick : a.sequence < b.sequence;
}

}  // namespace

const std::vector<DailySystem>& daily_schedule() {
    // Ticks of the day follow the engine family layout; new systems get their own slot (docs/05-SYMULACJA.md).
    static const std::vector<DailySystem> kSchedule = {
        {"calendar", 0, &run_calendar},
    };
    return kSchedule;
}

Simulation::Simulation(const DataCatalog& catalog, const SimulationConfig& config) : catalog_(catalog) {
    state_.rng.reseed(config.seed);
    state_.difficulty = config.difficulty.value_or(catalog.default_difficulty);
    state_.finance.treasury = config.initial_treasury;
}

Simulation::Simulation(const DataCatalog& catalog, WorldState state) : catalog_(catalog), state_(std::move(state)) {}

std::uint64_t Simulation::issue(const Command& command) { return issue_at(state_.time.tick(), command); }

std::uint64_t Simulation::issue_at(std::uint64_t tick, const Command& command) {
    ScheduledCommand sc{std::max(tick, state_.time.tick()), state_.next_command_sequence++, command};
    const auto pos = std::upper_bound(state_.pending_commands.begin(), state_.pending_commands.end(), sc, schedule_order);
    state_.pending_commands.insert(pos, sc);
    return sc.sequence;
}

void Simulation::step(std::uint64_t ticks) {
    for (std::uint64_t i = 0; i < ticks; ++i) {
        execute_due_commands();
        const std::uint32_t tick_in_day = state_.time.tick_in_day();
        for (const auto& system : daily_schedule()) {
            if (system.tick_in_day == tick_in_day) {
                system.run(*this);
            }
        }
        state_.time.advance();
    }
}

void Simulation::execute_due_commands() {
    auto& pending = state_.pending_commands;
    std::size_t done = 0;
    while (done < pending.size() && pending[done].tick <= state_.time.tick()) {
        apply(pending[done].command, static_cast<std::uint32_t>(pending[done].command.index()));
        executed_.push_back(pending[done]);
        ++done;
    }
    pending.erase(pending.begin(), pending.begin() + static_cast<std::ptrdiff_t>(done));
}

void Simulation::apply(const Command& command, std::uint32_t /*index*/) {
    // One handler per command type; a missing handler fails to compile.
    std::visit(Overloaded{
                   [this](const SetTaxRate& c) {
                       state_.finance.tax_rate = c.rate;
                       emit(TaxRateChanged{c.rate});
                   },
                   [this](const SetWageRate& c) {
                       state_.finance.wage_rate = c.rate;
                       emit(WageRateChanged{c.rate});
                   },
               },
               command);
}

void Simulation::emit(Event event) { events_.push_back(TimedEvent{state_.time.tick(), std::move(event)}); }

std::vector<TimedEvent> Simulation::drain_events() {
    std::vector<TimedEvent> out;
    out.swap(events_);
    return out;
}

std::uint64_t Simulation::state_hash() const {
    const auto bytes = serialize_state(state_, catalog_.version);
    Fnv1a64 h;
    h.add_bytes(bytes.data(), bytes.size());
    return h.value();
}

std::vector<std::uint8_t> Simulation::save() const { return serialize_state(state_, catalog_.version); }

std::optional<Simulation> Simulation::load(const DataCatalog& catalog, const std::vector<std::uint8_t>& bytes,
                                           std::string& error) {
    auto loaded = deserialize_state(bytes, error);
    if (!loaded) {
        return std::nullopt;
    }
    if (loaded->data_version != catalog.version) {
        // Data changed since the save: allowed, but the caller must know (balance may differ).
        error = "save.data_version_mismatch";
    }
    return Simulation(catalog, std::move(loaded->state));
}

}  // namespace bo::core
