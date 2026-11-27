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
import bo_asset as A  # noqa: E402
from bo_asset import FOUNDATION, WALLS, ROOF, M  # noqa: E402

MODELS = {}


def model(name, building, note):
    def wrap(fn):
        MODELS[name] = (fn, building, note)
        return fn
    return wrap


@model("common_housing_1", "common_housing", "chata: ubita ziemia na kamiennym cokole, strzecha czterospadowa")
def hut(b):
    b.box(0.25, 0.35, 0, 1.5, 1.3, 0.3 * M, "stone", stage=FOUNDATION)
    b.box(0.3, 0.4, 0.3 * M, 1.4, 1.2, 2.0 * M, "mudbrick", plaster=True)
    b.box(0.85, 1.6, 0.3 * M, 0.3, 0.02, 1.5 * M, "dark_wood")  # the door on the south side
    b.prism(0.18, 0.28, 2.3 * M, 1.64, 1.44, 1.3 * M, "thatch", along_x=True, hip=0.28)
    b.box(1.75, 1.2, 0, 0.18, 0.5, 0.35 * M, "wood", loose=True)  # a woodpile by the wall
    b.scaffold()


@model("common_housing_2", "common_housing", "szałas: tynkowany dom z przybudówką, dach dwuspadowy z trzciny")
def cottage(b):
    b.box(0.12, 0.25, 0, 1.76, 1.5, 0.3 * M, "stone", stage=FOUNDATION)
    b.box(0.15, 0.3, 0.3 * M, 1.25, 1.4, 2.4 * M, "mudbrick", plaster=True)
    b.box(1.4, 0.7, 0.3 * M, 0.45, 1.0, 1.6 * M, "mudbrick", plaster=True)  # the lean-to
    b.box(0.6, 1.7, 0.3 * M, 0.3, 0.02, 1.6 * M, "dark_wood")
    b.prism(0.08, 0.22, 2.7 * M, 1.39, 1.56, 1.2 * M, "thatch", along_x=False)
    b.prism(1.38, 0.62, 1.9 * M, 0.52, 1.16, 0.4 * M, "thatch", along_x=True, loose=True)
    b.scaffold()


@model("fountain", "fountain", "fontanna: kamienny basen, słup z wylewką z brązu, woda płynie, gdy pracuje")
def fountain(b):
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
def granary(b):
    b.box(0.15, 0.15, 0, 3.7, 3.7, 0.5 * M, "stone", stage=FOUNDATION)
    b.box(0.3, 0.3, 0.5 * M, 3.4, 2.7, 3.4 * M, "mudbrick", plaster=True)
    for i in range(4):  # the porch's columns along the south front
        b.column(0.55 + i * 0.97, 3.45, 0.5 * M, 0.09, 2.9 * M, "stone", sides=8)
    b.box(0.3, 3.3, 3.4 * M, 3.4, 0.3, 0.5 * M, "stone", stage=ROOF)  # the lintel
    b.prism(0.2, 0.2, 3.9 * M, 3.6, 3.5, 1.6 * M, "roof_tile", along_x=True)
    b.box(1.75, 3.0, 0.5 * M, 0.5, 0.02, 2.0 * M, "dark_wood")
    b.scaffold(height=4.2 * M)


@model("storehouse", "storehouse", "magazyn: dziedziniec w kamiennym murze, wiata kryta dachówką, skrzynie i amfory")
def storehouse(b):
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
def observatory(b):
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
        A.reset()
        w, h = A.footprint(building)
        b = A.Builder(name, w, h)
        fn(b)
        A.export(b.finish(), out_dir / (name + ".glb"))
        provenance[name + ".glb"] = {"building": building, "footprint": [w, h], "source": "tools/blender/models.py",
                                     "author": "Bogowie Olimpu (model ze skryptu)", "license": "własny (R-017)",
                                     "note": note}
    if out_dir.resolve() == (A.REPO / "game" / "models").resolve():
        provenance_path.write_text(json.dumps(dict(sorted(provenance.items())), ensure_ascii=False, indent=1) + "\n")


main()
