// Long-term relations between the player and the other cities (L-121). Rules: docs/gameplay/14-swiat.md
// (Z:169, Z:172, Z:178) and 08-administracja-finanse.md (Z:100, Z:179); numbers data/diplomacy.json.
#include <algorithm>

#include "bo/core/world.h"

namespace bo::core {

namespace {

std::uint8_t bump(std::uint8_t months) { return static_cast<std::uint8_t>(std::min(months + 1, 255)); }

bool friendly(const TradePartner& p) {
    return (p.relation == "ally" || p.relation == "vassal" || p.relation == "colony" || p.relation == "parent") &&
           !p.diplomacy.rebelling;
}

}  // namespace

void advance_relations(WorldState& st, const DataCatalog& catalog, WorldEvents& ev) {
    if (st.time.day_in_month() != 0) {
        return;
    }
    const WorldParams& w = catalog.world;
    for (std::uint32_t i = 0; i < st.trade.partners.size(); ++i) {
        TradePartner& p = st.trade.partners[i];
        CityDiplomacy& d = p.diplomacy;
        d.aid_cooldown = static_cast<std::uint8_t>(d.aid_cooldown > 0 ? d.aid_cooldown - 1 : 0);
        const bool subject = p.relation == "vassal" || p.relation == "colony";
        const bool bad = (subject && d.opinion < w.stops_tribute_below) || (p.relation == "rival" && d.opinion < w.hate_below);
        d.months_bad = bad && !d.rebelling ? bump(d.months_bad) : 0;
        if (subject && d.months_bad >= w.rebel_after_months) {
            d.rebelling = true;  // stops paying first (opinion below the limit), then rises (Z:178)
            d.months_bad = 0;
            ev.rebellions.push_back(i);
        } else if (p.relation == "rival" && d.months_bad >= w.rival_attack_after_months) {
            d.months_bad = 0;
            ev.rival_attacks.push_back(i);  // a rival who hates the player may attack (Z:178)
        }
        d.months_good = friendly(p) && d.opinion >= w.goodwill_above ? bump(d.months_good) : 0;
        if (d.months_good >= w.goodwill_after_months) {
            d.months_good = 0;
            st.finance.treasury += w.goodwill_gift;  // a gesture of goodwill (Z:178)
            ev.gifts.emplace_back(i, w.goodwill_gift);
        }
        if (friendly(p) && st.debt_days >= w.debt_aid_after_days && d.aid_cooldown == 0 &&
            d.opinion >= w.debt_aid_min_opinion) {
            d.aid_cooldown = static_cast<std::uint8_t>(w.debt_aid_every_months);
            st.finance.treasury += w.debt_aid;  // allies may help a city in debt (Z:100, Z:179)
            ev.gifts.emplace_back(i, w.debt_aid);
        }
    }
}

}  // namespace bo::core
