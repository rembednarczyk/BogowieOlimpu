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
import math  # noqa: E402
import random  # noqa: E402

import bo_asset as A  # noqa: E402
import bo_detail as D  # noqa: E402
import bo_story as S  # noqa: E402
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
          block=None, ornate=False, group="main", wreath=False):
    """Four walls of a room; doors and windows as (side, at[, w]) with side "n", "s", "w" or "e"."""
    walls = {"n": D.Wall(x, y, w, "x", z, h, thick), "s": D.Wall(x, y + d, w, "x", z, h, thick, outward=-1),
             "w": D.Wall(x, y + thick, d - 2 * thick, "y", z, h, thick),
             "e": D.Wall(x + w, y + thick, d - 2 * thick, "y", z, h, thick, outward=-1)}
    for side, at, *rest in doors:
        walls[side].door(at, *rest)
    for side, at, *rest in windows:
        walls[side].window(at, *rest)
    for wall in walls.values():
        wall.ornate = ornate
        wall.wreath = wreath
        D.brick_wall(b, wall, rng, part=part, stage=stage, plaster=plaster, block=block, group=group)


def _dress(b, story, level, rng):
    b.mirror = S.STORIES[story]["mirror"]
    left = S.dress(b, story, level, rng)
    if left:
        print("NO ROOM in %s for %s" % (b.name, ", ".join(left)))


def _acroteria(b, x, y, w, d, z, rise, overhang, size=0.34):
    """Palmettes on the gable's apex and its corners (a roof along x)."""
    for gx in (x - overhang, x + w + overhang):
        out = -1 if gx < x else 1
        D.palmette(b, (gx + out * 0.02, y + d / 2, z + rise + 0.06), False, out, size)
        for gy in (y - overhang, y + d + overhang):
            D.palmette(b, (gx + out * 0.02, gy, z + 0.02), False, out, size * 0.6)


# The goods a level needs to grow (data/housing.json: Fd food from level 2, W water from 3, Fl fleece from 4, Oil
# olive oil from 6) stand by the house from that level on; those it keeps show only while it holds them.

@model("common_housing_2", "common_housing", "chata z gliny: cegła suszona na kamiennym cokole, strzecha ze źdźbłami na okapie, drewno ułożone przy ścianie, pieniek z siekierą; worki ze zbożem i kosz cebuli, gdy dom ma żywność")
def hut(b, rng, story=None):
    acc = S.STORIES[story]["accent"] if story else {}
    D.footing(b, 0.5, 0.7, 3.0, 2.6, h=0.25, rng=rng)
    _room(b, rng, 0.6, 0.8, 2.8, 2.4, 0.25, 1.9, thick=0.36, doors=[("s", 0.9, 0.8, 1.6)], plaster=acc.get("walls"),
          windows=[("e", 0.55, 0.40, 0.95, 0.35)])
    D.thatch_roof(b, 0.6, 0.8, 2.8, 2.4, 2.15, 1.35 * D.ROOF_STEEP, rng, hip=0.3, overhang=0.35)
    D.woodpile(b, 3.55, 0.85, rng, along_x=False, length=1.0)
    D.pithos(b, 0.3, 3.62, scale=0.9, rng=rng, lid=True)
    with D.goods(b, "food"):
        D.sack(b, 0.95, 3.72, scale=0.8, rng=rng)
        D.sack(b, 1.36, 3.74, scale=0.72, rng=rng, name="sack 2")
        D.basket(b, 2.85, 3.72, "onions", rng=rng)
    if story:
        S.slot(b, "north", 0.5, 0.02, 3.5, 0.66)
        S.slot(b, "west", 0.02, 0.75, 0.47, 3.25, "strip")
        S.slot(b, "east", 3.53, 1.95, 3.98, 3.3, "strip")
        _dress(b, story, 2, rng)
    D.grass(b, rng, 38)
    b.scaffold(height=2.4 * M)


@model("common_housing_1", "common_housing", "namiot: drewniany szkielet obciągnięty płótnem i skórami, palenisko, drewno i pitos")
def shelter(b, rng, story=None):
    roof_part, wall_part = S.STORIES[story]["accent"]["tent"] if story else ("linen", "hide")
    x0, x1, y0, y1, eave, ridge = 0.5, 3.5, 0.7, 3.3, 1.55, 2.55
    b.solid("tent", t(x0 - 0.17), t(y0 - 0.14), 0, t(x1 + 0.17), t(y1 + 0.14), t(ridge + 0.2))
    posts = 4 if b.detail else 2
    for i in range(posts + 1):  # the side posts along both eaves
        px = x0 + (x1 - x0) * i / posts
        for py in (y0, y1):
            b.log((t(px), t(py), 0), (t(px), t(py), t(eave + 0.05)), t(0.05), sides=12 if b.detail else 5, stage=0.15,
                  tint=rng.uniform(0.3, 0.7))
    for px in (x0 - 0.02, x1 + 0.02):  # the forked posts and the ridge pole
        b.log((t(px), t((y0 + y1) / 2), 0), (t(px), t((y0 + y1) / 2), t(ridge + 0.1)), t(0.07),
              sides=14 if b.detail else 6, stage=0.2)
        if b.detail:
            for side in (-1, 1):  # the fork's two prongs
                b.tube_path([(t(px), t((y0 + y1) / 2), t(ridge - 0.02)), (t(px), t((y0 + y1) / 2 + side * 0.09), t(ridge + 0.16))],
                            t(0.03), "bark", sides=8, stage=0.2)
    b.log((t(x0 - 0.15), t((y0 + y1) / 2), t(ridge)), (t(x1 + 0.15), t((y0 + y1) / 2), t(ridge)), t(0.06),
          sides=14 if b.detail else 6, stage=0.3)
    for py in (y0, y1):  # the eave poles
        b.log((t(x0 - 0.1), t(py), t(eave)), (t(x1 + 0.1), t(py), t(eave)), t(0.04), sides=12 if b.detail else 5,
              stage=0.35)
    nu, nv = (48, 14) if b.detail else (6, 2)
    rng_wave = {-1: rng.uniform(0, 6), 1: rng.uniform(0, 6)}

    def roof(side):
        def fn(u, v):  # from the ridge (v 0) to the eave (v 1); sags between the posts, folds along the slope
            ey = y0 if side < 0 else y1
            mid = (y0 + y1) / 2
            x = x0 - 0.12 + (x1 - x0 + 0.24) * u
            y = mid + (ey + side * 0.12 - mid) * v
            sag = 0.10 * math.sin(math.pi * v) * (0.6 + 0.4 * abs(math.sin(u * math.pi * 4)))
            fold = 0.015 * math.sin(u * 40.0 + rng_wave[side]) * v
            z = ridge + 0.05 + (eave - 0.04 - ridge - 0.05) * v - sag + fold
            return (t(x), t(y), t(z))
        return fn
    for side in (-1, 1):
        b.surface(roof(side), nu, nv, roof_part, stage=0.7, tint=rng.uniform(0.35, 0.65))

    def wall(xa, ya, xb, yb, top0, top1):
        def fn(u, v):  # from the ground (v 0) up; folds hang down, the hem lies a little out on the ground
            x = xa + (xb - xa) * u
            y = ya + (yb - ya) * u
            top = top0 + (top1 - top0) * (1 - abs(2 * u - 1)) if top1 != top0 else top0
            fold = 0.03 * math.sin(u * 46.0 + xa * 3.0) * (1.0 - v * 0.7)
            nx, ny = (yb - ya), -(xb - xa)
            ln = max(math.hypot(nx, ny), 1e-6)
            out = fold + 0.05 * (1.0 - v) ** 3
            return (t(x + nx / ln * out), t(y + ny / ln * out), t(top * v + 0.01))
        return fn
    b.surface(wall(x0, y0, x1, y0, eave, eave), nu, nv // 2 + 1, wall_part, stage=0.5, tint=rng.uniform(0.3, 0.7))
    b.surface(wall(x0, y0, x0, y1, eave, ridge), nu // 2, nv // 2 + 1, "linen", stage=0.55, tint=0.4)
    b.surface(wall(x1, y1, x1, y0, eave, ridge), nu // 2, nv // 2 + 1, "linen", stage=0.55, tint=0.6)
    for xa, xb in ((x0, 1.55), (2.35, x1)):  # the south side, the door between
        b.surface(wall(xb, y1, xa, y1, eave, eave), max(2, nu // 3), nv // 2 + 1, wall_part, stage=0.5,
                  tint=rng.uniform(0.3, 0.7))
    if b.detail:  # the door flap rolled up under the eave, tied with rope
        b.tube_path([(t(1.5), t(y1 + 0.06), t(eave - 0.12)), (t(2.4), t(y1 + 0.06), t(eave - 0.12))], t(0.06), "hide",
                    sides=14, stage=0.75)
        for px in (1.65, 2.25):
            b.tube_path([(t(px), t(y1 + 0.02), t(eave)), (t(px), t(y1 + 0.13), t(eave - 0.1)),
                         (t(px), t(y1 + 0.02), t(eave - 0.2))], t(0.008), "rope", sides=6, stage=0.75)
        for cx, cy, gx, gy in ((x0, y0, x0 - 0.3, y0 - 0.45), (x1, y0, x1 + 0.3, y0 - 0.45), (x0, y1, x0 - 0.3, y1 + 0.45),
                               (x1, y1, x1 + 0.3, y1 + 0.45)):  # guy ropes to pegs
            gx, gy = min(max(gx, 0.08), 3.92), min(max(gy, 0.08), 3.92)
            b.tube_path([(t(cx), t(cy), t(eave)), (t(gx), t(gy), t(0.06))], t(0.008), "rope", sides=6, stage=0.8)
            b.log((t(gx), t(gy), 0), (t(gx), t(gy), t(0.12)), t(0.02), sides=8, stage=0.8)
        b.lathe(t(2.0), t(2.0), 0, [(t(0.28), 0), (t(0.3), t(0.04)), (t(0.22), t(0.06))], "stone", sides=20, stage=0.2)
    D.woodpile(b, 0.02, 1.2, rng, along_x=False, length=0.85, depth=0.3, height=0.5, block=False)
    D.pithos(b, 0.3, 3.7, scale=0.8, rng=rng, lid=True)
    if story:  # the door (x 1.5-2.4) keeps its way clear
        S.slot(b, "south west", 0.6, 3.47, 1.45, 3.98)
        S.slot(b, "south east", 2.45, 3.47, 3.5, 3.98)
        S.slot(b, "north", 0.7, 0.02, 3.3, 0.52)
        S.slot(b, "east", 3.71, 0.8, 3.98, 3.2, "strip")
        _dress(b, story, 1, rng)
    D.grass(b, rng, 34)
    b.scaffold(height=2.7 * M)


@model("common_housing_3", "common_housing", "lepianka: bielony dom pod dachówką, murek z bramką; zboże, figi, hydria z wodą, zioła w donicy")
def cabin(b, rng, story=None):
    acc = S.STORIES[story]["accent"] if story else {}
    D.footing(b, 0.35, 0.35, 3.3, 2.6, h=0.3, rng=rng)
    _room(b, rng, 0.4, 0.4, 3.2, 2.5, 0.3, 2.2, plaster="plaster", doors=[("s", 1.2, 0.85, 1.75)],
          windows=[("s", 2.3), ("e", 0.9), ("w", 0.9)])
    ridge = D.tiled_roof(b, 0.4, 0.4, 3.2, 2.5, 2.5, 0.85, rng, along_x=True, overhang=0.3)
    D.chimney_pot(b, 2.75, 1.65, ridge(2.75), rng, scale=0.9)
    for x, length in ((0.25, 1.1), (2.35, 1.4)):  # the yard's low wall, a gate between
        D.brick_wall(b, D.Wall(x, 3.55, length, "x", 0.0, 0.8, 0.28), rng, plaster="plaster", stage=(0.05, 0.4), both=True)
    with D.goods(b, "food"):
        D.sack(b, 0.6, 3.25, scale=0.7, rng=rng)
        D.basket(b, 1.0, 3.25, "figs", scale=0.8, rng=rng)
    if acc.get("pergola"):  # the gardeners' vine over the door: the jar steps aside, the herbs go to the beds
        D.hydria(b, 3.0, 3.25, scale=0.9, rng=rng)
        D.pergola(b, 1.45, 2.94, 2.62, 3.5, 2.05, rng)
    else:
        D.hydria(b, 2.6, 3.25, scale=0.9, rng=rng)
        D.potted_plant(b, 3.3, 3.25, scale=0.8, rng=rng)
    D.ivy(b, 3.6, 0.75, 0.8, 0.3, 1.9, rng, along_x=False, out=1)  # on the east wall, short of its window
    if story:
        S.slot(b, "north", 0.4, 0.02, 3.6, 0.33, "strip")
        S.slot(b, "west", 0.02, 0.4, 0.33, 2.9, "strip")
        S.slot(b, "east", 3.71, 1.6, 3.98, 2.95, "strip")
        _dress(b, story, 3, rng)
    D.grass(b, rng, 34)
    b.scaffold(height=2.9 * M)


@model("common_housing_4", "common_housing", "zagroda chłopska: dom na planie L z dziedzińcem; zboże, cebula, bele wełny, hydria")
def homestead(b, rng):
    D.footing(b, 0.25, 0.25, 3.5, 2.0, h=0.45, rng=rng)
    _room(b, rng, 0.3, 0.3, 3.4, 1.9, 0.45, 2.3, plaster="plaster", doors=[("s", 0.6, 0.85, 1.8)], wreath=True,
          windows=[("s", 1.8), ("w", 0.6), ("n", 1.6)])
    D.footing(b, 2.25, 2.15, 1.5, 1.6, h=0.4, rng=rng, group="wing")
    _room(b, rng, 2.3, 2.2, 1.4, 1.5, 0.4, 2.0, plaster="plaster", doors=[("w", 0.3, 0.8, 1.7)],
          windows=[("s", 0.5), ("e", 0.4)], stage=(0.15, 0.55), group="wing")
    ridge = D.tiled_roof(b, 0.3, 0.3, 3.4, 1.9, 2.75, 0.75, rng, along_x=True, overhang=0.16)
    D.chimney_pot(b, 1.1, 1.25, ridge(1.1), rng)
    D.shed_roof(b, 2.3, 2.2, 1.4, 1.5, 2.4, 2.7, rng, falls="s", overhang=0.18, group="wing")
    D.brick_wall(b, D.Wall(0.2, 3.65, 2.0, "x", 0.0, 1.0, 0.28), rng, plaster="plaster", stage=(0.05, 0.4), both=True)
    D.brick_wall(b, D.Wall(0.2, 2.35, 1.3, "y", 0.0, 1.0, 0.28), rng, plaster="plaster", stage=(0.05, 0.4), both=True)
    with D.goods(b, "food"):
        D.sack(b, 0.75, 2.55, scale=0.75, rng=rng)
        D.sack(b, 1.12, 2.52, scale=0.7, rng=rng, name="sack 2")
        D.basket(b, 0.8, 3.3, "onions", scale=0.8, rng=rng)
    with D.goods(b, "fleece"):
        D.wool_bale(b, 1.6, 2.55, scale=0.9, rng=rng)
        D.wool_bale(b, 1.55, 3.1, scale=0.8, rng=rng, name="wool 2")
    D.hydria(b, 2.0, 3.35, scale=0.8, rng=rng)
    D.laundry_line(b, (0.58, 2.32), (0.58, 3.56), 1.85, rng)
    D.grass(b, rng, 30)
    b.scaffold(height=3.2 * M)


@model("common_housing_5", "common_housing", "kamienica czynszowa: kamienny parter, piętro w tynku, balkon z toczonymi tralkami na wspornikach; zboże, wełna, krosno, hydria")
def row_house(b, rng):
    D.footing(b, 0.35, 0.35, 3.3, 3.1, h=0.3, rng=rng)
    _room(b, rng, 0.4, 0.4, 3.2, 3.0, 0.3, 2.3, thick=0.45, part="stone", block=(0.62, 0.32),
          doors=[("s", 0.6, 0.9, 1.9)], windows=[("s", 2.2, 0.4, 1.2, 0.45), ("e", 1.2, 0.4, 1.2, 0.45)])
    _room(b, rng, 0.4, 0.4, 3.2, 3.0, 2.6, 2.1, plaster="plaster", doors=[("s", 1.2, 0.8, 1.9)],
          windows=[("s", 0.35), ("s", 2.45), ("e", 1.2), ("w", 1.2), ("n", 1.5)], stage=(0.5, 0.68))
    # The balcony in front of the upper floor's door: a floor on beams and carved consoles, closed on its three
    # open sides by a railing of turned balusters between corner posts, all of it on the floor.
    fx0, fx1, fy0, fy1, fz = 0.45, 3.55, 3.40, 3.92, 2.65  # the floor's top
    b.box(t(fx0), t(3.36), t(fz - 0.05), t(fx1 - fx0), t(fy1 - 3.36), t(0.05), "wood", stage=0.56, tint=0.5)
    if b.detail:
        for i in range(5):  # beams out of the wall, each on a console
            bx = fx0 + 0.06 + i * (fx1 - fx0 - 0.2) / 4
            b.box(t(bx), t(3.3), t(fz - 0.15), t(0.08), t(fy1 - 3.32), t(0.1), "dark_wood", stage=0.55)
            # The console: a stone corbel in the wall and a strut from it up to the beam's underside. Over the
            # door and the window the corbel sits on their lintels.
            over = any(a <= bx + 0.04 <= a + w_ for a, w_ in ((1.0, 0.9), (2.6, 0.4)))
            base = fz - (0.3 if over else 0.6)
            b.cbox(t(bx - 0.01), t(3.4), t(base - 0.08), t(0.1), t(0.07), t(0.12), t(0.01), "stone", stage=0.55)
            b.tube_path([(t(bx + 0.04), t(3.43), t(base)), (t(bx + 0.04), t(3.40 + (fz - 0.15 - base) * 0.9 + 0.05),
                         t(fz - 0.15))], t(0.03), "dark_wood", sides=8, stage=0.55)
        for i in range(10):  # the planks
            b.box(t(fx0 + i * (fx1 - fx0) / 10 + 0.004), t(3.4), t(fz), t((fx1 - fx0) / 10 - 0.008), t(fy1 - 3.4),
                  t(0.012), "wood", stage=0.56, tint=rng.uniform(0.3, 0.7))
    top = fz + 0.85
    posts = [(fx0 + 0.04, fy1 - 0.04), (fx1 - 0.04, fy1 - 0.04), (fx0 + 0.04, fy0 + 0.04), (fx1 - 0.04, fy0 + 0.04)]
    for px, py in posts:  # the corner posts and the posts against the wall
        b.cbox(t(px - 0.04), t(py - 0.04), t(fz), t(0.08), t(0.08), t(top - fz + 0.05), t(0.01), "dark_wood", stage=0.6)
    rails = [(fx0, fy1 - 0.08, fx1 - fx0, 0.08), (fx0, fy0, 0.08, fy1 - fy0), (fx1 - 0.08, fy0, 0.08, fy1 - fy0)]
    for rx, ry, rw, rd in rails:  # the handrail and the bottom rail on all three sides
        b.cbox(t(rx), t(ry), t(top - 0.02), t(rw), t(rd), t(0.07), t(0.012), "dark_wood", stage=0.61)
        b.box(t(rx + 0.01), t(ry + 0.01), t(fz), t(rw - 0.02), t(rd - 0.02), t(0.05), "dark_wood", stage=0.6)
    balusters = []
    n_front = 12 if b.detail else 4
    for i in range(n_front):  # the front, between the corner posts
        balusters.append((fx0 + 0.16 + i * (fx1 - fx0 - 0.32) / (n_front - 1), fy1 - 0.04))
    n_side = 2
    for i in range(n_side):  # the two ends
        py = fy0 + 0.17 + i * (fy1 - fy0 - 0.34) / max(n_side - 1, 1)
        balusters += [(fx0 + 0.04, py), (fx1 - 0.04, py)]
    for bx_, by_ in balusters:
        assert fx0 + 0.035 <= bx_ <= fx1 - 0.035 and fy0 + 0.035 <= by_ <= fy1 - 0.035, "a baluster off the balcony"
        if b.detail:
            D.baluster(b, bx_, by_, fz + 0.05, top - fz - 0.07, stage=0.6)
        else:
            b.box(t(bx_ - 0.02), t(by_ - 0.02), t(fz + 0.05), t(0.04), t(0.04), t(top - fz - 0.07), "wood", stage=0.6)
    if b.detail:
        for x, s_ in ((0.75, 0.5), (3.2, 0.45)):  # potted herbs on the balcony, clear of the door
            D.potted_plant(b, x, 3.62, scale=s_, rng=rng, z=fz, name="balcony plant %d" % int(x * 10))
    ridge = D.tiled_roof(b, 0.4, 0.4, 3.2, 3.0, 4.7, 0.95, rng, along_x=True, overhang=0.3)
    D.chimney_pot(b, 0.95, 1.9, ridge(0.95), rng, scale=1.1)
    with D.goods(b, "food"):
        D.sack(b, 2.3, 3.72, scale=0.65, rng=rng)
        D.basket(b, 2.75, 3.72, "carrots", scale=0.7, rng=rng)
    with D.goods(b, "fleece"):
        D.wool_bale(b, 0.5, 3.75, scale=0.7, rng=rng)
        D.loom(b, 0.0, 1.0, along_x=False, rng=rng)
    D.hydria(b, 3.32, 3.72, scale=0.8, rng=rng)
    D.flower_box(b, 3.6, 2.02, 3.41, 0.51, rng, along_x=False, out=1)  # under the upper east window
    D.grass(b, rng, 28)
    b.scaffold(height=5.2 * M)


@model("common_housing_6", "common_housing", "mieszkanie: piętrowy blok i skrzydła wokół dziedzińca ze studnią; gzyms, meander, antefiksy i akroteria; zboże, oliwki, wełna, oliwa")
def flat(b, rng):
    D.footing(b, 0.3, 0.3, 3.4, 1.6, h=0.35, rng=rng)
    _room(b, rng, 0.35, 0.35, 3.3, 1.5, 0.35, 2.2, plaster="plaster", doors=[("s", 1.3, 0.9, 1.85)], wreath=True,
          windows=[("s", 0.45), ("s", 2.5), ("n", 0.9), ("n", 2.3)])
    _room(b, rng, 0.35, 0.35, 3.3, 1.5, 2.55, 2.0, plaster="plaster", windows=[("s", 0.5), ("s", 1.5), ("s", 2.5),
          ("w", 0.5), ("e", 0.5)], stage=(0.5, 0.66), ornate=True)
    b.box(t(0.32), t(0.32), t(2.5), t(3.36), t(1.56), t(0.12), "ochre", stage=0.5, plaster=True)  # the band
    if b.detail:
        D.meander(b, 0.4, 1.875, 3.2, 2.505, along_x=True, outward=1, unit=0.055, part="stone", stage=0.52)
        D.cornice(b, 0.35, 0.35, 3.3, 1.5, 4.43, stage=0.62)
    for x in (0.35, 2.6):  # the wings
        wing = "west wing" if x < 1 else "east wing"
        _room(b, rng, x, 1.85, 1.05, 1.85, 0.3, 1.75, plaster="plaster", doors=[("e" if x < 1 else "w", 0.6, 0.75, 1.6)],
              windows=[("s", 0.3)], stage=(0.15, 0.55), group=wing)
        D.shed_roof(b, x, 1.85, 1.05, 1.85, 2.05, 2.47, rng, falls="s", overhang=0.12, group=wing)
    ridge = D.tiled_roof(b, 0.35, 0.35, 3.3, 1.5, 4.57, 0.7, rng, along_x=True, overhang=0.16, antefix=True)
    D.chimney_pot(b, 2.55, 1.1, ridge(2.55), rng)
    if b.detail:
        _acroteria(b, 0.35, 0.35, 3.3, 1.5, 4.57, 0.7 * D.ROOF_STEEP, 0.16)
    b.box(t(1.7), t(2.6), 0, t(0.6), t(0.6), t(0.15), "stone", stage=0.05)  # the yard's paving and its well
    b.solid("well", t(1.7), t(2.6), 0, t(2.3), t(3.2), t(1.4))
    b.lathe(t(2.0), t(2.9), 0, [(t(0.28), 0), (t(0.29), t(0.5)), (t(0.31), t(0.55)), (t(0.2), t(0.55))], "stone",
            sides=24 if b.detail else 6, stage=0.3, top="water")
    if b.detail:  # the well's frame, its beam and the rope to the bucket
        for px in (1.74, 2.26):
            b.log((t(px), t(2.9), t(0.55)), (t(px), t(2.9), t(1.3)), t(0.025), sides=8, stage=0.35)
        b.log((t(1.7), t(2.9), t(1.3)), (t(2.3), t(2.9), t(1.3)), t(0.03), sides=10, stage=0.35)
        b.tube_path([(t(2.0), t(2.9), t(1.28)), (t(2.0), t(2.9), t(0.8))], t(0.006), "rope", sides=5, stage=0.35)
        b.lathe(t(2.0), t(2.9), t(0.66), [(t(0.07), 0), (t(0.09), t(0.14))], "wood", sides=14, stage=0.35, top="water")
    with D.goods(b, "food"):
        D.sack(b, 1.62, 2.15, scale=0.6, rng=rng)
        D.basket(b, 2.38, 2.15, "olives", scale=0.6, rng=rng)
    with D.goods(b, "fleece"):
        D.wool_bale(b, 1.62, 3.45, scale=0.6, rng=rng)
    with D.goods(b, "olive_oil"):
        D.pithos(b, 2.35, 3.42, scale=0.55, rng=rng, lid=True)
        D.amphora(b, 2.0, 3.86, scale=0.55, rng=rng)
    for i, at in enumerate((0.5, 1.5, 2.5)):  # flowers under the upper windows
        D.flower_box(b, 0.35 + at - 0.03, 1.85, 3.36, 0.51, rng, name="flower box %d" % (i + 1))
    D.ivy(b, 3.65, 0.7, 1.05, 0.35, 2.5, rng, along_x=False, out=1)
    D.hydria(b, 1.99, 2.25, scale=0.55, rng=rng)
    D.grass(b, rng, 30)
    b.scaffold(height=5.0 * M)


@model("common_housing_7", "common_housing", "dom miejski: portyk na kolumnach doryckich z fryzem tryglifów, gzyms, meander, antefiksy i akroteria, obramienia okien; dobra na portyku")
def city_house(b, rng):
    D.footing(b, 0.25, 0.25, 3.5, 2.5, h=0.55, rng=rng)
    _room(b, rng, 0.3, 0.3, 3.4, 2.4, 0.55, 2.4, thick=0.42, part="stone", block=(0.58, 0.3), wreath=True,
          doors=[("s", 1.25, 0.95, 2.0)], windows=[("e", 0.9, 0.4, 1.3, 0.5), ("w", 0.9, 0.4, 1.3, 0.5)])
    _room(b, rng, 0.3, 0.3, 3.4, 2.4, 2.95, 2.2, plaster="plaster", windows=[("s", 0.6), ("s", 1.5), ("s", 2.4),
          ("e", 0.9), ("w", 0.9), ("n", 1.5)], stage=(0.5, 0.68), ornate=True)
    b.box(t(0.27), t(0.27), t(2.85), t(3.46), t(2.46), t(0.16), "cloth", stage=0.5, plaster=True)  # red frieze
    if b.detail:
        D.meander(b, 0.32, 2.745, 3.36, 2.865, along_x=True, outward=1, unit=0.06, part="ochre", stage=0.52)
        D.cornice(b, 0.3, 0.3, 3.4, 2.4, 5.0, stage=0.66)
    b.box(t(0.3), t(2.7), 0, t(3.4), t(1.1), t(0.55), "stone", stage=0.05)  # the porch's floor and steps
    b.box(t(1.0), t(3.8), 0, t(2.0), t(0.15), t(0.28), "stone", stage=0.06)
    for x in (0.75, 1.9, 3.05):
        D.doric_column(b, x, 3.45, 0.55, 2.25, radius=0.15, stage=0.45)
    b.box(t(0.4), t(3.2), t(2.8), t(3.2), t(0.5), t(0.16), "stone", stage=0.5)  # the architrave
    if b.detail:  # the Doric frieze over it: triglyphs (three grooved bars) and painted metopes between
        n = 7
        for i in range(n):
            fx = 0.4 + i * 3.2 / (n - 1) - 0.08
            for g in range(3):
                b.box(t(fx + g * 0.055), t(3.66), t(2.96), t(0.04), t(0.04), t(0.2), "stone", stage=0.52)
            if i < n - 1:
                b.box(t(fx + 0.17), t(3.68), t(2.98), t(3.2 / (n - 1) - 0.18), t(0.02), t(0.16), "blue_paint", stage=0.52,
                      tint=0.4)
        b.box(t(0.36), t(3.18), t(3.16), t(3.28), t(0.56), t(0.05), "stone", stage=0.53)
    else:
        b.box(t(0.4), t(3.2), t(2.96), t(3.2), t(0.5), t(0.2), "stone", stage=0.52)
    D.shed_roof(b, 0.3, 2.7, 3.4, 1.0, 3.21, 3.62, rng, falls="s", overhang=0.12, gable="stone", stage=(0.62, 0.7),
                antefix=True, group="porch")
    ridge = D.tiled_roof(b, 0.3, 0.3, 3.4, 2.4, 5.15, 1.0, rng, along_x=True, overhang=0.16, antefix=True)
    D.chimney_pot(b, 2.65, 1.5, ridge(2.65), rng, scale=1.15)
    if b.detail:
        _acroteria(b, 0.3, 0.3, 3.4, 2.4, 5.15, 1.0 * D.ROOF_STEEP, 0.16, size=0.42)
    with D.goods(b, "olive_oil"):
        D.pithos(b, 1.32, 3.0, scale=0.5, rng=rng, z=0.55, lid=True)
        D.amphora(b, 2.8, 3.0, scale=0.6, rng=rng, z=0.55)
    with D.goods(b, "food"):
        D.basket(b, 0.65, 2.98, "figs", scale=0.55, rng=rng, z=0.55)
    with D.goods(b, "fleece"):
        D.wool_bale(b, 3.25, 2.98, scale=0.55, rng=rng, z=0.55)
    D.potted_plant(b, 0.35, 3.8, scale=0.6, rng=rng)
    D.potted_plant(b, 3.65, 3.8, scale=0.6, rng=rng, name="plant 2")
    for i, at in enumerate((0.6, 1.5, 2.4)):
        D.flower_box(b, 0.3 + at - 0.03, 2.7, 3.78, 0.51, rng, name="flower box %d" % (i + 1))
    D.grass(b, rng, 26)
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


def _stories():
    """Levels 1-3 in each family's version: common_housing_<level>_<story> (boot.gd picks it by the id)."""
    for level, fn in ((1, shelter), (2, hut), (3, cabin)):
        base = MODELS["common_housing_%d" % level]
        for story, spec in S.STORIES.items():
            MODELS["common_housing_%d_%s" % (level, story)] = (
                (lambda b, rng, fn=fn, story=story: fn(b, rng, story)), "common_housing",
                base[2] + "; historia: " + spec["name"])


_stories()


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
