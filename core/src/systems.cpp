// The daily systems and their schedule (docs/05-SYMULACJA.md): one function per system, one table for the order.
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
#include "bo/core/trade.h"
#include "bo/core/serialize.h"
#include "bo/core/walker.h"
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

// Daily: each staffed building with road access sends its roaming walker when the previous one is back.
static void run_walker_spawn(Simulation& sim) {
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
    allocate_labour(st.city, sim.catalog(), st.difficulty, st.finance.wage_rate, st.labour);
}

// C6: taxes and wages at the start of each month; the yearly record rolls over at the year start.
static void run_finance(Simulation& sim) {
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
    for (const BuildingId id : herd_births(st, sim.catalog())) {
        const Building& b = st.city.buildings.items.at(id);
        sim.emit(BuildingPlaced{b.id, b.type, b.origin.x, b.origin.y, b.width, b.height, b.rotation, b.parent});
    }
}

static void run_logistics(Simulation& sim) {
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
static void run_risks(Simulation& sim) { emit_risks(sim, advance_risks(sim.mutable_state(), sim.catalog())); }

// C9: traders arrive at trade buildings.
static void run_trade(Simulation& sim) {
    const auto ev = dispatch_traders(sim.mutable_state(), sim.catalog());
    for (const WalkerId id : ev.spawned) {
        sim.emit(WalkerSpawned{id, sim.state().walkers.items.at(id).type});
    }
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
        {"coverage_decay", 35, &run_coverage_decay},
    };
    return kSchedule;
}

}  // namespace bo::core
