#!/usr/bin/env python3
"""Generates the gods' demo: an adventure of two episodes (scenarios/demo_gods.json) and the autopilot of each
(scenarios/demo_gods_build.json for the parent city, scenarios/demo_gods_colony_build.json for the colony), for
the gate test "demo gate: gods" in core/tests/test_demo_gods.cpp (docs/PRAKTYKI.md ch. 4).

Owner's decision (DECYZJE.md, 2026-11-27): the eight gods the Greek demo has no room for (four sanctuaries a city,
data/gods_rules.json) show their gifts here, four in the parent city and four in a colony (Z:17, doc 23, L-119);
the colony is small, so a trireme's crew there is short when the generals call the rabble (Z:141).
Run: python3 tools/make_demo_gods.py   (then tools/sync_game_data.py copies scenarios/ into game/)
"""
import json
import os

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")


class City:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.grid = [["."] * w for _ in range(h)]
        self.steps = []

    def fill(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.grid[y][x] = c

    def hroad(self, y, x0=0, x1=None):
        self.fill(x0, y, self.w - 1 if x1 is None else x1, y, "=")

    def vroad(self, x, y0, y1):
        self.fill(x, y0, x, y1, "=")

    def put(self, t, x, y, day=0, rot=0, **kw):
        s = {"day": day, "place": t, "x": x, "y": y}
        if rot:
            s["rot"] = rot
        s.update(kw)
        self.steps.append(s)

    def houses(self, x, y, n, day=0):
        self.steps.append({"day": day, "place": "common_housing", "x": x, "y": y, "repeat": [n, 2, 0]})

    def step(self, **kw):
        self.steps.append(kw)

    def priorities(self, day, values):
        for sector, prio in enumerate(values):  # husbandry, industry, distribution, hygiene, admin, culture, myth, army
            self.steps.append({"day": day, "priority": [sector, prio]})

    def imports(self, day, at, goods):
        for good, n in goods:
            self.steps.append({"day": day, "at": at, "trade_order": {"good": good, "import": n}})

    def rows(self):
        return ["".join(r) for r in self.grid]


def services(c, y, x0, x1, day=0):
    """A fountain, an upkeep office and a watch every 12 tiles of a row of houses (Z:84-92)."""
    for x in range(x0, x1, 12):
        c.put("fountain", x, y, day=day); c.put("maintenance_office", x + 2, y, day=day)
        c.put("watchpost", x + 4, y, day=day)


# Partners: the marble, wood and sculptures of the sanctuaries come by caravan (Z:74-79, Z:117).
QUARRY_CITY = {"id": "corinth", "map": [28, 40], "relation": "ally", "by_sea": False, "opinion": 60, "military": 2, "economy": 3,
               "sells": {"marble": [80, 240], "wood": [40, 120], "sculpture": [150, 40], "armor": [150, 24],
                         "wheat": [30, 60]},
               "buys": {"fleece": [60, 12]}}

# --- The parent city: Zeus, Hades, Poseidon, Demeter -------------------------------------------------------
# Four big sanctuaries need a big city; its sea brings Poseidon's boats and Kraken, its rabble called by the
# generals empties the workforce, so the trireme's crew runs short (Z:141, Z:145).
P = City(92, 56)
P.fill(0, 40, 91, 49, ",")              # meadow: farms, sheep for the sacrifices (Z:43-46, Z:124)
P.fill(0, 50, 91, 55, "~")              # the sea; straight coast on row 49 (L-096)
P.fill(39, 43, 44, 45, "T")             # a grove of timber by the wharf (Z:52)
P.grid[55][91] = "S"                   # sea entry: ships of trade and war
for x in (8, 10, 30, 32):
    P.grid[52][x] = "f"                # fishing grounds (Z:47)
for x in (20, 22):
    P.grid[52][x] = "u"                # urchins (Z:47)
P.hroad(6)
for y in (24, 30, 36, 42):
    P.hroad(y)
P.hroad(47, 0, 38); P.hroad(47, 43, 60)  # the shore street: fisheries on rows 48-49 by the water (Z:47)
P.vroad(25, 42, 47); P.vroad(48, 42, 47)  # joined to the town: catches and timber reach the stores
P.hroad(46, 38, 43)                       # round the wharf: its supplies come 40 tiles at most (B-021)
for x in (25, 48, 71, 90):
    P.vroad(x, 6, 42)
P.grid[27][91] = "i"                   # invasion point (doc 23), off the streets
P.grid[6][0] = "E"                     # land entry: caravans, immigrants (Z:17); after the roads, or one hides it
P.step(day=0, auto_defend=True)
P.step(day=0, wage=5)                   # very high wages: more of the people work (Z:59-60, difficulty.json)
P.put("palace", 40, 0)                  # taxes, the army, the wharf's crews need it (Z:96, Z:135, Z:141)
P.put("maintenance_office", 50, 4); P.put("watchpost", 52, 4)  # a burnt Palace: no army, no taxes (Z:89-90)
P.put("infirmary", 55, 2)               # the plague spreads in a crowded city (Z:84-85)
P.put("trading_post", 2, 2); P.step(day=1, at=[2, 2], trade_partner=0)  # by the land entry: the caravans come in there (Z:17)
for x in (7, 10, 13):
    P.put("storehouse", x, 3)  # the sites' stone close at hand
P.put("maintenance_office", 17, 4); P.put("watchpost", 19, 4)  # or the post burns (Z:89-90)
P.imports(1, [2, 2], [("marble", 12), ("wood", 8), ("sculpture", 4), ("armor", 2), ("wheat", 6)])  # 32 loads
P.imports(100, [2, 2], [("armor", 0)])  # armour enough for the trireme's marines (Z:141)
P.put("granary", 17, 37); P.put("granary", 21, 37)
for x in (17, 21):  # room for fish and urchins too (Z:66-67, B-019)
    for good in ("wheat", "carrots"):
        P.step(day=1, at=[x, 37], storage_order={"good": good, "kind": 0, "limit": 8})
P.put("artisans_guild", 26, 37); P.put("artisans_guild", 28, 37)
P.put("maintenance_office", 30, 37); P.put("watchpost", 32, 37)
P.put("common_agora", 34, 36, rot=1); P.put("food_vendor_stall", 34, 37, day=1)
P.put("tax_office", 40, 37); P.put("fountain", 42, 37)
P.put("maintenance_office", 44, 37); P.put("watchpost", 49, 37); P.put("maintenance_office", 72, 37)
# Culture: 15 points make a shack a hovel (Z:30-31, B-008), whose rabble the generals call (Z:136, Z:145).
P.put("college", 52, 37); P.put("podium", 55, 37); P.put("gymnasium", 57, 37)
P.put("drama_school", 60, 37); P.put("theater", 63, 37); P.put("podium", 68, 37); P.put("podium", 74, 37)
for x in (33, 55, 78):
    P.put("podium", x, 22)
for x in range(0, 60, 3):
    if x not in (9, 12, 24, 39, 42, 45, 48, 51):
        P.put("wheat_farm" if x % 2 else "carrot_farm", x, 43)
P.put("granary", 9, 43)  # by the shore: a catch goes 25 tiles at most (production.json fishery)
P.put("storehouse", 49, 43)  # armour for the wharf within its 40 tiles (production.json trireme_wharf)
P.step(day=1, at=[49, 43], storage_order={"good": "armor", "kind": 1, "limit": 4})
P.put("timber_mill", 45, 43)  # its wood goes to a wharf in need first (buildings.json delivery_priority)
P.put("carding_shed", 60, 43)            # the shepherd keeps the sheep the priests take (Z:45, Z:124)
P.put("carding_shed", 12, 40)            # and a flock in the west for Zeus's priests: the east is far
for x in range(4, 12):
    P.put("sheep", x, 40)
for i in range(8):
    P.put("sheep", 66 + i % 4, 44 + 2 * (i // 4))
P.put("fishery", 8, 48); P.put("fishery", 30, 48); P.put("urchin_quay", 20, 48)  # Poseidon's quicker boats
P.put("maintenance_office", 12, 48); P.put("watchpost", 36, 48)
P.put("trireme_wharf", 39, 47, day=2)  # within 40 tiles of the storehouses: wood and armour (Z:141)
P.put("maintenance_office", 43, 48); P.put("watchpost", 45, 48)  # or the wharf burns (Z:89-90)
P.step(day=40, every=16, ships=4)       # crews called (Z:141): short-handed while the rabble is out
# Houses along the streets; services in their rows (Z:29).
for y in (25, 31):
    P.houses(0, y, 12); P.houses(27, y, 10); P.houses(49, y, 11); P.houses(72, y, 9)
P.houses(6, 28, 9); P.houses(27, 28, 10); P.houses(49, 28, 11); P.houses(72, 28, 9)
P.houses(0, 34, 12); P.houses(27, 34, 10); P.houses(49, 34, 11); P.houses(72, 34, 9)
P.put("fountain", 0, 28); P.put("maintenance_office", 2, 28); P.put("watchpost", 4, 28)  # Zeus's site ends on row 22
services(P, 22, 27, 47); services(P, 22, 49, 70); services(P, 22, 72, 89)
# Distribution first: a granary takes food only when fully staffed (Z:66); the sawmill and the wharf build the
# trireme (Z:141), the guilds the sanctuaries.
# Distribution first: a granary takes food only when fully staffed (Z:66); at equal priority the upkeep fills
# first (S-010), the rest share what is left (Z:60-62).
P.priorities(0, [4, 4, 5, 5, 3, 4, 4, 4])    # distribution and upkeep full; the rest share (Z:60-62)
P.priorities(950, [4, 4, 5, 5, 3, 5, 5, 5])  # then culture for hovels and the wharf: rabble and crews (Z:136, Z:141)
# The sanctuaries, the biggest first: each needs its first marble in the storehouses (Z:116).
SANCT = 30
P.put("sanctuary_zeus", 1, 7, day=SANCT, retry=True, when={"marble": 48})
P.put("sanctuary_hades", 27, 7, day=SANCT + 200, retry=True, when={"marble": 37})
P.put("sanctuary_poseidon", 50, 7, day=SANCT + 400, retry=True, when={"marble": 37})
P.put("sanctuary_demeter", 72, 7, day=SANCT + 600, retry=True, when={"marble": 25})
parent = {
    "id": "demo_gods",
    "name_key": "scenario.demo_gods.name",
    "note": "Demo bogów (Z:116-122, doc 23), generowane przez tools/make_demo_gods.py.",
    "seed": 20261128,
    "treasury": 150000,  # four big sanctuaries of imported stone (Z:116-117)
    "difficulty": "M",
    "map": P.rows(),
    "cities": [QUARRY_CITY, {"id": "sparta", "map": [18, 72], "relation": "rival", "opinion": 40, "military": 1, "economy": 2,
                             "sells": {}, "buys": {}}],
}
parent_goals = [{"type": "sanctuary", "target": g} for g in ("zeus", "hades", "poseidon", "demeter")] + \
    [{"type": "survive_until", "value": 7}]  # through both attacks (Z:144)
parent_events = [
    {"type": "god_attack", "god": "hermes", "year": 6, "month": 0},  # driven off by Zeus (Z:119)
    {"type": "invasion", "city": "sparta", "nation": "greek", "amount": 24, "year": 5, "month": 6},  # Cerberus
    {"type": "invasion", "city": "sparta", "nation": "greek", "amount": 16, "year": 6, "month": 6, "by_sea": True},
    {"type": "invasion", "city": "sparta", "nation": "greek", "amount": 24, "year": 6, "month": 10},  # the hovels'
]

# --- The colony: Artemis, Hephaestus, Aphrodite, Dionysus ----------------------------------------------------
C = City(64, 46)
C.fill(0, 34, 63, 45, ",")              # meadow: farms, sheep, the wolves' hunting ground (Z:43-45)
C.grid[44][62] = "w"                   # the wolves' den (Z:45)
C.hroad(4)
C.grid[4][0] = "E"                     # after the road, or the road hides it
for y in (16, 22, 28, 33):
    C.hroad(y)
for x in (17, 32, 48, 62):
    C.vroad(x, 4, 33)
C.step(day=0, auto_defend=True)
C.put("palace", 50, 34)                 # the Amazons' standards stand by it (Z:135, Z:140)
C.put("trading_post", 2, 0); C.step(day=1, at=[2, 0], trade_partner=1)  # by the land entry (Z:17)
for x in (7, 10):
    C.put("storehouse", x, 1)
C.put("maintenance_office", 14, 2); C.put("watchpost", 16, 2)  # Megara (0 is the parent city)
C.imports(1, [2, 0], [("marble", 12), ("wood", 8), ("sculpture", 4), ("wheat", 8)])
C.imports(SANCT + 600, [2, 0], [("marble", 0), ("wood", 0), ("sculpture", 0), ("wheat", 24)])  # grain at last
C.put("storehouse", 4, 29); C.put("storehouse", 7, 29); C.put("granary", 10, 29)
C.put("artisans_guild", 14, 29); C.put("artisans_guild", 18, 29)
C.put("common_agora", 24, 28, rot=1); C.put("food_vendor_stall", 24, 29, day=1)
C.put("maintenance_office", 33, 29); C.put("watchpost", 35, 29); C.put("fountain", 37, 29)
C.put("tax_office", 39, 29); C.put("maintenance_office", 41, 29)
for x in range(0, 43, 3):
    C.put("wheat_farm" if x % 2 else "onion_farm", x, 34)
C.put("carding_shed", 46, 34)
for i in range(8):
    C.put("sheep", 47 + 2 * (i % 4), 41 + 2 * (i // 4))
for y in (17, 23):
    C.houses(0, y, 8); C.houses(18, y, 7); C.houses(33, y, 7); C.houses(49, y, 6)
C.houses(0, 20, 6); C.houses(18, 20, 7); C.houses(33, 20, 7); C.houses(49, 20, 6)
C.houses(0, 26, 8); C.houses(18, 26, 7); C.houses(33, 26, 7); C.houses(49, 26, 6)
for x in (18, 33, 49):  # under the smaller sanctuaries; the west houses get theirs by the stores
    C.put("fountain", x, 14); C.put("maintenance_office", x + 2, 14)
C.put("fountain", 12, 20); C.put("maintenance_office", 14, 20)  # in the west row, where two houses would stand
C.priorities(0, [4, 1, 5, 4, 3, 1, 4, 2])  # distribution first: the granary (Z:66)
C.put("sanctuary_artemis", 1, 5, day=SANCT, retry=True, when={"marble": 24})
C.put("sanctuary_hephaestus", 19, 5, day=SANCT + 100, retry=True, when={"marble": 11})
C.put("sanctuary_aphrodite", 34, 5, day=SANCT + 200, retry=True, when={"marble": 13})
C.put("sanctuary_dionysus", 50, 5, day=SANCT + 300, retry=True, when={"marble": 9})
colony = {
    "id": "corinth",  # the parent's partner city that becomes its colony (scenarios/README.md)
    "treasury": 60000,
    "difficulty": "M",
    "map": C.rows(),
    "cities": [{"id": "parent", "map": [60, 20], "relation": "parent_city"}, QUARRY_CITY | {"id": "megara", "map": [26, 64]}],
    "goals": [{"type": "sanctuary", "target": g} for g in ("artemis", "hephaestus", "aphrodite", "dionysus")],
    "events": [],
}

adventure = dict(parent)
adventure["steps"] = [
    {"type": "parent", "goals": parent_goals, "events": parent_events},
    {"type": "colony", "colonies": [colony]},
    {"type": "victory"},
]


def write_build(path, note, steps):
    with open(os.path.join(ROOT, "scenarios", path), "w", encoding="utf-8") as f:
        f.write('{"note": ' + json.dumps(note, ensure_ascii=False) + ',\n "steps": [\n')
        f.write(",\n".join("  " + json.dumps(s, ensure_ascii=False) for s in steps))
        f.write("\n ]\n}\n")


with open(os.path.join(ROOT, "scenarios", "demo_gods.json"), "w", encoding="utf-8") as f:
    json.dump(adventure, f, ensure_ascii=False, indent=1)
    f.write("\n")
write_build("demo_gods_build.json", "Autopilot demo bogów, miasto macierzyste (tools/make_demo_gods.py).", P.steps)
write_build("demo_gods_colony_build.json", "Autopilot demo bogów, kolonia (tools/make_demo_gods.py).", C.steps)
print("demo_gods.json written:", len(P.steps), "+", len(C.steps), "steps")
