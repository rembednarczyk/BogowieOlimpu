"""The game's building models, built in code (docs/ASSETY.md, R-017): stylized realism from Greek domestic and
public architecture (rammed earth and mudbrick on stone footings, lime plaster, terracotta tiles, timber), our own
shapes, nothing from the reference game.

Usage: blender -b --factory-startup --python tools/blender/models.py -- [out_dir] [model ...]
Writes <out_dir>/<model>.glb (default game/models) and game/models/provenance.json.
"""
import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import random  # noqa: E402

import bo_asset as A  # noqa: E402
import bo_detail as D  # noqa: E402
from bo_detail import t  # noqa: E402
from bo_asset import FOUNDATION, WALLS, ROOF, M  # noqa: E402

MODELS = {}


def model(name, building, note):
    def wrap(fn):
        MODELS[name] = (fn, building, note)
        return fn
    return wrap


# The seven levels of the common house (data/housing.json), each its own silhouette on a 2x2 plot (4 x 4 m):
# the level reads from afar (docs/08-OPRAWA.md). Built brick by brick and tile by tile (bo_detail); b.detail
# False gives the far view (LOD1). Positions in metres from the plot's north-west corner.
def _room(b, rng, x, y, w, d, z, h, thick=0.38, part="mudbrick", plaster=None, doors=(), windows=(), stage=(0.08, 0.5),
          block=None):
    """Four walls of a room; doors and windows as (side, at[, w]) with side "n", "s", "w" or "e"."""
    walls = {"n": D.Wall(x, y, w, "x", z, h, thick), "s": D.Wall(x, y + d, w, "x", z, h, thick, outward=-1),
             "w": D.Wall(x, y + thick, d - 2 * thick, "y", z, h, thick),
             "e": D.Wall(x + w, y + thick, d - 2 * thick, "y", z, h, thick, outward=-1)}
    for side, at, *rest in doors:
        walls[side].door(at, *rest)
    for side, at, *rest in windows:
        walls[side].window(at, *rest)
    for wall in walls.values():
        D.brick_wall(b, wall, rng, part=part, stage=stage, plaster=plaster, block=block)


@model("common_housing_1", "common_housing", "chata: cegła suszona na kamiennym cokole, strzecha czterospadowa w warstwach")
def hut(b, rng):
    D.footing(b, 0.5, 0.7, 3.0, 2.6, h=0.25, rng=rng)
    _room(b, rng, 0.6, 0.8, 2.8, 2.4, 0.25 * 0 + 0.25, 1.9, thick=0.36, doors=[("s", 0.9, 0.8, 1.6)],
          windows=[("e", 0.55, 0.40, 0.95, 0.35)])
    D.thatch_roof(b, 0.6, 0.8, 2.8, 2.4, 2.15, 1.35, rng, hip=0.3, overhang=0.35)
    D.woodpile(b, 3.56, 1.3, rng=rng, along_x=False)
    D.pithos(b, 0.3, 3.55, rng=rng)
    b.scaffold(height=2.4 * M)


@model("common_housing_2", "common_housing", "szałas: tynkowany dom z przybudówką z nieotynkowanej cegły, trzcina dwuspadowa")
def cottage(b, rng):
    D.footing(b, 0.3, 0.4, 2.6, 3.1, h=0.3, rng=rng)
    _room(b, rng, 0.4, 0.5, 2.4, 2.9, 0.3, 2.1, plaster="plaster", doors=[("s", 0.75, 0.85, 1.7)],
          windows=[("w", 0.9), ("n", 1.0)])
    D.footing(b, 2.8, 1.35, 0.95, 1.9, h=0.2, rng=rng)
    _room(b, rng, 2.8, 1.45, 0.9, 1.75, 0.2, 1.45, thick=0.3, doors=[("s", 0.2, 0.5, 1.2)], stage=(0.2, 0.55))
    D.thatch_roof(b, 0.4, 0.5, 2.4, 2.9, 2.4, 1.25, rng, hip=0.0, along_x=False, overhang=0.3)
    D.thatch_roof(b, 2.8, 1.45, 0.9, 1.75, 1.65, 0.45, rng, hip=0.0, along_x=False, overhang=0.15, layers=3)
    D.woodpile(b, 0.25, 3.65, rng=rng)
    D.pithos(b, 3.6, 0.5, scale=0.9, rng=rng)
    b.scaffold(height=2.6 * M)


@model("common_housing_3", "common_housing", "lepianka: bielony dom pod dachówką, murek z bramką przed wejściem")
def cabin(b, rng):
    D.footing(b, 0.35, 0.35, 3.3, 2.6, h=0.3, rng=rng)
    _room(b, rng, 0.4, 0.4, 3.2, 2.5, 0.3, 2.2, plaster="plaster", doors=[("s", 1.2, 0.85, 1.75)],
          windows=[("s", 2.3), ("e", 0.9), ("w", 0.9)])
    D.tiled_roof(b, 0.4, 0.4, 3.2, 2.5, 2.5, 0.85, rng, along_x=True, overhang=0.3)
    for x, length in ((0.25, 1.1), (2.35, 1.4)):  # the yard's low wall, a gate between
        D.brick_wall(b, D.Wall(x, 3.55, length, "x", 0.0, 0.8, 0.28), rng, plaster="plaster", stage=(0.05, 0.4))
    D.pithos(b, 0.55, 3.25, scale=0.8, rng=rng)
    D.pithos(b, 3.4, 3.25, scale=0.7, rng=rng)
    b.scaffold(height=2.9 * M)


@model("common_housing_4", "common_housing", "zagroda chłopska: dom na planie litery L z dziedzińcem, dachówka, cokół z kamienia")
def homestead(b, rng):
    D.footing(b, 0.25, 0.25, 3.5, 2.0, h=0.45, rng=rng)
    _room(b, rng, 0.3, 0.3, 3.4, 1.9, 0.45, 2.3, plaster="plaster", doors=[("s", 0.6, 0.85, 1.8)],
          windows=[("s", 1.8), ("w", 0.6), ("n", 1.6)])
    D.footing(b, 2.25, 2.15, 1.5, 1.6, h=0.4, rng=rng)
    _room(b, rng, 2.3, 2.2, 1.4, 1.5, 0.4, 2.0, plaster="plaster", doors=[("w", 0.3, 0.8, 1.7)],
          windows=[("s", 0.5), ("e", 0.4)], stage=(0.15, 0.55))
    D.tiled_roof(b, 0.3, 0.3, 3.4, 1.9, 2.75, 0.75, rng, along_x=True, overhang=0.16)
    D.tiled_roof(b, 2.3, 2.2, 1.4, 1.5, 2.4, 0.55, rng, along_x=False, overhang=0.22)
    D.brick_wall(b, D.Wall(0.2, 3.65, 2.0, "x", 0.0, 1.0, 0.28), rng, plaster="plaster", stage=(0.05, 0.4))
    D.brick_wall(b, D.Wall(0.2, 2.35, 1.3, "y", 0.0, 1.0, 0.28), rng, plaster="plaster", stage=(0.05, 0.4))
    for x, y, sc in ((0.75, 2.6, 0.9), (1.25, 2.6, 0.8), (0.75, 3.1, 0.75)):
        D.pithos(b, x, y, scale=sc, rng=rng)
    D.woodpile(b, 1.4, 3.2, rng=rng)
    b.scaffold(height=3.2 * M)


@model("common_housing_5", "common_housing", "kamienica czynszowa: parter z kamiennych bloków, piętro z cegły w tynku, drewniany balkon")
def row_house(b, rng):
    D.footing(b, 0.35, 0.35, 3.3, 3.1, h=0.3, rng=rng)
    _room(b, rng, 0.4, 0.4, 3.2, 3.0, 0.3, 2.3, thick=0.45, part="stone", block=(0.62, 0.32),
          doors=[("s", 0.6, 0.9, 1.9)], windows=[("s", 2.2, 0.4, 1.2, 0.45), ("e", 1.2, 0.4, 1.2, 0.45)])
    _room(b, rng, 0.4, 0.4, 3.2, 3.0, 2.6, 2.1, plaster="plaster", windows=[("s", 0.7), ("s", 2.2), ("e", 1.2),
          ("w", 1.2), ("n", 1.5)], stage=(0.5, 0.68))
    if b.detail:  # the balcony: a floor on beams, a railing of posts
        for i in range(5):
            b.box(t(0.5 + i * 0.7), t(3.3), t(2.5), t(0.08), t(0.6), t(0.1), "dark_wood", stage=0.55)
        b.box(t(0.45), t(3.35), t(2.6), t(3.1), t(0.55), t(0.06), "wood", stage=0.56)
        for i in range(12):
            b.box(t(0.5 + i * 0.27), t(3.82), t(2.66), t(0.04), t(0.04), t(0.85), "wood", stage=0.6)
        b.box(t(0.45), t(3.8), t(3.5), t(3.1), t(0.08), t(0.06), "dark_wood", stage=0.6)
    D.tiled_roof(b, 0.4, 0.4, 3.2, 3.0, 4.7, 0.95, rng, along_x=True, overhang=0.3)
    b.scaffold(height=5.2 * M)


@model("common_housing_6", "common_housing", "mieszkanie: piętrowy blok i dwa skrzydła wokół dziedzińca ze studnią, pas malowany ochrą")
def flat(b, rng):
    D.footing(b, 0.3, 0.3, 3.4, 1.6, h=0.35, rng=rng)
    _room(b, rng, 0.35, 0.35, 3.3, 1.5, 0.35, 2.2, plaster="plaster", doors=[("s", 1.3, 0.9, 1.85)],
          windows=[("s", 0.45), ("s", 2.5), ("n", 0.9), ("n", 2.3)])
    _room(b, rng, 0.35, 0.35, 3.3, 1.5, 2.55, 2.0, plaster="plaster", windows=[("s", 0.5), ("s", 1.5), ("s", 2.5),
          ("w", 0.5), ("e", 0.5)], stage=(0.5, 0.66))
    b.box(t(0.32), t(0.32), t(2.5), t(3.36), t(1.56), t(0.12), "ochre", stage=0.5, plaster=True)  # the band
    for x in (0.35, 2.6):  # the wings
        _room(b, rng, x, 1.85, 1.05, 1.85, 0.3, 2.0, plaster="plaster", doors=[("e" if x < 1 else "w", 0.6, 0.75, 1.7)],
              windows=[("s", 0.3)], stage=(0.15, 0.55))
        D.tiled_roof(b, x, 1.85, 1.05, 1.85, 2.3, 0.45, rng, along_x=False, overhang=0.15)
    D.tiled_roof(b, 0.35, 0.35, 3.3, 1.5, 4.55, 0.7, rng, along_x=True, overhang=0.16, antefix=True)
    b.box(t(1.7), t(2.6), 0, t(0.6), t(0.6), t(0.15), "stone", stage=0.05)  # the yard's paving and its well
    b.column(t(2.0), t(2.9), 0, t(0.28), t(0.55), "stone", sides=14 if b.detail else 6, stage=0.3)
    b.column(t(2.0), t(2.9), t(0.55), t(0.2), t(0.02), "water", sides=12 if b.detail else 6, stage=0.35)
    D.pithos(b, 1.6, 3.55, scale=0.75, rng=rng)
    b.scaffold(height=5.0 * M)


@model("common_housing_7", "common_housing", "dom miejski: piętrowy, z portykiem na kolumnach doryckich, fryz i antefiksy")
def city_house(b, rng):
    D.footing(b, 0.25, 0.25, 3.5, 2.5, h=0.55, rng=rng)
    _room(b, rng, 0.3, 0.3, 3.4, 2.4, 0.55, 2.4, thick=0.42, part="stone", block=(0.58, 0.3),
          doors=[("s", 1.25, 0.95, 2.0)], windows=[("e", 0.9, 0.4, 1.3, 0.5), ("w", 0.9, 0.4, 1.3, 0.5)])
    _room(b, rng, 0.3, 0.3, 3.4, 2.4, 2.95, 2.2, plaster="plaster", windows=[("s", 0.6), ("s", 1.5), ("s", 2.4),
          ("e", 0.9), ("w", 0.9), ("n", 1.5)], stage=(0.5, 0.68))
    b.box(t(0.27), t(0.27), t(2.85), t(3.46), t(2.46), t(0.16), "cloth", stage=0.5, plaster=True)  # red frieze
    b.box(t(0.3), t(2.7), 0, t(3.4), t(1.1), t(0.55), "stone", stage=0.05)  # the porch's floor and steps
    b.box(t(1.0), t(3.8), 0, t(2.0), t(0.15), t(0.28), "stone", stage=0.06)
    for x in (0.75, 1.9, 3.05):
        D.doric_column(b, x, 3.45, 0.55, 2.25, radius=0.15, stage=0.45)
    b.box(t(0.4), t(3.2), t(2.8), t(3.2), t(0.5), t(0.25), "stone", stage=0.5)  # the porch's beam
    D.tiled_roof(b, 0.3, 2.7, 3.4, 0.95, 3.05, 0.35, rng, along_x=True, overhang=0.15, gable="stone",
                 stage=(0.62, 0.7))
    D.tiled_roof(b, 0.3, 0.3, 3.4, 2.4, 5.15, 1.0, rng, along_x=True, overhang=0.16, antefix=True)
    D.pithos(b, 0.45, 3.7, scale=0.6, rng=rng)
    b.scaffold(height=5.6 * M)


@model("fountain", "fountain", "fontanna: kamienny basen, słup z wylewką z brązu, woda płynie, gdy pracuje")
def fountain(b, rng):
    b.box(0.2, 0.2, 0, 1.6, 1.6, 0.25 * M, "stone", stage=FOUNDATION)
    for x, y, sx, sy in ((0.25, 0.25, 1.5, 0.12), (0.25, 1.63, 1.5, 0.12), (0.25, 0.37, 0.12, 1.26),
                         (1.63, 0.37, 0.12, 1.26)):  # the basin's rim
        b.box(x, y, 0.25 * M, sx, sy, 0.6 * M, "stone")
    b.box(0.37, 0.37, 0.25 * M, 1.26, 1.26, 0.45 * M, "water", anim=A.FLOW)
    b.column(1.0, 1.0, 0.25 * M, 0.14, 2.2 * M, "marble", sides=10)
    b.column(1.0, 1.0, 2.45 * M, 0.28, 0.15 * M, "marble", sides=12, stage=ROOF)
    b.box(0.96, 1.12, 1.8 * M, 0.08, 0.2, 0.08, "bronze", stage=ROOF)  # the spout, to the south
    b.scaffold(height=2.6 * M)


@model("granary", "granary", "spichlerz: podest z kamienia, ściany z cegły w tynku, dach z dachówki, portyk")
def granary(b, rng):
    b.box(0.15, 0.15, 0, 3.7, 3.7, 0.5 * M, "stone", stage=FOUNDATION)
    b.box(0.3, 0.3, 0.5 * M, 3.4, 2.7, 3.4 * M, "mudbrick", plaster=True)
    for i in range(4):  # the porch's columns along the south front
        b.column(0.55 + i * 0.97, 3.45, 0.5 * M, 0.09, 2.9 * M, "stone", sides=8)
    b.box(0.3, 3.3, 3.4 * M, 3.4, 0.3, 0.5 * M, "stone", stage=ROOF)  # the lintel
    b.prism(0.2, 0.2, 3.9 * M, 3.6, 3.5, 1.6 * M, "roof_tile", along_x=True)
    b.box(1.75, 3.0, 0.5 * M, 0.5, 0.02, 2.0 * M, "dark_wood")
    b.scaffold(height=4.2 * M)


@model("storehouse", "storehouse", "magazyn: dziedziniec w kamiennym murze, wiata kryta dachówką, skrzynie i amfory")
def storehouse(b, rng):
    w = b.width
    b.box(0.05, 0.05, 0, w - 0.1, w - 0.1, 0.15 * M, "stone", stage=FOUNDATION)
    for x, y, sx, sy in ((0.05, 0.05, w - 0.1, 0.15), (0.05, 0.05, 0.15, w - 0.1), (w - 0.2, 0.05, 0.15, w - 0.1)):
        b.box(x, y, 0.15 * M, sx, sy, 1.6 * M, "stone", plaster=True)
    b.box(0.05, w - 0.2, 0.15 * M, 0.9, 0.15, 1.6 * M, "stone", plaster=True)  # the south wall, a gate in it
    b.box(w - 1.15, w - 0.2, 0.15 * M, 1.0, 0.15, 1.6 * M, "stone", plaster=True)
    for x in (0.4, 1.5, 2.6):  # the shed's posts
        b.column(x, 1.25, 0.15 * M, 0.06, 2.6 * M, "wood", sides=6)
    b.prism(0.1, 0.1, 2.75 * M, w - 0.2, 1.3, 0.9 * M, "roof_tile", along_x=True)
    b.box(0.5, 1.7, 0.15 * M, 0.35, 0.3, 0.5 * M, "wood", loose=True)  # crates in the yard
    b.box(0.9, 1.75, 0.15 * M, 0.3, 0.3, 0.4 * M, "wood", loose=True)
    for i, x in enumerate((1.7, 1.9, 2.1)):  # amphorae
        b.column(x, 1.85, 0.15 * M, 0.08, 0.6 * M, "roof_tile", sides=8, stage=ROOF if i else WALLS)
    b.scaffold(height=3.0 * M)


@model("observatory", "observatory", "obserwatorium atlantydzkie: kamienny bęben, kopuła i luneta obracają się, gdy pracuje")
def observatory(b, rng):
    c = b.width / 2.0
    b.box(0.3, 0.3, 0, b.width - 0.6, b.height - 0.6, 0.5 * M, "stone", stage=FOUNDATION)
    b.column(c, c, 0.5 * M, 1.7, 4.0 * M, "marble", sides=16)
    b.column(c, c, 4.5 * M, 1.55, 0.8 * M, "bronze", sides=16, stage=ROOF, anim=A.ROTATE, pivot=(c, c))
    b.column(c, c, 5.3 * M, 1.1, 0.8 * M, "bronze", sides=16, stage=ROOF, anim=A.ROTATE, pivot=(c, c))
    b.box(c - 0.12, c - 0.2, 5.6 * M, 0.24, 1.9, 0.24, "dark_wood", stage=ROOF, anim=A.ROTATE, pivot=(c, c))
    for i in range(6):  # stairs to the door
        b.box(c - 0.4, b.height - 0.7 - i * 0.1, 0, 0.8, 0.1, (0.5 - i * 0.08) * M, "stone", stage=FOUNDATION)
    b.scaffold(height=5.0 * M)


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out_dir = pathlib.Path(args[0]) if args else A.REPO / "game" / "models"
    names = args[1:] or sorted(MODELS)
    provenance_path = A.REPO / "game" / "models" / "provenance.json"
    provenance = json.loads(provenance_path.read_text()) if provenance_path.exists() else {}
    for name in names:
        fn, building, note = MODELS[name]
        w, h = A.footprint(building)
        views = ((True, ""), (False, "_lod1")) if building == "common_housing" else ((True, ""),)
        for detail, suffix in views:  # the close view and, for the houses, the far view
            A.reset()
            b = A.Builder(name + suffix, w, h, detail=detail)
            fn(b, random.Random(name))  # the same bricks every run
            A.export(b.finish(), out_dir / (name + suffix + ".glb"))
            provenance[name + suffix + ".glb"] = {"building": building, "footprint": [w, h], "lod": 0 if detail else 1,
                                                  "source": "tools/blender/models.py",
                                                  "author": "Bogowie Olimpu (model ze skryptu)",
                                                  "license": "własny (R-017)", "note": note}
    if out_dir.resolve() == (A.REPO / "game" / "models").resolve():
        provenance_path.write_text(json.dumps(dict(sorted(provenance.items())), ensure_ascii=False, indent=1) + "\n")


main()
