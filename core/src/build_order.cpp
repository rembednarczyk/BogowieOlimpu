#include "bo/core/build_order.h"

#include "bo/core/storage.h"

#include <stdexcept>

#include <nlohmann/json.hpp>

namespace bo::core {

using json = nlohmann::json;

namespace {

TilePos pos_of(const json& a) { return {a.at(0).get<std::int32_t>(), a.at(1).get<std::int32_t>()}; }

// Tiles of a straight road from (x1, y1) to (x2, y2): first along x, then along y (an L).
void road_line(const json& a, std::vector<Command>& out) {
    const std::int32_t x1 = a.at(0), y1 = a.at(1), x2 = a.at(2), y2 = a.at(3);
    const std::int32_t sx = x2 >= x1 ? 1 : -1;
    const std::int32_t sy = y2 >= y1 ? 1 : -1;
    for (std::int32_t x = x1;; x += sx) {
        out.emplace_back(PlaceRoad{x, y1});
        if (x == x2) {
            break;
        }
    }
    for (std::int32_t y = y1 + sy; y1 != y2; y += sy) {
        out.emplace_back(PlaceRoad{x2, y});
        if (y == y2) {
            break;
        }
    }
}

// One JSON step to one or more steps (a road line or a repeated building expands).
void parse_step(const json& s, std::vector<BuildStep>& out) {
    BuildStep base;
    base.day = s.value("day", std::uint64_t{0});
    base.retry = s.value("retry", false);
    base.every = s.value("every", std::uint64_t{0});
    base.until = s.value("until", std::uint64_t{0});
    const json when = s.value("when", json::object());  // a named copy: items() of a temporary dangles
    for (const auto& [good, n] : when.items()) {
        base.when[good] = n.get<std::uint32_t>();
    }
    auto add = [&](Command c, std::optional<TilePos> at = std::nullopt) {
        BuildStep step = base;
        step.command = std::move(c);
        step.at = at;
        out.push_back(std::move(step));
    };
    if (s.contains("place")) {
        const json rep = s.contains("repeat") ? s.at("repeat") : json::array({1, 0, 0});  // count, dx, dy
        for (std::int32_t i = 0; i < rep.at(0).get<std::int32_t>(); ++i) {
            add(PlaceBuilding{s.at("place").get<std::string>(), s.at("x").get<std::int32_t>() + i * rep.at(1).get<std::int32_t>(),
                              s.at("y").get<std::int32_t>() + i * rep.at(2).get<std::int32_t>(),
                              s.value("rot", std::uint8_t{0})});
        }
    } else if (s.contains("road")) {
        std::vector<Command> roads;
        road_line(s.at("road"), roads);
        for (auto& c : roads) {
            add(std::move(c));
        }
    } else if (s.contains("roadblock")) {
        const TilePos p = pos_of(s.at("roadblock"));
        add(PlaceRoadblock{p.x, p.y});
    } else if (s.contains("priority")) {
        add(SetSectorPriority{s.at("priority").at(0).get<std::uint8_t>(), s.at("priority").at(1).get<std::uint8_t>()});
    } else if (s.contains("tax")) {
        add(SetTaxRate{static_cast<TaxRate>(s.at("tax").get<std::uint8_t>())});
    } else if (s.contains("wage")) {
        add(SetWageRate{static_cast<WageRate>(s.at("wage").get<std::uint8_t>())});
    } else if (s.contains("trade_partner")) {
        add(SetTradePartner{0, s.at("trade_partner").get<std::int32_t>()}, pos_of(s.at("at")));
    } else if (s.contains("trade_order")) {
        const auto& o = s.at("trade_order");
        add(SetTradeOrder{0, o.at("good").get<std::string>(), o.value("import", 0U) > 0, o.value("import", 0U),
                          o.contains("export_keep"), o.value("export_keep", 0U)},
            pos_of(s.at("at")));
    } else if (s.contains("man_towers")) {
        add(ManTowers{s.at("man_towers").get<bool>()});
    } else if (s.contains("stockpile")) {
        add(SetStockpile{s.at("stockpile").get<std::string>(), s.value("on", true)});
    } else if (s.contains("not_buying")) {
        add(SetNotBuying{0, s.value("on", true)}, pos_of(s.at("not_buying")));
    } else if (s.contains("ship_order")) {  // [ship, order, x, y]: one trireme's order or post (Z:142)
        const json& o = s.at("ship_order");
        add(SetShipOrder{o.at(0).get<std::uint32_t>(), o.at(1).get<std::uint8_t>(), o.size() > 3 ? o.at(2).get<std::int32_t>() : -1,
                         o.size() > 3 ? o.at(3).get<std::int32_t>() : -1});
    } else if (s.contains("ships")) {  // every trireme's order: 0-3 orders, 4 call the crew (Z:141-142)
        BuildStep step = base;
        step.all_ships = true;
        step.command = SetShipOrder{0, s.at("ships").get<std::uint8_t>(), -1, -1};
        out.push_back(std::move(step));
    } else if (s.contains("tactics")) {  // every company's tactic: 0 defensive, 1 offensive, 2 special
        BuildStep step = base;
        step.all_companies = true;
        step.command = SetCompanyTactic{0, s.at("tactics").get<std::uint8_t>()};
        out.push_back(std::move(step));
    } else if (s.contains("dispatch")) {  // {"city": i, "raid": bool, "good": g}: companies picked when due
        const json& d = s.at("dispatch");
        BuildStep step = base;
        step.dispatch_ready = true;
        step.command = DispatchArmy{d.at("city").get<std::uint32_t>(), d.value("raid", false), d.value("good", ""), {}};
        out.push_back(std::move(step));
    } else if (s.contains("fulfill_requests")) {
        BuildStep step = base;
        step.fulfill_all = true;
        step.command = FulfillRequest{0};
        out.push_back(std::move(step));
    } else if (s.contains("demolish")) {  // the building on a tile (Z:20), e.g. a unique one moved elsewhere
        const TilePos p = pos_of(s.at("demolish"));
        add(Demolish{p.x, p.y});
    } else if (s.contains("oracle")) {  // Consult Oracle at the sanctuary on a tile (Z:124)
        add(ConsultOracle{0}, pos_of(s.at("oracle")));
    } else if (s.contains("gift")) {  // {"city": i, "good": g, "amount": n} (Z:176-177)
        const json& g = s.at("gift");
        add(GiftToCity{g.at("city").get<std::uint32_t>(), g.value("good", std::string("drachmas")), g.value("amount", 500U)});
    } else if (s.contains("military_help")) {  // {"city": i, "strike": bool, "target": j} (Z:174)
        const json& h = s.at("military_help");
        add(RequestMilitaryHelp{h.at("city").get<std::uint32_t>(), h.value("strike", false), h.value("target", 0U)});
    } else if (s.contains("summon_hero")) {
        add(SummonHero{0}, pos_of(s.at("summon_hero")));
    } else if (s.contains("hero_quest")) {
        add(SendHeroOnQuest{s.at("hero_quest").get<std::uint32_t>()});
    } else if (s.contains("pray")) {
        add(Pray{0}, pos_of(s.at("pray")));
    } else if (s.contains("storage_order")) {
        const auto& o = s.at("storage_order");
        add(SetStorageOrder{0, o.at("good").get<std::string>(), o.value("kind", std::uint8_t{0}), o.value("limit", 32U)},
            pos_of(s.at("at")));
    } else {
        throw std::runtime_error("unknown build step: " + s.dump());
    }
}

bool stands(const Simulation& sim, const PlaceBuilding& p) {
    const auto& city = sim.state().city;
    const TilePos t{p.x, p.y};
    if (!city.map.in_bounds(t) || city.map.at(t).building == kNoBuilding) {
        return false;
    }
    return city.buildings.items.at(city.map.at(t).building).type == p.type;
}

}  // namespace

std::optional<BuildOrder> parse_build_order(const std::string& json_text, std::string& error) {
    try {
        const json j = json::parse(json_text);
        BuildOrder order;
        for (const auto& s : j.at("steps")) {
            parse_step(s, order.steps);
        }
        return order;
    } catch (const std::exception& e) {
        error = std::string("build order: ") + e.what();
        return std::nullopt;
    }
}

std::vector<Command> due_commands(const BuildOrder& order, const Simulation& sim, std::uint64_t day) {
    std::vector<Command> out;
    for (const BuildStep& s : order.steps) {
        const auto* place = std::get_if<PlaceBuilding>(&s.command);
        const bool due = s.day == day || (s.retry && place != nullptr && s.day < day && !stands(sim, *place)) ||
                         (s.every > 0 && day > s.day && (day - s.day) % s.every == 0 && (s.until == 0 || day <= s.until));
        if (!due) {
            continue;
        }
        bool stocked = true;
        for (const auto& [good, n] : s.when) {
            stocked = stocked && city_stock(sim.state().city, sim.catalog(), good) >= n;  // without trade buildings (Z:79)
        }
        if (!stocked) {
            continue;
        }
        if (s.all_ships) {
            const auto order = std::get<SetShipOrder>(s.command).order;
            for (std::uint32_t i = 0; i < sim.state().military.ships.size(); ++i) {
                out.emplace_back(SetShipOrder{i, order, -1, -1});
            }
            continue;
        }
        if (s.all_companies) {
            const auto tactic = std::get<SetCompanyTactic>(s.command).tactic;
            for (std::uint32_t i = 0; i < sim.state().military.companies.size(); ++i) {
                out.emplace_back(SetCompanyTactic{i, tactic});
            }
            continue;
        }
        if (s.fulfill_all) {  // last first: a fulfilled request shifts the ones after it
            for (std::size_t i = sim.state().scenario.requests.size(); i-- > 0;) {
                out.emplace_back(FulfillRequest{static_cast<std::uint32_t>(i)});
            }
            continue;
        }
        Command c = s.command;
        if (s.dispatch_ready) {
            auto& army = std::get<DispatchArmy>(c);
            const auto& companies = sim.state().military.companies;
            for (std::uint32_t i = 0; i < companies.size(); ++i) {
                if (companies[i].status == CompanyStatus::AtHome && companies[i].unit != "rabble" &&
                    !companies[i].members.empty()) {
                    army.companies.push_back(i);
                }
            }
            if (army.companies.empty()) {
                continue;  // nobody to send today
            }
        }
        if (s.at) {
            const auto& map = sim.state().city.map;
            if (!map.in_bounds(*s.at) || map.at(*s.at).building == kNoBuilding) {
                continue;
            }
            const std::uint32_t id = map.at(*s.at).building;
            if (auto* tp = std::get_if<SetTradePartner>(&c)) {
                tp->building = id;
            } else if (auto* to = std::get_if<SetTradeOrder>(&c)) {
                to->building = id;
            } else if (auto* so = std::get_if<SetStorageOrder>(&c)) {
                so->building = id;
            } else if (auto* sh = std::get_if<SummonHero>(&c)) {
                sh->hall = id;
            } else if (auto* pr = std::get_if<Pray>(&c)) {
                pr->building = id;
            } else if (auto* oc = std::get_if<ConsultOracle>(&c)) {
                oc->building = id;
            } else if (auto* nb = std::get_if<SetNotBuying>(&c)) {
                const auto& b = sim.state().city.buildings.items.at(id);
                nb->building = b.slots.empty() ? id : b.slots.front();  // a tile of an agora: its first stall
            }
        }
        out.push_back(std::move(c));
    }
    return out;
}

}  // namespace bo::core
