#!/usr/bin/env python3
"""Generates the Atlantean army demo: scenarios/demo_atlantis_army.json and its autopilot
scenarios/demo_atlantis_army_build.json, for the gate test "demo gate: Atlantean army"
(core/tests/test_demo_atlantis_army.cpp, docs/PRAKTYKI.md ch. 4).

Owner's rule (DECYZJE.md, 2026-11-26): the demo shows every mechanic. The expansion's army (P:15-19) needs a city
of a thousand people and a coast, more than the Atlantis demo's small town holds, so it has its own city: archers
from the common houses, a frigate with Atlantean Fire, a tower that fires it, an army from the sea and the
Oceanids (DECYZJE.md, 2026-11-28).
Run: python3 tools/make_demo_atlantis_army.py   (then tools/sync_game_data.py copies scenarios/ into game/)
"""
import json
import os

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
W, H = 72, 48
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


fill(0, 40, W - 1, H - 1, "~")   # the sea; straight coast on row 39
fill(0, 37, 11, 39, "T")         # a wood by the shore for the frigate (P:18)
grid[H - 1][W - 1] = "S"         # sea entry: the landing and the Oceanids come in here
for y in (6, 14, 22, 30):
    fill(0, y, W - 1, y, "=")
fill(12, 36, W - 1, 36, "=")     # the shore street
for x in (20, 45, W - 1):  # the east road on the map edge: services fit beside the houses
    fill(x, 6, x, 36, "=")
grid[6][0] = "E"                 # land entry: caravans, immigrants (Z:17)

steps.append({"day": 0, "auto_defend": True})  # the generals take the field on their own (Z:145)
steps.append({"day": 0, "wage": 5})            # more of the people work (Z:59-60)
put("palace", 24, 15)                          # no army without it (Z:135)
put("maintenance_office", 34, 15); put("watchpost", 34, 18)
put("trading_post", 2, 2); steps.append({"day": 1, "at": [2, 2], "trade_partner": 0})  # by the land entry
put("maintenance_office", 6, 2); put("watchpost", 8, 2)
imports(1, [2, 2], [("wheat", 16), ("armor", 4), ("orichalc", 2), ("wood", 8)])
# Food from the caravans: a granary by the post, agoras along the streets (Z:66-71, Z:74-79).
put("granary", 10, 1)
put("common_agora", 46, 22, rot=1); put("food_vendor_stall", 46, 23, day=1)
put("common_agora", 46, 14, rot=1); put("food_vendor_stall", 46, 15, day=1)
put("common_agora", 21, 30, rot=1); put("food_vendor_stall", 21, 31, day=1)
# Houses along the streets with water, upkeep and a watch (Z:29, Z:84-90); scholars for the science that makes
# a shack a hovel, whose archers the generals call (P:10-11, P:15).
for y in (7, 12, 15, 20, 23, 28, 31):
    houses(0, y, 9)
for y in (7, 12, 20, 28):
    houses(47, y, 11)
houses(52, 15, 8); houses(52, 23, 8); houses(47, 31, 9)  # agoras at x 46-51 on rows 14 and 22; the tower
for y in (7, 23, 28):
    houses(21, y, 12)
houses(27, 31, 9)  # the agora on row 30
for x, y in [(18, 7), (18, 12), (18, 20), (18, 28), (69, 7), (69, 15), (69, 23), (69, 31)]:
    put("fountain", x, y)
put("maintenance_office", 18, 15); put("watchpost", 18, 23); put("maintenance_office", 18, 31)
put("maintenance_office", 69, 12); put("watchpost", 69, 20); put("maintenance_office", 69, 28)
put("watchpost", 46, 9); put("maintenance_office", 46, 33)
for x, y in [(12, 17), (36, 20), (58, 17), (12, 25), (36, 25), (58, 25), (58, 9), (36, 9)]:
    put("bibliotheke", x, y, day=30)
priorities(0, [3, 3, 5, 5, 3, 5, 1, 4])  # distribution and upkeep full, then science (Z:60-62)
# The fleet (P:15-18): wood from the shore's mill, armor and orichalc from the caravans through a storehouse by
# the wharf; the orichalc gives the frigate Atlantean Fire.
put("timber_mill", 13, 37, day=60)
put("storehouse", 36, 37, day=60)
for good, kind in (("wood", 0), ("armor", 1), ("orichalc", 1)):
    steps.append({"day": 61, "at": [36, 37], "storage_order": {"good": good, "kind": kind, "limit": 4}})
put("frigate_wharf", 28, 37, day=60)
put("maintenance_office", 32, 37, day=60); put("watchpost", 40, 37, day=60)
steps.append({"day": 200, "every": 32, "ships": 4})  # Crew All (Z:141)
# A tower on a double wall by the east shore, its road to the Palace (Z:146); orichalc in store: Atlantean Fire.
for y in range(31, 36):
    for x in (66, 67):
        put("wall", x, y, day=60)
put("tower", 66, 31, day=61)  # by the y = 30 street
steps.append({"day": 62, "man_towers": True})  # Man All (Z:146)

scenario = {
    "id": "demo_atlantis_army",
    "name_key": "scenario.demo_atlantis_army.name",
    "note": "Demo wojska Atlantydy (P:15-19), generowane przez tools/make_demo_atlantis_army.py.",
    "seed": 20261128,
    "treasury": 100000,
    "difficulty": "M",
    "culture": "atlantean",
    "map": ["".join(r) for r in grid],
    "cities": [
        {"id": "samos", "relation": "ally", "by_sea": False, "opinion": 70, "military": 2, "economy": 3,
         "sells": {"wheat": [30, 200], "armor": [150, 24], "orichalc": [200, 12], "wood": [40, 40]},
         "buys": {}},
        {"id": "lemnos", "relation": "rival", "by_sea": True, "opinion": 20, "military": 3, "economy": 2,
         "sells": {}, "buys": {}},
    ],
    "events": [
        {"type": "invasion", "city": "lemnos", "nation": "greek", "amount": 16, "year": 2, "month": 0, "by_sea": True},
        {"type": "invasion", "city": "lemnos", "nation": "oceanid", "amount": 16, "year": 2, "month": 8,
         "by_sea": True},
    ],
    "goals": [
        {"type": "population", "value": 600},
        {"type": "survive_until", "value": 3},
    ],
}
build = {"note": "Autopilot demo wojska Atlantydy (tools/make_demo_atlantis_army.py). Do not edit by hand.",
         "steps": steps}

with open(os.path.join(ROOT, "scenarios", "demo_atlantis_army.json"), "w", encoding="utf-8") as f:
    json.dump(scenario, f, ensure_ascii=False, indent=1)
    f.write("\n")
with open(os.path.join(ROOT, "scenarios", "demo_atlantis_army_build.json"), "w", encoding="utf-8") as f:
    f.write('{"note": ' + json.dumps(build["note"], ensure_ascii=False) + ',\n "steps": [\n')
    f.write(",\n".join("  " + json.dumps(s, ensure_ascii=False) for s in steps))
    f.write("\n ]\n}\n")
print("demo_atlantis_army.json and demo_atlantis_army_build.json written:", len(steps), "steps")
