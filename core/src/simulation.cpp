#include "bo/core/simulation.h"

#include <algorithm>
#include <type_traits>

#include "bo/core/appeal.h"
#include "bo/core/finance.h"
#include "bo/core/hash.h"
#include "bo/core/housing.h"
#include "bo/core/logistics.h"
#include "bo/core/migration.h"
#include "bo/core/risks.h"
#include "bo/core/serialize.h"
#include "bo/core/walker.h"

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

// Spawn delay in days by staffing (S-007 Akhenaten, doc 24): 100% -> 0, >=75% -> 1, >=50% -> 3, >=25% -> 7, else 15.
std::uint32_t spawn_delay_days(std::uint8_t staffing) {
    if (staffing >= 100) {
        return 0;
    }
    if (staffing >= 75) {
        return 1;
    }
    if (staffing >= 50) {
        return 3;
    }
    return staffing >= 25 ? 7 : 15;
}

bool walker_from_building(const WalkerType& w, const std::string& building_type) {
    for (const auto& f : w.from) {
        if (f == building_type || (f == "agora" && building_type.find("agora") != std::string::npos)) {
            return true;
        }
    }
    return false;
}

// Daily: each staffed building with road access sends its roaming walker when the previous one is back.
void run_walker_spawn(Simulation& sim) {
    WorldState& st = sim.mutable_state();
    for (auto& [id, b] : st.city.buildings.items) {
        if (b.roaming_walker != kNoWalker || !b.road_access || b.staffing_percent == 0) {
            continue;
        }
        if (b.days_since_spawn++ < spawn_delay_days(b.staffing_percent)) {
            continue;
        }
        for (const auto& [wid, wt] : sim.catalog().walkers) {
            if (wt.kind != WalkerKind::Roaming || !wt.roam_ticks || !walker_from_building(wt, b.type)) {
                continue;
            }
            const WalkerId w = spawn_roaming_walker(st.city, st.walkers, sim.catalog(), wid, id, b.next_roam_dir);
            if (w != kNoWalker) {
                b.roaming_walker = w;
                b.days_since_spawn = 0;
                b.next_roam_dir = rotate(b.next_roam_dir, 1);
                sim.emit(WalkerSpawned{w, wid});
            }
            break;
        }
    }
}

// Daily: every service counter decays by 1 (96 days per visit, doc 24).
void run_coverage_decay(Simulation& sim) {
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        for (auto& [service, days] : b.coverage) {
            if (days > 0) {
                --days;
            }
        }
    }
}

// C1: the appeal field is derived data, recomputed once a day before the houses read it.
void run_appeal(Simulation& sim) {
    CityState& city = sim.mutable_state().city;
    city.appeal = compute_appeal(city, sim.catalog());
}

// C2: consumption on days 0 and 8 of the month (doc 24).
void run_house_consumption(Simulation& sim) {
    const std::uint32_t day = sim.state().time.day_in_month();
    if (day == 0 || day == 8) {
        consume_house_goods(sim.mutable_state().city, sim.catalog());
    }
}

// C2: daily evolution check (doc 24).
void run_house_evolution(Simulation& sim) {
    WorldState& st = sim.mutable_state();
    for (const auto& c : update_houses(st.city, sim.catalog(), st.difficulty)) {
        const bool elite = st.city.buildings.items.at(c.id).house->elite;
        sim.emit(HouseChanged{c.id, c.level, elite, c.abandoned});
    }
}

// C3: daily labour allocation, before walkers are sent (tick 31).
void run_labour(Simulation& sim) {
    WorldState& st = sim.mutable_state();
    allocate_labour(st.city, sim.catalog(), st.difficulty, st.finance.wage_rate, st.labour);
}

// C6: taxes and wages at the start of each month; the yearly record rolls over at the year start.
void run_finance(Simulation& sim) {
    WorldState& st = sim.mutable_state();
    if (st.time.day_in_month() != 0) {
        return;
    }
    if (st.time.month() == 0) {
        close_year(st.finance);
    }
    const auto r = settle_month(st, sim.catalog());
    sim.emit(MonthlyFinance{r.taxes, r.wages});
}

// C8: moods twice a month (days 0 and 8), migration daily.
void run_moods(Simulation& sim) {
    WorldState& st = sim.mutable_state();
    const std::uint32_t day = st.time.day_in_month();
    if (day == 0) {
        st.migration.arrived_this_month = 0;
        st.migration.left_this_month = 0;
    }
    if (day == 0 || day == 8) {
        update_moods(st, sim.catalog());
    }
}

void run_migration(Simulation& sim) {
    const auto r = migrate(sim.mutable_state(), sim.catalog());
    if (r.arrived > 0 || r.left > 0) {
        sim.emit(PeopleMoved{r.arrived, r.left});
    }
}

// C4/C5: daily production and dispatch of deliverymen and vendors.
void run_production(Simulation& sim) {
    WorldState& st = sim.mutable_state();
    advance_production(st, sim.catalog());
    advance_husbandry(st, sim.catalog());
    advance_extraction(st, sim.catalog());
    advance_wildlife(st, sim.catalog());
    for (const BuildingId id : herd_births(st, sim.catalog())) {
        const Building& b = st.city.buildings.items.at(id);
        sim.emit(BuildingPlaced{b.id, b.type, b.origin.x, b.origin.y, b.width, b.height, b.rotation, b.parent});
    }
}

void run_logistics(Simulation& sim) {
    const auto ev = dispatch_workers(sim.mutable_state(), sim.catalog());
    for (const WalkerId id : ev.spawned) {
        sim.emit(WalkerSpawned{id, sim.state().walkers.items.at(id).type});
    }
}

void emit_risks(Simulation& sim, const RiskEvents& ev) {
    for (const BuildingId id : ev.removed) {
        sim.emit(BuildingRemoved{id});
    }
    for (const BuildingId id : ev.placed) {
        const Building& b = sim.state().city.buildings.items.at(id);
        sim.emit(BuildingPlaced{b.id, b.type, b.origin.x, b.origin.y, b.width, b.height, b.rotation, b.parent});
    }
    for (const WalkerId id : ev.spawned) {
        sim.emit(WalkerSpawned{id, sim.state().walkers.items.at(id).type});
    }
    for (const auto& [kind, id] : ev.disasters) {
        sim.emit(Disaster{kind, id});
    }
}

// C7: daily risks (fire, collapse, plague, crime).
void run_risks(Simulation& sim) { emit_risks(sim, advance_risks(sim.mutable_state(), sim.catalog())); }

bool is_build_command(const Command& c) {
    return std::holds_alternative<PlaceBuilding>(c) || std::holds_alternative<PlaceRoad>(c) ||
           std::holds_alternative<PlaceRoadblock>(c) || std::holds_alternative<ClearLand>(c) ||
           std::holds_alternative<Demolish>(c);
}

bool schedule_order(const ScheduledCommand& a, const ScheduledCommand& b) {
    return a.tick != b.tick ? a.tick < b.tick : a.sequence < b.sequence;
}

}  // namespace

const std::vector<DailySystem>& daily_schedule() {
    // Ticks of the day follow the engine family layout; new systems get their own slot (docs/05-SYMULACJA.md).
    static const std::vector<DailySystem> kSchedule = {
        {"calendar", 0, &run_calendar},
        {"appeal", 20, &run_appeal},
        {"house_consumption", 21, &run_house_consumption},
        {"house_evolution", 22, &run_house_evolution},
        {"labour", 23, &run_labour},
        {"finance", 24, &run_finance},
        {"moods", 25, &run_moods},
        {"migration", 26, &run_migration},
        {"production", 27, &run_production},
        {"logistics", 28, &run_logistics},
        {"risks", 29, &run_risks},
        {"walker_spawn", 31, &run_walker_spawn},
        {"coverage_decay", 35, &run_coverage_decay},
    };
    return kSchedule;
}

Simulation::Simulation(const DataCatalog& catalog, const SimulationConfig& config) : catalog_(catalog) {
    state_.rng.reseed(config.seed);
    state_.difficulty = config.difficulty.value_or(catalog.default_difficulty);
    state_.finance.treasury = config.initial_treasury;
    state_.labour.full_staffing = config.full_staffing;
    state_.migration.popularity =
        static_cast<std::uint8_t>(catalog.migration.base_happiness[static_cast<std::size_t>(state_.difficulty)]);
    if (!config.text_map.empty()) {
        auto map = parse_text_map(config.text_map, setup_error_, config.elevation_map);
        if (map) {
            state_.city.map = std::move(*map);
            state_.city.map.generate_random_bytes(state_.rng.stream(RngStream::MapGeneration));
            update_roads(state_.city);
        }
    }
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
        step_walkers();
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
    std::optional<CityState> before;  // the city before this tick's build commands (undo, Z:20)
    tick_build_cost_ = 0;
    while (done < pending.size() && pending[done].tick <= state_.time.tick()) {
        if (!before && is_build_command(pending[done].command)) {
            before = state_.city;
        }
        apply(pending[done].command, static_cast<std::uint32_t>(pending[done].command.index()));
        executed_.push_back(pending[done]);
        ++done;
    }
    pending.erase(pending.begin(), pending.begin() + static_cast<std::ptrdiff_t>(done));
    if (before) {
        record_undo(state_.city, *before, state_.time.tick(), tick_build_cost_);
    }
    UndoRecord& undo = state_.city.undo;
    if (undo.active && state_.time.tick() >= undo.tick + kUndoWindowTicks) {
        undo = UndoRecord{};  // the undo button goes grey (window: L-035)
    }
}

void Simulation::apply_undo_command(std::uint32_t index) {
    CityState& city = state_.city;
    if (!city.undo.active) {
        emit(CommandRejected{index, "undo.nothing"});
        return;
    }
    const UndoRecord rec = city.undo;
    for (const BuildingId id : rec.added) {
        emit(BuildingRemoved{id});
    }
    for (const Building& b : rec.buildings_before) {
        if (city.buildings.items.count(b.id) != 0) {
            emit(BuildingRemoved{b.id});
        }
    }
    const auto refund = apply_undo(city);
    state_.finance.treasury += *refund;
    state_.finance.this_year.construction -= *refund;
    for (const Building& b : rec.buildings_before) {
        Building& restored = city.buildings.items.at(b.id);
        if (state_.walkers.items.count(restored.roaming_walker) == 0) {
            restored.roaming_walker = kNoWalker;  // its walker was removed meanwhile
        }
        emit(BuildingPlaced{b.id, b.type, b.origin.x, b.origin.y, b.width, b.height, b.rotation, b.parent});
    }
    for (const auto& [pos, tile] : rec.tiles_before) {
        emit(TileChanged{pos.x, pos.y});
    }
    emit(UndoApplied{*refund});
}

void Simulation::step_walkers() {
    auto& walkers = state_.walkers.items;
    for (auto& [id, w] : walkers) {
        const TilePos before = w.tile;
        step_walker(state_.city, w);
        if (w.type == "peddler" && w.tile != before) {
            peddler_sells(state_, catalog_, w);  // sells on every new tile, like coverage (doc 06)
        }
    }
    RiskEvents crimes;
    for (auto& [id, w] : walkers) {
        if (w.phase == WalkerPhase::AtTarget && w.job == WalkerJob::Crime) {
            crime_at_target(state_, catalog_, w, crimes);
        } else if (w.phase == WalkerPhase::AtTarget && w.job == WalkerJob::Extinguish) {
            extinguish_at_target(state_, catalog_, w);
        }
    }
    emit_risks(*this, crimes);
    catch_outlaws(state_, catalog_);
    muggings(state_, catalog_);
    handle_arrivals(state_, catalog_);
    for (auto it = walkers.begin(); it != walkers.end();) {
        if (it->second.phase != WalkerPhase::Done) {
            ++it;
            continue;
        }
        unload_at_home(state_, catalog_, it->second);
        const auto home = state_.city.buildings.items.find(it->second.home);
        if (home != state_.city.buildings.items.end() && home->second.roaming_walker == it->first) {
            home->second.roaming_walker = kNoWalker;
        }
        emit(WalkerRemoved{it->first});
        it = walkers.erase(it);
    }
}

void Simulation::apply_build(const BuildResult& r, std::uint32_t index, TilePos tile, bool tile_changed) {
    if (!r.ok()) {
        emit(CommandRejected{index, r.reason});
        return;
    }
    state_.finance.treasury -= r.cost;  // treasury may go below zero (doc 08); building in debt: L-094
    state_.finance.this_year.construction += r.cost;
    tick_build_cost_ += r.cost;
    if (tile_changed) {
        emit(TileChanged{tile.x, tile.y});
    }
    for (const BuildingId id : r.removed) {
        emit(BuildingRemoved{id});
    }
    if (r.building != kNoBuilding) {
        const Building& b = state_.city.buildings.items.at(r.building);
        emit(BuildingPlaced{b.id, b.type, b.origin.x, b.origin.y, b.width, b.height, b.rotation, b.parent});
        if (b.road_mode == RoadMode::OwnRoad) {
            for (const TilePos p : b.tiles) {
                emit(TileChanged{p.x, p.y});
            }
        }
    }
}

void Simulation::apply(const Command& command, std::uint32_t index) {
    // One handler per command type; a missing handler fails to compile.
    CityState& city = state_.city;
    const Difficulty diff = state_.difficulty;
    std::visit(Overloaded{
                   [this](const SetTaxRate& c) {
                       state_.finance.tax_rate = c.rate;
                       emit(TaxRateChanged{c.rate});
                   },
                   [this](const SetWageRate& c) {
                       state_.finance.wage_rate = c.rate;
                       emit(WageRateChanged{c.rate});
                   },
                   [&](const PlaceBuilding& c) {
                       apply_build(place_building(city, catalog_, diff, c.type, {c.x, c.y}, c.rotation), index,
                                   {c.x, c.y}, false);
                   },
                   [&](const PlaceRoad& c) { apply_build(place_road(city, catalog_, diff, {c.x, c.y}), index, {c.x, c.y}, true); },
                   [&](const PlaceRoadblock& c) {
                       apply_build(place_roadblock(city, catalog_, diff, {c.x, c.y}), index, {c.x, c.y}, true);
                   },
                   [&](const ClearLand& c) { apply_build(clear_land(city, catalog_, diff, {c.x, c.y}), index, {c.x, c.y}, true); },
                   [&](const Demolish& c) {
                       const auto r = demolish(city, {c.x, c.y});
                       apply_build(r, index, {c.x, c.y}, true);
                   },
                   [&](const Undo&) { apply_undo_command(index); },
                   [&](const SetSectorPriority& c) {
                       if (c.sector >= kSectors || c.priority >= kPriorityLevels) {
                           emit(CommandRejected{index, "labour.bad_priority"});
                           return;
                       }
                       state_.labour.priority[c.sector] = c.priority;
                   },
                   [&](const SetStorageOrder& c) {
                       const auto b = city.buildings.items.find(c.building);
                       if (b == city.buildings.items.end() || !b->second.storage ||
                           catalog_.goods.count(c.good) == 0 || c.kind > 3) {
                           emit(CommandRejected{index, "storage.bad_order"});
                       } else {
                           b->second.storage->orders[c.good] =
                               StorageOrder{static_cast<StorageOrderKind>(c.kind), c.limit};
                       }
                   },
                   [&](const SetIndustryEnabled& c) {
                       if (catalog_.buildings.count(c.type) == 0) {
                           emit(CommandRejected{index, "labour.unknown_industry"});
                       } else if (c.enabled) {
                           state_.labour.disabled_types.erase(c.type);
                       } else {
                           state_.labour.disabled_types.insert(c.type);
                       }
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
    if (loaded->format_version < 6) {
        // Migration v5 -> v6: saves before module C8 start at the base mood of their difficulty.
        WorldState& st = loaded->state;
        st.migration.popularity =
            static_cast<std::uint8_t>(catalog.migration.base_happiness[static_cast<std::size_t>(st.difficulty)]);
    }
    return Simulation(catalog, std::move(loaded->state));
}

}  // namespace bo::core
