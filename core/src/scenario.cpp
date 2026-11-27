// Module D5 stage 1: adventure format, events, city requests, goals (docs/gameplay/13, 23; Z:161-164; L-114).
#include "bo/core/scenario.h"

#include "bo/core/world.h"

#include <algorithm>

#include "bo/core/gods.h"
#include "bo/core/migration.h"
#include "bo/core/monsters.h"
#include "bo/core/state.h"
#include "bo/core/simulation.h"
#include "bo/core/storage.h"
#include "nlohmann/json.hpp"

namespace bo::core {
namespace {

using nlohmann::json;

std::vector<std::string> strings(const json& j, const char* key) {
    std::vector<std::string> out;
    for (const auto& s : j.value(key, json::array())) {
        out.push_back(s.get<std::string>());
    }
    return out;
}

TradePartner parse_city(const json& c) {
    TradePartner p;
    p.id = c.at("id").get<std::string>();
    p.relation = c.value("relation", "ally");
    p.by_sea = c.value("by_sea", false);
    for (const auto* key : {"sells", "buys"}) {
        const json offers = c.value(key, json::object());
        for (const auto& [good, v] : offers.items()) {
            const TradeOffer o{v.at(0).get<std::int32_t>(), v.at(1).get<std::uint32_t>(), 0};
            (std::string(key) == "sells" ? p.sells : p.buys)[good] = o;
        }
    }
    CityDiplomacy& d = p.diplomacy;
    d.opinion = c.value("opinion", d.opinion);
    d.military = c.value("military", d.military);
    d.economy = c.value("economy", d.economy);
    d.tribute = c.value("tribute", d.tribute);
    return p;
}

std::int32_t city_index(const std::vector<TradePartner>& cities, const json& e) {
    const std::string id = e.value("city", "");
    for (std::size_t i = 0; i < cities.size(); ++i) {
        if (cities[i].id == id) {
            return static_cast<std::int32_t>(i);
        }
    }
    return -1;
}

bool goal_met(const WorldState& st, const DataCatalog& catalog, const ScenarioGoal& g) {
    if (g.type == "population") {
        return city_population(st.city) >= static_cast<std::uint64_t>(g.value);
    }
    if (g.type == "treasury") {
        return st.finance.treasury >= g.value;
    }
    if (g.type == "sanctuary") {
        // B-011, owner's decision: the sanctuary must be finished and standing; the god's favour does not count.
        return std::any_of(st.city.buildings.items.begin(), st.city.buildings.items.end(), [&](const auto& kv) {
            const auto p = catalog.sanctuaries.find(kv.second.type);
            return kv.second.sanctuary && kv.second.sanctuary->complete && p != catalog.sanctuaries.end() &&
                   p->second.god == g.target;
        });
    }
    if (g.type == "vassal") {
        return std::any_of(st.trade.partners.begin(), st.trade.partners.end(),
                           [&g](const TradePartner& p) { return p.id == g.target && p.relation == "vassal"; });
    }
    if (g.type == "quest") {
        return std::any_of(st.scenario.quests.begin(), st.scenario.quests.end(),
                           [&g](const Quest& q) { return q.god == g.target && q.status == QuestStatus::Done; });
    }
    if (g.type == "houses") {
        std::int64_t n = 0;
        for (const auto& [id, b] : st.city.buildings.items) {
            if (!b.house || b.house->population == 0) {
                continue;
            }
            const auto& levels = b.house->elite ? catalog.elite_levels : catalog.common_levels;
            const auto it = std::find_if(levels.begin(), levels.end(),
                                         [&g](const HousingLevel& lv) { return lv.id == g.target; });
            if (it != levels.end() && b.house->level >= static_cast<std::size_t>(it - levels.begin())) {
                ++n;
            }
        }
        return n >= g.value;
    }
    return false;
}

void fire(WorldState& st, const DataCatalog& catalog, const ScenarioEvent& e, ScenarioEvents& ev) {
    const bool has_city = e.city >= 0 && static_cast<std::size_t>(e.city) < st.trade.partners.size();
    if (e.type == "invasion") {
        ev.invasions.emplace_back(e.nation, e.amount);
        ev.invasion_cities.push_back(e.city);
        ev.invasion_by_sea.push_back(e.good == "sea");
    } else if (e.type == "request" && has_city) {
        const std::uint64_t today = st.time.tick() / kTicksPerDay;
        st.scenario.requests.push_back(CityRequest{static_cast<std::uint32_t>(e.city), e.good, e.amount,
                                                   today + static_cast<std::uint64_t>(e.deadline_months) * kDaysPerMonth});
        ev.new_requests.push_back(st.scenario.requests.size() - 1);
    } else if (e.type == "gift" && has_city) {
        if (e.good == "drachmas") {
            st.finance.treasury += e.amount;
        } else {
            store_in_city(st.city, catalog, e.good, e.amount);
        }
    } else if (e.type == "god_attack") {
        ev.god_attacks.push_back(&e);
    } else if (e.type == "disaster") {
        ev.disasters.push_back(&e);
    } else if (e.type == "monster") {
        ev.monsters.push_back(&e);
    } else if (e.type == "quest") {
        Quest q;
        q.god = e.god;
        q.index = e.amount;
        q.hero = e.nation;
        if (q.hero.empty()) {
            const auto t = catalog.quest_heroes.find(e.god + ":" + std::to_string(e.amount));
            q.hero = t == catalog.quest_heroes.end() ? "" : t->second;  // the traditional hero (doc 21)
        }
        st.scenario.quests.push_back(q);
        ev.quests_given.push_back(e.god);
    } else if (e.type == "games_host") {
        st.games_host_year = static_cast<std::int32_t>(e.amount);  // the scenario grants the right (Z:110-111)
    } else if (e.type == "relation" && has_city) {
        std::string& relation = st.trade.partners[static_cast<std::size_t>(e.city)].relation;
        if (e.relation == "ally" && relation != "ally") {
            ++st.scenario.monuments["diplomacy"];  // a city becomes an ally (Z:156; eZeus cityBecomes ally)
        }
        relation = e.relation;
    }
}

}  // namespace

// Population and Happiness monuments (Z:155, L-092): each population threshold once; the happiness once.
static void award_milestones(WorldState& st, const DataCatalog& catalog) {
    auto& given = st.scenario.awarded;  // read with find: an empty record stays empty (it is saved)
    const auto count = [&given](const char* key) {
        const auto it = given.find(key);
        return it == given.end() ? 0U : it->second;
    };
    const auto& thresholds = catalog.monuments.population_thresholds;
    const std::uint64_t people = city_population(st.city);
    while (count("population") < thresholds.size() &&
           people >= static_cast<std::uint64_t>(thresholds[count("population")])) {
        ++given["population"];
        ++st.scenario.monuments["population"];
    }
    if (count("happiness") == 0 && people > 0 && st.migration.popularity >= catalog.monuments.happiness_popularity) {
        given["happiness"] = 1;
        ++st.scenario.monuments["happiness"];
    }
}

std::string send_hero_on_quest(WorldState& st, const DataCatalog&, std::uint32_t index) {
    if (index >= st.scenario.quests.size() || st.scenario.quests[index].status != QuestStatus::Open) {
        return "myth.no_quest";
    }
    Quest& q = st.scenario.quests[index];
    const auto h = std::find_if(st.heroes.begin(), st.heroes.end(), [&q](const HeroState& x) { return x.id == q.hero; });
    if (h == st.heroes.end() || st.walkers.items.count(h->walker) == 0) {
        return "myth.hero_not_here";
    }
    st.walkers.items.at(h->walker).phase = WalkerPhase::Done;  // he leaves for the enchanted place
    h->walker = kNoWalker;
    q.status = QuestStatus::Underway;
    q.days_left = kQuestDays;
    return {};
}

std::optional<SimulationConfig> parse_scenario(const std::string& text, std::string& error) {
    try {
        const json j = json::parse(text);
        SimulationConfig cfg;
        cfg.seed = j.value("seed", std::uint64_t{1});
        cfg.initial_treasury = j.value("treasury", std::int64_t{0});
        if (j.contains("difficulty")) {
            cfg.difficulty = difficulty_from_code(j.at("difficulty").get<std::string>());
        }
        cfg.text_map = strings(j, "map");
        cfg.elevation_map = strings(j, "elevation");
        for (const auto& c : j.value("cities", json::array())) {
            cfg.trade_partners.push_back(parse_city(c));
        }
        ScenarioState& s = cfg.scenario;
        s.id = j.value("id", "");
        for (const auto& g : j.value("goals", json::array())) {
            ScenarioGoal goal;
            goal.type = g.at("type").get<std::string>();
            goal.target = g.value("target", "");
            goal.value = g.value("value", std::int64_t{0});
            s.goals.push_back(goal);
        }
        for (const auto& e : j.value("events", json::array())) {
            ScenarioEvent ev;
            ev.type = e.at("type").get<std::string>();
            ev.year = e.value("year", 0U);
            ev.month = e.value("month", 0U);
            ev.repeat = e.value("repeat", false) && ev.type != "monster";  // monsters never repeat (doc 23)
            ev.city = city_index(cfg.trade_partners, e);
            ev.good = ev.type == "invasion" && e.value("by_sea", false) ? "sea" : e.value("good", "");
            ev.nation = e.value("nation", "");
            ev.relation = e.value("relation", "");
            ev.amount = e.value("amount", 0U);
            ev.deadline_months = e.value("deadline_months", 12U);
            ev.god = e.value("god", "");
            ev.x = e.value("x", 0);
            ev.y = e.value("y", 0);
            if (ev.type == "games_host") {
                ev.amount = e.value("host_year", 0U);  // an Olympian year (year mod 4 = 0)
            }
            if (ev.type == "quest") {
                ev.amount = e.value("index", 0U);
                ev.nation = e.value("hero", "");
            }
            if (ev.type == "monster") {
                ev.nation = e.value("monster", "");
                ev.amount = e.value("strength", 1U);
                ev.good = e.value("target", "food");
            }
            if (ev.type == "disaster") {
                ev.nation = e.value("kind", "earthquake");
                ev.amount = e.value("radius", 2U);
            }
            if (ev.month > 11) {
                error = "scenario.bad_month";
                return std::nullopt;
            }
            s.events.push_back(ev);
        }
        if (cfg.text_map.empty()) {
            error = "scenario.no_map";
            return std::nullopt;
        }
        return cfg;
    } catch (const std::exception& e) {
        error = std::string("scenario.invalid: ") + e.what();
        return std::nullopt;
    }
}

ScenarioEvents advance_scenario(WorldState& st, const DataCatalog& catalog) {
    ScenarioEvents ev;
    ScenarioState& s = st.scenario;
    const std::uint64_t today = st.time.tick() / kTicksPerDay;
    const auto year = static_cast<std::uint32_t>(st.time.year());
    const auto month = static_cast<std::uint32_t>(st.time.month());
    if (st.time.day_in_month() == 0) {
        award_milestones(st, catalog);
        for (ScenarioEvent& e : s.events) {
            const bool now = e.month == month && (e.year == year || (e.repeat && e.year < year));
            if (now && e.last_year != static_cast<std::int32_t>(year)) {
                e.last_year = static_cast<std::int32_t>(year);
                fire(st, catalog, e, ev);
            }
        }
    }
    for (Quest& q : s.quests) {
        if (q.status != QuestStatus::Underway || q.days_left-- > 1) {
            continue;
        }
        q.status = QuestStatus::Done;
        ++s.monuments["god_statue"];  // the god's reward (Z:156)
        ev.quests_done.push_back(q.god);
        for (HeroState& h : st.heroes) {
            if (h.id == q.hero && h.walker == kNoWalker) {
                h.walker = hero_walker(st, catalog, h);  // back in his Hall
            }
        }
    }
    for (auto it = s.requests.begin(); it != s.requests.end();) {
        if (it->good == "army" && today > it->due_day) {  // never overdue: after the battle it is gone (Z:161)
            ev.closed.emplace_back(it->city, false);
            it = s.requests.erase(it);
            continue;
        }
        if (today > it->due_day + kWithdrawAfterDays) {
            if (it->city < st.trade.partners.size()) {
                CityDiplomacy& d = st.trade.partners[it->city].diplomacy;
                d.opinion = std::max(0, d.opinion - 15);  // refused or too late (doc 24)
            }
            ev.closed.emplace_back(it->city, false);
            it = s.requests.erase(it);
        } else {
            ++it;
        }
    }
    if (st.time.day_in_month() == 0 && !s.won && !s.goals.empty()) {
        bool all = true;
        for (ScenarioGoal& g : s.goals) {
            g.met = goal_met(st, catalog, g);
            all = all && g.met;
        }
        if (all) {
            s.won = true;
            ev.won = true;
        }
    }
    return ev;
}

std::string fulfill_request(WorldState& st, const DataCatalog& catalog, std::uint32_t index, bool free) {
    ScenarioState& s = st.scenario;
    if (index >= s.requests.size()) {
        return "scenario.no_request";
    }
    const CityRequest r = s.requests[index];
    if (r.good == "army") {
        if (free) {
            return "scenario.not_for_the_gods";  // Hermes fulfils requests for goods, not for troops
        }
        const std::string err = dispatch_aid(st, catalog, r.city, std::max<std::uint32_t>(r.amount, 1));
        if (!err.empty()) {
            return err;
        }
    } else if (free) {
        // Hermes's prayer (Z:122): the request is fulfilled without the player's goods.
    } else if (r.good == "drachmas") {
        if (st.finance.treasury < r.amount) {
            return "scenario.cannot_afford";
        }
        st.finance.treasury -= r.amount;
    } else {
        if (city_stock(st.city, catalog, r.good) < r.amount) {
            return "scenario.not_in_storage";
        }
        take_from_city(st.city, r.good, r.amount);
    }
    const bool late = st.time.tick() / kTicksPerDay > r.due_day;
    if (r.city < st.trade.partners.size()) {
        CityDiplomacy& d = st.trade.partners[r.city].diplomacy;
        d.opinion = std::clamp(d.opinion + (late ? -5 : 10), 0, 100);  // late: -5 (eZeus; owner, L-114)
    }
    s.requests.erase(s.requests.begin() + static_cast<std::ptrdiff_t>(index));
    return {};
}

}  // namespace bo::core
