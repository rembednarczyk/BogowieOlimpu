// Tactics, strength at the Palace and the gods' help in battle (Z:119-120, Z:138-139, Z:144; L-110).
#include <algorithm>

#include "bo/core/military.h"
#include "bo/core/naval.h"
#include "bo/core/blessings.h"
#include "combat_internal.h"

namespace bo::core::combat_detail {

// Strength of the living units on one side: hit points times attack, melee and missile [ZAŁOŻENIE, L-110].
std::int64_t side_strength(const WorldState& st, const DataCatalog& catalog, bool invaders) {
    std::int64_t total = 0;
    for (const auto& [id, w] : st.walkers.items) {
        const bool side = invaders ? w.job == WalkerJob::Invade : (w.job == WalkerJob::Soldier || w.job == WalkerJob::Hero);
        const UnitStats* s = side && w.hp > 0 && w.phase != WalkerPhase::Done ? stats_of(catalog, w) : nullptr;
        if (s) {
            total += static_cast<std::int64_t>(w.hp) * (s->attack + s->missile_attack);
        }
    }
    return total;
}

// Z:144: the Palace falls to an army stronger than the player's forces.
bool invaders_stronger(const WorldState& st, const DataCatalog& catalog) {
    return side_strength(st, catalog, true) > side_strength(st, catalog, false);
}

// Zeus strikes the army down (Z:119); Talos guards the city (Z:120).
void divine_help(WorldState& st) {
    Invasion& inv = st.military.invasion;
    if (!inv.active || (!st.zeus_guard && st.talos_days == 0)) {
        return;
    }
    for (const WalkerId id : inv.invaders) {
        const auto it = st.walkers.items.find(id);
        if (it != st.walkers.items.end()) {
            it->second.hp = 0;
        }
    }
    st.zeus_guard = false;
}

// How near an enemy must be for a fighter to turn on it: a soldier's company tactic (Z:138-139), else L-110.
std::int32_t engage_tiles(const DataCatalog& catalog, const Company* c, const UnitStats& s) {
    if (!c) {
        return kEngageTiles;
    }
    const TacticsParams& t = catalog.tactics;
    // Defensive: they hold their ground; those who throw do so as far as they reach (Z:138).
    return c->tactic == Tactic::Defensive ? std::max(t.defensive_engage, s.missile_attack > 0 ? s.missile_range : 0)
           : c->tactic == Tactic::Offensive ? t.offensive_engage
                                            : t.special_engage;
}

// Armor of a target: shields locked in a defensive company stand firmer (Z:138) [ZAŁOŻENIE, L-110].
std::int32_t armor_of(WorldState& st, const DataCatalog& catalog, const Walker& o, const UnitStats& os) {
    const Company* oc = o.job == WalkerJob::Soldier ? company_of(st, o.id) : nullptr;
    return os.armor + (oc && oc->tactic == Tactic::Defensive ? catalog.tactics.defensive_armor : 0);
}

// One fighter against its nearest foe: a blow, a shot, a chase, or the moves of the special tactics (Z:139):
// rabble skirmish (step back from a foe at hand), hoplites in a wedge (stronger blows), horsemen harass (back
// to the standard after a blow).
void fight(WorldState& st, const DataCatalog& catalog, Walker& w, const UnitStats& s, Company* c,
           std::pair<std::int32_t, WalkerId> foe, bool rechase) {
    Walker& o = st.walkers.items.at(foe.second);
    const UnitStats* os = stats_of(catalog, o);
    if (c && c->status == CompanyStatus::Still) {
        c->status = CompanyStatus::Fighting;
    }
    const bool special = c && c->tactic == Tactic::Special;
    if (special && unit_role(w.type) == "rabble" && foe.first <= 1 && s.missile_attack > 0) {
        const TilePos away{w.tile.x + (w.tile.x - o.tile.x) * catalog.tactics.skirmish_retreat,
                           w.tile.y + (w.tile.y - o.tile.y) * catalog.tactics.skirmish_retreat};
        if (route(st.city, w, away)) {
            return;  // skirmish: out of reach, then throw again
        }
    }
    if (special && unit_role(w.type) == "horseman" && w.cooldown > 0 && w.phase != WalkerPhase::AtTarget) {
        return;  // harass: still riding back to the standard after the last blow
    }
    if (foe.first <= 1 && s.attack > 0) {
        stand(w);
        if (w.cooldown == 0 && os) {
            const std::int32_t attack = with_atlantean_fire(st, catalog, w, blessed_attack(
                st, catalog, w, special && unit_role(w.type) == "hoplite" ? s.attack * catalog.tactics.wedge_attack_percent / 100 : s.attack));
            o.hp -= std::max(0, attack - armor_of(st, catalog, o, *os));
            w.cooldown = static_cast<std::uint16_t>(s.melee_rate);
            if (special && unit_role(w.type) == "horseman") {
                route(st.city, w, company_slot(st.city, c->standard, w.want_loads));  // harass: ride back
            }
        }
    } else if (s.missile_attack > 0 && foe.first <= s.missile_range) {
        stand(w);
        if (w.cooldown == 0 && os) {
            const std::int32_t shot = with_atlantean_fire(st, catalog, w, blessed_attack(st, catalog, w, s.missile_attack));
            o.hp -= std::max(0, shot - os->armor_vs_missiles);
            w.cooldown = static_cast<std::uint16_t>(s.missile_rate);
        }
    } else if (rechase || w.phase == WalkerPhase::AtTarget) {
        route(st.city, w, o.tile);
    }
}


}  // namespace bo::core::combat_detail
