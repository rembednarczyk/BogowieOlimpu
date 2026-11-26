#include "bo_simulation.h"

#include <cstdio>
#include <map>
#include <string>
#include <variant>

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>

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
    ClassDB::bind_method(D_METHOD("drain_events"), &BoSimulation::drain_events);
    ClassDB::bind_method(D_METHOD("save"), &BoSimulation::save);
    ClassDB::bind_method(D_METHOD("load", "bytes"), &BoSimulation::load);
}
