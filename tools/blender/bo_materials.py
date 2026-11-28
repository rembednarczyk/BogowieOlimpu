"""The materials of the game's buildings: one table for every part of the palette (bo_asset.PALETTE), read when a
model is built (its pieces are made to their material's thickness) and when it falls (bo_collapse.py: how a
piece of it breaks, how heavy it is, how it moves). Rule R-019 (owner, 2026-11-28: "these rules of physics,
sizes, density, weight and shapes apply to every material of the game; a global rule for building every
building"). A new colour in the palette needs its line here: tools/checks/check_models.py fails without it.

Each material:
  cls        how it behaves (CLASSES below)
  thickness  metres: a piece of it is at least this thick (a tile 1.2 cm, a board 7 cm, a brick 10 cm); 0: it has
             no thickness of its own (a liquid)
  density    kg/m³ (a piece's weight: its size x the class's fill x this)
  coat       True: laid over other pieces (plaster, paint, mortar): built thin as it is, it goes with what it lies on
             when it falls, and only then counts as its thickness (the wall behind it)
  shape      what a piece of it is, in words (for the style guide)
Values for real materials (orders of magnitude from common engineering tables); [ZAŁOŻENIE] where we chose.
"""

# How a class breaks and moves when a building falls. bond: how far apart (tiles) the points are that hold
# its pieces together (0: each piece alone); fill: of a chunk's box that is material; bounce: the share of the
# speed it keeps on a hit; keep: of its speed along the ground on a hit; spin: how fast it tumbles; topple: a tall
# section tips over outward from its foot; board: pieces thinner than the material are built to its thickness.
CLASSES = {
    "masonry": dict(bond=0.30, fill=0.6, bounce=0.02, keep=0.15, spin=0.6, topple=1.0, board=True),
    "tile": dict(bond=0.07, fill=0.15, bounce=0.04, keep=0.2, spin=1.2, topple=0.0, board=False),
    "wood": dict(bond=0.0, fill=0.5, bounce=0.04, keep=0.25, spin=0.7, topple=0.4, board=True),
    "soft": dict(bond=0.22, fill=0.3, bounce=0.0, keep=0.1, spin=0.4, topple=0.0, board=False),
    "metal": dict(bond=0.0, fill=0.3, bounce=0.05, keep=0.2, spin=1.0, topple=0.0, board=False),
    "loose": dict(bond=0.10, fill=0.6, bounce=0.0, keep=0.05, spin=0.1, topple=0.0, board=False),
    "liquid": dict(bond=0.0, fill=1.0, bounce=0.0, keep=0.0, spin=0.0, topple=0.0, board=False),
    "food": dict(bond=0.0, fill=0.5, bounce=0.05, keep=0.2, spin=0.5, topple=0.0, board=False),
}

MATERIALS = {
    # Walls
    "plaster": dict(cls="masonry", thickness=0.10, density=1700, coat=True, shape="lime plaster on a wall; falls with its bricks"),
    "ochre": dict(cls="masonry", thickness=0.10, density=1700, coat=True, shape="painted plaster (ochre)"),
    "blue_paint": dict(cls="masonry", thickness=0.10, density=1700, coat=True, shape="painted plaster (blue)"),
    "mortar": dict(cls="masonry", thickness=0.08, density=1800, coat=True, shape="lime mortar: joints, beds, verges"),
    "mudbrick": dict(cls="masonry", thickness=0.10, density=1600, coat=False, shape="sun-dried brick 40 x 40 x 10 cm"),
    "stone": dict(cls="masonry", thickness=0.14, density=2600, coat=False, shape="limestone block, slab, sill, cornice"),
    "stone_cool": dict(cls="masonry", thickness=0.14, density=2650, coat=False, shape="blue-grey stone block"),
    "marble": dict(cls="masonry", thickness=0.12, density=2700, coat=False, shape="marble block, column drum, slab"),
    # Roofs
    "roof_tile": dict(cls="tile", thickness=0.012, density=1900, coat=False, shape="terracotta pan or cover tile"),
    "roof_brown": dict(cls="tile", thickness=0.012, density=1900, coat=False, shape="brown terracotta tile"),
    "roof_slate": dict(cls="tile", thickness=0.010, density=2800, coat=False, shape="slate-blue glazed tile"),
    "roof_gold": dict(cls="tile", thickness=0.012, density=2000, coat=False, shape="gold glazed tile"),
    "roof_patina": dict(cls="tile", thickness=0.012, density=2000, coat=False, shape="green glazed tile"),
    "thatch": dict(cls="soft", thickness=0.12, density=250, coat=False, shape="reed thatch, a layer; tears into mats"),
    # Wood
    "wood": dict(cls="wood", thickness=0.07, density=650, coat=False, shape="board, beam, post; never splits"),
    "dark_wood": dict(cls="wood", thickness=0.07, density=750, coat=False, shape="oak board, door, lintel"),
    "bark": dict(cls="wood", thickness=0.10, density=600, coat=False, shape="a log with its bark"),
    "end_grain": dict(cls="wood", thickness=0.10, density=650, coat=False, shape="the cut end of a log"),
    # Cloth, hides, plants
    "cloth": dict(cls="soft", thickness=0.004, density=500, coat=False, shape="woven wool, an awning, a canvas"),
    "linen": dict(cls="soft", thickness=0.004, density=500, coat=False, shape="linen, laundry, a tent"),
    "sky_cloth": dict(cls="soft", thickness=0.004, density=500, coat=False, shape="dyed cloth (blue)"),
    "sage_cloth": dict(cls="soft", thickness=0.004, density=500, coat=False, shape="dyed cloth (green)"),
    "hide": dict(cls="soft", thickness=0.008, density=900, coat=False, shape="a tanned hide"),
    "rope": dict(cls="soft", thickness=0.02, density=600, coat=False, shape="a rope, a cord"),
    "leaves": dict(cls="soft", thickness=0.003, density=600, coat=False, shape="leaves, a vine, ivy"),
    "grass": dict(cls="soft", thickness=0.003, density=500, coat=False, shape="grass blades"),
    "flower_red": dict(cls="soft", thickness=0.003, density=500, coat=False, shape="flowers"),
    "flower_yellow": dict(cls="soft", thickness=0.003, density=500, coat=False, shape="flowers"),
    "flower_white": dict(cls="soft", thickness=0.003, density=500, coat=False, shape="flowers"),
    # Metal
    "bronze": dict(cls="metal", thickness=0.008, density=8800, coat=False, shape="bronze fitting, vessel, statue skin"),
    "iron": dict(cls="metal", thickness=0.008, density=7870, coat=False, shape="iron strap, hinge, tool, tie"),
    # Food in a granary's bins (each piece whole: a carrot, a cheese, a fish)
    "carrot": dict(cls="food", thickness=0.03, density=1000, coat=False, shape="a carrot; carrots heaped in a bin"),
    "onion": dict(cls="food", thickness=0.06, density=950, coat=False, shape="an onion; onions heaped in a bin"),
    "cheese": dict(cls="food", thickness=0.08, density=1100, coat=False, shape="a cheese wheel; stacked"),
    "fish": dict(cls="food", thickness=0.04, density=1050, coat=False, shape="a dried fish; laid in baskets"),
    "urchin": dict(cls="food", thickness=0.06, density=1100, coat=False, shape="a sea urchin; in shallow baskets"),
    "pork": dict(cls="food", thickness=0.12, density=1050, coat=False, shape="a ham; hung or laid in a row"),
    "meat": dict(cls="food", thickness=0.10, density=1050, coat=False, shape="a joint of meat; on straw"),
    "orange_fruit": dict(cls="food", thickness=0.08, density=900, coat=False, shape="an orange; heaped in a bin"),
    # What fills vessels and sacks
    "grain": dict(cls="loose", thickness=0.0, density=750, coat=False, shape="grain in bulk; spills, does not tumble"),
    "water": dict(cls="liquid", thickness=0.0, density=1000, coat=False, shape="water; drains away when it falls"),
    "oil": dict(cls="liquid", thickness=0.0, density=920, coat=False, shape="olive oil; drains away when it falls"),
}

THICKNESS = {name: m["thickness"] for name, m in MATERIALS.items()}
# Parts built to their thickness: the solid pieces of a class that has boards (not the coats laid over them).
BOARDS = tuple(name for name, m in MATERIALS.items() if CLASSES[m["cls"]]["board"] and not m["coat"])
