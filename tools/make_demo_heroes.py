#!/usr/bin/env python3
"""Generates the heroes' demo: scenarios/demo_heroes.json and its autopilot scenarios/demo_heroes_build.json, for
the gate test "demo gate: heroes" (core/tests/test_demo_heroes.cpp, docs/PRAKTYKI.md ch. 4).

Owner's rule (DECYZJE.md, 2026-11-26): the demo shows every mechanic. The expansion's heroes, Atalanta and
Bellerophon (P:24), answer a hall only when the city meets their many demands; no other demo city meets them,
so this one is laid out for them (DECYZJE.md, 2026-11-28): Artemis's sanctuary, a working stadium, meat, wood
and eight companies for Atalanta against Hera's Sphinx; fifteen horses, tax collectors in every street, money,
bronze and wine for Bellerophon against Echidna. No Apollo: he would kill the monsters first (Z:129).
Run: python3 tools/make_demo_heroes.py   (then tools/sync_game_data.py copies scenarios/ into game/)
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
    for sector, prio in enumerate(values):  # husbandry, industry, distribution, hygiene, admin, culture, myth, army
        steps.append({"day": day, "priority": [sector, prio]})


def imports(day, at, goods):
    for good, n in goods:
        steps.append({"day": day, "at": at, "trade_order": {"good": good, "import": n}})


for y in (6, 14, 22, 30, 46):
    fill(0, y, W - 1, y, "=")
fill(W - 1, 6, W - 1, 46, "=")
fill(31, 6, 31, 30, "=")
fill(0, 47, W - 1, H - 1, ",")   # meadow: the deer's ground
grid[52][6] = "d"                # deer for the hunters: Atalanta wants meat (P:8, P:24)
grid[6][0] = "E"                 # land entry: caravans, immigrants (Z:17)

steps.append({"day": 0, "wage": 5})            # more of the people work (Z:59-60)
put("palace", 40, 0)                           # taxes and the army need it (Z:96, Z:135)
# Caravans, one post a partner and one good a trip each (Z:78-79): grain; bronze and wine; stone and wood.
put("trading_post", 0, 2); steps.append({"day": 1, "at": [0, 2], "trade_partner": 0})
put("trading_post", 5, 2); steps.append({"day": 1, "at": [5, 2], "trade_partner": 1})
put("trading_post", 10, 2); steps.append({"day": 1, "at": [10, 2], "trade_partner": 2})
imports(1, [0, 2], [("wheat", 24), ("meat", 8)])  # meat for Atalanta besides the hunters' (P:24)
imports(1, [5, 2], [("bronze", 8), ("wine", 8)])
imports(1, [10, 2], [("marble", 12), ("wood", 12), ("sculpture", 4)])
imports(200, [5, 2], [("bronze", 0), ("wine", 12)])  # bronze enough in store: the caravans bring wine
put("granary", 27, 2)
STORED = ("bronze", "wine", "marble", "sculpture", "wood")
for x, goods in ((15, ("bronze",)), (19, ("marble", "sculpture")), (23, ("wood",)), (53, ("wine",))):
    put("storehouse", x, 3)
    for good in STORED:  # each store its own goods only
        steps.append({"day": 2, "at": [x, 3], "storage_order": {"good": good, "kind": 0 if good in goods else 3,
                                                                 "limit": 32 if good in goods else 0}})
put("maintenance_office", 35, 2); put("watchpost", 37, 2); put("maintenance_office", 50, 2)
# Houses on two streets, their water, upkeep and watch; food from the agora, the athletes' 20 points make
# hovels (Z:29-31), whose rabble fill the companies (Z:136).
for y in (7, 12):
    houses(0, y, 14); houses(33, y, 14)
put("fountain", 28, 7); put("fountain", 61, 7); put("maintenance_office", 28, 12); put("maintenance_office", 61, 12)
put("watchpost", 28, 15); put("watchpost", 61, 15)
put("common_agora", 33, 14, rot=1); put("food_vendor_stall", 33, 15, day=1)
for x in (0, 7, 14, 21, 26, 56):
    put("tax_office", x, 15)  # clerks in every street (Z:96)
put("gymnasium", 40, 15); put("stadium", 45, 15)  # competitors to the stadium: it works (L-120)
priorities(0, [4, 3, 5, 5, 4, 4, 4, 4])  # the granaries first: they take food only fully staffed (Z:66)
# Sanctuaries of Artemis (Amazons, Atalanta's demand) and Ares (Sparti): companies the city cannot raise alone.
put("artisans_guild", 35, 31); put("artisans_guild", 35, 34)
put("sanctuary_artemis", 0, 31, day=30, retry=True, when={"marble": 20})
put("sanctuary_ares", 18, 31, day=200, retry=True, when={"marble": 13})
put("maintenance_office", 17, 41); put("watchpost", 33, 41)
# Horses for Bellerophon (P:24): three ranches, nobody to take their horses (Z:147-148).
for x, y in ((40, 31), (40, 42), (52, 31)):
    put("horse_ranch", x, y)
put("maintenance_office", 52, 40); put("watchpost", 54, 40)
for x in (40, 43, 46, 49, 52):
    put("wheat_farm", x, 47)  # grain to the ranches first (buildings.json delivery_priority)
# Meat for Atalanta: the hunters, a granary of their own, the meat kept from the stalls (Z:80).
put("hunting_lodge", 8, 47); put("granary", 16, 47)
steps.append({"day": 2, "at": [16, 47], "storage_order": {"good": "wheat", "kind": 3, "limit": 0}})
steps.append({"day": 0, "stockpile": "meat"})
put("maintenance_office", 21, 47)
# Sheep for the altars (Z:124): long without a sacrifice the god turns away and Atalanta's demand fails.
put("carding_shed", 26, 47)
for x in range(26, 34):
    put("sheep", x, 52, day=1)
# The monsters and their heroes (Z:130): a hall once each is in the city, then the summons.
SPHINX, ECHIDNA = 4 * 192, 5 * 192
put("heros_hall", 24, 23, day=SPHINX + 2)
steps.append({"day": SPHINX + 3, "every": 16, "summon_hero": [24, 23]})
put("heros_hall", 50, 23, day=ECHIDNA + 2)
steps.append({"day": ECHIDNA + 3, "every": 16, "summon_hero": [50, 23]})

scenario = {
    "id": "demo_heroes",
    "name_key": "scenario.demo_heroes.name",
    "note": "Demo herosów dodatku (P:23-24, Z:130), generowane przez tools/make_demo_heroes.py.",
    "seed": 20261131,
    "treasury": 120000,
    "difficulty": "M",
    "map": ["".join(r) for r in grid],
    "cities": [
        {"id": "paros", "relation": "ally", "by_sea": False, "opinion": 70, "military": 1, "economy": 3,
         "sells": {"wheat": [30, 300], "meat": [40, 80]}, "buys": {}},
        {"id": "naxos", "relation": "ally", "by_sea": False, "opinion": 70, "military": 2, "economy": 3,
         "sells": {"bronze": [80, 60], "wine": [110, 60]}, "buys": {}},
        {"id": "delos", "relation": "ally", "by_sea": False, "opinion": 70, "military": 1, "economy": 3,
         "sells": {"marble": [80, 200], "sculpture": [150, 20], "wood": [40, 100]}, "buys": {}},
    ],
    "events": [
        {"type": "monster", "monster": "sphinx", "strength": 1, "target": "citizens", "year": 4, "month": 0},
        {"type": "monster", "monster": "echidna", "strength": 1, "target": "citizens", "year": 5, "month": 0},
    ],
    "goals": [{"type": "slay", "target": "sphinx"}, {"type": "slay", "target": "echidna"}],
}
build = {"note": "Autopilot demo herosów (tools/make_demo_heroes.py). Do not edit by hand.", "steps": steps}

with open(os.path.join(ROOT, "scenarios", "demo_heroes.json"), "w", encoding="utf-8") as f:
    json.dump(scenario, f, ensure_ascii=False, indent=1)
    f.write("\n")
with open(os.path.join(ROOT, "scenarios", "demo_heroes_build.json"), "w", encoding="utf-8") as f:
    f.write('{"note": ' + json.dumps(build["note"], ensure_ascii=False) + ',\n "steps": [\n')
    f.write(",\n".join("  " + json.dumps(s, ensure_ascii=False) for s in steps))
    f.write("\n ]\n}\n")
print("demo_heroes.json and demo_heroes_build.json written:", len(steps), "steps")
