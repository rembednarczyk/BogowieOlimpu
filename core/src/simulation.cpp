#include "bo/core/simulation.h"

#include "systems.h"

#include <algorithm>
#include <type_traits>

#include "bo/core/appeal.h"
#include "bo/core/finance.h"
#include "bo/core/gods.h"
#include "bo/core/hash.h"
#include "bo/core/housing.h"
#include "bo/core/logistics.h"
#include "bo/core/migration.h"
#include "bo/core/risks.h"
#include "bo/core/trade.h"
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

bool is_build_command(const Command& c) {
    return std::holds_alternative<PlaceBuilding>(c) || std::holds_alternative<PlaceRoad>(c) ||
           std::holds_alternative<PlaceRoadblock>(c) || std::holds_alternative<ClearLand>(c) ||
           std::holds_alternative<Demolish>(c);
}

bool schedule_order(const ScheduledCommand& a, const ScheduledCommand& b) {
    return a.tick != b.tick ? a.tick < b.tick : a.sequence < b.sequence;
}

}  // namespace

Simulation::Simulation(const DataCatalog& catalog, const SimulationConfig& config) : catalog_(catalog) {
    state_.rng.reseed(config.seed);
    state_.difficulty = config.difficulty.value_or(catalog.default_difficulty);
    state_.finance.treasury = config.initial_treasury;
    state_.labour.full_staffing = config.full_staffing;
    state_.trade.partners = config.trade_partners;
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
        } else if (w.phase == WalkerPhase::AtTarget && w.job == WalkerJob::Settle) {
            settle_at_target(state_, catalog_, w);
        } else if (w.phase == WalkerPhase::AtTarget && w.job == WalkerJob::Trade) {
            TradeEvents trades;
            trade_at_target(state_, catalog_, w, trades);
            for (const auto& t : trades.done) {
                emit(t);
            }
        }
    }
    emit_risks(*this, crimes);
    CombatEvents combat;
    combat_tick(state_, catalog_, combat);
    emit_combat(combat);
    catch_outlaws(state_, catalog_);
    muggings(state_, catalog_);
    handle_arrivals(state_, catalog_);
    apply_sanctification(state_.city);  // hunters and extractors brought their catch (4x when sanctified)
    for (auto it = walkers.begin(); it != walkers.end();) {
        if (it->second.phase != WalkerPhase::Done) {
            ++it;
            continue;
        }
        if (it->second.job == WalkerJob::Settle) {
            settle_failed(state_, it->second);  // removed on the way: the group becomes homeless
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
                   [&](const SetTradePartner& c) {
                       const auto b = city.buildings.items.find(c.building);
                       if (b == city.buildings.items.end() || !b->second.trade || c.partner < -1 ||
                           c.partner >= static_cast<std::int32_t>(state_.trade.partners.size())) {
                           emit(CommandRejected{index, "trade.bad_partner"});
                       } else {
                           b->second.trade->partner = c.partner;
                       }
                   },
                   [&](const SetTradeOrder& c) {
                       const auto b = city.buildings.items.find(c.building);
                       if (b == city.buildings.items.end() || !b->second.trade || catalog_.goods.count(c.good) == 0) {
                           emit(CommandRejected{index, "trade.bad_order"});
                       } else {
                           b->second.trade->orders[c.good] = TradeOrder{c.import_on, c.import_limit, c.export_on, c.export_keep};
                       }
                   },
                   [&](const MusterCompany& c) {
                       std::vector<WalkerId> spawned;
                       const std::string err = muster_company(state_, catalog_, c.company, spawned);
                       if (!err.empty()) {
                           emit(CommandRejected{index, err});
                       }
                       for (const WalkerId id : spawned) {
                           emit(WalkerSpawned{id, state_.walkers.items.at(id).type});
                       }
                   },
                   [&](const MoveCompany& c) {
                       const std::string err = move_company(state_, c.company, {c.x, c.y});
                       if (!err.empty()) {
                           emit(CommandRejected{index, err});
                       }
                   },
                   [&](const SendCompanyHome& c) {
                       const std::string err = send_company_home(state_, c.company);
                       if (!err.empty()) {
                           emit(CommandRejected{index, err});
                       }
                   },
                   [&](const StartInvasion&) { apply_combat_command(command, index); },
                   [&](const BribeInvaders&) { apply_combat_command(command, index); },
                   [&](const Surrender&) { apply_combat_command(command, index); },
                   [&](const Pray& c) {
                       const std::string err = pray(state_, catalog_, c.building);
                       if (err.empty()) {
                           emit(PrayerAnswered{c.building});
                       } else {
                           emit(CommandRejected{index, err});
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

// Invasion commands (D3 stage 2): start, bribe, surrender.
void Simulation::apply_combat_command(const Command& command, std::uint32_t index) {
    CombatEvents ev;
    std::string err;
    if (const auto* c = std::get_if<StartInvasion>(&command)) {
        err = start_invasion(state_, catalog_, c->nation, c->soldiers, ev);
        if (err.empty()) {
            emit(InvasionStarted{c->nation, static_cast<std::uint32_t>(ev.spawned.size()), bribe_cost(state_, catalog_)});
        }
    } else if (std::holds_alternative<BribeInvaders>(command)) {
        err = bribe_invaders(state_, catalog_, ev);
    } else {
        err = surrender(state_, ev);
    }
    if (!err.empty()) {
        emit(CommandRejected{index, err});
    }
    emit_combat(ev);
}

void Simulation::emit_combat(const CombatEvents& ev) {
    for (const WalkerId id : ev.spawned) {
        emit(WalkerSpawned{id, state_.walkers.items.at(id).type});
    }
    if (ev.ended) {
        emit(InvasionEnded{*ev.ended});
    }
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
