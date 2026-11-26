#include "bo_simulation.h"

#include <cstdio>
#include <map>
#include <string>
#include <variant>

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "bo/core/migration.h"

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
    // Partners: {"id", "relation", "by_sea", "sells": {good: [price, yearly_limit]}, "buys": {...}}
    for (int64_t i = 0; i < trade_partners.size(); ++i) {
        const Dictionary d = trade_partners[i];
        bo::core::TradePartner p;
        p.id = to_std(d.get("id", ""));
        p.relation = to_std(d.get("relation", "ally"));
        p.by_sea = d.get("by_sea", false);
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
    const auto bytes = sim_->save();
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
    auto loaded = bo::core::Simulation::load(*catalog_, raw, error);
    if (!loaded) {
        return String(error.c_str());
    }
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
