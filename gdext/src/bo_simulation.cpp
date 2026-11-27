#include "bo_simulation.h"

#include <cstdio>
#include <map>
#include <string>
#include <variant>

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "bo/core/adventure.h"
#include "bo/core/hippodrome.h"
#include "bo/core/migration.h"
#include "bo/core/scenario.h"

using namespace godot;
using bo::core::Difficulty;
using bo::core::TaxRate;
using bo::core::WageRate;

namespace {

std::string to_std(const String& s) { return std::string(s.utf8().get_data()); }

String hex64(std::uint64_t v) {
    char buf[19];
    std::snprintf(buf, sizeof(buf), "0x%016llx", static_cast<unsigned long long>(v));
    return String(buf);
}

template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

Dictionary event_to_dict(const bo::core::TimedEvent& te) {
    Dictionary d;
    d["tick"] = static_cast<int64_t>(te.tick);
    std::visit(Overloaded{
                   [&d](const bo::core::DayStarted& e) {
                       d["type"] = "day_started";
                       d["day"] = static_cast<int64_t>(e.day);
                   },
                   [&d](const bo::core::MonthStarted& e) {
                       d["type"] = "month_started";
                       d["month"] = static_cast<int64_t>(e.month);
                       d["year"] = static_cast<int64_t>(e.year);
                   },
                   [&d](const bo::core::YearStarted& e) {
                       d["type"] = "year_started";
                       d["year"] = static_cast<int64_t>(e.year);
                   },
                   [&d](const bo::core::TaxRateChanged& e) {
                       d["type"] = "tax_rate_changed";
                       d["rate"] = static_cast<int64_t>(e.rate);
                   },
                   [&d](const bo::core::WageRateChanged& e) {
                       d["type"] = "wage_rate_changed";
                       d["rate"] = static_cast<int64_t>(e.rate);
                   },
                   [&d](const bo::core::CommandRejected& e) {
                       d["type"] = "command_rejected";
                       d["reason"] = String(e.reason_key.c_str());
                   },
                   [&d](const bo::core::BuildingPlaced& e) {
                       d["type"] = "building_placed";
                       d["id"] = static_cast<int64_t>(e.id);
                       d["building_type"] = String(e.type.c_str());
                       d["x"] = static_cast<int64_t>(e.x);
                       d["y"] = static_cast<int64_t>(e.y);
                       d["width"] = static_cast<int64_t>(e.width);
                       d["height"] = static_cast<int64_t>(e.height);
                       d["rotation"] = static_cast<int64_t>(e.rotation);
                       d["parent"] = static_cast<int64_t>(e.parent);
                   },
                   [&d](const bo::core::BuildingRemoved& e) {
                       d["type"] = "building_removed";
                       d["id"] = static_cast<int64_t>(e.id);
                   },
                   [&d](const bo::core::TileChanged& e) {
                       d["type"] = "tile_changed";
                       d["x"] = static_cast<int64_t>(e.x);
                       d["y"] = static_cast<int64_t>(e.y);
                   },
                   [&d](const bo::core::WalkerSpawned& e) {
                       d["type"] = "walker_spawned";
                       d["id"] = static_cast<int64_t>(e.id);
                       d["walker_type"] = String(e.type.c_str());
                   },
                   [&d](const bo::core::WalkerRemoved& e) {
                       d["type"] = "walker_removed";
                       d["id"] = static_cast<int64_t>(e.id);
                   },
                   [&d](const bo::core::UndoApplied& e) {
                       d["type"] = "undo_applied";
                       d["refund"] = static_cast<int64_t>(e.refund);
                   },
                   [&d](const bo::core::HouseChanged& e) {
                       d["type"] = "house_changed";
                       d["id"] = static_cast<int64_t>(e.id);
                       d["level"] = static_cast<int64_t>(e.level);
                       d["elite"] = e.elite;
                       d["abandoned"] = e.abandoned;
                   },
                   [&d](const bo::core::Disaster& e) {
                       constexpr const char* kKinds[] = {"fire_started", "burned_down", "collapsed", "plague", "crime"};
                       d["type"] = "disaster";
                       d["kind"] = kKinds[static_cast<int>(e.kind)];
                       d["building"] = static_cast<int64_t>(e.building);
                   },
                   [&d](const bo::core::InvasionStarted& e) {
                       d["type"] = "invasion_started";
                       d["nation"] = String(e.nation.c_str());
                       d["soldiers"] = static_cast<int64_t>(e.soldiers);
                       d["bribe"] = static_cast<int64_t>(e.bribe);
                   },
                   [&d](const bo::core::CityRequestReceived& e) {
                       d["type"] = "city_request";
                       d["request"] = static_cast<int64_t>(e.request);
                       d["city"] = static_cast<int64_t>(e.city);
                       d["good"] = String(e.good.c_str());
                       d["amount"] = static_cast<int64_t>(e.amount);
                   },
                   [&d](const bo::core::RequestClosed& e) {
                       d["type"] = "request_closed";
                       d["city"] = static_cast<int64_t>(e.city);
                       d["fulfilled"] = e.fulfilled;
                   },
                   [&d](const bo::core::EpisodeWon&) { d["type"] = "episode_won"; },
                   [&d](const bo::core::MonsterAppeared& e) {
                       d["type"] = "monster_appeared";
                       d["monster"] = String(e.monster.c_str());
                   },
                   [&d](const bo::core::MonsterSlain& e) {
                       d["type"] = "monster_slain";
                       d["monster"] = String(e.monster.c_str());
                       d["by_hero"] = e.by_hero;
                   },
                   [&d](const bo::core::WalkerPetrified& e) {
                       d["type"] = "walker_petrified";
                       d["monster"] = String(e.monster.c_str());
                       d["walker"] = static_cast<int64_t>(e.walker);
                   },
                   [&d](const bo::core::HeroArrived& e) {
                       d["type"] = "hero_arrived";
                       d["hero"] = String(e.hero.c_str());
                   },
                   [&d](const bo::core::QuestGiven& e) {
                       d["type"] = "quest_given";
                       d["god"] = String(e.god.c_str());
                   },
                   [&d](const bo::core::QuestCompleted& e) {
                       d["type"] = "quest_completed";
                       d["god"] = String(e.god.c_str());
                   },
                   [&d](const bo::core::GodAttacked& e) {
                       d["type"] = "god_attacked";
                       d["god"] = String(e.god.c_str());
                       d["repelled"] = e.repelled;
                   },
                   [&d](const bo::core::GodBlessing& e) {  // L-131
                       d["type"] = "god_blessing";
                       d["god"] = String(e.god.c_str());
                       d["kind"] = String(e.kind.c_str());
                   },
                   [&d](const bo::core::GodHelped& e) {  // Z:119-122
                       d["type"] = "god_helped";
                       d["god"] = String(e.god.c_str());
                   },
                   [&d](const bo::core::TributePaid& e) {
                       d["type"] = "tribute_paid";
                       d["city"] = static_cast<int64_t>(e.city);
                       d["amount"] = static_cast<int64_t>(e.amount);
                   },
                   [&d](const bo::core::ExpeditionReturned& e) {
                       d["type"] = "expedition_returned";
                       d["city"] = static_cast<int64_t>(e.city);
                       d["won"] = e.won;
                       d["losses"] = static_cast<int64_t>(e.losses);
                       d["loot"] = static_cast<int64_t>(e.loot);
                   },
                   [&d](const bo::core::EpisodeLost&) { d["type"] = "episode_lost"; },
                   [&d](const bo::core::CityRebelled& e) {
                       d["type"] = "city_rebelled";
                       d["city"] = static_cast<int64_t>(e.city);
                   },
                   [&d](const bo::core::OracleSpoke& e) {  // Z:124
                       d["type"] = "oracle_spoke";
                       d["building"] = static_cast<int64_t>(e.building);
                       d["kind"] = String(e.kind.c_str());
                       d["subject"] = String(e.subject.c_str());
                       d["months"] = static_cast<int64_t>(e.months);
                   },
                   [&d](const bo::core::MilitaryHelpGiven& e) {  // Z:174
                       d["type"] = "military_help";
                       d["city"] = static_cast<int64_t>(e.city);
                       d["strike"] = e.strike;
                   },
                   [&d](const bo::core::CityGift& e) {
                       d["type"] = "city_gift";
                       d["city"] = static_cast<int64_t>(e.city);
                       d["amount"] = e.amount;
                   },
                   [&d](const bo::core::GamesHeld& e) {
                       d["type"] = "games_held";
                       d["game"] = String(e.game.c_str());
                       d["chance"] = static_cast<int64_t>(e.chance);
                       d["invited"] = e.invited;
                       d["won"] = e.won;
                       d["hosted"] = e.hosted;
                       d["right_lost"] = e.right_lost;
                   },
                   [&d](const bo::core::DebtNotice& e) {
                       d["type"] = "debt_notice";
                       d["years"] = static_cast<int64_t>(e.years);
                   },
                   [&d](const bo::core::PrayerAnswered& e) {
                       d["type"] = "prayer_answered";
                       d["building"] = static_cast<int64_t>(e.building);
                   },
                   [&d](const bo::core::SanctuaryCompleted& e) {
                       d["type"] = "sanctuary_completed";
                       d["building"] = static_cast<int64_t>(e.building);
                   },
                   [&d](const bo::core::InvasionEnded& e) {
                       d["type"] = "invasion_ended";
                       d["result"] = static_cast<int64_t>(e.result);
                   },
                   [&d](const bo::core::TradeDone& e) {
                       d["type"] = "trade_done";
                       d["building"] = static_cast<int64_t>(e.building);
                       d["bought"] = static_cast<int64_t>(e.loads_bought);
                       d["sold"] = static_cast<int64_t>(e.loads_sold);
                       d["money"] = static_cast<int64_t>(e.money);
                   },
                   [&d](const bo::core::PeopleMoved& e) {
                       d["type"] = "people_moved";
                       d["arrived"] = static_cast<int64_t>(e.arrived);
                       d["left"] = static_cast<int64_t>(e.left);
                   },
                   [&d](const bo::core::MonthlyFinance& e) {
                       d["type"] = "monthly_finance";
                       d["taxes"] = static_cast<int64_t>(e.taxes);
                       d["wages"] = static_cast<int64_t>(e.wages);
                   },
               },
               te.event);
    return d;
}

}  // namespace

String BoSimulation::load_catalog(const String& data_dir) {
    std::map<std::string, std::string> files;
    const PackedStringArray names = DirAccess::get_files_at(data_dir);
    for (int64_t i = 0; i < names.size(); ++i) {
        const String name = names[i];
        if (!name.ends_with(".json")) {
            continue;
        }
        const PackedByteArray bytes = FileAccess::get_file_as_bytes(data_dir.path_join(name));
        files.emplace(to_std(name), std::string(reinterpret_cast<const char*>(bytes.ptr()),
                                                static_cast<std::size_t>(bytes.size())));
    }
    auto result = bo::core::load_data_catalog_from_memory(files);
    if (!result.catalog) {
        String errors;
        for (const auto& e : result.errors) {
            errors += String(e.c_str()) + "\n";
        }
        return errors.is_empty() ? String("no data files in ") + data_dir : errors;
    }
    catalog_ = std::make_unique<bo::core::DataCatalog>(std::move(*result.catalog));
    return String();
}

String BoSimulation::start(const String& data_dir, int64_t seed, const String& difficulty_code,
                           int64_t initial_treasury) {
    sim_.reset();
    const String err = load_catalog(data_dir);
    if (!err.is_empty()) {
        return err;
    }
    bo::core::SimulationConfig cfg;
    cfg.seed = static_cast<std::uint64_t>(seed);
    cfg.initial_treasury = initial_treasury;
    if (!difficulty_code.is_empty()) {
        cfg.difficulty = bo::core::difficulty_from_code(to_std(difficulty_code));
        if (!cfg.difficulty) {
            return String("unknown difficulty code: ") + difficulty_code;
        }
    }
    sim_.emplace(*catalog_, cfg);
    return String();
}

String BoSimulation::start_scenario(const String& data_dir, const String& scenario_path, bool full_staffing) {
    sim_.reset();
    const String err = load_catalog(data_dir);
    if (!err.is_empty()) {
        return err;
    }
    const String text = FileAccess::get_file_as_string(scenario_path);
    if (text.is_empty()) {
        return String("cannot read scenario: ") + scenario_path;
    }
    std::string error;
    auto cfg = bo::core::parse_scenario(to_std(text), error);
    if (!cfg) {
        return String(error.c_str());
    }
    cfg->full_staffing = full_staffing;
    sim_.emplace(*catalog_, *cfg);
    if (!sim_->setup_error().empty()) {
        const String e(sim_->setup_error().c_str());
        sim_.reset();
        return e;
    }
    return String();
}

String BoSimulation::load_build_order(const String& path) {
    std::string error;
    auto order = bo::core::parse_build_order(to_std(FileAccess::get_file_as_string(path)), error);
    if (!order) {
        return String(error.c_str());
    }
    build_order_ = std::move(*order);
    build_order_day_.reset();
    if (sim_ && sim_->state().time.tick() > 0) {
        build_order_day_ = sim_->state().time.total_days();  // a loaded game: only the days still to come
    }
    return String();
}

// Issues the commands due on every day since the last call, up to today. Returns how many.
int64_t BoSimulation::apply_build_order() {
    if (!sim_ || !build_order_) {
        return 0;
    }
    const std::uint64_t today = sim_->state().time.total_days();
    std::uint64_t day = build_order_day_ ? *build_order_day_ + 1 : 0;
    int64_t n = 0;
    for (; day <= today; ++day) {
        for (auto& c : bo::core::due_commands(*build_order_, *sim_, day)) {
            sim_->issue(std::move(c));
            ++n;
        }
        for (auto& c : bo::core::due_commands(*build_order_, *sim_, day, true)) {  // a tick later (BuildStep::later)
            sim_->issue_at(sim_->state().time.tick() + 1, c);
            ++n;
        }
    }
    build_order_day_ = today;
    return n;
}

String BoSimulation::start_adventure(const String& data_dir, const String& path) {
    sim_.reset();
    adventure_.reset();
    const String err = load_catalog(data_dir);
    if (!err.is_empty()) {
        return err;
    }
    std::string error;
    adventure_text_ = to_std(FileAccess::get_file_as_string(path));
    auto a = bo::core::parse_adventure(adventure_text_, error);
    if (!a) {
        return String(error.c_str());
    }
    adventure_ = std::move(*a);
    sim_.emplace(bo::core::begin_adventure(*catalog_, *adventure_, run_));
    return String();
}

String BoSimulation::continue_adventure() {
    if (!sim_ || !adventure_ || !sim_->state().scenario.won) {
        return "not_won";
    }
    switch (bo::core::advance_adventure(*adventure_, run_, sim_)) {
        case bo::core::NextEpisode::ParentEpisode:
        case bo::core::NextEpisode::BackToParent:
            return "parent";
        case bo::core::NextEpisode::ChooseColony:
            return "choose_colony";
        case bo::core::NextEpisode::Victory:
            return "victory";
    }
    return "victory";
}

String BoSimulation::found_colony(const String& colony) {
    if (!sim_ || !adventure_) {
        return "no_adventure";
    }
    return String(bo::core::found_colony(*catalog_, *adventure_, run_, to_std(colony), sim_).c_str());
}

Dictionary BoSimulation::get_adventure() const {
    Dictionary d;
    if (!adventure_) {
        return d;
    }
    PackedStringArray choices;
    for (const auto& c : bo::core::colony_choices(*adventure_, run_)) {
        choices.append(String(c.c_str()));
    }
    d["id"] = String(adventure_->id.c_str());
    d["step"] = static_cast<int64_t>(run_.step);
    d["steps"] = static_cast<int64_t>(adventure_->steps.size());
    d["kind"] = run_.step < adventure_->steps.size() ? String(adventure_->steps[run_.step].kind.c_str()) : String("victory");
    d["in_colony"] = run_.in_colony;
    d["colony_choices"] = choices;
    return d;
}

void BoSimulation::fulfill_request(int64_t request) {
    if (sim_ && request >= 0) {
        sim_->issue(bo::core::FulfillRequest{static_cast<std::uint32_t>(request)});
    }
}

Dictionary BoSimulation::get_scenario() const {
    Dictionary d;
    if (!sim_) {
        return d;
    }
    const auto& s = sim_->state().scenario;
    Array goals;
    for (const auto& g : s.goals) {
        Dictionary x;
        x["type"] = String(g.type.c_str());
        x["target"] = String(g.target.c_str());
        x["value"] = g.value;
        x["met"] = g.met;
        goals.append(x);
    }
    Array requests;
    const std::uint64_t today = sim_->state().time.tick() / bo::core::kTicksPerDay;
    for (const auto& r : s.requests) {
        Dictionary x;
        x["city"] = static_cast<int64_t>(r.city);
        x["good"] = String(r.good.c_str());
        x["amount"] = static_cast<int64_t>(r.amount);
        x["days_left"] = static_cast<int64_t>(r.due_day) - static_cast<int64_t>(today);  // < 0: overdue
        requests.append(x);
    }
    Array quests;
    for (const auto& q : s.quests) {
        Dictionary x;
        x["god"] = String(q.god.c_str());
        x["index"] = static_cast<int64_t>(q.index);
        x["hero"] = String(q.hero.c_str());
        x["status"] = static_cast<int64_t>(q.status);  // 0 open, 1 underway, 2 done
        x["days_left"] = static_cast<int64_t>(q.days_left);
        quests.append(x);
    }
    Dictionary monuments;
    for (const auto& [kind, n] : s.monuments) {
        monuments[String(kind.c_str())] = static_cast<int64_t>(n);
    }
    d["quests"] = quests;
    d["monuments"] = monuments;
    d["id"] = String(s.id.c_str());
    d["goals"] = goals;
    d["requests"] = requests;
    d["won"] = s.won;
    d["atlantean"] = sim_->state().city.atlantean;  // the city's culture (P:16-17)
    d["lost"] = sim_->state().episode_lost;
    d["towers_manned"] = sim_->state().towers_manned;
    d["auto_defend"] = sim_->state().auto_defend;  // Z:145
    d["games_host_year"] = sim_->state().games_host_year;  // -1: no right to host (Z:111)
    const auto track = bo::core::hippodrome_track(sim_->state().city);  // P:13-15
    d["hippodrome_stades"] = static_cast<int64_t>(track.segments.size());
    d["hippodrome_closed"] = track.closed;
    d["hippodrome_horses"] = static_cast<int64_t>(sim_->state().hippodrome_horses);
    d["hippodrome_this_year"] = sim_->state().hippodrome_this_year;  // bets and snacks (P:14)
    d["hippodrome_last_year"] = sim_->state().hippodrome_last_year;
    d["hippodrome_working"] = static_cast<int64_t>(bo::core::working_hippodrome(sim_->state(), *catalog_));
    const auto year = static_cast<std::uint32_t>(sim_->state().time.year());
    for (const auto& g : catalog_->games.games) {  // this year's Panhellenic games and the city's chance (L-123)
        if (g.year_mod_4 == year % 4) {
            d["games"] = String(g.id.c_str());
            d["games_chance"] = static_cast<int64_t>(bo::core::games_chance(sim_->state(), *catalog_, g));
        }
    }
    d["debt_days"] = static_cast<int64_t>(sim_->state().debt_days);
    return d;
}

String BoSimulation::start_with_map(const String& data_dir, int64_t seed, const String& difficulty_code,
                                    int64_t initial_treasury, const PackedStringArray& rows, bool full_staffing,
                                    const PackedStringArray& elevation, const Array& trade_partners) {
    const String err = start(data_dir, seed, difficulty_code, initial_treasury);
    if (!err.is_empty()) {
        return err;
    }
    bo::core::SimulationConfig cfg;
    cfg.seed = static_cast<std::uint64_t>(seed);
    cfg.initial_treasury = initial_treasury;
    cfg.difficulty = sim_->state().difficulty;
    cfg.full_staffing = full_staffing;
    for (int64_t i = 0; i < rows.size(); ++i) {
        cfg.text_map.push_back(to_std(rows[i]));
    }
    for (int64_t i = 0; i < elevation.size(); ++i) {
        cfg.elevation_map.push_back(to_std(elevation[i]));
    }
    // Partners: {"id", "relation", "by_sea", "sells": {good: [price, yearly_limit]}, "buys": {...},
    // "opinion", "military", "economy", "tribute"}
    for (int64_t i = 0; i < trade_partners.size(); ++i) {
        const Dictionary d = trade_partners[i];
        bo::core::TradePartner p;
        p.id = to_std(d.get("id", ""));
        p.relation = to_std(d.get("relation", "ally"));
        p.by_sea = d.get("by_sea", false);
        p.diplomacy.opinion = static_cast<std::int32_t>(int64_t(d.get("opinion", 60)));  // D4
        p.diplomacy.military = static_cast<std::int32_t>(int64_t(d.get("military", 1)));
        p.diplomacy.economy = static_cast<std::int32_t>(int64_t(d.get("economy", 1)));
        p.diplomacy.tribute = int64_t(d.get("tribute", 0));
        for (const char* key : {"sells", "buys"}) {
            const Dictionary offers = d.get(key, Dictionary());
            const Array goods = offers.keys();
            for (int64_t g = 0; g < goods.size(); ++g) {
                const Array v = offers[goods[g]];
                bo::core::TradeOffer o{static_cast<std::int32_t>(int64_t(v[0])), static_cast<std::uint32_t>(int64_t(v[1])), 0};
                (std::string(key) == "sells" ? p.sells : p.buys)[to_std(goods[g])] = o;
            }
        }
        cfg.trade_partners.push_back(p);
    }
    sim_.emplace(*catalog_, cfg);
    if (!sim_->setup_error().empty()) {
        const String e(sim_->setup_error().c_str());
        sim_.reset();
        return e;
    }
    return String();
}

namespace {
bo::core::TilePos pos(int64_t x, int64_t y) { return {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)}; }
}  // namespace

void BoSimulation::place_building(const String& type, int64_t x, int64_t y, int64_t rotation) {
    if (sim_ && rotation >= 0 && rotation <= 3) {
        const auto p = pos(x, y);
        sim_->issue(bo::core::PlaceBuilding{to_std(type), p.x, p.y, static_cast<std::uint8_t>(rotation)});
    }
}

void BoSimulation::undo() {
    if (sim_) {
        sim_->issue(bo::core::Undo{});
    }
}

void BoSimulation::set_sector_priority(int64_t sector, int64_t priority) {
    if (sim_ && sector >= 0 && sector < 256 && priority >= 0 && priority < 256) {
        sim_->issue(bo::core::SetSectorPriority{static_cast<std::uint8_t>(sector), static_cast<std::uint8_t>(priority)});
    }
}

void BoSimulation::set_industry_enabled(const String& type, bool enabled) {
    if (sim_) {
        sim_->issue(bo::core::SetIndustryEnabled{to_std(type), enabled});
    }
}

int64_t BoSimulation::get_population() const {
    return sim_ ? static_cast<int64_t>(bo::core::city_population(sim_->state().city)) : 0;
}
int64_t BoSimulation::get_popularity() const { return sim_ ? sim_->state().migration.popularity : 0; }
int64_t BoSimulation::get_homeless() const { return sim_ ? sim_->state().city.homeless : 0; }

void BoSimulation::set_storage_order(int64_t building, const String& good, int64_t kind, int64_t limit) {
    if (sim_ && building > 0 && kind >= 0 && kind <= 3 && limit >= 0) {
        sim_->issue(bo::core::SetStorageOrder{static_cast<std::uint32_t>(building), to_std(good),
                                              static_cast<std::uint8_t>(kind), static_cast<std::uint32_t>(limit)});
    }
}

void BoSimulation::set_stockpile(const String& good, bool on) {
    if (sim_) {
        sim_->issue(bo::core::SetStockpile{to_std(good), on});
    }
}

void BoSimulation::set_not_buying(int64_t stall, bool on) {
    if (sim_ && stall > 0) {
        sim_->issue(bo::core::SetNotBuying{static_cast<std::uint32_t>(stall), on});
    }
}

void BoSimulation::set_company_tactic(int64_t company, int64_t tactic) {
    if (sim_ && company >= 0 && tactic >= 0 && tactic <= 2) {
        sim_->issue(bo::core::SetCompanyTactic{static_cast<std::uint32_t>(company), static_cast<std::uint8_t>(tactic)});
    }
}

Array BoSimulation::get_ships() const {
    Array out;
    if (!sim_) {
        return out;
    }
    const auto& player = catalog_->combat.nations.at("player_greek").units;
    for (const auto& sh : sim_->state().military.ships) {
        const auto it = sim_->state().walkers.items.find(sh.walker);  // none while abroad (Z:177)
        const std::int64_t full = player.count("trireme") != 0 ? player.at("trireme").hp : 0;
        Dictionary d;
        d["order"] = static_cast<int64_t>(sh.order);
        d["hp"] = it != sim_->state().walkers.items.end() ? static_cast<int64_t>(it->second.hp) : full;
        d["max_hp"] = full;
        d["x"] = it != sim_->state().walkers.items.end() ? it->second.tile.x : -1;
        d["y"] = it != sim_->state().walkers.items.end() ? it->second.tile.y : -1;
        d["station_x"] = sh.station.x;
        d["station_y"] = sh.station.y;
        d["crew"] = static_cast<int64_t>(sh.crew);  // 0 unmanned, 1 aboard, 2 going home (Z:141, Z:150)
        d["hands"] = static_cast<int64_t>(sh.hands);  // a short-handed ship is slower (Z:141)
        d["places"] = static_cast<int64_t>(catalog_->naval.crew_workers);
        out.append(d);
    }
    return out;
}

void BoSimulation::set_ship_order(int64_t ship, int64_t order, int64_t x, int64_t y) {
    if (sim_ && ship >= 0 && order >= 0 && order <= 5) {
        sim_->issue(bo::core::SetShipOrder{static_cast<std::uint32_t>(ship), static_cast<std::uint8_t>(order),
                                           static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)});
    }
}

void BoSimulation::request_military_help(int64_t city, bool strike, int64_t target) {
    if (sim_ && city >= 0 && target >= 0) {
        sim_->issue(bo::core::RequestMilitaryHelp{static_cast<std::uint32_t>(city), strike, static_cast<std::uint32_t>(target)});
    }
}

void BoSimulation::consult_oracle(int64_t building) {
    if (sim_ && building > 0) {
        sim_->issue(bo::core::ConsultOracle{static_cast<std::uint32_t>(building)});
    }
}

void BoSimulation::man_towers(bool on) {
    if (sim_) {
        sim_->issue(bo::core::ManTowers{on});
    }
}

void BoSimulation::set_construction_paused(int64_t building, bool on) {
    if (sim_) {
        sim_->issue(bo::core::SetConstructionPaused{static_cast<std::uint32_t>(building), on});
    }
}

void BoSimulation::set_auto_defend(bool on) {
    if (sim_) {
        sim_->issue(bo::core::SetAutoDefend{on});
    }
}

PackedStringArray BoSimulation::get_stockpiled() const {
    PackedStringArray out;
    if (sim_) {
        for (const std::string& g : sim_->state().stockpiled) {
            out.append(String(g.c_str()));
        }
    }
    return out;
}

void BoSimulation::muster_company(int64_t company) {
    if (sim_ && company >= 0) {
        sim_->issue(bo::core::MusterCompany{static_cast<std::uint32_t>(company)});
    }
}

void BoSimulation::move_company(int64_t company, int64_t x, int64_t y) {
    if (sim_ && company >= 0) {
        sim_->issue(bo::core::MoveCompany{static_cast<std::uint32_t>(company), static_cast<std::int32_t>(x),
                                          static_cast<std::int32_t>(y)});
    }
}

void BoSimulation::start_invasion(const String& nation, int64_t soldiers, int64_t city) {
    if (sim_ && soldiers > 0) {
        sim_->issue(bo::core::StartInvasion{to_std(nation), static_cast<std::uint32_t>(soldiers),
                                            static_cast<std::int32_t>(city)});
    }
}

void BoSimulation::give_gift(int64_t city, const String& good, int64_t amount) {
    if (sim_ && city >= 0 && amount > 0) {
        sim_->issue(bo::core::GiftToCity{static_cast<std::uint32_t>(city), to_std(good), static_cast<std::uint32_t>(amount)});
    }
}

void BoSimulation::request_from_city(int64_t city, const String& good) {
    if (sim_ && city >= 0) {
        sim_->issue(bo::core::RequestFromCity{static_cast<std::uint32_t>(city), to_std(good)});
    }
}

void BoSimulation::dispatch_army(int64_t city, bool raid, const String& good, const Array& companies, bool ships) {
    if (!sim_ || city < 0) {
        return;
    }
    bo::core::DispatchArmy c{static_cast<std::uint32_t>(city), raid, to_std(good), {}, ships};
    for (int64_t i = 0; i < companies.size(); ++i) {
        c.companies.push_back(static_cast<std::uint32_t>(int64_t(companies[i])));
    }
    sim_->issue(c);
}

// Armies abroad (D4) for the world panel: where, what for, days to go.
Array BoSimulation::get_expeditions() const {
    Array out;
    if (!sim_) {
        return out;
    }
    for (const auto& e : sim_->state().expeditions) {
        Dictionary d;
        d["city"] = static_cast<int64_t>(e.city);
        d["raid"] = e.raid;
        d["aid"] = e.aid;
        d["returning"] = e.returning;
        d["days_left"] = static_cast<int64_t>(e.days_left);
        d["companies"] = static_cast<int64_t>(e.companies.size());
        out.append(d);
    }
    return out;
}

// The invasion now in the city (Z:137): for the army panel.
Dictionary BoSimulation::get_invasion() const {
    Dictionary d;
    if (!sim_) {
        return d;
    }
    const auto& inv = sim_->state().military.invasion;
    d["active"] = inv.active;
    d["nation"] = String(inv.nation.c_str());
    d["invaders"] = static_cast<int64_t>(inv.invaders.size());
    d["by_sea"] = inv.by_sea;  // Z:144
    d["city"] = static_cast<int64_t>(inv.city);
    d["bribe"] = bo::core::bribe_cost(sim_->state(), *catalog_);
    return d;
}

Array BoSimulation::get_cities() const {
    Array out;
    if (!sim_) {
        return out;
    }
    for (const auto& p : sim_->state().trade.partners) {
        Dictionary d;
        d["id"] = String(p.id.c_str());
        d["relation"] = String(p.relation.c_str());
        d["opinion"] = p.diplomacy.opinion;
        d["military"] = p.diplomacy.military;
        d["economy"] = p.diplomacy.economy;
        d["by_sea"] = p.by_sea;  // triremes can sail there (Z:123)
        d["tribute"] = p.diplomacy.tribute;
        d["tribute_demand"] = p.diplomacy.tribute_demand;
        d["rebelling"] = p.diplomacy.rebelling;
        d["by_sea"] = p.by_sea;
        PackedStringArray sells;
        for (const auto& [good, offer] : p.sells) {
            sells.append(String(good.c_str()));
        }
        d["sells"] = sells;
        out.append(d);
    }
    return out;
}

Dictionary BoSimulation::check_placement(const String& type, int64_t x, int64_t y, int64_t rotation) const {
    Dictionary d;
    if (!sim_) {
        return d;
    }
    // A trial on a copy of the city: the same rules as the command, nothing changes (E7 ghost preview).
    bo::core::CityState copy = sim_->state().city;
    const auto r = bo::core::place_building(copy, *catalog_, sim_->state().difficulty, to_std(type),
                                            {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)},
                                            static_cast<std::uint8_t>(rotation & 3));
    d["ok"] = r.ok();
    d["reason"] = String(r.reason.c_str());
    d["cost"] = r.cost;
    PackedVector2Array tiles;
    if (r.ok()) {
        const auto& b = copy.buildings.items.at(r.building);
        for (const auto& t : b.tiles) {
            tiles.append(Vector2(static_cast<float>(t.x), static_cast<float>(t.y)));
        }
    } else {
        const auto bt = catalog_->buildings.find(to_std(type));
        const auto st = bt == catalog_->buildings.end() ? catalog_->building_stats.end()
                                                         : catalog_->building_stats.find(bt->second.stats_id);
        std::int32_t w = 1;
        std::int32_t h = 1;
        if (st != catalog_->building_stats.end() && st->second.footprint) {
            w = st->second.footprint->width;
            h = st->second.footprint->height;
            if (rotation % 2 == 1) {
                std::swap(w, h);
            }
        }
        for (std::int32_t dy = 0; dy < h; ++dy) {
            for (std::int32_t dx = 0; dx < w; ++dx) {
                tiles.append(Vector2(static_cast<float>(x + dx), static_cast<float>(y + dy)));
            }
        }
    }
    d["tiles"] = tiles;
    return d;
}

void BoSimulation::send_hero_on_quest(int64_t quest) {
    if (sim_ && quest >= 0) {
        sim_->issue(bo::core::SendHeroOnQuest{static_cast<std::uint32_t>(quest)});
    }
}

void BoSimulation::summon_hero(int64_t hall) {
    if (sim_ && hall > 0) {
        sim_->issue(bo::core::SummonHero{static_cast<std::uint32_t>(hall)});
    }
}

Array BoSimulation::get_monsters() const {
    Array out;
    if (!sim_) {
        return out;
    }
    for (const auto& m : sim_->state().monsters) {
        Dictionary d;
        d["monster"] = String(m.id.c_str());
        d["slain"] = m.slain;
        d["resting"] = m.resting;
        const auto slayer = catalog_->monster_slayer.find(m.id);
        d["hero"] = String(slayer == catalog_->monster_slayer.end() ? "" : slayer->second.c_str());
        out.append(d);
    }
    return out;
}

void BoSimulation::pray(int64_t sanctuary) {
    if (sim_ && sanctuary > 0) {
        sim_->issue(bo::core::Pray{static_cast<std::uint32_t>(sanctuary)});
    }
}

void BoSimulation::bribe_invaders() {
    if (sim_) {
        sim_->issue(bo::core::BribeInvaders{});
    }
}

void BoSimulation::surrender() {
    if (sim_) {
        sim_->issue(bo::core::Surrender{});
    }
}

int64_t BoSimulation::get_bribe_cost() const { return sim_ ? bo::core::bribe_cost(sim_->state(), *catalog_) : 0; }

void BoSimulation::send_company_home(int64_t company) {
    if (sim_ && company >= 0) {
        sim_->issue(bo::core::SendCompanyHome{static_cast<std::uint32_t>(company)});
    }
}

Array BoSimulation::get_companies() const {
    Array out;
    if (!sim_) {
        return out;
    }
    for (const auto& c : sim_->state().military.companies) {
        int64_t soldiers = 0;
        for (const auto& m : c.members) {
            soldiers += m.soldiers;
        }
        Dictionary d;
        d["unit"] = String(c.unit.c_str());
        d["nation"] = String(c.nation.c_str());  // a god's company (Z:140): "divine", "amazon"
        d["fled"] = c.fled;
        d["tactic"] = static_cast<int64_t>(c.tactic);
        d["in_field"] = static_cast<int64_t>(c.soldiers.size());
        d["soldiers"] = soldiers;
        d["status"] = static_cast<int64_t>(c.status);
        d["morale"] = static_cast<int64_t>(c.morale);
        d["x"] = c.standard.x;
        d["y"] = c.standard.y;
        out.append(d);
    }
    return out;
}

void BoSimulation::set_trade_partner(int64_t building, int64_t partner) {
    if (sim_ && building > 0) {
        sim_->issue(bo::core::SetTradePartner{static_cast<std::uint32_t>(building), static_cast<std::int32_t>(partner)});
    }
}

void BoSimulation::set_trade_order(int64_t building, const String& good, bool import_on, int64_t import_limit,
                                   bool export_on, int64_t export_keep) {
    if (sim_ && building > 0 && import_limit >= 0 && export_keep >= 0) {
        sim_->issue(bo::core::SetTradeOrder{static_cast<std::uint32_t>(building), to_std(good), import_on,
                                            static_cast<std::uint32_t>(import_limit), export_on,
                                            static_cast<std::uint32_t>(export_keep)});
    }
}

int64_t BoSimulation::get_workforce() const { return sim_ ? sim_->state().labour.workforce : 0; }
int64_t BoSimulation::get_employed() const { return sim_ ? sim_->state().labour.employed : 0; }

int64_t BoSimulation::get_appeal(int64_t x, int64_t y) const {
    if (!sim_) {
        return 0;
    }
    const auto& city = sim_->state().city;
    const auto p = pos(x, y);
    if (!city.map.in_bounds(p) || city.appeal.empty()) {
        return 0;
    }
    return city.appeal[city.map.index(p)];
}

void BoSimulation::place_road(int64_t x, int64_t y) {
    if (sim_) {
        sim_->issue(bo::core::PlaceRoad{pos(x, y).x, pos(x, y).y});
    }
}

void BoSimulation::place_roadblock(int64_t x, int64_t y) {
    if (sim_) {
        sim_->issue(bo::core::PlaceRoadblock{pos(x, y).x, pos(x, y).y});
    }
}

void BoSimulation::clear_land(int64_t x, int64_t y) {
    if (sim_) {
        sim_->issue(bo::core::ClearLand{pos(x, y).x, pos(x, y).y});
    }
}

void BoSimulation::demolish(int64_t x, int64_t y) {
    if (sim_) {
        sim_->issue(bo::core::Demolish{pos(x, y).x, pos(x, y).y});
    }
}

int64_t BoSimulation::get_map_width() const { return sim_ ? sim_->state().city.map.width() : 0; }
int64_t BoSimulation::get_map_height() const { return sim_ ? sim_->state().city.map.height() : 0; }

Dictionary BoSimulation::get_tile(int64_t x, int64_t y) const {
    Dictionary d;
    const auto p = pos(x, y);
    if (!sim_ || !sim_->state().city.map.in_bounds(p)) {
        return d;
    }
    const auto& t = sim_->state().city.map.at(p);
    d["terrain"] = String(std::string(bo::core::terrain_id(t.terrain)).c_str());
    d["elevation"] = static_cast<int64_t>(t.elevation);
    d["ramp"] = static_cast<int64_t>(t.ramp);  // 0 none, 1 + direction up (N, E, S, W)
    d["road"] = t.road;
    d["roadblock"] = t.roadblock;
    d["building"] = static_cast<int64_t>(t.building);
    return d;
}

Array BoSimulation::get_buildings() const {
    Array out;
    if (!sim_) {
        return out;
    }
    for (const auto& [id, b] : sim_->state().city.buildings.items) {
        Dictionary d;
        d["id"] = static_cast<int64_t>(id);
        d["type"] = String(b.type.c_str());
        d["x"] = static_cast<int64_t>(b.origin.x);
        d["y"] = static_cast<int64_t>(b.origin.y);
        d["width"] = static_cast<int64_t>(b.width);
        d["height"] = static_cast<int64_t>(b.height);
        d["rotation"] = static_cast<int64_t>(b.rotation);
        d["parent"] = static_cast<int64_t>(b.parent);
        d["has_road_access"] = b.road_access.has_value();
        d["workers"] = static_cast<int64_t>(b.workers);
        d["burning"] = b.burning_days > 0;
        d["fire_risk"] = static_cast<int64_t>(b.fire_counter);      // of 960 (data/risks.json)
        d["damage_risk"] = static_cast<int64_t>(b.damage_counter);
        d["disease_risk"] = static_cast<int64_t>(b.disease_counter);
        d["crime_risk"] = static_cast<int64_t>(b.crime_counter);
        d["staffing"] = static_cast<int64_t>(b.staffing_percent);
        if (b.stall) {
            d["not_buying"] = b.stall->not_buying;
        }
        d["sanctified"] = b.sanctified;
        d["cursed_days"] = static_cast<int64_t>(b.cursed_days);
        d["performing_days"] = static_cast<int64_t>(b.performing_days);
        d["hits"] = static_cast<int64_t>(b.hits);  // walls, gates, towers (D3)
        if (b.sanctuary) {  // D1: loads still to build in, delivered loads, sacrifices
            int64_t needed = 0;
            int64_t on_site = 0;
            for (const auto& [g, n] : b.sanctuary->needed) {
                needed += n;
            }
            for (const auto& [g, n] : b.sanctuary->on_site) {
                on_site += n;
            }
            d["sanctuary_complete"] = b.sanctuary->complete;
            d["monument"] = catalog_->sanctuaries.count(b.type) != 0 && catalog_->sanctuaries.at(b.type).monument;
            d["sanctuary_loads_needed"] = needed;
            int64_t total = 0;  // every load the site takes (gods.json, pyramids.json)
            const auto params = catalog_->sanctuaries.find(b.type);
            if (params != catalog_->sanctuaries.end()) {
                for (const auto& [g, n] : params->second.materials) {
                    total += n;
                }
                d["monument_levels"] = static_cast<int64_t>(params->second.levels);
            }
            d["sanctuary_loads_total"] = total;
            d["construction_paused"] = b.sanctuary->paused;  // P:26-28
            d["sanctuary_loads_on_site"] = on_site;
            d["days_since_sacrifice"] = static_cast<int64_t>(b.sanctuary->days_since_sacrifice);
            d["favor_lost"] = b.sanctuary->favor_lost;
            d["prayer_cooldown"] = static_cast<int64_t>(b.sanctuary->prayer_cooldown);
            d["prayer_chance"] = static_cast<int64_t>(bo::core::prayer_chance(*b.sanctuary, *catalog_));
            const auto sp = catalog_->sanctuaries.find(b.type);
            d["oracle"] = sp != catalog_->sanctuaries.end() && sp->second.oracle;  // Zeus, Apollo (Z:124)
            d["oracle_silent"] = static_cast<int64_t>(b.sanctuary->oracle_silent);
        }
        if (b.house) {
            d["house_level"] = static_cast<int64_t>(b.house->level);
            d["population"] = static_cast<int64_t>(b.house->population);
            d["elite"] = b.house->elite;
            Array goods;
            for (const auto g : b.house->goods) {
                goods.push_back(static_cast<int64_t>(g));
            }
            d["house_goods"] = goods;  // food, fleece, olive oil, wine, armor, horses (units)
        }
        auto counts = [](const std::map<std::string, std::uint32_t>& m) {
            Dictionary out;
            for (const auto& [k, v] : m) {
                out[String(k.c_str())] = static_cast<int64_t>(v);
            }
            return out;
        };
        if (b.storage) {
            d["stock_loads"] = counts(b.storage->loads);
        }
        if (b.stall) {
            d["stall_units"] = counts(b.stall->units);
        }
        if (b.producer) {
            d["output_loads"] = static_cast<int64_t>(b.producer->output_loads);
            d["input_loads"] = static_cast<int64_t>(b.producer->input_loads);
            d["fruit_units"] = counts(b.producer->units);
        }
        out.push_back(d);
    }
    return out;
}

Array BoSimulation::get_walkers() const {
    Array out;
    if (!sim_) {
        return out;
    }
    constexpr double kStepsPerTileRaw = 15.0 * 65536.0;  // presentation only: floats never enter the core
    for (const auto& [id, w] : sim_->state().walkers.items) {
        Dictionary d;
        d["id"] = static_cast<int64_t>(id);
        d["type"] = String(w.type.c_str());
        d["x"] = static_cast<int64_t>(w.tile.x);
        d["y"] = static_cast<int64_t>(w.tile.y);
        d["dir"] = static_cast<int64_t>(w.dir);
        d["phase"] = static_cast<int64_t>(w.phase);
        d["progress"] = static_cast<double>(w.step_accum_raw) / kStepsPerTileRaw;
        out.push_back(d);
    }
    return out;
}

void BoSimulation::step(int64_t ticks) {
    if (sim_ && ticks > 0) {
        sim_->step(static_cast<std::uint64_t>(ticks));
    }
}

void BoSimulation::run_days(int64_t days) {
    if (sim_ && days > 0) {
        sim_->run_days(static_cast<std::uint64_t>(days));
    }
}

int64_t BoSimulation::get_tick() const { return sim_ ? static_cast<int64_t>(sim_->state().time.tick()) : 0; }
int64_t BoSimulation::get_day_in_month() const { return sim_ ? sim_->state().time.day_in_month() : 0; }
int64_t BoSimulation::get_month() const { return sim_ ? sim_->state().time.month() : 0; }
int64_t BoSimulation::get_year() const { return sim_ ? static_cast<int64_t>(sim_->state().time.year()) : 0; }
int64_t BoSimulation::get_treasury() const { return sim_ ? sim_->state().finance.treasury : 0; }
int64_t BoSimulation::get_tax_rate() const { return sim_ ? static_cast<int64_t>(sim_->state().finance.tax_rate) : 0; }

String BoSimulation::get_difficulty() const {
    return sim_ ? String(std::string(bo::core::difficulty_code(sim_->state().difficulty)).c_str()) : String();
}

String BoSimulation::get_state_hash() const { return sim_ ? hex64(sim_->state_hash()) : String(); }
String BoSimulation::get_data_version() const { return catalog_ ? hex64(catalog_->version) : String(); }

void BoSimulation::set_tax_rate(int64_t rate) {
    if (sim_ && rate >= 0 && rate < static_cast<int64_t>(TaxRate::Count)) {
        sim_->issue(bo::core::SetTaxRate{static_cast<TaxRate>(rate)});
    }
}

void BoSimulation::set_wage_rate(int64_t rate) {
    if (sim_ && rate >= 0 && rate < static_cast<int64_t>(WageRate::Count)) {
        sim_->issue(bo::core::SetWageRate{static_cast<WageRate>(rate)});
    }
}

Array BoSimulation::drain_events() {
    Array out;
    if (sim_) {
        for (const auto& e : sim_->drain_events()) {
            out.push_back(event_to_dict(e));
        }
    }
    return out;
}

PackedByteArray BoSimulation::save() const {
    PackedByteArray out;
    if (!sim_) {
        return out;
    }
    // With an adventure under way the save holds it too (B-007): the text, the step, the parent city.
    const auto bytes = adventure_ ? bo::core::save_adventure(adventure_text_, run_, *sim_) : sim_->save();
    out.resize(static_cast<int64_t>(bytes.size()));
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        out.set(static_cast<int64_t>(i), bytes[i]);
    }
    return out;
}

String BoSimulation::load(const PackedByteArray& bytes) {
    if (!catalog_) {
        return "no data loaded";
    }
    std::vector<std::uint8_t> raw(static_cast<std::size_t>(bytes.size()));
    for (int64_t i = 0; i < bytes.size(); ++i) {
        raw[static_cast<std::size_t>(i)] = bytes[i];
    }
    std::string error;
    if (bo::core::is_adventure_save(raw)) {
        auto adv = bo::core::load_adventure(*catalog_, raw, error);
        if (!adv) {
            return String(error.c_str());
        }
        sim_.reset();
        sim_.emplace(std::move(*adv->sim));
        adventure_ = std::move(adv->data);
        adventure_text_ = std::move(adv->text);
        run_ = std::move(adv->run);
        return String();
    }
    auto loaded = bo::core::Simulation::load(*catalog_, raw, error);
    if (!loaded) {
        return String(error.c_str());
    }
    adventure_.reset();  // a plain save: no adventure left over from before (B-007)
    run_ = bo::core::AdventureRun{};
    sim_.reset();
    sim_.emplace(std::move(*loaded));
    return String(error.c_str());  // may carry "save.data_version_mismatch" as a warning
}

void BoSimulation::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start", "data_dir", "seed", "difficulty_code", "initial_treasury"),
                         &BoSimulation::start);
    ClassDB::bind_method(D_METHOD("step", "ticks"), &BoSimulation::step);
    ClassDB::bind_method(D_METHOD("run_days", "days"), &BoSimulation::run_days);
    ClassDB::bind_method(D_METHOD("get_tick"), &BoSimulation::get_tick);
    ClassDB::bind_method(D_METHOD("get_day_in_month"), &BoSimulation::get_day_in_month);
    ClassDB::bind_method(D_METHOD("get_month"), &BoSimulation::get_month);
    ClassDB::bind_method(D_METHOD("get_year"), &BoSimulation::get_year);
    ClassDB::bind_method(D_METHOD("get_treasury"), &BoSimulation::get_treasury);
    ClassDB::bind_method(D_METHOD("get_tax_rate"), &BoSimulation::get_tax_rate);
    ClassDB::bind_method(D_METHOD("get_difficulty"), &BoSimulation::get_difficulty);
    ClassDB::bind_method(D_METHOD("get_state_hash"), &BoSimulation::get_state_hash);
    ClassDB::bind_method(D_METHOD("get_data_version"), &BoSimulation::get_data_version);
    ClassDB::bind_method(D_METHOD("set_tax_rate", "rate"), &BoSimulation::set_tax_rate);
    ClassDB::bind_method(D_METHOD("set_wage_rate", "rate"), &BoSimulation::set_wage_rate);
    ClassDB::bind_method(
        D_METHOD("start_with_map", "data_dir", "seed", "difficulty_code", "initial_treasury", "rows", "full_staffing",
                 "elevation", "trade_partners"),
        &BoSimulation::start_with_map, DEFVAL(false), DEFVAL(PackedStringArray()), DEFVAL(Array()));
    ClassDB::bind_method(D_METHOD("set_sector_priority", "sector", "priority"), &BoSimulation::set_sector_priority);
    ClassDB::bind_method(D_METHOD("set_industry_enabled", "type", "enabled"), &BoSimulation::set_industry_enabled);
    ClassDB::bind_method(D_METHOD("get_workforce"), &BoSimulation::get_workforce);
    ClassDB::bind_method(D_METHOD("set_trade_partner", "building", "partner"), &BoSimulation::set_trade_partner);
    ClassDB::bind_method(D_METHOD("muster_company", "company"), &BoSimulation::muster_company);
    ClassDB::bind_method(D_METHOD("move_company", "company", "x", "y"), &BoSimulation::move_company);
    ClassDB::bind_method(D_METHOD("send_company_home", "company"), &BoSimulation::send_company_home);
    ClassDB::bind_method(D_METHOD("get_companies"), &BoSimulation::get_companies);
    ClassDB::bind_method(D_METHOD("start_invasion", "nation", "soldiers", "city"), &BoSimulation::start_invasion,
                         DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("bribe_invaders"), &BoSimulation::bribe_invaders);
    ClassDB::bind_method(D_METHOD("pray", "sanctuary"), &BoSimulation::pray);
    ClassDB::bind_method(D_METHOD("summon_hero", "hall"), &BoSimulation::summon_hero);
    ClassDB::bind_method(D_METHOD("send_hero_on_quest", "quest"), &BoSimulation::send_hero_on_quest);
    ClassDB::bind_method(D_METHOD("check_placement", "type", "x", "y", "rotation"), &BoSimulation::check_placement);
    ClassDB::bind_method(D_METHOD("get_monsters"), &BoSimulation::get_monsters);
    ClassDB::bind_method(D_METHOD("give_gift", "city", "good", "amount"), &BoSimulation::give_gift);
    ClassDB::bind_method(D_METHOD("request_from_city", "city", "good"), &BoSimulation::request_from_city);
    ClassDB::bind_method(D_METHOD("dispatch_army", "city", "raid", "good", "companies", "ships"), &BoSimulation::dispatch_army,
                         DEFVAL(false));
    ClassDB::bind_method(D_METHOD("get_cities"), &BoSimulation::get_cities);
    ClassDB::bind_method(D_METHOD("start_scenario", "data_dir", "scenario_path", "full_staffing"),
                         &BoSimulation::start_scenario, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("fulfill_request", "request"), &BoSimulation::fulfill_request);
    ClassDB::bind_method(D_METHOD("load_build_order", "path"), &BoSimulation::load_build_order);
    ClassDB::bind_method(D_METHOD("set_stockpile", "good", "on"), &BoSimulation::set_stockpile);
    ClassDB::bind_method(D_METHOD("set_not_buying", "stall", "on"), &BoSimulation::set_not_buying);
    ClassDB::bind_method(D_METHOD("get_stockpiled"), &BoSimulation::get_stockpiled);
    ClassDB::bind_method(D_METHOD("man_towers", "on"), &BoSimulation::man_towers);
    ClassDB::bind_method(D_METHOD("set_auto_defend", "on"), &BoSimulation::set_auto_defend);
    ClassDB::bind_method(D_METHOD("set_construction_paused", "building", "on"), &BoSimulation::set_construction_paused);
    ClassDB::bind_method(D_METHOD("set_company_tactic", "company", "tactic"), &BoSimulation::set_company_tactic);
    ClassDB::bind_method(D_METHOD("get_ships"), &BoSimulation::get_ships);
    ClassDB::bind_method(D_METHOD("consult_oracle", "building"), &BoSimulation::consult_oracle);
    ClassDB::bind_method(D_METHOD("request_military_help", "city", "strike", "target"),
                         &BoSimulation::request_military_help, DEFVAL(false), DEFVAL(0));
    ClassDB::bind_method(D_METHOD("set_ship_order", "ship", "order", "x", "y"), &BoSimulation::set_ship_order,
                         DEFVAL(-1), DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("get_expeditions"), &BoSimulation::get_expeditions);
    ClassDB::bind_method(D_METHOD("get_invasion"), &BoSimulation::get_invasion);
    ClassDB::bind_method(D_METHOD("apply_build_order"), &BoSimulation::apply_build_order);
    ClassDB::bind_method(D_METHOD("start_adventure", "data_dir", "path"), &BoSimulation::start_adventure);
    ClassDB::bind_method(D_METHOD("continue_adventure"), &BoSimulation::continue_adventure);
    ClassDB::bind_method(D_METHOD("found_colony", "colony"), &BoSimulation::found_colony);
    ClassDB::bind_method(D_METHOD("get_adventure"), &BoSimulation::get_adventure);
    ClassDB::bind_method(D_METHOD("get_scenario"), &BoSimulation::get_scenario);
    ClassDB::bind_method(D_METHOD("surrender"), &BoSimulation::surrender);
    ClassDB::bind_method(D_METHOD("get_bribe_cost"), &BoSimulation::get_bribe_cost);
    ClassDB::bind_method(D_METHOD("set_trade_order", "building", "good", "import_on", "import_limit", "export_on",
                                  "export_keep"),
                         &BoSimulation::set_trade_order);
    ClassDB::bind_method(D_METHOD("set_storage_order", "building", "good", "kind", "limit"),
                         &BoSimulation::set_storage_order);
    ClassDB::bind_method(D_METHOD("get_population"), &BoSimulation::get_population);
    ClassDB::bind_method(D_METHOD("get_popularity"), &BoSimulation::get_popularity);
    ClassDB::bind_method(D_METHOD("get_homeless"), &BoSimulation::get_homeless);
    ClassDB::bind_method(D_METHOD("get_employed"), &BoSimulation::get_employed);
    ClassDB::bind_method(D_METHOD("get_appeal", "x", "y"), &BoSimulation::get_appeal);
    ClassDB::bind_method(D_METHOD("place_building", "type", "x", "y", "rotation"), &BoSimulation::place_building,
                         DEFVAL(0));
    ClassDB::bind_method(D_METHOD("undo"), &BoSimulation::undo);
    ClassDB::bind_method(D_METHOD("place_road", "x", "y"), &BoSimulation::place_road);
    ClassDB::bind_method(D_METHOD("place_roadblock", "x", "y"), &BoSimulation::place_roadblock);
    ClassDB::bind_method(D_METHOD("clear_land", "x", "y"), &BoSimulation::clear_land);
    ClassDB::bind_method(D_METHOD("demolish", "x", "y"), &BoSimulation::demolish);
    ClassDB::bind_method(D_METHOD("get_map_width"), &BoSimulation::get_map_width);
    ClassDB::bind_method(D_METHOD("get_map_height"), &BoSimulation::get_map_height);
    ClassDB::bind_method(D_METHOD("get_tile", "x", "y"), &BoSimulation::get_tile);
    ClassDB::bind_method(D_METHOD("get_buildings"), &BoSimulation::get_buildings);
    ClassDB::bind_method(D_METHOD("get_walkers"), &BoSimulation::get_walkers);
    ClassDB::bind_method(D_METHOD("drain_events"), &BoSimulation::drain_events);
    ClassDB::bind_method(D_METHOD("save"), &BoSimulation::save);
    ClassDB::bind_method(D_METHOD("load", "bytes"), &BoSimulation::load);
}
