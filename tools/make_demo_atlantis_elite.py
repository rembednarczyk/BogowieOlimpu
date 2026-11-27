#!/usr/bin/env python3
"""Generates the Atlantean elite demo: scenarios/demo_atlantis_elite.json and its autopilot
scenarios/demo_atlantis_elite_build.json, for the gate test "demo gate: Atlantean elite"
(core/tests/test_demo_atlantis_elite.cpp, docs/PRAKTYKI.md ch. 4).

Owner's rule (DECYZJE.md, 2026-11-26): the demo shows every mechanic. Spearmen and charioteers come from the
elite houses of an Atlantean city (P:15), the charioteers with a chariot from the chariot stall (P:18-19). The
elite need appeal, all four sciences and six goods (Z:32-34, P:10-12); a district for them in the army demo
drew its workers from the fleet, so they have a city of their own (DECYZJE.md, 2026-11-28).
Run: python3 tools/make_demo_atlantis_elite.py   (then tools/sync_game_data.py copies scenarios/ into game/)
"""
import json
import os

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
W, H = 60, 52  # rows 37-51: huts for the hands the sciences and the chariots need
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


for y in (6, 14, 22, 30, 38, 46):
    fill(0, y, W - 1, y, "=")
for x in (W - 1,):  # one road down the east edge; the lane at x = 33 joins the elite street
    fill(x, 6, x, 46, "=")
fill(33, 15, 33, 21, "=")        # a short lane down to the elite street
grid[6][0] = "E"                 # land entry: caravans, immigrants (Z:17)

steps.append({"day": 0, "wage": 5})            # more of the people work (Z:59-60)
put("palace", 41, 31)                          # no army without it (Z:135)
put("maintenance_office", 50, 31); put("watchpost", 52, 31)
# Caravans: one post for the elite's goods, one for grain alone (a post's carrier takes one good a trip to the
# nearest taker, Z:78-79); a storehouse and a granary keep the stock an elite plot needs first (doc 18).
put("trading_post", 2, 2); steps.append({"day": 1, "at": [2, 2], "trade_partner": 0})
put("trading_post", 7, 2); steps.append({"day": 1, "at": [7, 2], "trade_partner": 1})
imports(1, [2, 2], [("fleece", 6), ("olive_oil", 6), ("wine", 4), ("armor", 4), ("wood", 4)])
imports(400, [2, 2], [("armor", 0), ("wine", 8)])  # the arms stall full: room on the caravans for wine
imports(1, [7, 2], [("wheat", 16)])
put("storehouse", 12, 3)
for good in ("fleece", "olive_oil", "wine", "armor"):
    steps.append({"day": 2, "at": [12, 3], "storage_order": {"good": good, "kind": 0, "limit": 8}})
put("granary", 15, 2)
put("maintenance_office", 21, 4); put("watchpost", 23, 4); put("maintenance_office", 25, 4)
# Hands: huts on the north streets with water, upkeep and a watch (Z:29, Z:84-90).
for y in (7, 12):
    houses(0, y, 9); houses(21, y, 9); houses(41, y, 8)
for x in (18, 39, 57):
    put("fountain", x, 7); put("maintenance_office", x, 12)
for y in (39, 44):
    houses(0, y, 9); houses(21, y, 9); houses(41, y, 8)
houses(6, 47, 6); houses(21, 47, 9); houses(41, 47, 8)
# Food for the huts once they eat (Z:69-71): hunger lowers the mood and the people stop coming (doc 24).
put("common_agora", 40, 14, rot=1); put("food_vendor_stall", 40, 15, day=1)
put("common_agora", 0, 46, rot=1); put("food_vendor_stall", 0, 47, day=1)
for x in (18, 39, 57):
    put("fountain", x, 39); put("maintenance_office", x, 44); put("watchpost", x, 47)
priorities(0, [4, 4, 5, 5, 3, 5, 1, 4])  # distribution, upkeep and science full; the ranch is the army's (Z:148)
# Science along the north side of the elite street (P:10-12): every kind of it, 80 points and the museum's 10.
put("museum", 0, 16); put("observatory", 7, 17); put("university", 13, 19); put("inventors_workshop", 16, 19)
put("laboratory", 21, 18); put("bibliotheke", 26, 20); put("bibliotheke", 29, 20)
put("maintenance_office", 26, 17); put("watchpost", 29, 17); put("maintenance_office", 35, 20)
# The elite street: plot, agora, plot, agora, plot; stone circles and baths east of them, topiaries and
# fountains of sculpture behind (Z:32, Z:47): an estate's appeal.
ELITE = 300  # once the stalls hold every good: a family that finds a need unmet leaves for good (Z:34)
for i, stall in enumerate(["food_vendor_stall", "fleece_vendor_stall", "oil_vendor_stall", "wine_vendor_stall",
                           "arms_vendor_stall", "chariot_vendor_stall"]):
    if i % 3 == 0:
        put("common_agora", 4 + 10 * (i // 3), 22, rot=1)
    put(stall, 4 + 10 * (i // 3) + 2 * (i % 3), 23, day=2)
for x in (0, 10, 20):
    put("elite_housing", x, 23, day=ELITE, retry=True, when={"fleece": 1, "olive_oil": 1, "food": 2})
put("stone_circle", 24, 23, day=ELITE - 10); put("baths", 28, 23, day=ELITE - 10)
put("stone_circle", 34, 23, day=ELITE - 10)
for i, x in enumerate(range(0, 39, 3)):
    put(["topiary", "dolphin_sculpture", "spring", "orrery"][i % 4], x, 27, day=ELITE - 10)
# Chariots (P:18-19): the ranch's horses, the caravans' wood.
put("horse_ranch", 2, 31)
put("chariot_factory", 10, 31)
put("maintenance_office", 15, 31); put("watchpost", 17, 31)

scenario = {
    "id": "demo_atlantis_elite",
    "name_key": "scenario.demo_atlantis_elite.name",
    "note": "Demo elity Atlantydy (P:10-19), generowane przez tools/make_demo_atlantis_elite.py.",
    "seed": 20261130,
    "treasury": 100000,
    "difficulty": "M",
    "culture": "atlantean",
    "map": ["".join(r) for r in grid],
    "cities": [
        {"id": "naxos", "relation": "ally", "by_sea": False, "opinion": 70, "military": 2, "economy": 3,
         "sells": {"fleece": [60, 60], "olive_oil": [100, 60], "wine": [110, 60], "armor": [150, 40],
                   "wood": [40, 40]},
         "buys": {}},
        {"id": "paros", "relation": "ally", "by_sea": False, "opinion": 70, "military": 1, "economy": 3,
         "sells": {"wheat": [30, 300]}, "buys": {}},
    ],
    "events": [],
    "goals": [{"type": "population", "value": 500}],  # the estates come and go with their needs (Z:34): the gate sees one
}
build = {"note": "Autopilot demo elity Atlantydy (tools/make_demo_atlantis_elite.py). Do not edit by hand.",
         "steps": steps}

with open(os.path.join(ROOT, "scenarios", "demo_atlantis_elite.json"), "w", encoding="utf-8") as f:
    json.dump(scenario, f, ensure_ascii=False, indent=1)
    f.write("\n")
with open(os.path.join(ROOT, "scenarios", "demo_atlantis_elite_build.json"), "w", encoding="utf-8") as f:
    f.write('{"note": ' + json.dumps(build["note"], ensure_ascii=False) + ',\n "steps": [\n')
    f.write(",\n".join("  " + json.dumps(s, ensure_ascii=False) for s in steps))
    f.write("\n ]\n}\n")
print("demo_atlantis_elite.json and demo_atlantis_elite_build.json written:", len(steps), "steps")
