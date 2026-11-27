#!/usr/bin/env python3
"""Generates the demo of the expansion's gods: scenarios/demo_atlantis_gods.json and its autopilot
scenarios/demo_atlantis_gods_build.json, for the gate test "demo gate: Atlantean gods"
(core/tests/test_demo_atlantis_gods.cpp, docs/PRAKTYKI.md ch. 4).

Owner's rule (DECYZJE.md, 2026-11-26): the demo shows every mechanic. Hera, Atlas and the Citadel of Poseidon
(P:20-26) need some 200 hands between them, more than any other demo city spares, so they have their own
Atlantean city, laid out like the colony of the gods' demo (tools/make_demo_gods.py): stone by caravan, two
artisans' guilds, cattle for the altars, an orange lodge for Hera's groves, and Hera's Sphinx (DECYZJE.md,
2026-11-28).
Run: python3 tools/make_demo_atlantis_gods.py   (then tools/sync_game_data.py copies scenarios/ into game/)
"""
import json
import os

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
W, H = 64, 56
grid = [["."] * W for _ in range(H)]
steps = []


def fill(x0, y0, x1, y1, c):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            grid[y][x] = c


def put(t, x, y, day=0, rot=0, **kw):
    s = {"day": day, "place": t, "x": x, "y": y}
    if rot:
        s["rot"] = rot
    s.update(kw)
    steps.append(s)


def houses(x, y, n, day=0):
    steps.append({"day": day, "place": "common_housing", "x": x, "y": y, "repeat": [n, 2, 0]})


def priorities(day, values):
    for sector, prio in enumerate(values):  # husbandry, industry, distribution, hygiene, admin, science, myth, army
        steps.append({"day": day, "priority": [sector, prio]})


def imports(day, at, goods):
    for good, n in goods:
        steps.append({"day": day, "at": at, "trade_order": {"good": good, "import": n}})


fill(0, 44, W - 1, H - 1, ",")   # meadow: farms, cattle (P:7)
fill(0, 4, W - 1, 4, "=")
for y in (26, 32, 38, 43):
    fill(0, y, W - 1, y, "=")
for x in (21, 36, 62):
    fill(x, 4, x, 43, "=")
grid[4][0] = "E"                 # land entry (Z:17), after the road
grid[54][2] = "M"                # monster point, off the fields (doc 23)

steps.append({"day": 0, "auto_defend": True})
put("palace", 50, 44)
put("trading_post", 2, 0); steps.append({"day": 1, "at": [2, 0], "trade_partner": 0})
for x in (7, 10):
    put("storehouse", x, 1)
put("maintenance_office", 14, 2); put("watchpost", 16, 2)
SANCT = 30
imports(1, [2, 0], [("marble", 12), ("wood", 8), ("sculpture", 4), ("orichalc", 0), ("wheat", 8)])
imports(SANCT + 500, [2, 0], [("marble", 12), ("wood", 4), ("sculpture", 0), ("orichalc", 8), ("wheat", 8)])
put("storehouse", 4, 39); put("storehouse", 7, 39); put("granary", 10, 39)
put("artisans_guild", 14, 39); put("artisans_guild", 18, 39)
put("common_agora", 24, 38, rot=1); put("food_vendor_stall", 24, 39, day=1)
put("maintenance_office", 33, 39); put("watchpost", 37, 39); put("fountain", 39, 39)
put("maintenance_office", 41, 39)
for x in range(0, 40, 3):
    put("wheat_farm" if x % 2 else "onion_farm", x, 45)
# Cattle for the altars (P:22): the priests take them before the granary's food (Z:124).
put("corral", 42, 45)
for x in range(42, 49):
    put("cattle", x, 50, day=1)
# Sheep breed (doc 19), so the altars never go without (Z:124: long without a sacrifice the god turns away).
put("carding_shed", 60, 45)
for x in range(52, 60):
    put("sheep", x, 52, day=1)
# The orange tenders pick Hera's groves (P:8, P:21).
put("orange_tenders_lodge", 19, 22, day=SANCT)  # by Hera's site: her groves come up round it
put("granary", 22, 20, day=SANCT)  # oranges go 10 tiles at most (data/production.json)
for y in (27, 30, 33):
    houses(0, y, 9); houses(22, y, 6); houses(37, y, 11)
houses(0, 36, 9); houses(22, 36, 6); houses(44, 36, 8)
for x, y in [(18, 27), (34, 30), (34, 36), (60, 27), (60, 33)]:
    put("fountain", x, y)
for x, y in [(18, 30), (34, 27), (60, 30), (60, 36), (18, 36)]:
    put("maintenance_office", x, y)
put("watchpost", 34, 33); put("watchpost", 18, 33)
for x, y in [(28, 41), (46, 41), (56, 41)]:
    put("bibliotheke", x, y, day=10)  # science for the hovels (P:10-11)
priorities(0, [4, 1, 5, 4, 3, 3, 4, 2])  # distribution first: the granary (Z:66)
put("sanctuary_hera", 1, 6, day=SANCT, retry=True, when={"marble": 30})
put("sanctuary_atlas", 22, 6, day=SANCT + 60, retry=True, when={"marble": 25})
put("sanctuary_poseidon_citadel", 39, 5, day=SANCT + 300, retry=True, when={"marble": 32})
steps.append({"day": 600, "pray": [22, 6]})  # Atlas: the Citadel's stone comes at once (P:22)

# Stage MONUMENT (P:26-28, L-128), after the Citadel: a modest pyramid set down off the road; its plank reaches
# the street later, only then the carts come. Its building is held a while and resumed. The white marble is
# stockpiled (Z:80), so black marble from the caravans goes in. The expansion's aesthetics round the fields.
MONUMENT = 860
imports(MONUMENT, [2, 0], [("marble", 0), ("black_marble", 12), ("orichalc", 8)])
steps.append({"day": MONUMENT, "stockpile": "marble"})
put("pyramid_modest", 30, 21, day=MONUMENT)
steps.append({"day": MONUMENT + 30, "road": [31, 24, 31, 25]})  # the plank meets the y = 26 street
steps.append({"day": MONUMENT + 40, "hold_building": [30, 21], "on": True})
steps.append({"day": MONUMENT + 70, "hold_building": [30, 21], "on": False})
x = 0
for t, w in [("water_park", 2), ("bird_bath", 1), ("short_obelisk", 1), ("tall_obelisk", 1), ("shell_garden", 2),
             ("sundial", 2), ("dolphin_sculpture", 3), ("orrery", 3), ("spring", 3), ("topiary", 3), ("baths", 4),
             ("stone_circle", 4)]:
    put(t, x, 48, day=MONUMENT)
    x += w + 1

scenario = {
    "id": "demo_atlantis_gods",
    "name_key": "scenario.demo_atlantis_gods.name",
    "note": "Demo bogów dodatku (P:20-26), generowane przez tools/make_demo_atlantis_gods.py.",
    "seed": 20261129,
    "treasury": 120000,
    "difficulty": "M",
    "culture": "atlantean",
    "map": ["".join(r) for r in grid],
    "cities": [
        {"id": "samos", "relation": "ally", "by_sea": False, "opinion": 60, "military": 2, "economy": 3,
         "sells": {"marble": [80, 300], "wood": [40, 80], "sculpture": [150, 20], "orichalc": [200, 40],
                   "wheat": [30, 80], "black_marble": [90, 30]},
         "buys": {}},
    ],
    "events": [
        {"type": "monster", "monster": "sphinx", "strength": 1, "year": 5, "month": 0},  # Hera's (P:23)
    ],
    "goals": [{"type": "sanctuary", "target": g} for g in ("hera", "atlas", "poseidon")] +
             [{"type": "monument", "target": "pyramid_modest"}],
}
build = {"note": "Autopilot demo bogów dodatku (tools/make_demo_atlantis_gods.py). Do not edit by hand.",
         "steps": steps}

with open(os.path.join(ROOT, "scenarios", "demo_atlantis_gods.json"), "w", encoding="utf-8") as f:
    json.dump(scenario, f, ensure_ascii=False, indent=1)
    f.write("\n")
with open(os.path.join(ROOT, "scenarios", "demo_atlantis_gods_build.json"), "w", encoding="utf-8") as f:
    f.write('{"note": ' + json.dumps(build["note"], ensure_ascii=False) + ',\n "steps": [\n')
    f.write(",\n".join("  " + json.dumps(s, ensure_ascii=False) for s in steps))
    f.write("\n ]\n}\n")
print("demo_atlantis_gods.json and demo_atlantis_gods_build.json written:", len(steps), "steps")
