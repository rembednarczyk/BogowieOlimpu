// Thin binding of bo::core::Simulation for GDScript. No game rules here: only conversion between Godot types
// and core types (docs/SILNIK.md). Floats never enter the core.
#pragma once

#include <memory>
#include <optional>

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include "bo/core/data.h"
#include "bo/core/simulation.h"

class BoSimulation : public godot::RefCounted {
    GDCLASS(BoSimulation, godot::RefCounted)

public:
    // Loads data from a res:// (or absolute) directory and starts a new simulation.
    // Returns an empty string on success, otherwise the error list joined by newlines.
    godot::String start(const godot::String& data_dir, int64_t seed, const godot::String& difficulty_code,
                        int64_t initial_treasury);
    void step(int64_t ticks);
    void run_days(int64_t days);

    int64_t get_tick() const;
    int64_t get_day_in_month() const;
    int64_t get_month() const;
    int64_t get_year() const;
    int64_t get_treasury() const;
    int64_t get_tax_rate() const;
    godot::String get_difficulty() const;
    godot::String get_state_hash() const;
    godot::String get_data_version() const;

    void set_tax_rate(int64_t rate);
    void set_wage_rate(int64_t rate);

    // Array of Dictionaries: {"tick": int, "type": String, ...fields}.
    godot::Array drain_events();

    godot::PackedByteArray save() const;
    godot::String load(const godot::PackedByteArray& bytes);

protected:
    static void _bind_methods();

private:
    godot::String load_catalog(const godot::String& data_dir);

    std::unique_ptr<bo::core::DataCatalog> catalog_;
    std::optional<bo::core::Simulation> sim_;
};
