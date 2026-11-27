#!/usr/bin/env python3
"""Generates the Atlantis demo: scenarios/demo_atlantis.json and scenarios/demo_atlantis_build.json (the autopilot
for its gate test "demo gate: Atlantis" in core/tests/test_demo.cpp, docs/PRAKTYKI.md ch. 4).

Owner's decision (DECYZJE.md, 2026-11-26): the demo shows every mechanic we have. Atlantean buildings stand only
in an Atlantean city (P:16-17), so they have their own small city: science instead of culture (P:10-13) and the
hippodrome (P:13-15).
Run: python3 tools/make_demo_atlantis.py   (then tools/sync_game_data.py copies scenarios/ into game/)
"""
import json
import os

W, H = 48, 40
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
grid = [["."] * W for _ in range(H)]


def fill(x0, y0, x1, y1, c):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            grid[y][x] = c


fill(31, 23, 46, 29, ",")     # meadow: farms (Z:43)
grid[6][0] = "E"              # land entry: immigrants (Z:17)
for y in (6, 14, 22, 37):      # streets
    fill(1, y, W - 1, y, "=")
for x in (12, 30):
    fill(x, 6, x, 37, "=")

steps = []


def put(t, x, y, day=0, rot=0, **kw):
    s = {"day": day, "place": t, "x": x, "y": y}
    if rot:
        s["rot"] = rot
    s.update(kw)
    steps.append(s)


def houses(x, y, n, day=0):
    steps.append({"day": day, "place": "common_housing", "x": x, "y": y, "repeat": [n, 2, 0]})


FOOD, SCIENCE, TRACK = 0, 120, 300  # in stages, as the workforce grows (B-003)
for sector, prio in enumerate([4, 1, 5, 5, 3, 2, 2, 1]):  # distribution first: the granary takes food only
    steps.append({"day": 0, "priority": [sector, prio]})   # when fully staffed (Z:66)

# Houses along the streets, water, upkeep and a watch in their rows (Z:29, Z:84-90).
houses(1, 7, 2); put("fountain", 5, 7); put("maintenance_office", 7, 7); houses(9, 7, 1)
houses(13, 7, 3); put("fountain", 19, 7); put("watchpost", 21, 7); houses(23, 7, 3)
houses(1, 12, 2); houses(7, 12, 2)
houses(13, 12, 3); houses(23, 12, 3)
# Food: farms on the meadow, a granary, an agora on the y = 14 street (Z:66-71).
put("carrot_farm", 32, 23); put("onion_farm", 35, 23)
put("granary", 13, 15)
put("common_agora", 19, 14, rot=1)
put("food_vendor_stall", 19, 15, day=1)
# Stage SCIENCE: scholars for the houses (P:10-11), more food.
put("bibliotheke", 5, 12, day=SCIENCE); put("bibliotheke", 19, 12, day=SCIENCE)
put("maintenance_office", 21, 12, day=SCIENCE)
put("carrot_farm", 38, 23, day=SCIENCE); put("onion_farm", 41, 23, day=SCIENCE)
# Stage TRACK: the hippodrome (P:13-15): a ring of eight stades round an empty middle, the first by the y = 22
# street keeps the horses; a crosswalk over the top stade (P:14); the ranch takes wheat from the storehouse.
put("wheat_farm", 44, 23, day=TRACK)
put("horse_ranch", 32, 15, day=TRACK)
put("storehouse", 40, 15, day=TRACK)
steps.append({"day": TRACK + 1, "at": [40, 15], "storage_order": {"good": "wheat", "kind": 1, "limit": 8}})
for x, y in [(13, 23), (17, 23), (21, 23), (21, 27), (21, 31), (17, 31), (13, 31), (13, 27)]:
    put("hippodrome", x, y, day=TRACK)
put("crosswalk", 18, 23, day=TRACK + 1)
put("maintenance_office", 25, 15, day=TRACK); put("watchpost", 27, 15, day=TRACK)
put("maintenance_office", 26, 23, day=TRACK); put("watchpost", 28, 23, day=TRACK)

scenario = {
    "id": "demo_atlantis",
    "name_key": "scenario.demo_atlantis.name",
    "note": "Demo Atlantydy (P:10-17), generowane przez tools/make_demo_atlantis.py.",
    "seed": 20261127,
    "treasury": 20000,
    "difficulty": "M",
    "culture": "atlantean",
    "map": ["".join(r) for r in grid],
    "cities": [],
    "events": [],
    "goals": [
        {"type": "population", "value": 250},
        {"type": "houses", "target": "hovel", "value": 6},  # 15 points: the scholars' science (P:10-11)
        {"type": "hippodrome", "value": 8},
    ],
}
build = {"note": "Autopilot demo Atlantydy (tools/make_demo_atlantis.py). Do not edit by hand.", "steps": steps}

with open(os.path.join(ROOT, "scenarios", "demo_atlantis.json"), "w", encoding="utf-8") as f:
    json.dump(scenario, f, ensure_ascii=False, indent=1)
    f.write("\n")
with open(os.path.join(ROOT, "scenarios", "demo_atlantis_build.json"), "w", encoding="utf-8") as f:
    f.write('{"note": ' + json.dumps(build["note"], ensure_ascii=False) + ',\n "steps": [\n')
    f.write(",\n".join("  " + json.dumps(s, ensure_ascii=False) for s in steps))
    f.write("\n ]\n}\n")
print("demo_atlantis.json and demo_atlantis_build.json written:", len(steps), "steps")
