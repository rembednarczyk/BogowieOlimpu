// World commands of the Simulation (D4: gifts, requests, help, armies abroad; Z:167-178), apart from
// simulation.cpp for its size.
#include "bo/core/military.h"
#include "bo/core/oracle.h"
#include "bo/core/scenario.h"
#include "bo/core/simulation.h"
#include "bo/core/world.h"

namespace bo::core {

// World commands (D4): gifts, requests, armies abroad.
void Simulation::apply_world_command(const Command& command, std::uint32_t index) {
    std::string err;
    if (const auto* o = std::get_if<ConsultOracle>(&command)) {  // Z:124 (here for simulation.cpp's size)
        OracleSpoke spoke;
        err = consult_oracle(state_, catalog_, o->building, spoke);
        if (err.empty()) {
            emit(spoke);
        }
    } else if (const auto* g = std::get_if<GiftToCity>(&command)) {
        err = give_gift(state_, catalog_, g->city, g->good, g->amount);
    } else if (const auto* r = std::get_if<RequestFromCity>(&command)) {
        err = request_from_city(state_, catalog_, r->city, r->good);
    } else if (const auto* h = std::get_if<RequestMilitaryHelp>(&command)) {
        CombatEvents ev;
        err = request_military_help(state_, catalog_, h->city, h->strike, h->target, ev.spawned);
        emit_combat(ev);
        if (err.empty()) {
            emit(MilitaryHelpGiven{h->city, h->strike});
        }
    } else if (const auto* d = std::get_if<DispatchArmy>(&command)) {
        err = dispatch_army(state_, catalog_, d->city, d->raid, d->good, d->companies, d->ships);
    } else if (const auto* f = std::get_if<FulfillRequest>(&command)) {
        const auto city = f->request < state_.scenario.requests.size() ? state_.scenario.requests[f->request].city : 0U;
        err = fulfill_request(state_, catalog_, f->request);
        if (err.empty()) {
            emit(RequestClosed{city, true});
        }
    }
    if (!err.empty()) {
        emit(CommandRejected{index, err});
    }
}

}  // namespace bo::core
