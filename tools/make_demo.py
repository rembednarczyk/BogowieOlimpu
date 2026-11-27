#!/usr/bin/env python3
"""Generates the demo scenario: scenarios/demo.json (map, world, events, goals) and scenarios/demo_build.json
(the autopilot build order read by the game and by the whole-game gate test, docs/PRAKTYKI.md ch. 4).

Owner's decision (DECYZJE.md, 2026-11-26): the demo can be won and always holds every mechanic we have.
Run: python3 tools/make_demo.py   (then tools/sync_game_data.py copies scenarios/ into game/)
"""
import json
import os

W, H = 120, 60  # the east district (x 76-113, stage EAST) beyond the first wall line
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")

grid = [["."] * W for _ in range(H)]


def fill(x0, y0, x1, y1, c):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            grid[y][x] = c


def hline(y, x0, x1, c="="):
    fill(x0, y, x1, y, c)


def vline(x, y0, y1, c="="):
    fill(x, y0, x, y1, c)


# --- Terrain --------------------------------------------------------------------------------------------
fill(0, 0, 23, 6, "T")             # forest: timber (Z:52)
fill(26, 0, 33, 6, "#")            # rock with copper ore inside (Z:53)
fill(27, 1, 32, 6, "c")            # open to the south: miners walk up to the ore
fill(36, 0, 41, 5, "s")            # silver ore: the mint (Z:54)
fill(44, 0, 55, 6, "q")            # marble quarry (Z:52)
fill(58, 0, 61, 3, "k")            # black marble (expansion terrain, shown only)
fill(62, 0, W - 1, 7, "#")         # rock closes the north-east corner
fill(2, 24, 11, 44, ",")           # meadows: farms, sheep, goats (Z:43-46)
fill(13, 36, 31, 44, ",")
fill(0, 50, W - 1, 59, "~")        # the sea; straight coast on row 49 (L-096)
fill(100, 40, 111, 44, ",")        # east meadow: wheat for the horses (Z:148)
for x, y in [(18, 53), (24, 55), (30, 53)]:
    grid[y][x] = "f"               # fishing grounds (Z:47)
for x, y in [(42, 51), (44, 51)]:
    grid[y][x] = "u"               # urchins (Z:47)
grid[56][79] = "S"                 # sea entry: trade ships, enemy ships
grid[8][14] = "h"                  # boars (Z:46)
grid[8][20] = "d"                  # deer
grid[47][3] = "w"                  # wolves by the south-west shore (Z:45)
grid[30][W - 2] = "i"              # invasion point east of the wall (Z:136)
grid[9][66] = "M"                  # monster point (doc 23)

# --- Roads ----------------------------------------------------------------------------------------------
grid[11][1] = "E"
hline(11, 2, W - 1)                # main road from the land entry, through the gate to the east edge
for x in (12, 32, 52, 72):
    vline(x, 11, 45)
hline(23, 2, 73)
hline(35, 2, 52)
hline(45, 2, 73)                   # the roads of the east district come with stages ARMY and EAST
hline(17, 12, 52)                  # inner streets of the house blocks
hline(29, 12, 52)

world = [
    {"id": "corinth", "relation": "ally", "by_sea": False, "opinion": 60, "military": 2, "economy": 3,
     "sells": {"olive_oil": [145, 12], "fleece": [60, 24], "marble": [80, 60], "wood": [50, 24], "sculpture": [200, 8],
               "wine": [120, 24], "armor": [150, 12]},
     "buys": {"bronze": [110, 16], "wood": [70, 16], "drachmas_note": [0, 0]}},
    {"id": "sparta", "relation": "rival", "opinion": 20, "military": 3, "economy": 2, "sells": {}, "buys": {}},
    {"id": "rhodes", "relation": "ally", "by_sea": True, "opinion": 85, "military": 2, "economy": 3,
     "sells": {"wine": [110, 24], "fish": [40, 24]}, "buys": {"olive_oil": [160, 12], "sculpture": [260, 8]}},
]
# Argos: a vassal that dislikes the player and rises after a year (L-121); Rhodes above is fond of the player
# and sends a goodwill gift after two years.
world.append({"id": "argos", "relation": "vassal", "opinion": 25, "military": 1, "economy": 2, "tribute": 300,
              "sells": {}, "buys": {}})
for c in world:
    c["buys"].pop("drachmas_note", None)

scenario = {
    "id": "demo_first_city",
    "name_key": "scenario.demo_first_city.name",
    "note": "Demo w naszym formacie (doc 23, scenarios/README.md), generowane przez tools/make_demo.py. Nazwy miast robocze.",
    "seed": 20261126,
    "treasury": 40000,
    "difficulty": "M",
    "map": ["".join(r) for r in grid],
    "elevation": ["".join("1" if (x < 2 and y < 11) else "0" for x in range(W)) for y in range(H)],
    "cities": world,
    "events": [
        {"type": "gift", "city": "corinth", "good": "drachmas", "amount": 500, "year": 0, "month": 2},
        {"type": "request", "city": "corinth", "good": "fleece", "amount": 2, "year": 0, "month": 5,
         "deadline_months": 6, "repeat": True},
        {"type": "invasion", "city": "sparta", "nation": "greek", "amount": 8, "year": 2, "month": 3},
        {"type": "god_attack", "god": "hermes", "year": 1, "month": 8},
        {"type": "disaster", "kind": "earthquake", "x": 69, "y": 48, "radius": 1, "year": 3, "month": 6},
        {"type": "quest", "god": "hermes", "index": 0, "hero": "perseus", "year": 3, "month": 0},
        {"type": "games_host", "host_year": 8, "year": 7, "month": 0},
        {"type": "request", "city": "corinth", "good": "army", "amount": 1, "year": 5, "month": 6,
         "deadline_months": 3},
        {"type": "invasion", "city": "sparta", "nation": "greek", "amount": 16, "year": 5, "month": 9,
         "by_sea": True},  # transports with a warship from the sea entry (Z:144); the triremes meet them
        {"type": "monster", "monster": "medusa", "strength": 1, "target": "resources", "year": 6, "month": 0},
    ],
    "goals": [
        {"type": "population", "value": 1200},
        {"type": "houses", "target": "tenement", "value": 10},
        {"type": "houses", "target": "residence", "value": 2},
        {"type": "sanctuary", "target": "hermes"},
        {"type": "quest", "target": "hermes"},
        # Goals of the editor (S-008, L-093b), met before the ones above: trade, an army, goods set aside, time.
        {"type": "trading_partners", "value": 2},
        {"type": "support", "target": "rabble", "value": 40},
        {"type": "set_aside_goods", "target": "marble", "value": 20},
        {"type": "survive_until", "value": 4},
        {"type": "complete_before", "value": 9},
    ],
}

# --- Build order ----------------------------------------------------------------------------------------
# In stages, as a player builds (B-003): unattended buildings burn and collapse within a year (C7), so each
# stage adds only what the workforce of the houses standing by then can staff.
FOOD, FLEECE, CULTURE, INDUSTRY, MYTH, ARMY = 0, 48, 128, 224, 256, 320
ELITE = 400  # gardens, culture and stalls near the elite plots first: an elite family that finds its needs
             # unmet leaves for good (Z:34); the plots wait for oil left in storage (doc 18)
OIL = 300
steps = []


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


# The Granary takes food only when fully staffed (Z:66): distribution before husbandry.
priorities(FOOD, [4, 1, 5, 5, 3, 2, 2, 1])
priorities(CULTURE, [4, 3, 5, 5, 3, 4, 3, 1])
priorities(ARMY, [4, 3, 5, 5, 3, 4, 3, 2])
priorities(MYTH + 400, [4, 3, 5, 5, 3, 4, 3, 4])  # the armory staffed: armour for the mansions (Z:33)
put("palace", 53, 12)  # first: the Tax Office, the trireme wharf and the towers need it (Z:96, Z:141, Z:146)

# Stage FOOD: houses, water, maintenance, safety, farms, granaries, the first agora, fishing.
houses(13, 12, 9)
houses(13, 15, 4); put("fountain", 21, 15); houses(23, 15, 4)
houses(13, 18, 4); put("maintenance_office", 21, 18); houses(23, 18, 3)
houses(13, 21, 4); put("watchpost", 21, 21); houses(23, 21, 4)
put("infirmary", 33, 12); houses(37, 12, 7)
houses(37, 15, 2); put("fountain", 41, 15); houses(43, 15, 4)
houses(33, 18, 4); put("maintenance_office", 41, 18); houses(43, 18, 3)
houses(33, 21, 4); put("fountain", 41, 21); houses(43, 21, 4)
houses(13, 24, 4); put("fountain", 21, 24); houses(30, 24, 1)
# Agoras lie along a street with their long side on it (rotation 1: 6 wide, Z:69-70).
put("common_agora", 24, 23, rot=1)
put("food_vendor_stall", 24, 24, day=1)
houses(13, 27, 4); put("watchpost", 21, 27); houses(23, 27, 3)
houses(13, 30, 4); put("maintenance_office", 21, 30); houses(30, 30, 1)
houses(13, 33, 4); put("fountain", 21, 33); houses(23, 33, 4)
for y in (24, 27, 30, 36, 39, 42):
    put("onion_farm" if y in (24, 36) else "carrot_farm", 9, y)
put("wheat_farm", 2, 24)
put("maintenance_office", 7, 33); put("maintenance_office", 7, 43)
put("granary", 13, 36); put("granary", 17, 36)
put("maintenance_office", 21, 36)
put("maintenance_office", 17, 9); put("maintenance_office", 42, 9); put("maintenance_office", 62, 9)
for x in (18, 25):
    steps.append({"day": 0, "road": [x, 46, x, 47]})
    put("fishery", x - 2, 48)
put("maintenance_office", 20, 46)

# Stage FLEECE: sheep and goats, the storehouse, fleece and cheese, the trading post, oil.
put("carding_shed", 6, 24, day=FLEECE); put("dairy", 6, 36, day=FLEECE)
for i in range(8):
    put("sheep", 2 + i % 4, 28 + i // 4, day=FLEECE + 1)
    put("goat", 2 + i % 4, 31 + i // 4, day=FLEECE + 1)
put("storehouse", 23, 36, day=FLEECE); put("storehouse", 9, 46, day=FLEECE)
put("carding_shed", 9, 33, day=FLEECE)
for i in range(8):
    put("sheep", 2 + i % 4, 39 + i // 4, day=FLEECE + 1)
# Food, fleece and oil for the east blocks: the second agora on the y = 29 street (Z:69-71).
put("common_agora", 44, 29, rot=1, day=FLEECE)
for i, stall in enumerate(["food_vendor_stall", "fleece_vendor_stall", "oil_vendor_stall"]):
    put(stall, 44 + 2 * i, 30, day=[FLEECE + 1, CULTURE, OIL][i])
for x, y in [(23, 36), (9, 46), (23, 8), (56, 8)]:
    steps.append({"day": FLEECE + 2, "at": [x, y], "storage_order": {"good": "wine", "kind": 0, "limit": 8}})
for x, y in [(23, 36), (9, 46)]:  # fleece leaves room for the oil of the presses (B-014: oil stuck at the press)
    steps.append({"day": FLEECE + 2, "at": [x, y], "storage_order": {"good": "fleece", "kind": 0, "limit": 12}})
put("storehouse", 10, 8, day=INDUSTRY)
for x, y in [(10, 8), (23, 8), (56, 8)]:  # the storehouses by the workshops keep room for wood, bronze, sculpture
    steps.append({"day": INDUSTRY + 1, "at": [x, y], "storage_order": {"good": "fleece", "kind": 3, "limit": 0}})
    steps.append({"day": INDUSTRY + 1, "at": [x, y], "storage_order": {"good": "marble", "kind": 0, "limit": 12}})
# Fleece and oil stalls come with the CULTURE stage: until then the goods stay in storage, which elite houses
# need to be placed (doc 18: 2 food, 1 fleece, 1 oil in storage).
put("fleece_vendor_stall", 26, 24, day=CULTURE); put("oil_vendor_stall", 28, 24, day=OIL)
put("growers_lodge", 26, 36, day=FLEECE); put("olive_press", 28, 36, day=FLEECE)
for i in range(10):
    put("olive_tree", 18 + i, 40, day=FLEECE)
for x, y in [(x, y) for y in (42, 43, 44) for x in range(2, 7)] + [(7, 42), (8, 42)]:  # a second grove
    put("olive_tree", x, y, day=CULTURE)
put("growers_lodge", 6, 46, day=CULTURE); put("olive_press", 13, 46, day=CULTURE); put("olive_press", 15, 46, day=CULTURE)
for x in (13, 16, 19, 22, 25, 28):  # more food along the south road (B-003: food first)
    put("carrot_farm" if x % 2 else "onion_farm", x, 42, day=CULTURE)
put("trading_post", 33, 36, day=FLEECE)
put("maintenance_office", 33, 40, day=FLEECE)
steps.append({"day": FLEECE + 1, "at": [33, 36], "trade_partner": 0})
for good, n in [("olive_oil", 4), ("marble", 8), ("wood", 4), ("sculpture", 2)]:
    steps.append({"day": FLEECE + 1, "at": [33, 36], "trade_order": {"good": good, "import": n}})
# Perseus wants 16 fleece and 6 sculptures in storage (doc 21).
steps.append({"day": MYTH + 160, "at": [33, 36], "trade_order": {"good": "fleece", "import": 16}})
steps.append({"day": MYTH + 160, "at": [33, 36], "trade_order": {"good": "sculpture", "import": 6}})

# Stage CULTURE (Z:105-109): gymnasium, college and podiums, drama school and theater; more houses.
put("college", 33, 24, day=CULTURE); put("podium", 36, 24, day=CULTURE); put("gymnasium", 38, 24, day=CULTURE)
put("drama_school", 41, 24, day=CULTURE); put("theater", 44, 24, day=CULTURE); put("podium", 49, 24, day=CULTURE)
put("podium", 49, 27, day=CULTURE)
houses(33, 30, 4, day=CULTURE); put("fountain", 41, 30, day=CULTURE)
houses(33, 33, 4, day=CULTURE); put("watchpost", 41, 33, day=CULTURE); houses(43, 33, 3, day=CULTURE)
# Tax collectors register houses for 50 days a visit (doc 24): an office in every block (Z:96).
for x, y in [(29, 18), (49, 18), (29, 27), (49, 33), (62, 12)]:
    put("tax_office", x, y, day=CULTURE)
steps.append({"day": CULTURE, "tax": 3})

# Stage INDUSTRY: timber, marble, bronze, silver, sculpture, armour, wine; the north storehouses.
for t, x in [("timber_mill", 4), ("timber_mill", 6), ("boar_hunters_lodge", 14), ("hunting_lodge", 20),
             ("foundry", 27), ("foundry", 30), ("sculpture_studio", 33), ("sculpture_studio", 35), ("mint", 37), ("masonry_shop", 45),
             ("masonry_shop", 48)]:
    put(t, x, 9, day=INDUSTRY)
put("armory", 40, 9, day=MYTH + 400)  # bronze for the sculptures of the sanctuaries first (Z:117)
put("storehouse", 23, 8, day=INDUSTRY); put("storehouse", 56, 8, day=INDUSTRY)
put("watchpost", 60, 9, day=INDUSTRY)
put("winery", 30, 36, day=INDUSTRY)
for i in range(10):
    put("grapevine", 18 + i, 41, day=INDUSTRY)
put("common_agora", 24, 29, rot=1, day=INDUSTRY)
put("wine_vendor_stall", 24, 30, day=INDUSTRY + 1)  # armour only at the east agora, for the mansions
put("food_vendor_stall", 28, 30, day=INDUSTRY + 1)
put("common_agora", 50, 36, day=INDUSTRY)  # along the x = 52 road, for the south and the elite houses
for i, stall in enumerate(["food_vendor_stall", "fleece_vendor_stall", "oil_vendor_stall"]):
    put(stall, 50, 36 + 2 * i, day=[INDUSTRY + 1, INDUSTRY + 1, OIL][i])
steps.append({"day": 0, "road": [41, 46, 41, 47]})
put("urchin_quay", 42, 48, day=INDUSTRY)
put("maintenance_office", 44, 46, day=INDUSTRY)
put("pier", 34, 46, day=INDUSTRY)
steps.append({"day": INDUSTRY + 1, "at": [34, 46], "trade_partner": 2})
steps.append({"day": INDUSTRY + 1, "at": [34, 46], "trade_order": {"good": "wine", "export_keep": 16}})

# Stage MYTH: the guild, Hermes's sanctuary (the goal), later Athena's (Perseus needs two, doc 21).
put("artisans_guild", 68, 12, day=MYTH)
put("maintenance_office", 68, 43, day=MYTH); put("watchpost", 70, 24, day=MYTH)
put("sanctuary_hermes", 53, 24, day=MYTH, retry=True, when={"marble": 9})
put("sanctuary_athena", 53, 31, day=MYTH + 160, retry=True, when={"marble": 24})
put("heros_hall", 64, 12, day=MYTH, retry=True)
# Ares's fortress by the south road, once Athena's is up: two companies of Sparti for the player (Z:121, Z:140).
put("sanctuary_ares", 37, 37, day=800, retry=True, when={"marble": 13})  # only once a hero is needed (Z:130)

# Stage ARMY (Z:145-146): the east wall two thick, a gatehouse on the main road, towers, horses, the wharf.
WALL = W - 6  # the east wall beyond the east district
# Roadblocks at the district's edge keep the roaming walkers of the old city at home (Z:19-20); the road along
# the wall reaches its towers (they need a road to the Palace, Z:146).
steps.append({"day": 0, "roadblock": [76, 11]})
steps.append({"day": ARMY, "road": [WALL - 2, 11, WALL - 2, 45]})
steps.append({"day": ARMY, "road": [74, 45, WALL - 1, 45]})
steps.append({"day": ARMY + 1, "roadblock": [76, 45]})
steps.append({"day": ARMY, "road": [WALL - 1, 23, WALL - 1, 23]})
for y in list(range(8, 9)) + list(range(14, 50)):
    for x in (WALL, WALL + 1):
        put("wall", x, y, day=ARMY)
put("gatehouse", WALL - 1, 9, rot=1, day=ARMY)
put("tower", WALL, 22, day=ARMY + 1)
steps.append({"day": ARMY + 2, "man_towers": True})  # Man All: the generals send sentries (Z:146)
put("tower", WALL, 44, day=ARMY + 1)

# Stage JOBS: work for the people of the grown city (unemployment lowers the popularity, Z:159): culture in
# the west block, more fishing, a third sawmill; wood and bronze go to Corinth.
JOBS = 450
put("stadium", 2, 12, day=JOBS)  # one per city (Z:109); competitors from the gymnasiums; hosting the games (Z:111)
put("infirmary", 2, 19, day=JOBS)
put("college", 9, 20, day=JOBS); put("gymnasium", 6, 20, day=JOBS); put("fountain", 10, 17, day=JOBS)
for x in (30, 46):
    steps.append({"day": JOBS, "road": [x, 46, x, 47]})
    put("fishery", x - 2, 48, day=JOBS)
put("timber_mill", 8, 9, day=JOBS)
put("masonry_shop", 50, 9, day=JOBS); put("masonry_shop", 53, 9, day=JOBS)
steps.append({"day": JOBS, "road": [54, 46, 54, 46]})
put("trireme_wharf", 53, 47, day=JOBS)
steps.append({"day": JOBS + 1, "at": [33, 36], "trade_order": {"good": "wood", "export_keep": 16}})
steps.append({"day": JOBS + 1, "at": [33, 36], "trade_order": {"good": "bronze", "export_keep": 8}})
put("horse_ranch", 56, 46, day=ARMY)
put("maintenance_office", 64, 46, day=ARMY)
steps.append({"day": ARMY, "road": [49, 46, 49, 46]})
put("trireme_wharf", 50, 47, day=ARMY)
put("storehouse", 71, 46, day=ARMY)  # wood for the triremes (Z:141): the timber mills are 50 tiles away
steps.append({"day": ARMY + 1, "at": [71, 46], "storage_order": {"good": "wood", "kind": 1, "limit": 8}})

# Stage ELITE (Z:32-34): gardens raise the appeal, elite houses once their goods are in storage.
for x, y in [(62, 15), (68, 18)]:
    put("flower_garden", x, y, day=ELITE)
put("gazebo", 62, 17, day=ELITE); put("column", 68, 21, day=ELITE)
put("gymnasium", 61, 20, day=ELITE)  # athletes along the y = 23 road past the elite plots (20 points)
for x, y in [(53, 19), (57, 19), (64, 19)]:
    put("elite_housing", x, y, day=ELITE + 200, retry=True)
put("podium", 70, 18, day=ELITE); put("hedge_maze", 69, 20, day=ELITE)
# Mansions (Z:33, Z:136): a theater by the elite plots (actors, 25 culture points), columns along their
# north side for the appeal, and two agoras by the east wall: food, fleece and oil; armour, wine and horses.
put("theater", 65, 24, day=ELITE)
for x in range(53, 62):
    put("column", x, 18, day=ELITE)
# Roadblocks (Z:19-20) keep the peddlers on the road past the elite plots: north up the wall road, then west
# along y = 23.
for x, y in [(72, 22), (72, 44)]:
    steps.append({"day": ELITE, "roadblock": [x, y]})
put("granary", 68, 14, day=ELITE)  # food for the east agora, fetched from the west granaries
for good in ["onions", "carrots"]:
    steps.append({"day": ELITE + 1, "at": [68, 14], "storage_order": {"good": good, "kind": 1, "limit": 8}})
put("storehouse", 69, 29, day=ELITE)  # vendors fetch within 40 tiles (doc 24): the agoras' goods brought east
for good, limit in [("fleece", 8), ("olive_oil", 8), ("armor", 4), ("wine", 4)]:
    steps.append({"day": ELITE + 1, "at": [69, 29], "storage_order": {"good": good, "kind": 1, "limit": limit}})
for y0, stalls in [(32, ["food_vendor_stall", "fleece_vendor_stall", "oil_vendor_stall"]),
                   (38, ["arms_vendor_stall", "wine_vendor_stall", "horse_trainer_stall"])]:
    put("common_agora", 70, y0, day=ELITE)
    for i, stall in enumerate(stalls):
        put(stall, 70, y0 + 2 * i, day=ELITE + 1)

# A commemorative monument (Z:155-156) once the city has earned one (population, games, a slain monster...).
put("commemorative_monument", 33, 42, day=600, retry=True)

# Stage EAST (after the victory): the east district. The stadium moves by the estate (its competitors walk
# 35 tiles, walkers.json): an estate needs 80 culture points and 100 appeal (Z:34, housing.json M), and its
# horsemen (Z:136). Apollo's Oracle foretells (Z:124). Two agoras, a granary and a storehouse for the estate
# and the new houses; a horse ranch with its wheat.
EAST = 960
steps.append({"day": EAST, "road": [92, 11, 92, 45]})
for y in (29, 37):
    steps.append({"day": EAST, "road": [76, y, WALL - 3, y]})
steps.append({"day": EAST, "demolish": [2, 12]})
# The stadium's road is y = 11 (its north side): from (82, 11) its competitors reach the estate by the x = 92
# road in about 24 tiles.
put("maintenance_office", 78, 12, day=EAST); put("watchpost", 80, 12, day=EAST)  # or it burns (Z:89-90)
put("stadium", 82, 12, day=EAST + 1, retry=True)
steps.append({"day": EAST + 1, "roadblock": [93, 11]})  # its competitors turn down the x = 92 road (Z:19-20)
# Wine for the manor (Z:33): Rhodes's wine comes in by the pier, none goes out any more (Z:77-79).
steps.append({"day": EAST, "at": [34, 46], "trade_order": {"good": "wine", "import": 12}})
put("sanctuary_apollo", 94, 12, day=EAST + 200, retry=True, when={"marble": 20})  # after Perseus has his sculptures
put("theater", 94, 24, day=EAST)
put("gymnasium", 84, 30, day=EAST); put("podium", 88, 30, day=EAST)  # by the estate: athletes, philosophers
put("college", 106, 26, day=EAST); put("maintenance_office", 109, 27, day=EAST)
for x in (79, 82, 85, 88):  # hedge mazes around the estate plot (88, 25), by the x = 92 road
    put("hedge_maze", x, 18, day=EAST)
    put("hedge_maze", x, 21, day=EAST)
for x in (79, 82):
    put("hedge_maze", x, 25, day=EAST)
for x in range(79, 92):
    put("column", x, 24, day=EAST)
put("commemorative_monument", 85, 25, day=EAST + 200, retry=True)  # a second one, as earned (Z:155)
put("elite_housing", 88, 25, day=EAST + 120, retry=True)
put("maintenance_office", 82, 35, day=EAST); put("watchpost", 84, 35, day=EAST)  # the granary and the stores
put("granary", 76, 30, day=EAST); put("storehouse", 80, 30, day=EAST)
for good in ["onions", "carrots"]:
    steps.append({"day": EAST + 1, "at": [76, 30], "storage_order": {"good": good, "kind": 1, "limit": 8}})
for good, limit in [("fleece", 8), ("olive_oil", 8), ("armor", 4), ("wine", 4)]:
    steps.append({"day": EAST + 1, "at": [80, 30], "storage_order": {"good": good, "kind": 1, "limit": limit}})
for x0, stalls in [(93, ["food_vendor_stall", "fleece_vendor_stall", "oil_vendor_stall"]),
                   (100, ["arms_vendor_stall", "wine_vendor_stall", "horse_trainer_stall"])]:
    put("common_agora", x0, 29, rot=1, day=EAST)
    for i, stall in enumerate(stalls):
        put(stall, x0 + 2 * i, 30, day=EAST + 1)
put("horse_ranch", 93, 38, day=EAST); put("wheat_farm", 104, 41, day=EAST)
for y in (38, 43):  # houses for the workers of the district, a fountain, upkeep and a watch in each row
    houses(76, y, 6, day=EAST); put("fountain", 88, y, day=EAST); put("watchpost", 90, y, day=EAST)
houses(93, 43, 3, day=EAST); put("maintenance_office", 99, 43, day=EAST)
steps.append({"day": EAST + 400, "every": 200, "oracle": [94, 12]})  # Consult Oracle (Z:124)

# Stockpiling (Z:80): oil kept from the stalls before the elite plots, so a load waits in storage (doc 18).
steps.append({"day": ELITE + 120, "stockpile": "olive_oil", "on": True})
steps.append({"day": ELITE + 240, "stockpile": "olive_oil", "on": False})
# Not Buying (Z:72): the wine stall stops fetching for half a year.
steps.append({"day": INDUSTRY + 100, "not_buying": [24, 30], "on": True})
steps.append({"day": INDUSTRY + 200, "not_buying": [24, 30], "on": False})

# Surplus oil goes to Rhodes by sea once the elite houses stand (Z:78-79).
steps.append({"day": 700, "at": [34, 46], "trade_order": {"good": "olive_oil", "export_keep": 12}})

# Armies abroad (Z:177-178) with the Sparti, the companies at home that may go: a raid on Sparta for drachmas
# while the first company helps Corinth, then the rising vassal Argos is put down.
steps.append({"day": 1000, "every": 64, "tactics": 2})  # special: rabble skirmish, hoplite wedge (Z:139)
steps.append({"day": 1070, "dispatch": {"city": 1, "raid": True, "good": "drachmas"}})
steps.append({"day": 1480, "every": 16, "dispatch": {"city": 3}})

# Military help (Z:174): Rhodes lends companies against the landing of year 5 (asked every day of the
# invasion, refused when there is none); later, with gifts first, Rhodes strikes Sparta.
steps.append({"day": 5 * 192 + 9 * 16, "every": 1, "until": 5 * 192 + 11 * 16, "military_help": {"city": 2}})
steps.append({"day": 6 * 192, "every": 32, "until": 9 * 192, "gift": {"city": 2, "good": "drachmas", "amount": 500}})
steps.append({"day": 6 * 192, "every": 32, "until": 9 * 192, "military_help": {"city": 2, "strike": True, "target": 1}})

# City requests (Z:161): goods from storage, troops from the companies at home; tried twice a month.
steps.append({"day": 64, "every": 8, "fulfill_requests": True})

# Prayers (Z:124): Ares marches with the next army abroad; the god may refuse (Z:132, L-053). Goods (Athena: 24
# loads) would fill the storehouses that keep the sculptures for Perseus; Hermes finds no request left open.
steps.append({"day": 1000, "every": 48, "pray": [37, 37]})

# Perseus (doc 21): summoned when the hall's needs are met, then sent on Hermes's quest.
steps.append({"day": MYTH, "every": 16, "summon_hero": [64, 12]})
steps.append({"day": MYTH, "every": 16, "hero_quest": 0})

build = {"note": "Autopilot demo (tools/make_demo.py; docs/PRAKTYKI.md ch. 4). Do not edit by hand.",
         "steps": steps}

with open(os.path.join(ROOT, "scenarios", "demo.json"), "w", encoding="utf-8") as f:
    json.dump(scenario, f, ensure_ascii=False, indent=1)
    f.write("\n")
with open(os.path.join(ROOT, "scenarios", "demo_build.json"), "w", encoding="utf-8") as f:
    f.write('{"note": ' + json.dumps(build["note"], ensure_ascii=False) + ',\n "steps": [\n')
    f.write(",\n".join("  " + json.dumps(s, ensure_ascii=False) for s in steps))
    f.write("\n ]\n}\n")
print("demo.json and demo_build.json written:", len(steps), "steps")
