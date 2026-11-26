// Data loader, economy sections: migration, risks, production (data/*.json, docs/gameplay/19, 23, 24).
#include <map>
#include <string>

#include "data_internal.h"

namespace bo::core::detail {

using nlohmann::json;

void load_migration(const json& m, const json& diff, MigrationParams& p) {
    for (std::size_t d = 0; d < p.base_happiness.size(); ++d) {
        p.base_happiness[d] = m.at("base_happiness").at(std::string(difficulty_code(static_cast<Difficulty>(d))));
    }
    p.small_city_population = m.at("small_city_population");
    for (const auto& row : m.at("unemployment_mood")) {
        p.unemployment_mood.emplace_back(row.at("above_percent"), row.at("mood"));
    }
    constexpr const char* kWageKeys[] = {"none", "very_low", "low", "normal", "high", "very_high"};
    for (std::size_t i = 0; i < p.wage_mood.size(); ++i) {
        p.wage_mood[i] = m.at("wage_mood").at(kWageKeys[i]);
    }
    p.food_stocked = m.at("food_mood").at("stocked");
    p.food_missing_first = m.at("food_mood").at("missing_first");
    p.food_missing_max = m.at("food_mood").at("missing_max");
    p.debt_mood = m.at("debt_mood");
    p.idle_army_mood = m.value("idle_army_mood", p.idle_army_mood);
    for (const auto& row : m.at("migration_table")) {
        p.migration_table.emplace_back(row.at("above"), row.at("percent"));
    }
    p.batch_people = m.at("batch_people");
    p.min_batch = m.at("min_batch");
    p.emigration_min_population = m.at("emigration_min_population");
    p.fill_first_free_slots = m.at("fill_first_free_slots");
    constexpr const char* kTaxKeys[] = {"none", "very_low", "low", "normal", "high", "very_high", "outrageous"};
    for (std::size_t t = 0; t < p.tax_mood.size(); ++t) {
        for (std::size_t d = 0; d < p.tax_mood[t].size(); ++d) {
            p.tax_mood[t][d] = diff.at("tax_sentiment").at(kTaxKeys[t]).at(d);
        }
    }
}

void load_risks(const std::map<std::string, json>& docs, RiskParams& p) {
    if (docs.count("risks.json") != 0) {
        const auto& rk = docs.at("risks.json");
        p.hazard_threshold = rk.value("hazard_threshold", p.hazard_threshold);
        p.burn_days = rk.value("burn_days", p.burn_days);
        p.spread_every_days = rk.value("spread_every_days", p.spread_every_days);
        p.spread_chance_per_4 = rk.value("spread_chance_per_4", p.spread_chance_per_4);
        p.disease_threshold = rk.value("disease_threshold", p.disease_threshold);
        p.disease_hungry_multiplier = rk.value("disease_hungry_multiplier", p.disease_hungry_multiplier);
        p.plague_death_one_in = rk.value("plague_death_one_in", p.plague_death_one_in);
        p.quarantine_days = rk.value("quarantine_days", p.quarantine_days);
        p.crime_threshold = rk.value("crime_threshold", p.crime_threshold);
        p.criminal_roam_ticks = rk.value("criminal_roam_ticks", p.criminal_roam_ticks);
        p.fire_factor_min = rk.value("fire_factor_min", p.fire_factor_min);
        p.fire_factor_max = rk.value("fire_factor_max", p.fire_factor_max);
        p.hygiene_gate = rk.value("hygiene_gate", p.hygiene_gate);
        p.hygiene_start = rk.value("hygiene_start", p.hygiene_start);
        p.hygiene_step = rk.value("hygiene_step", p.hygiene_step);
        p.plague_spread_radius = rk.value("plague_spread_radius", p.plague_spread_radius);
        p.plague_spread_percent = rk.value("plague_spread_percent", p.plague_spread_percent);
        p.crime_escalation = rk.value("crime_escalation", std::vector<std::string>{});
        p.crime_happiness_below = rk.value("crime_happiness_below", p.crime_happiness_below);
        p.mugger_victims = rk.value("mugger_victims", p.mugger_victims);
        p.firefighter_reach = rk.value("firefighter_reach", p.firefighter_reach);
        p.extinguish_days = rk.value("extinguish_days", p.extinguish_days);
    }
}

void load_production(const std::map<std::string, json>& docs, DataCatalog& cat) {
    if (docs.count("production.json") != 0) {
        for (const auto& p : docs.at("production.json")) {
            ProductionParams pp;
            const auto& lpy = p.value("loads_per_year", json());
            pp.loads_per_year = lpy.is_array() ? lpy.at(0).get<std::int32_t>() : opt_int(lpy).value_or(0);
            if (const auto m = opt_int(p.value("harvest_month", json()))) {
                pp.harvest_month = *m - 1;  // data: 1-12
            }
            const auto& msd = p.value("max_storage_distance", json());
            pp.max_storage_distance =
                msd.is_array() ? msd.at(msd.size() - 1).get<std::int32_t>() : opt_int(msd).value_or(300);
            const auto& in = p.value("input", json());
            if (in.is_object() && in.size() == 1 && in.begin().value().is_number_integer() &&
                !p.contains("input_units_good")) {
                pp.input_good = in.begin().key();
                pp.input_per_output = in.begin().value().get<std::int32_t>();
            }
            pp.field_workers = p.value("field_workers", 0);
            pp.units_per_trip = p.value("units_per_trip", 0);
            pp.work_days_per_trip = p.value("work_days_per_trip", 0);
            pp.source_distance = p.value("source_distance", 0);
            pp.fixed_rate_near_resource = p.value("rate_model", "") == "fixed_rate_near_resource";
            pp.silver_per_batch = p.value("silver_per_batch", 0);
            pp.drachmas_per_batch = p.value("drachmas_per_batch", 0);
            pp.batch_days = p.value("batch_days", 0);
            pp.input_capacity_units = p.value("input_capacity_units", 0);
            pp.boat_build_days = p.value("boat_build_days", 0);
            pp.input_units_good = p.value("input_units_good", "");
            pp.input_units_per_output = p.value("input_units_per_output", 0);
            pp.input_units_capacity = p.value("input_units_capacity", 0);
            pp.upkeep_units_per_month = p.value("upkeep_units_per_month", 0);
            const json multi = p.value("inputs_multi", json::object());
            for (const auto& [g, n] : multi.items()) {
                pp.inputs_multi[g] = n.get<std::int32_t>();
            }
            pp.input_capacity = p.value("input_capacity", 0);
            pp.storage_loads = p.value("storage_loads", 0);
            pp.animals_max = p.value("animals_max", 0);
            const auto& hm = p.value("harvest_months", json());
            if (hm.is_object()) {
                for (const auto& [good, months] : hm.items()) {
                    for (const auto& m : months) {
                        pp.harvest_months[good].push_back(m.get<std::int32_t>() - 1);
                    }
                }
            }
            cat.production[p.at("id").get<std::string>()] = pp;
        }
    }
}

void load_combat(const std::map<std::string, json>& docs, CombatParams& p) {
    if (docs.count("enemies.json") == 0) {
        return;
    }
    const auto& e = docs.at("enemies.json");
    p.max_invasion = e.value("max_invasion_soldiers", p.max_invasion);
    const auto& rules = e.value("player_rules", json::object());
    p.rabble_workers_per_soldier = rules.value("rabble_workers_per_soldier", p.rabble_workers_per_soldier);
    p.rabble_population_loss_per_death = rules.value("rabble_population_loss_per_death", p.rabble_population_loss_per_death);
    for (const auto& [id, n] : e.at("nations").items()) {
        NationParams np;
        np.composition = n.value("composition", std::vector<std::int32_t>{});
        np.composition_units = n.value("composition_units", std::vector<std::string>{});
        const json& units = n.contains("units") ? n.at("units") : n;  // player_greek lists its units directly
        for (const auto& [uid, u] : units.items()) {
            if (!u.is_object() || !u.contains("hp")) {
                continue;
            }
            UnitStats s;
            s.hp = opt_int(u.value("hp", json())).value_or(0);
            s.attack = opt_int(u.value("attack", json())).value_or(0);
            s.armor = opt_int(u.value("armor", json())).value_or(0);
            s.armor_vs_missiles = opt_int(u.value("armor_vs_missiles", json())).value_or(0);
            s.missile_attack = opt_int(u.value("missile_attack", json())).value_or(0);
            s.missile_range = opt_int(u.value("missile_range", json())).value_or(0);
            s.missile_rate = opt_int(u.value("missile_rate", json())).value_or(0);
            s.melee_rate = opt_int(u.value("melee_rate", json())).value_or(0);
            const double speed = u.value("speed", json()).is_number() ? u.at("speed").get<double>() : 54.4;
            s.speed_raw = speed_raw_from_tiles_per_month(speed);
            s.mounted = s.missile_attack == 0 && speed >= 90.0;  // same split as composition_units
            np.units[uid] = s;
        }
        p.nations[id] = np;
    }
    if (docs.count("military.json") != 0) {
        const json fort = docs.at("military.json").value("fortification_hits", json::object());
        for (const auto& [k, v] : fort.items()) {
            p.fortification_hits[k] = v.get<std::int32_t>();
        }
        p.tower_shots = docs.at("military.json").value("tower_shots", p.tower_shots);
    }
    if (docs.count("difficulty.json") != 0) {
        const auto& d = docs.at("difficulty.json");
        const auto& pct = d.value("bribe_percent_of_hero", json::object());
        for (std::size_t i = 0; i < p.bribe_percent.size(); ++i) {
            p.bribe_percent[i] = pct.value(std::string(difficulty_code(static_cast<Difficulty>(i))), 100);
        }
        const auto& price = d.value("bribe_hero_drachmas", json::object());
        p.bribe_foot = price.value("foot_soldier", p.bribe_foot);
        p.bribe_mounted = price.value("mounted", p.bribe_mounted);
    }
}

void load_sanctuaries(const std::map<std::string, json>& docs, DataCatalog& cat) {
    if (docs.count("gods_rules.json") != 0) {
        const auto& gr = docs.at("gods_rules.json");
        cat.max_sanctuaries = gr.value("max_sanctuaries_per_city", cat.max_sanctuaries);
        cat.prayer_goods_loads = gr.value("prayer_goods_loads", cat.prayer_goods_loads);
        cat.prayer_money = gr.value("prayer_money", cat.prayer_money);
        cat.sanctify_every_days = gr.value("sanctify_every_days", cat.sanctify_every_days);
        cat.sanctify_reach_tiles = gr.value("sanctify_reach_tiles", cat.sanctify_reach_tiles);
        const json wrath = gr.value("wrath_numbers", json::object());
        for (const auto& [k, v] : wrath.items()) {
            cat.wrath[k] = v.get<std::int32_t>();
        }
        const json monster = gr.value("monster_numbers", json::object());
        for (const auto& [k, v] : monster.items()) {
            if (v.is_number_integer()) {
                cat.monster_numbers[k] = v.get<std::int32_t>();
            }
        }
    }
    if (docs.count("monsters.json") != 0) {
        for (const auto& m : docs.at("monsters.json")) {
            cat.monster_slayer[m.at("id").get<std::string>()] = m.value("slain_by", "");
        }
    }
    if (docs.count("quests.json") != 0) {
        for (const auto& q : docs.at("quests.json")) {
            cat.quest_heroes[q.at("god").get<std::string>() + ":" + std::to_string(q.at("index").get<int>())] =
                q.value("traditional_hero", "");
        }
    }
    if (docs.count("heroes.json") != 0) {
        for (const auto& h : docs.at("heroes.json")) {
            cat.hero_requirements[h.at("id").get<std::string>()] = h.value("requirements", std::vector<std::string>{});
        }
    }
    if (docs.count("gods.json") == 0) {
        return;
    }
    const json rules = docs.count("gods_rules.json") != 0 ? docs.at("gods_rules.json") : json::object();
    const json cooldowns = rules.value("prayer_cooldown_days", json::object());
    for (const auto& g : docs.at("gods.json")) {
        const std::string god = g.at("id").get<std::string>();
        for (const auto& [suffix, key] : {std::pair<std::string, std::string>{"", "sanctuary_data"},
                                          std::pair<std::string, std::string>{"_citadel", "atlantean_sanctuary_data"}}) {
            const auto& d = g.value(key, json());
            if (!d.is_object()) {
                continue;
            }
            SanctuaryParams p;
            p.god = god;
            p.initial_marble = opt_int(d.value("initial_marble", json())).value_or(0);
            p.materials["marble"] = opt_int(d.value("marble", json())).value_or(0);
            p.materials["wood"] = opt_int(d.value("wood", json())).value_or(0);
            p.materials["sculpture"] = opt_int(d.value("sculptures", json())).value_or(0);
            p.sanctifies = g.value("sanctifies", std::vector<std::string>{});
            p.prayer = g.value("prayer", "");
            p.prayer_cooldown_days = cooldowns.value(god, cooldowns.value("default", p.prayer_cooldown_days));
            cat.sanctuaries["sanctuary_" + god + suffix] = p;
        }
    }
}

void load_world(const std::map<std::string, json>& docs, WorldParams& p) {
    if (docs.count("diplomacy.json") == 0) {
        return;
    }
    const json w = docs.at("diplomacy.json").value("world_numbers", json::object());
    const json attitudes = w.value("attitude", json::object());
    for (const auto& [name, v] : attitudes.items()) {
        p.attitude[name] = v.get<std::int32_t>();
    }
    p.gift_needed_x2 = w.value("gift_needed_x2", p.gift_needed_x2);
    p.gift_other_x2 = w.value("gift_other_x2", p.gift_other_x2);
    p.gift_units_per_year = w.value("gift_units_per_year", p.gift_units_per_year);
    p.gift_money_unit = w.value("gift_money_unit", p.gift_money_unit);
    p.request_opinion = w.value("request_opinion", p.request_opinion);
    p.request_loads = w.value("request_loads", p.request_loads);
    p.request_money_per_coin = w.value("request_money_per_coin", p.request_money_per_coin);
    p.request_cooldown_days = w.value("request_cooldown_days", p.request_cooldown_days);
    p.request_delay_days = w.value("request_delay_days", p.request_delay_days);
    p.march_days = w.value("march_days", p.march_days);
    p.city_strength_per_shield = w.value("city_strength_per_shield", p.city_strength_per_shield);
    p.tribute_per_coin = w.value("tribute_per_coin", p.tribute_per_coin);
    p.stops_tribute_below = w.value("stops_tribute_below", p.stops_tribute_below);
    p.refused_tribute_opinion = w.value("refused_tribute_opinion", p.refused_tribute_opinion);
    p.attacked_ally_opinion = w.value("attacked_ally_opinion", p.attacked_ally_opinion);
    p.attacked_rival_respect = w.value("attacked_rival_respect", p.attacked_rival_respect);
    const json raid = w.value("raid_percent", json::object());
    for (std::size_t i = 0; i < p.raid_percent.size(); ++i) {
        p.raid_percent[i] = raid.value(std::string(difficulty_code(static_cast<Difficulty>(i))), 100);
    }
}

}  // namespace bo::core::detail
