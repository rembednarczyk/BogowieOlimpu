// The daily systems and their schedule (docs/05-SYMULACJA.md): one function per system, one table for the order.
#include "bo/core/simulation.h"
#include <algorithm>
#include <type_traits>
#include "bo/core/appeal.h"
#include "bo/core/auto_defend.h"
#include "bo/core/culture.h"
#include "bo/core/games.h"
#include "bo/core/finance.h"
#include "bo/core/gods.h"
#include "bo/core/hash.h"
#include "bo/core/hippodrome.h"
#include "bo/core/housing.h"
#include "bo/core/logistics.h"
#include "bo/core/migration.h"
#include "bo/core/monsters.h"
#include "bo/core/naval.h"
#include "bo/core/risks.h"
#include "bo/core/sanctuary.h"
#include "bo/core/trade.h"
#include "bo/core/serialize.h"
#include "bo/core/walker.h"
#include "bo/core/world.h"
#include "bo/core/wrath.h"
#include "systems.h"

namespace bo::core {

// Calendar system: tick 0 of each day. Emits day/month/year events for presentation.
static void run_calendar(Simulation& sim) {
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
static std::uint32_t spawn_delay_days(std::uint8_t staffing) {
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

static bool walker_from_building(const WalkerType& w, const std::string& building_type) {
    for (const auto& f : w.from) {
        if (f == building_type || (f == "agora" && building_type.find("agora") != std::string::npos)) {
            return true;
        }
    }
    return false;
}

// Daily: each staffed building with road access sends its roaming walker when the previous one is back;
// an inhabited elite house sends its nobleman (Z:25, Z:88).
static void run_walker_spawn(Simulation& sim) {
    WorldState& st = sim.mutable_state();
    for (auto& [id, b] : st.city.buildings.items) {
        const bool nobles = b.house && b.house->elite && b.house->population > 0 && !b.house->abandoned;
        const std::uint8_t staffing = b.house ? (nobles ? std::uint8_t{100} : std::uint8_t{0}) : b.staffing_percent;
        if (b.roaming_walker != kNoWalker || !b.road_access || staffing == 0) {
            continue;
        }
        if (b.days_since_spawn++ < spawn_delay_days(staffing)) {
            continue;
        }
        for (const auto& [wid, wt] : sim.catalog().walkers) {
            const bool serves = (wt.kind == WalkerKind::Roaming && walker_from_building(wt, b.type)) || venue_performs(b, wt);
            if (!wt.roam_ticks || !serves) {
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

// Daily (B-008): schools send performers to venues; performances run down.
static void run_culture(Simulation& sim) {
    WorldState& st = sim.mutable_state();
    run_schools(st.city, st.walkers, sim.catalog());
    if (const auto g = advance_games(st, sim.catalog())) {
        sim.emit(GamesHeld{g->game, g->chance, g->invited, g->won, g->hosted, g->right_lost});
    }
}

// Daily: every service counter decays by 1 (96 days per visit, doc 24).
static void run_coverage_decay(Simulation& sim) {
    for (auto& [id, b] : sim.mutable_state().city.buildings.items) {
        for (auto& [service, days] : b.coverage) {
            if (days > 0) {
                --days;
            }
        }
    }
}

// C1: the appeal field is derived data, recomputed once a day before the houses read it.
static void run_appeal(Simulation& sim) {
    CityState& city = sim.mutable_state().city;
    city.appeal = compute_appeal(city, sim.catalog());
}

// C2: consumption on days 0 and 8 of the month (doc 24).
static void run_house_consumption(Simulation& sim) {
    const std::uint32_t day = sim.state().time.day_in_month();
    if (day == 0 || day == 8) {
        consume_house_goods(sim.mutable_state().city, sim.catalog());
    }
}

// C2: daily evolution check (doc 24).
static void run_house_evolution(Simulation& sim) {
    WorldState& st = sim.mutable_state();
    for (const auto& c : update_houses(st.city, sim.catalog(), st.difficulty)) {
        const bool elite = st.city.buildings.items.at(c.id).house->elite;
        sim.emit(HouseChanged{c.id, c.level, elite, c.abandoned});
    }
}

// C3: daily labour allocation, before walkers are sent (tick 31).
static void run_labour(Simulation& sim) {
    WorldState& st = sim.mutable_state();
    allocate_labour(st.city, sim.catalog(), st.difficulty, st.finance.wage_rate, st.labour,
                    workers_in_the_field(st, sim.catalog()));
}

// C6: taxes and wages at the start of each month; the yearly record rolls over at the year start.
static void run_finance(Simulation& sim) {
    WorldState& st = sim.mutable_state();
    // L-034: continuous debt; a notice every year, the episode is lost after `loss_days` (Z:16).
    st.debt_days = st.finance.treasury < 0 ? st.debt_days + 1 : 0;
    const DebtParams& debt = sim.catalog().debt;
    if (st.debt_days > 0 && st.debt_days % debt.warning_every_days == 0) {
        sim.emit(DebtNotice{st.debt_days / debt.warning_every_days});
    }
    if (st.debt_days == debt.loss_days && !st.episode_lost) {
        st.episode_lost = true;
        sim.emit(EpisodeLost{});
    }
    if (st.time.day_in_month() != 0) {
        return;
    }
    if (st.time.month() == 0) {
        close_year(st.finance);
        for (const auto& [good, n] : st.produced_this_year) {  // the best year of each good (S-008)
            auto& best = st.produced_best_year[good];
            best = std::max(best, n);
        }
        st.produced_this_year.clear();
        st.hippodrome_last_year = st.hippodrome_this_year;  // the track's takings (P:14)
        st.hippodrome_this_year = 0;
    }
    const auto r = settle_month(st, sim.catalog());
    sim.emit(MonthlyFinance{r.taxes, r.wages});
}

// C8: moods twice a month (days 0 and 8), migration daily.
static void run_moods(Simulation& sim) {
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

static void run_migration(Simulation& sim) {
    const auto r = migrate(sim.mutable_state(), sim.catalog());
    for (const WalkerId id : r.spawned) {
        sim.emit(WalkerSpawned{id, sim.state().walkers.items.at(id).type});
    }
    if (r.arrived > 0 || r.left > 0) {
        sim.emit(PeopleMoved{r.arrived, r.left});
    }
}

// C4/C5: daily production and dispatch of deliverymen and vendors.
static void run_production(Simulation& sim) {
    WorldState& st = sim.mutable_state();
    advance_production(st, sim.catalog());
    advance_husbandry(st, sim.catalog());
    advance_extraction(st, sim.catalog());
    advance_wildlife(st, sim.catalog());
    apply_sanctification(st.city);  // farms, processors, extraction: 4x when sanctified (doc 21)
    const WolfEvents wolves = advance_wolves(st, sim.catalog());
    for (const BuildingId id : wolves.removed) {
        sim.emit(BuildingRemoved{id});
    }
    for (const WalkerId id : wolves.spawned) {
        sim.emit(WalkerSpawned{id, st.walkers.items.at(id).type});
    }
    for (const BuildingId id : herd_births(st, sim.catalog())) {
        const Building& b = st.city.buildings.items.at(id);
        sim.emit(BuildingPlaced{b.id, b.type, b.origin.x, b.origin.y, b.width, b.height, b.rotation, b.parent});
    }
}

static void run_logistics(Simulation& sim) {
    auto ev = dispatch_workers(sim.mutable_state(), sim.catalog());
    advance_hippodrome(sim.mutable_state(), sim.catalog(), ev);  // P:13-15
    apply_sanctification(sim.mutable_state().city);  // note what the deliverymen took
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
static void run_risks(Simulation& sim) { emit_risks(sim, advance_risks(sim.mutable_state(), sim.catalog())); }

// C9: traders arrive at trade buildings.
static void run_trade(Simulation& sim) {
    const auto ev = dispatch_traders(sim.mutable_state(), sim.catalog());
    for (const WalkerId id : ev.spawned) {
        sim.emit(WalkerSpawned{id, sim.state().walkers.items.at(id).type});
    }
}

// D1: sanctuary construction and sacrifices.
static void run_mythology(Simulation& sim) {
    const MythEvents ev = advance_sanctuaries(sim.mutable_state(), sim.catalog());
    advance_gods(sim.mutable_state(), sim.catalog());
    for (const WalkerId id : ev.spawned) {
        sim.emit(WalkerSpawned{id, sim.state().walkers.items.at(id).type});
    }
    for (const BuildingId id : ev.removed) {
        sim.emit(BuildingRemoved{id});
    }
    for (const BuildingId id : ev.completed) {
        sim.emit(SanctuaryCompleted{id});
    }
}

// D4: expeditions, request cooldowns, yearly tribute.
static void run_world(Simulation& sim) {
    WorldEvents ev = advance_world(sim.mutable_state(), sim.catalog());
    advance_relations(sim.mutable_state(), sim.catalog(), ev);
    for (const std::uint32_t city : ev.rebellions) {
        sim.emit(CityRebelled{city});
    }
    for (const auto& [city, amount] : ev.gifts) {
        sim.emit(CityGift{city, amount});
    }
    for (const std::uint32_t city : ev.rival_attacks) {  // the rival's army: ten soldiers a shield (L-063)
        WorldState& st = sim.mutable_state();
        const auto& d = st.trade.partners[city].diplomacy;
        CombatEvents combat;
        const auto soldiers = static_cast<std::uint32_t>(d.military * sim.catalog().world.city_strength_per_shield);
        if (start_invasion(st, sim.catalog(), sim.catalog().world.rival_attack_nation, soldiers, combat,
                           static_cast<std::int32_t>(city)).empty()) {
            sim.emit(InvasionStarted{sim.catalog().world.rival_attack_nation,
                                     static_cast<std::uint32_t>(combat.spawned.size()), bribe_cost(st, sim.catalog())});
            sim.emit_combat(combat);
        }
    }
    for (const auto& [city, amount] : ev.tribute) {
        sim.emit(TributePaid{city, amount});
    }
    for (const Expedition& e : ev.returned) {
        sim.emit(ExpeditionReturned{e.city, e.won, e.losses, e.loot});
    }
}

static void emit_wrath(Simulation& sim, const WrathEvents& w) {
    for (const BuildingId id : w.removed) {
        sim.emit(BuildingRemoved{id});
    }
    for (const TilePos p : w.tiles_changed) {
        sim.emit(TileChanged{p.x, p.y});
    }
}

// D5: scenario events, city requests, goals.
static void run_scenario(Simulation& sim) {
    WorldState& st = sim.mutable_state();
    const ScenarioEvents ev = advance_scenario(st, sim.catalog());
    for (const std::size_t i : ev.new_requests) {
        const CityRequest& r = st.scenario.requests[i];
        sim.emit(CityRequestReceived{static_cast<std::uint32_t>(i), r.city, r.good, r.amount});
    }
    for (const auto& [city, fulfilled] : ev.closed) {
        sim.emit(RequestClosed{city, fulfilled});
    }
    for (std::size_t i = 0; i < ev.invasions.size(); ++i) {
        CombatEvents combat;
        const std::string err =
            start_invasion(st, sim.catalog(), ev.invasions[i].first, ev.invasions[i].second, combat, ev.invasion_cities[i],
                           ev.invasion_by_sea[i]);
        if (err.empty()) {
            sim.emit(InvasionStarted{ev.invasions[i].first, static_cast<std::uint32_t>(combat.spawned.size()),
                                     bribe_cost(st, sim.catalog())});
            sim.emit_combat(combat);
        }
    }
    std::vector<std::pair<std::string, std::uint32_t>> armies;
    for (const ScenarioEvent* e : ev.god_attacks) {
        const WrathEvents w = god_attack(st, sim.catalog(), e->god);
        sim.emit(GodAttacked{e->god, w.repelled});
        emit_wrath(sim, w);
        if (w.invasion_soldiers > 0) {
            armies.emplace_back(w.invasion_nation, w.invasion_soldiers);
        }
    }
    for (const ScenarioEvent* e : ev.monsters) {
        MonsterEvents m;
        if (spawn_monster(st, sim.catalog(), e->nation, e->amount, e->good, m).empty()) {
            sim.emit(MonsterAppeared{e->nation});
            for (const WalkerId id : m.spawned) {
                sim.emit(WalkerSpawned{id, st.walkers.items.at(id).type});
            }
        }
    }
    for (const ScenarioEvent* e : ev.disasters) {
        emit_wrath(sim, disaster(st, e->nation, {e->x, e->y}, static_cast<std::int32_t>(e->amount)));
    }
    for (const auto& [nation, soldiers] : armies) {  // the Amazons of Artemis, the Sparti of Ares (Z:126)
        CombatEvents combat;
        if (start_invasion(st, sim.catalog(), nation, soldiers, combat).empty()) {
            sim.emit(InvasionStarted{nation, static_cast<std::uint32_t>(combat.spawned.size()), bribe_cost(st, sim.catalog())});
            sim.emit_combat(combat);
        }
    }
    for (const std::string& god : ev.quests_given) {
        sim.emit(QuestGiven{god});
    }
    for (const std::string& god : ev.quests_done) {
        sim.emit(QuestCompleted{god});
    }
    if (ev.lost) {
        sim.emit(EpisodeLost{});
    }
    if (ev.won) {
        sim.emit(EpisodeWon{});
    }
}

// D5: monsters walk, wreck and rest; heroes hunt them.
static void run_monsters(Simulation& sim) {
    MonsterEvents ev;
    advance_monsters(sim.mutable_state(), sim.catalog(), ev);
    for (const BuildingId id : ev.removed) {
        sim.emit(BuildingRemoved{id});
    }
    for (const auto& [monster, by_hero] : ev.slain) {
        sim.emit(MonsterSlain{monster, by_hero});
    }
}

// D3: companies in the field settle, companies at home re-form from the houses.
static void run_military(Simulation& sim) {
    update_companies(sim.mutable_state());
    recover_morale(sim.mutable_state());
    form_companies(sim.mutable_state(), sim.catalog());
    CombatEvents fleet;
    advance_fleet(sim.mutable_state(), sim.catalog(), fleet);  // triremes launched, repaired (Z:141-142)
    advance_auto_defend(sim.mutable_state(), sim.catalog(), fleet);  // the generals and admirals (Z:145)
    sim.emit_combat(fleet);
}

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
        {"trade", 30, &run_trade},
        {"walker_spawn", 31, &run_walker_spawn},
        {"military", 32, &run_military},
        {"mythology", 33, &run_mythology},
        {"world", 34, &run_world},
        {"coverage_decay", 35, &run_coverage_decay},
        {"culture", 35, &run_culture},
        {"scenario", 36, &run_scenario},
        {"monsters", 37, &run_monsters},
    };
    return kSchedule;
}

}  // namespace bo::core
