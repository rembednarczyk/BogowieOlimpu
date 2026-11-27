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
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include "bo/core/data.h"
#include "bo/core/adventure.h"
#include "bo/core/build_order.h"
#include "bo/core/games.h"
#include "bo/core/gods.h"
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
                                 const godot::PackedStringArray& elevation = godot::PackedStringArray(),
                                 const godot::Array& trade_partners = godot::Array());
    // Starts an adventure file (scenarios/*.json, res://scenarios): map, cities, events, goals (D5).
    // Autopilot (docs/PRAKTYKI.md ch. 4): the demo's build order, run day by day like the player's commands.
    godot::String load_build_order(const godot::String& path);
    int64_t apply_build_order();
    godot::String start_scenario(const godot::String& data_dir, const godot::String& scenario_path,
                                 bool full_staffing = false);
    void fulfill_request(int64_t request);
    // Adventures of several episodes (doc 23): start, go on after a won episode ("parent", "choose_colony",
    // "victory"), found a colony; {"id", "step", "steps", "kind", "in_colony", "colony_choices"}.
    godot::String start_adventure(const godot::String& data_dir, const godot::String& path);
    godot::String continue_adventure();
    godot::String found_colony(const godot::String& colony);
    godot::Dictionary get_adventure() const;
    // {"id", "goals": [{"type", "target", "value", "met"}], "requests": [{"city", "good", "amount",
    // "days_left"}], "won", "lost"}
    godot::Dictionary get_scenario() const;
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
    void set_stockpile(const godot::String& good, bool on);   // Z:80
    void set_not_buying(int64_t stall, bool on);              // Z:72
    void man_towers(bool on);                                 // Man All, Z:146
    void set_company_tactic(int64_t company, int64_t tactic); // 0 defensive, 1 offensive, 2 special (Z:138-139)
    // Triremes (Z:141-144): {"order" (0 defend, 1 attack all, 2 repair, 3 patrol), "crew", "hp", "max_hp", "x", "y",
    // "station_x", "station_y"}; set_ship_order(ship, order, x, y): Defend Location or Patrol at a water tile
    // (-1: where it is); 4 calls the crew, 5 sends it home (Z:141).
    godot::Array get_ships() const;
    void consult_oracle(int64_t building);  // Z:124: the answer comes as an "oracle_spoke" event
    // Military help (Z:174): defensive aid while invaded, or with `strike` an attack of `city` on `target`.
    void request_military_help(int64_t city, bool strike = false, int64_t target = 0);
    void set_ship_order(int64_t ship, int64_t order, int64_t x = -1, int64_t y = -1);
    godot::PackedStringArray get_stockpiled() const;
    void set_trade_partner(int64_t building, int64_t partner);
    void set_trade_order(int64_t building, const godot::String& good, bool import_on, int64_t import_limit,
                         bool export_on, int64_t export_keep);
    // Army (D3): companies are indexed as in get_companies().
    void muster_company(int64_t company);
    void move_company(int64_t company, int64_t x, int64_t y);
    void send_company_home(int64_t company);
    // Array of {"unit", "soldiers", "status" (0 home, 1 marching, 2 still, 3 going home), "morale", "x", "y"}
    godot::Array get_companies() const;
    // Invasions (D3 stage 2): started by the scenario later; the player bribes, surrenders or fights.
    void start_invasion(const godot::String& nation, int64_t soldiers, int64_t city = -1);
    void bribe_invaders();
    // Gods (D1): a prayer at a finished sanctuary; the answer comes as "prayer_answered" or "command_rejected".
    void pray(int64_t sanctuary);
    // Monsters and heroes (D5): calls the hero to a Hero's Hall; [{"monster", "slain", "resting", "hero"}]
    void summon_hero(int64_t hall);
    void send_hero_on_quest(int64_t quest);
    // Placement preview (E7): {"ok", "reason" (i18n key), "cost", "tiles": PackedVector2Array}; nothing changes.
    godot::Dictionary check_placement(const godot::String& type, int64_t x, int64_t y, int64_t rotation) const;
    godot::Array get_monsters() const;
    // World (D4): `city` indexes the partners given to start_with_map; "drachmas" is money.
    void give_gift(int64_t city, const godot::String& good, int64_t amount);
    void request_from_city(int64_t city, const godot::String& good);
    void dispatch_army(int64_t city, bool raid, const godot::String& good, const godot::Array& companies);
    // Array of {"id", "relation", "opinion", "military", "economy", "tribute", "tribute_demand", "rebelling"}
    godot::Array get_cities() const;
    godot::Array get_expeditions() const;
    godot::Dictionary get_invasion() const;
    void surrender();
    int64_t get_bribe_cost() const;
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
    std::optional<bo::core::AdventureData> adventure_;
    bo::core::AdventureRun run_;
    std::string adventure_text_;
    std::optional<bo::core::BuildOrder> build_order_;
    std::optional<std::uint64_t> build_order_day_;  // last day applied; not saved (a loaded game goes on)
};
