// Thin binding of bo::core::Simulation for GDScript. No game rules here: only conversion between Godot types
// and core types (docs/SILNIK.md). Floats never enter the core.
#pragma once

#include <memory>
#include <optional>

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
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
    // Same as start, with a map in the text format of bo::core::parse_text_map (one String per row).
    godot::String start_with_map(const godot::String& data_dir, int64_t seed, const godot::String& difficulty_code,
                                 int64_t initial_treasury, const godot::PackedStringArray& rows,
                                 bool full_staffing = false,
                                 const godot::PackedStringArray& elevation = godot::PackedStringArray());
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

    // Build commands (executed at the next tick; failures arrive as "command_rejected" events).
    void place_building(const godot::String& type, int64_t x, int64_t y, int64_t rotation = 0);
    void undo();
    void set_sector_priority(int64_t sector, int64_t priority);
    void set_industry_enabled(const godot::String& type, bool enabled);
    // kind: 0 Accept, 3 Don't Accept (Get and Empty come with stage 3)
    void set_storage_order(int64_t building, const godot::String& good, int64_t kind, int64_t limit);
    int64_t get_workforce() const;
    int64_t get_employed() const;
    int64_t get_appeal(int64_t x, int64_t y) const;
    int64_t get_population() const;
    int64_t get_popularity() const;
    int64_t get_homeless() const;
    void place_road(int64_t x, int64_t y);
    void place_roadblock(int64_t x, int64_t y);
    void clear_land(int64_t x, int64_t y);
    void demolish(int64_t x, int64_t y);

    // Read-only queries for presentation.
    int64_t get_map_width() const;
    int64_t get_map_height() const;
    // {"terrain": String, "road": bool, "roadblock": bool, "building": int}
    godot::Dictionary get_tile(int64_t x, int64_t y) const;
    // Array of {"id", "type", "x", "y", "width", "height", "rotation", "parent", "has_road_access", "workers",
    // "staffing"} plus for houses {"house_level", "population", "elite"}
    godot::Array get_buildings() const;
    // Array of {"id", "type", "x", "y", "dir", "phase", "progress"}; progress 0..1 toward the next tile.
    godot::Array get_walkers() const;

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
