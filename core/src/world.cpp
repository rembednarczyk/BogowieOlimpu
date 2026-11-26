// Module D4 stage 1: opinions, gifts, requests, tribute, armies abroad (docs/gameplay/14, 24; Z:167-178;
// assumptions L-063, L-064, L-065, L-113).
#include "bo/core/world.h"

#include <algorithm>

#include "bo/core/housing.h"
#include "bo/core/military.h"
#include "bo/core/state.h"
#include "bo/core/storage.h"

namespace bo::core {
namespace {

void change_opinion(CityDiplomacy& d, std::int32_t delta) { d.opinion = std::clamp(d.opinion + delta, 0, 100); }

std::int32_t attitude(const DataCatalog& catalog, const std::string& name) {
    const auto it = catalog.world.attitude.find(name);
    return it == catalog.world.attitude.end() ? 50 : it->second;
}

bool diplomatic(const TradePartner& p) { return p.relation != "distant"; }

bool is_food(const DataCatalog& catalog, const std::string& good) {
    const auto it = catalog.goods.find(good);
    return it != catalog.goods.end() && it->second == "food";
}

// The opinion a request needs from this city (Z:174-175), or -1: only by demand (rival, backed by force).
std::int32_t request_threshold(const DataCatalog& catalog, const TradePartner& p, bool food) {
    if (p.relation == "rival") {
        return attitude(catalog, food ? "very_good" : "extremely_good");
    }
    if (p.relation == "ally") {
        return food ? attitude(catalog, "any_attitude") : attitude(catalog, "fairly_happy");
    }
    return food ? attitude(catalog, "even_very_bad") : attitude(catalog, "fairly_happy");
}

// A soldier lost abroad: his house loses a resident and his armor and horse (Z:144).
void lose_soldier(WorldState& st, const std::string& unit, BuildingId house) {
    const auto it = st.city.buildings.items.find(house);
    if (it == st.city.buildings.items.end() || !it->second.house) {
        return;
    }
    HouseState& h = *it->second.house;
    h.population -= std::min<std::uint32_t>(h.population, 1);
    auto& armor = h.goods[static_cast<std::size_t>(HouseGood::Armor)];
    auto& horses = h.goods[static_cast<std::size_t>(HouseGood::Horses)];
    armor -= armor > 0 ? 1 : 0;
    horses -= unit == "horseman" && horses > 0 ? 1 : 0;
}

void battle_abroad(WorldState& st, const DataCatalog& catalog, Expedition& e) {
    TradePartner& p = st.trade.partners[e.city];
    const std::int64_t ours_x2 = army_strength_x2(st, e.companies);
    const std::int64_t theirs_x2 = 2LL * p.diplomacy.military * catalog.world.city_strength_per_shield;
    e.won = ours_x2 > theirs_x2;
    // Each side loses 0.5 x enemy strength / own strength of its soldiers (doc 24, L-063).
    std::uint32_t soldiers = 0;
    for (const std::uint32_t c : e.companies) {
        for (const auto& m : st.military.companies[c].members) {
            soldiers += m.soldiers;
        }
    }
    const std::int64_t lost = ours_x2 > 0 ? soldiers * theirs_x2 / (2 * ours_x2) : soldiers;
    e.losses = static_cast<std::uint32_t>(std::min<std::int64_t>(lost, soldiers));
    if (!e.won) {
        return;
    }
    if (e.raid) {
        const std::int32_t pct = catalog.world.raid_percent[static_cast<std::size_t>(st.difficulty)];
        const auto offer = p.sells.find(e.good);
        const std::uint32_t yearly = offer != p.sells.end() ? offer->second.yearly_limit : 0;
        e.loot = e.good == "drachmas" ? static_cast<std::uint32_t>(p.diplomacy.economy * catalog.world.tribute_per_coin)
                                      : yearly * static_cast<std::uint32_t>(pct) / 100;
        return;
    }
    p.relation = "vassal";  // conquered: pays tribute (Z:177)
    ++st.scenario.monuments["conquest"];  // and a Conquest monument (Z:177)
    p.diplomacy.rebelling = false;
    p.diplomacy.tribute_demand = 0;
    if (p.diplomacy.tribute == 0) {
        p.diplomacy.tribute = p.diplomacy.economy * catalog.world.tribute_per_coin;
    }
}

void come_home(WorldState& st, const DataCatalog& catalog, Expedition& e) {
    std::uint32_t lose = e.losses;
    for (const std::uint32_t c : e.companies) {
        Company& company = st.military.companies[c];
        for (auto& m : company.members) {
            while (lose > 0 && m.soldiers > 0) {
                lose_soldier(st, company.unit, m.house);
                --m.soldiers;
                --lose;
            }
        }
        company.status = CompanyStatus::AtHome;
    }
    if (e.won && e.raid && e.loot > 0) {
        if (e.good == "drachmas") {
            st.finance.treasury += e.loot;
        } else {
            store_in_city(st.city, catalog, e.good, e.loot);
        }
    }
}

}  // namespace

std::int64_t army_strength_x2(const WorldState& st, const std::vector<std::uint32_t>& companies) {
    std::int64_t x2 = 0;
    for (const std::uint32_t c : companies) {
        const Company& company = st.military.companies[c];
        std::int64_t n = 0;
        for (const auto& m : company.members) {
            n += m.soldiers;
        }
        x2 += company.unit == "horseman" ? 3 * n : 2 * n;
    }
    return x2;
}

std::string give_gift(WorldState& st, const DataCatalog& catalog, std::uint32_t city, const std::string& good,
                      std::uint32_t amount) {
    if (city >= st.trade.partners.size() || !diplomatic(st.trade.partners[city]) || amount == 0) {
        return "world.bad_city";
    }
    const WorldParams& w = catalog.world;
    const bool money = good == "drachmas";
    const std::uint32_t units = money ? amount / static_cast<std::uint32_t>(w.gift_money_unit) : amount;
    CityDiplomacy& d = st.trade.partners[city].diplomacy;
    if (units == 0 || d.gift_units_this_year[good] + units > static_cast<std::uint32_t>(w.gift_units_per_year)) {
        return "world.gift_refused";  // too many gifts of one kind (Z:177)
    }
    if (money) {
        if (st.finance.treasury < amount) {
            return "world.cannot_afford";
        }
        st.finance.treasury -= amount;
    } else if (city_stock(st.city, catalog, good) < amount) {
        return "world.not_in_storage";
    } else {
        take_from_city(st.city, good, amount);
    }
    d.gift_units_this_year[good] += units;
    const bool needed = money || st.trade.partners[city].buys.count(good) != 0;
    const auto gain_x2 = static_cast<std::int32_t>(units) * (needed ? w.gift_needed_x2 : w.gift_other_x2);
    change_opinion(d, gain_x2 / 2);
    const std::string relation = st.trade.partners[city].relation;
    for (std::size_t i = 0; i < st.trade.partners.size(); ++i) {
        TradePartner& other = st.trade.partners[i];
        if (i != city && relation != "rival" && other.relation == relation) {
            change_opinion(other.diplomacy, gain_x2 / 4);  // the others of the same state hear of it (Z:177)
        }
    }
    return {};
}

std::string request_from_city(WorldState& st, const DataCatalog& catalog, std::uint32_t city, const std::string& good) {
    if (city >= st.trade.partners.size() || !diplomatic(st.trade.partners[city])) {
        return "world.bad_city";
    }
    TradePartner& p = st.trade.partners[city];
    const bool money = good == "drachmas";
    if (!money && p.sells.count(good) == 0) {
        return "world.city_does_not_make_it";  // the city must produce the good (Z:175)
    }
    if (p.diplomacy.request_cooldown > 0) {
        return "world.asked_too_recently";
    }
    const std::int32_t need = request_threshold(catalog, p, is_food(catalog, good));
    const bool forced = p.relation == "rival" &&
                        army_strength_x2(st, [&st] {
                            std::vector<std::uint32_t> all;
                            for (std::uint32_t i = 0; i < st.military.companies.size(); ++i) {
                                all.push_back(i);
                            }
                            return all;
                        }()) > 2LL * p.diplomacy.military * catalog.world.city_strength_per_shield;
    if (p.diplomacy.opinion < need && !forced) {
        return "world.request_refused";
    }
    const WorldParams& w = catalog.world;
    if (money) {
        st.finance.treasury += static_cast<std::int64_t>(p.diplomacy.economy) * w.request_money_per_coin;
    } else if (store_in_city(st.city, catalog, good, static_cast<std::uint32_t>(w.request_loads)) == 0) {
        return "world.no_room";
    }
    for (TradePartner& other : st.trade.partners) {
        change_opinion(other.diplomacy, w.request_opinion);  // doc 24: every city thinks less of a beggar
    }
    p.diplomacy.request_cooldown = static_cast<std::uint16_t>(w.request_cooldown_days);
    return {};
}

std::string dispatch_army(WorldState& st, const DataCatalog& catalog, std::uint32_t city, bool raid,
                          const std::string& good, const std::vector<std::uint32_t>& companies) {
    if (city >= st.trade.partners.size() || !diplomatic(st.trade.partners[city]) || companies.empty()) {
        return "world.bad_city";
    }
    TradePartner& p = st.trade.partners[city];
    const bool own = p.relation == "vassal" || p.relation == "colony";
    if (own && !p.diplomacy.rebelling) {
        return "world.not_at_war";  // vassals and colonies only while they rebel (Z:177)
    }
    if (raid && good != "drachmas" && p.sells.count(good) == 0) {
        return "world.city_does_not_make_it";
    }
    for (const std::uint32_t c : companies) {
        if (c >= st.military.companies.size() || st.military.companies[c].status != CompanyStatus::AtHome ||
            st.military.companies[c].unit == "rabble" || st.military.companies[c].members.empty()) {
            return "world.bad_company";  // no rabble abroad (Z:136, Z:177)
        }
    }
    for (const std::uint32_t c : companies) {
        st.military.companies[c].status = CompanyStatus::ServingAbroad;
    }
    Expedition e;
    e.city = city;
    e.raid = raid;
    e.good = good;
    e.companies = companies;
    e.days_left = static_cast<std::uint32_t>(catalog.world.march_days);
    st.expeditions.push_back(e);
    const WorldParams& w = catalog.world;
    if (raid || p.relation == "ally") {
        change_opinion(p.diplomacy, -100);  // the target turns hostile (Z:177-178)
    }
    for (std::size_t i = 0; i < st.trade.partners.size(); ++i) {
        TradePartner& other = st.trade.partners[i];
        if (i == city) {
            continue;
        }
        if (p.relation == "ally" && other.relation == "ally") {
            change_opinion(other.diplomacy, w.attacked_ally_opinion);  // all allies turn away
        } else if (p.relation == "rival" && other.relation == "rival") {
            change_opinion(other.diplomacy, w.attacked_rival_respect);  // other rivals respect it
        }
    }
    return {};
}

WorldEvents advance_world(WorldState& st, const DataCatalog& catalog) {
    WorldEvents ev;
    for (TradePartner& p : st.trade.partners) {
        p.diplomacy.request_cooldown = static_cast<std::uint16_t>(p.diplomacy.request_cooldown > 0 ? p.diplomacy.request_cooldown - 1 : 0);
    }
    for (auto it = st.expeditions.begin(); it != st.expeditions.end();) {
        Expedition& e = *it;
        if (e.days_left > 0) {
            --e.days_left;
        }
        if (e.days_left > 0) {
            ++it;
            continue;
        }
        if (!e.returning) {
            battle_abroad(st, catalog, e);
            e.returning = true;
            e.days_left = static_cast<std::uint32_t>(catalog.world.march_days);
            ++it;
            continue;
        }
        come_home(st, catalog, e);
        ev.returned.push_back(e);
        it = st.expeditions.erase(it);
    }
    if (st.time.month() != 0 || st.time.day_in_month() != 0) {
        return ev;
    }
    for (std::uint32_t i = 0; i < st.trade.partners.size(); ++i) {
        CityDiplomacy& d = st.trade.partners[i].diplomacy;
        d.gift_units_this_year.clear();
        const std::string& rel = st.trade.partners[i].relation;
        if ((rel == "vassal" || rel == "colony") && !d.rebelling && d.tribute > 0 &&
            d.opinion >= catalog.world.stops_tribute_below) {
            st.finance.treasury += d.tribute;
            ev.tribute.emplace_back(i, d.tribute);
        }
        if (d.tribute_demand > 0) {
            if (st.finance.treasury >= d.tribute_demand) {
                st.finance.treasury -= d.tribute_demand;
                ev.tribute.emplace_back(i, -d.tribute_demand);
            } else {
                change_opinion(d, catalog.world.refused_tribute_opinion);  // not paid: they will come (Z:172)
            }
        }
    }
    return ev;
}

void invasion_outcome(WorldState& st, const DataCatalog& catalog, std::int32_t city, std::uint8_t result,
                      WorldEvents& ev) {
    if (city < 0 || static_cast<std::size_t>(city) >= st.trade.partners.size()) {
        return;
    }
    CityDiplomacy& d = st.trade.partners[static_cast<std::size_t>(city)].diplomacy;
    if (result == 0) {
        d.tribute_demand = 0;  // beaten off at home: the tribute ends (Z:172)
        return;
    }
    if (result == 2 || result == 3) {
        ++d.player_defeats;
        if (d.tribute_demand == 0) {
            d.tribute_demand = d.economy * catalog.world.tribute_per_coin;
        }
        if (d.player_defeats >= 2) {
            st.episode_lost = true;  // taken twice by the same rival (Z:172)
            ev.episode_lost = true;
        }
    }
}

}  // namespace bo::core
