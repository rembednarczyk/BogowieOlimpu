"""The game's building models, built in code (docs/ASSETY.md, R-017): stylized realism from Greek domestic and
public architecture (rammed earth and mudbrick on stone footings, lime plaster, terracotta tiles, timber), our own
shapes, nothing from the reference game.

Usage: blender -b --factory-startup --python tools/blender/models.py -- [out_dir] [model ...]
Writes <out_dir>/<model>.glb (default game/models) and game/models/provenance.json.
"""
import json
import os
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import math  # noqa: E402
import random  # noqa: E402

import bo_asset as A  # noqa: E402
import bo_detail as D  # noqa: E402
import bo_story as S  # noqa: E402
import bo_collapse as C  # noqa: E402
from bo_detail import t  # noqa: E402
from bo_asset import FOUNDATION, WALLS, ROOF, M  # noqa: E402

MODELS = {}

# The proportions under comparison (owner, 2026-11-28, docs/STYL-BUDOWLI.md): "A" the houses as they are, "B"
# nearer the reference's sprites (eZeus): low and squat up to level 5 (roofs at the plans' rise, the row house's
# storeys lower), rising only at levels 6-7. BO_PROPORTION=B exports B; A stays the game's until the owner picks.
PROFILE = os.environ.get("BO_PROPORTION", "B")  # B chosen by the owner, 2026-11-28
LOW_STOREYS = {"A": (2.3, 2.1, 1.9), "B": (2.0, 1.85, 1.7), "C": (2.0, 1.85, 1.7)}  # the row house: storeys, doors
# "C": B and the flat and the city house with lower storeys too (ground floor, upper floor: how much lower).
HIGH_DROP = {6: (-0.25, -0.2), 7: (-0.3, -0.25)}


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
    """Level 1: a small shelter of cloth and hides, of a family's own shape (owner, 2026-11-28: smaller, of
    different sizes and shapes, at most half the plot with its eaves, nothing like an army's tents): the
    shepherds a round tent of hides on poles, the potters a lean-to open at the front over their work, the
    gardeners (and the plain house) a small ridge tent. Set off the plot's middle, the yard round it."""
    roof_part, wall_part = S.STORIES[story]["accent"]["tent"] if story else ("linen", "hide")
    if story == "shepherds":
        box, door = _round_tent(b, rng, 2.15, 1.85, 1.3, 1.95, roof_part, wall_part)
    elif story == "potters":
        box, door = _lean_to(b, rng, 0.55, 0.75, 3.35, 2.35, 2.05, 1.45, roof_part, wall_part)
    elif story == "gardeners":
        box, door = _ridge_tent(b, rng, 1.25, 0.85, 3.45, 2.65, 1.3, 2.15, roof_part, wall_part)
    else:
        box, door = _ridge_tent(b, rng, 0.9, 0.9, 3.3, 2.9, 1.35, 2.25, roof_part, wall_part)
    D.woodpile(b, 0.02, 1.2, rng, along_x=False, length=0.85, depth=0.3, height=0.5, block=False)
    D.pithos(b, 0.3, 3.7, scale=0.8, rng=rng, lid=True)
    if story:  # the ground round the shelter, the door's way kept clear
        bx0, by0, bx1, by1 = box
        if by0 > 0.45:
            S.slot(b, "north", 0.1, 0.02, 3.9, by0 - 0.1)
        if by1 < 3.55:
            S.slot(b, "south west", 0.45, by1 + 0.1, door - 0.5, 3.98)
            S.slot(b, "south east", door + 0.5, by1 + 0.1, 3.95, 3.98)
        if 4.0 - bx1 > 0.3:
            S.slot(b, "east", bx1 + 0.08, by0, 3.98, by1, "strip")
        if bx0 > 0.6:
            S.slot(b, "west", 0.35, by0, bx0 - 0.08, by1, "strip")
        _dress(b, story, 1, rng)
    D.grass(b, rng, 40)


def _fire(b, x, y):
    if b.detail:
        b.lathe(t(x), t(y), 0, [(t(0.24), 0), (t(0.26), t(0.04)), (t(0.19), t(0.06))], "stone", sides=20, stage=0.2)


def _ridge_tent(b, rng, x0, y0, x1, y1, eave, ridge, roof_part, wall_part):
    """A small ridge tent: forked posts at the ends, a ridge pole, the cloth over it, hide walls, the door flap
    rolled up in the south side, guy ropes to pegs. Returns its box with eaves and its door's x."""
    b.solid("tent", t(x0 - 0.17), t(y0 - 0.14), 0, t(x1 + 0.17), t(y1 + 0.14), t(ridge + 0.2))
    mid, dc = (y0 + y1) / 2, (x0 + x1) / 2
    posts = 3 if b.detail else 1
    for i in range(posts + 1):  # the side posts along both eaves
        px = x0 + (x1 - x0) * i / posts
        for py in (y0, y1):
            b.log((t(px), t(py), 0), (t(px), t(py), t(eave + 0.05)), t(0.045), sides=12 if b.detail else 5, stage=0.15,
                  tint=rng.uniform(0.3, 0.7))
    for px in (x0 - 0.02, x1 + 0.02):  # the forked posts and the ridge pole
        b.log((t(px), t(mid), 0), (t(px), t(mid), t(ridge + 0.1)), t(0.06), sides=14 if b.detail else 6, stage=0.2)
        if b.detail:
            for side in (-1, 1):
                b.tube_path([(t(px), t(mid), t(ridge - 0.02)), (t(px), t(mid + side * 0.08), t(ridge + 0.15))],
                            t(0.028), "bark", sides=8, stage=0.2)
    b.log((t(x0 - 0.12), t(mid), t(ridge)), (t(x1 + 0.12), t(mid), t(ridge)), t(0.05), sides=14 if b.detail else 6,
          stage=0.3)
    for py in (y0, y1):
        b.log((t(x0 - 0.08), t(py), t(eave)), (t(x1 + 0.08), t(py), t(eave)), t(0.035), sides=12 if b.detail else 5,
              stage=0.35)
    nu, nv = (36, 12) if b.detail else (6, 2)
    wave = {-1: rng.uniform(0, 6), 1: rng.uniform(0, 6)}

    def roof(side):
        def fn(u, v):
            ey = y0 if side < 0 else y1
            x = x0 - 0.1 + (x1 - x0 + 0.2) * u
            y = mid + (ey + side * 0.1 - mid) * v
            sag = 0.09 * math.sin(math.pi * v) * (0.6 + 0.4 * abs(math.sin(u * math.pi * 3)))
            z = ridge + 0.04 + (eave - 0.04 - ridge - 0.04) * v - sag + 0.013 * math.sin(u * 36.0 + wave[side]) * v
            return (t(x), t(y), t(z))
        return fn
    for side in (-1, 1):
        b.surface(roof(side), nu, nv, roof_part, stage=0.7, tint=rng.uniform(0.35, 0.65))

    def wall(xa, ya, xb, yb, top0, top1):
        def fn(u, v):
            x, y = xa + (xb - xa) * u, ya + (yb - ya) * u
            top = top0 + (top1 - top0) * (1 - abs(2 * u - 1)) if top1 != top0 else top0
            nx, ny = (yb - ya), -(xb - xa)
            ln = max(math.hypot(nx, ny), 1e-6)
            out = 0.028 * math.sin(u * 40.0 + xa * 3.0) * (1.0 - v * 0.7) + 0.05 * (1.0 - v) ** 3
            return (t(x + nx / ln * out), t(y + ny / ln * out), t(top * v + 0.01))
        return fn
    b.surface(wall(x0, y0, x1, y0, eave, eave), nu, nv // 2 + 1, wall_part, stage=0.5, tint=rng.uniform(0.3, 0.7))
    b.surface(wall(x0, y0, x0, y1, eave, ridge), nu // 2, nv // 2 + 1, "linen", stage=0.55, tint=0.4)
    b.surface(wall(x1, y1, x1, y0, eave, ridge), nu // 2, nv // 2 + 1, "linen", stage=0.55, tint=0.6)
    for xa, xb in ((x0, dc - 0.38), (dc + 0.38, x1)):  # the south side, the door between
        b.surface(wall(xb, y1, xa, y1, eave, eave), max(2, nu // 3), nv // 2 + 1, wall_part, stage=0.5,
                  tint=rng.uniform(0.3, 0.7))
    if b.detail:
        b.tube_path([(t(dc - 0.42), t(y1 + 0.06), t(eave - 0.11)), (t(dc + 0.42), t(y1 + 0.06), t(eave - 0.11))], t(0.055),
                    wall_part, sides=14, stage=0.75)
        for cx, cy, gx, gy in ((x0, y0, x0 - 0.3, y0 - 0.4), (x1, y0, x1 + 0.3, y0 - 0.4), (x0, y1, x0 - 0.3, y1 + 0.4),
                               (x1, y1, x1 + 0.3, y1 + 0.4)):  # guy ropes to pegs
            gx, gy = min(max(gx, 0.08), 3.92), min(max(gy, 0.08), 3.92)
            b.tube_path([(t(cx), t(cy), t(eave)), (t(gx), t(gy), t(0.06))], t(0.008), "rope", sides=6, stage=0.8)
            b.log((t(gx), t(gy), 0), (t(gx), t(gy), t(0.12)), t(0.02), sides=8, stage=0.8)
    _fire(b, dc, mid)
    return (x0 - 0.17, y0 - 0.14, x1 + 0.17, y1 + 0.14), dc


def _round_tent(b, rng, cx, cy, r, h, roof_part, wall_part):
    """A round herdsman's hut: bent poles from a ring on the ground arched to a low dome, hides drawn over them
    in two bands (cloth above, darker hides low), sagging between the poles, a smoke hole at the top with a
    rope ring, the door a gap on the south with its flap folded back, stones on the hem."""
    b.solid("tent", t(cx - r - 0.06), t(cy - r - 0.06), 0, t(cx + r + 0.06), t(cy + r + 0.06), t(h + 0.1))
    n = 10 if b.detail else 6
    door, gap = math.pi / 2, 0.34  # the door faces south (+y)

    def dome(a, vv, out=0.0):  # vv 0 at the top, 1 at the ground: a low dome, a little bulging
        q = vv * math.pi / 2
        rr = 0.14 + (r - 0.14) * math.sin(q) ** 0.85 + out
        return (cx + rr * math.cos(a), cy + rr * math.sin(a), 0.035 + (h - 0.035) * math.cos(q))
    for k in range(n):  # the bent poles
        a = door + gap + (2 * math.pi - 2 * gap) * (k + 0.5) / n
        pts = [dome(a, 1.0 - m / 8, -0.01) for m in range(9)]
        b.tube_path([tuple(t(v) for v in q) for q in pts], t(0.025), "bark", sides=6 if b.detail else 4, stage=0.2)
    for side in (-1, 1):  # the door frame
        a = door + side * gap
        pts = [dome(a, 1.0 - m / 8, -0.01) for m in range(7)]
        b.tube_path([tuple(t(v) for v in q) for q in pts], t(0.03), "dark_wood", sides=6 if b.detail else 4, stage=0.2)
    nu, nv = (40, 14) if b.detail else (8, 3)

    def cover(v0, v1):
        def fn(u, v):
            vv = v0 + (v1 - v0) * v
            a = door + gap + (2 * math.pi - 2 * gap) * u
            sag = -0.05 * math.sin(math.pi * vv) * abs(math.sin(n * math.pi * u))
            return tuple(t(c) for c in dome(a, vv, sag + 0.012))
        return fn
    b.surface(cover(0.12, 0.55), nu, nv // 2, roof_part, stage=0.7, tint=rng.uniform(0.4, 0.7))
    b.surface(cover(0.55, 1.0), nu, nv // 2, wall_part, stage=0.5, tint=rng.uniform(0.3, 0.6))
    if b.detail:
        ring = [tuple(t(c) for c in dome(2 * math.pi * k / 16, 0.12, 0.02)) for k in range(17)]
        b.tube_path(ring, t(0.02), "rope", sides=6, stage=0.72, caps=False)  # the smoke hole's rim
        for side in (-1, 1):  # the door flap, folded back on each side
            a = door + side * (gap + 0.02)
            def flap(u, v, a=a, side=side):
                return tuple(t(c) for c in dome(a + side * 0.3 * u, 0.5 + 0.5 * v, 0.05))
            b.surface(flap, 3, 4, wall_part, stage=0.75, tint=0.25)
        for k in range(14):  # stones holding the hem down
            a = door + gap + 0.1 + (2 * math.pi - 2 * gap - 0.2) * k / 13
            b.lathe(t(cx + (r + 0.03) * math.cos(a)), t(cy + (r + 0.03) * math.sin(a)), 0,
                    [(t(0.05), 0), (t(0.055), t(0.04)), (t(0.02), t(0.06))], "stone", sides=8, stage=0.6,
                    tint=rng.uniform(0.3, 0.7))
    _fire(b, cx, cy)
    return (cx - r - 0.06, cy - r - 0.06, cx + r + 0.06, cy + r + 0.06), cx


def _lean_to(b, rng, x0, y0, x1, y1, hb, hf, roof_part, wall_part):
    """A lean-to, open at the front over the work: posts at the back and the front, the cloth roof falling from
    the back to the front, a hide wall at the back and the two sides cut to the slope, the front open."""
    b.solid("tent", t(x0 - 0.06), t(y0 - 0.06), 0, t(x1 + 0.06), t(y1 + 0.2), t(hb + 0.12))
    posts = 3 if b.detail else 1
    for i in range(posts + 1):
        px = x0 + (x1 - x0) * i / posts
        for py, ph in ((y0, hb), (y1, hf)):
            b.log((t(px), t(py), 0), (t(px), t(py), t(ph + 0.04)), t(0.045), sides=10 if b.detail else 4, stage=0.15,
                  tint=rng.uniform(0.3, 0.7))
    for py, ph in ((y0, hb), (y1, hf)):
        b.log((t(x0 - 0.1), t(py), t(ph)), (t(x1 + 0.1), t(py), t(ph)), t(0.04), sides=10 if b.detail else 4, stage=0.3)
    nu, nv = (36, 10) if b.detail else (6, 2)
    ph_ = rng.uniform(0, 6)

    def roof(u, v):  # from the back (v 0) to past the front
        x = x0 - 0.1 + (x1 - x0 + 0.2) * u
        y = y0 - 0.04 + (y1 + 0.16 - y0 + 0.04) * v
        z = hb + 0.05 + (hf - hb - 0.08) * v - 0.08 * math.sin(math.pi * min(v, 1.0)) * (0.6 + 0.4 * abs(math.sin(u * math.pi * 3)))
        return (t(x), t(y), t(z + 0.012 * math.sin(u * 30.0 + ph_) * v))
    b.surface(roof, nu, nv, roof_part, stage=0.7, tint=rng.uniform(0.4, 0.7))

    def back(u, v):
        x = x0 + (x1 - x0) * u
        return (t(x), t(y0 - 0.03 - 0.025 * math.sin(u * 34.0) * (1 - v)), t(0.01 + hb * v))
    b.surface(back, nu, nv // 2 + 1, wall_part, stage=0.5, tint=rng.uniform(0.3, 0.6))
    for xs in (x0, x1):
        def side(u, v, xs=xs):
            y = y0 + (y1 - y0) * u
            top = hb + (hf - hb) * u
            return (t(xs + (0.025 if xs == x1 else -0.025) * math.sin(u * 20.0) * (1 - v)), t(y), t(0.01 + top * v))
        b.surface(side, nu // 3, nv // 2 + 1, wall_part, stage=0.52, tint=rng.uniform(0.3, 0.6))
    if b.detail:  # a bench and a shelf under the roof: where the work is done
        b.box(t(x0 + 0.15), t(y0 + 0.1), 0, t(x1 - x0 - 0.3), t(0.3), t(0.42), "wood", stage=0.8, tint=0.4)
        for k in range(6):
            b.lathe(t(x0 + 0.3 + k * (x1 - x0 - 0.6) / 5), t(y0 + 0.25), t(0.42),
                    [(t(0.05), 0), (t(0.07), t(0.06)), (t(0.045), t(0.12)), (t(0.05), t(0.14))], "roof_tile", sides=12,
                    stage=0.8, tint=rng.uniform(0.3, 0.8), smooth=2)
    _fire(b, x1 - 0.45, (y0 + y1) / 2 + 0.2)
    return (x0 - 0.06, y0 - 0.06, x1 + 0.06, y1 + 0.2), (x0 + x1) / 2


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
def homestead(b, rng, story=None):
    D.footing(b, 0.25, 0.25, 3.5, 2.0, h=0.45, rng=rng)
    _room(b, rng, 0.3, 0.3, 3.4, 1.9, 0.45, 2.3, plaster="plaster", doors=[("s", 0.6, 0.85, 1.8)],
          wreath=story in (None, "shepherds"),  # the awning and the vine take the wreath's place
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
    if story == "shepherds":  # the line carries the family's dyed wool, not the washing
        for i, (px, py) in enumerate(((0.58, 2.32), (0.58, 3.56))):
            b.prop("skein post %d" % (i + 1), t(px - 0.04), t(py - 0.04), 0, t(px + 0.04), t(py + 0.04), t(1.9))
            b.log((t(px), t(py), 0), (t(px), t(py), t(1.9)), t(0.03), sides=8, stage=0.9, tint=0.4)
        S.skeins(b, (0.58, 2.36, 1.82), (0.58, 3.52, 1.82), rng)  # the pole from post to post
    else:
        D.laundry_line(b, (0.58, 2.32), (0.58, 3.56), 1.85, rng)
    if story == "potters":
        S.awning(b, 0.78, 1.9, 2.2, 2.62, 0.55, 0.3, rng, pots=4)
    elif story == "gardeners":
        S.vine_canopy(b, 0.76, 1.95, 2.2, 2.45, 0.5, rng, root=(0.72, 2.31))
    if story:
        b.mirror = S.STORIES[story]["mirror"]
    D.grass(b, rng, 30)
    b.scaffold(height=3.2 * M)


@model("common_housing_6", "common_housing", "kamienica czynszowa: kamienny parter, piętro w tynku, balkon z toczonymi tralkami na wspornikach; zboże, wełna, krosno, hydria, pitos i amfora oliwy")
def row_house(b, rng, story=None):
    low = False  # level 6 since 2026-11-28 (swapped with the flat): full storeys in every profile
    hg, hu, hd = LOW_STOREYS["A"]
    zu = 0.3 + hg  # the upper floor's foot
    D.footing(b, 0.35, 0.35, 3.3, 3.1, h=0.3, rng=rng)
    _room(b, rng, 0.4, 0.4, 3.2, 3.0, 0.3, hg, thick=0.45, part="stone", block=(0.62, 0.32),
          doors=[("s", 0.6, 0.9, hd)], windows=[("s", 2.2, 0.4, 1.2, 0.45), ("e", 1.2, 0.4, 1.2, 0.45)])
    _room(b, rng, 0.4, 0.4, 3.2, 3.0, zu, hu, plaster="plaster", doors=[("s", 1.2, 0.8, hd)],
          windows=[("s", 0.35), ("s", 2.45), ("e", 1.2), ("w", 1.2), ("n", 1.5)], stage=(0.5, 0.68))
    # The balcony in front of the upper floor's door: a floor on beams and carved consoles, closed on its three
    # open sides by a railing of turned balusters between corner posts, all of it on the floor.
    fx0, fx1, fy0, fy1, fz = 0.45, 3.55, 3.40, 3.92, zu + 0.05  # the floor's top
    b.box(t(fx0), t(3.36), t(fz - 0.05), t(fx1 - fx0), t(fy1 - 3.36), t(0.05), "wood", stage=0.56, tint=0.5)
    b.solid("balcony", t(fx0), t(3.36), t(fz - 0.05), t(fx1), t(fy1), t(fz), block="balcony")
    if b.detail:
        for i in range(5):  # beams out of the wall, each on a console
            bx = fx0 + 0.06 + i * (fx1 - fx0 - 0.2) / 4
            b.box(t(bx), t(3.3), t(fz - 0.15), t(0.08), t(fy1 - 3.32), t(0.1), "dark_wood", stage=0.55)
        # (2026-11-28) No struts under it: the balcony hangs from the wall above, see the ties after the railing.
        for i in range(10):  # the planks
            b.box(t(fx0 + i * (fx1 - fx0) / 10 + 0.004), t(3.4), t(fz), t((fx1 - fx0) / 10 - 0.008), t(fy1 - 3.4),
                  t(0.012), "wood", stage=0.56, tint=rng.uniform(0.3, 0.7))
    top = fz + 0.85
    posts = [(fx0 + 0.04, fy1 - 0.04), (fx1 - 0.04, fy1 - 0.04), (fx0 + 0.04, fy0 + 0.04), (fx1 - 0.04, fy0 + 0.04)]
    for px, py in posts:  # the corner posts and the posts against the wall
        b.cbox(t(px - 0.04), t(py - 0.04), t(fz), t(0.08), t(0.08), t(top - fz + 0.05), t(0.01), "dark_wood", stage=0.6)
        b.solid("balcony post", t(px - 0.04), t(py - 0.04), t(fz), t(px + 0.04), t(py + 0.04), t(top + 0.05), block="balcony")
    b.solid("balcony rail", t(fx0), t(fy1 - 0.08), t(top - 0.02), t(fx1), t(fy1), t(top + 0.05), block="balcony")
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
    # The balcony hung from the building (owner, 2026-11-28: the struts below leaned out): iron ties from plates
    # in the wall under the frieze down to the front rail, clear of the door and the windows of the upper floor.
    for tx in (fx0 + 0.04, 1.4, 2.62, fx1 - 0.04):
        anchor = (tx, 3.43, zu + hu - 0.42)
        foot = (tx, fy1 - 0.04, top + 0.02)
        b.tube_path([tuple(t(v) for v in anchor), tuple(t(v) for v in foot)], t(0.012), "iron", sides=6, stage=0.62)
        b.box(t(tx - 0.04), t(3.4), t(zu + hu - 0.47), t(0.08), t(0.04), t(0.1), "iron", stage=0.62)  # the wall plate
    for bx_, by_ in balusters:
        assert fx0 + 0.035 <= bx_ <= fx1 - 0.035 and fy0 + 0.035 <= by_ <= fy1 - 0.035, "a baluster off the balcony"
        if b.detail:
            D.baluster(b, bx_, by_, fz + 0.05, top - fz - 0.07, stage=0.6)
        else:
            b.box(t(bx_ - 0.02), t(by_ - 0.02), t(fz + 0.05), t(0.04), t(0.04), t(top - fz - 0.07), "wood", stage=0.6)
    if story == "potters":  # painted amphorae for sale on the balcony, an awning over it
        for x in (0.75, 3.2):
            D.amphora(b, x, 3.62, scale=0.55, rng=rng, z=fz, name="balcony amphora %d" % int(x * 10))
        S.awning(b, 0.5, 3.5, 3.4, zu + hu - 0.1, 0.52, 0.3 if not low else 0.18, rng, pots=5)  # clear of the rail
    elif b.detail:
        for x, s_ in ((0.75, 0.5), (3.2, 0.45)):  # potted herbs on the balcony, clear of the door
            D.potted_plant(b, x, 3.62, scale=s_, rng=rng, z=fz, name="balcony plant %d" % int(x * 10))
    if story == "shepherds":  # dyed wool thrown over the balcony's rail to dry
        S.skeins(b, (0.65, 3.95, top + 0.03), (3.35, 3.95, top + 0.03), rng, pole=False, drop=0.5, pad=0.03)
    elif story == "gardeners":  # a vine from the ground over a frame above the balcony
        S.vine_canopy(b, 0.5, 3.5, 3.4, zu + hu - 0.2, 0.52, rng, root=(3.7, 3.56))
    # Level 6 (2026-11-28): a step above the flat in detail too: a painted frieze under a cornice, a meander
    # on the front, antefixes along the eaves and acroteria on the gables.
    b.box(t(0.37), t(0.37), t(zu + hu - 0.3), t(3.26), t(3.06), t(0.16), "cloth", stage=0.66, plaster=True)
    if b.detail:
        D.meander(b, 0.45, 3.425, 3.15, zu + hu - 0.295, along_x=True, outward=1, unit=0.055, part="ochre", stage=0.67)
        D.cornice(b, 0.4, 0.4, 3.2, 3.0, zu + hu - 0.14, stage=0.68)
    ridge = D.tiled_roof(b, 0.4, 0.4, 3.2, 3.0, zu + hu, 0.95, rng, along_x=True, overhang=0.3, antefix=True)
    if b.detail:
        _acroteria(b, 0.4, 0.4, 3.2, 3.0, zu + hu, 0.95 * D.ROOF_STEEP, 0.3, size=0.38)
    D.chimney_pot(b, 0.95, 1.9, ridge(0.95), rng, scale=1.1)
    with D.goods(b, "food"):
        D.sack(b, 2.3, 3.72, scale=0.65, rng=rng)
        D.basket(b, 2.75, 3.72, "carrots", scale=0.7, rng=rng)
    with D.goods(b, "fleece"):
        D.wool_bale(b, 0.5, 3.75, scale=0.7, rng=rng)
        D.loom(b, 0.0, 1.0, along_x=False, rng=rng, toward=True)  # its top leaning on the west wall
    D.hydria(b, 3.32, 3.72, scale=0.8, rng=rng)
    with D.goods(b, "olive_oil"):  # level 6 needs olive oil: the jars along the east wall
        D.pithos(b, 3.82, 1.0, scale=0.42, rng=rng, lid=True)
        D.amphora(b, 3.84, 1.55, scale=0.48, rng=rng)
    D.flower_box(b, 3.6, 2.02, zu + 0.81, 0.51, rng, along_x=False, out=1)  # under the upper east window
    if story:
        b.mirror = S.STORIES[story]["mirror"]
    D.grass(b, rng, 28)
    b.scaffold(height=5.2 * M)


@model("common_housing_5", "common_housing", "mieszkanie: piętrowy blok i skrzydła wokół dziedzińca ze studnią; gzyms, meander, antefiksy i akroteria; zboże, oliwki, wełna")
def flat(b, rng, story=None):
    d1, d2 = HIGH_DROP[6] if PROFILE == "C" else (0.0, 0.0)  # C: lower storeys

    def up(z):  # a height on the upper floor
        return z + d1

    def top(z):  # a height at the roof
        return z + d1 + d2
    wz = 1.05 if PROFILE != "C" else 0.9  # the upper windows sit lower in a lower storey (their pediments below the eaves)
    D.footing(b, 0.3, 0.3, 3.4, 1.6, h=0.35, rng=rng)
    _room(b, rng, 0.35, 0.35, 3.3, 1.5, 0.35, up(2.2), plaster="plaster", doors=[("s", 1.3, 0.9, 1.85 if d1 == 0 else 1.7)],
          wreath=d1 == 0,
          windows=[("s", 0.45), ("s", 2.5), ("n", 0.9), ("n", 2.3)])
    _room(b, rng, 0.35, 0.35, 3.3, 1.5, up(2.55), 2.0 + d2, plaster="plaster", windows=[("s", 0.5, 0.45, wz), ("s", 1.5, 0.45, wz), ("s", 2.5, 0.45, wz),
          ("w", 0.5), ("e", 0.5)], stage=(0.5, 0.66), ornate=True)
    b.box(t(0.32), t(0.32), t(up(2.5)), t(3.36), t(1.56), t(0.12), "ochre", stage=0.5, plaster=True)  # the band
    if b.detail:
        D.meander(b, 0.4, 1.875, 3.2, up(2.505), along_x=True, outward=1, unit=0.055, part="stone", stage=0.52)
        D.cornice(b, 0.35, 0.35, 3.3, 1.5, top(4.43), stage=0.62)
    for x in (0.35, 2.6):  # the wings, each on its own stone footing (2026-11-28: they hung 30 cm over the ground)
        wing = "west wing" if x < 1 else "east wing"
        D.footing(b, x - 0.05, 1.85, 1.15, 1.9, h=0.3, rng=rng, group=wing)
        _room(b, rng, x, 1.85, 1.05, 1.85, 0.3, 1.75, plaster="plaster", doors=[("e" if x < 1 else "w", 0.6, 0.75, 1.6)],
              windows=[("s", 0.3)], stage=(0.15, 0.55), group=wing)
        D.shed_roof(b, x, 1.85, 1.05, 1.85, 2.05, 2.47, rng, falls="s", overhang=0.12, group=wing)
    ridge = D.tiled_roof(b, 0.35, 0.35, 3.3, 1.5, top(4.57), 0.7, rng, along_x=True, overhang=0.16, antefix=True)
    D.chimney_pot(b, 2.55, 1.1, ridge(2.55), rng)
    if b.detail:
        _acroteria(b, 0.35, 0.35, 3.3, 1.5, top(4.57), 0.7 * D.ROOF_STEEP, 0.16)
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
    D.pithos(b, 2.35, 3.42, scale=0.55, rng=rng, lid=True, name="water jar")  # level 5 holds no oil (data/housing.json)
    for i, at in enumerate((0.5, 1.5, 2.5)):  # flowers under the upper windows
        D.flower_box(b, 0.35 + at - 0.03, 1.85, up(3.36) + wz - 1.05, 0.51, rng, name="flower box %d" % (i + 1))
    D.ivy(b, 3.65, 0.7, 1.05, 0.35, up(2.5), rng, along_x=False, out=1)
    D.hydria(b, 1.99, 2.25, scale=0.55, rng=rng)
    if story == "shepherds":  # dyed wool on a pole across the courtyard, wing to wing
        S.skeins(b, (1.445, 2.45, 1.95), (2.555, 2.45, 1.95), rng)  # the pole set into the two wing walls
    elif story == "potters":  # a striped awning over the courtyard on two poles, pots hung from it
        S.awning(b, 1.5, 2.5, 1.85, up(2.95), 1.0, 0.35, rng, poles=True, pots=3)
    elif story == "gardeners":  # the courtyard under a vine
        D.pergola(b, 1.47, 1.88, 2.53, 3.05, 2.75 if d1 == 0 else 2.68, rng)  # C: under the lower flower boxes
    if story:
        b.mirror = S.STORIES[story]["mirror"]
    D.grass(b, rng, 30)
    b.scaffold(height=5.0 * M)


@model("common_housing_7", "common_housing", "dom miejski: portyk na kolumnach doryckich z fryzem tryglifów, gzyms, meander, antefiksy i akroteria, obramienia okien; dobra na portyku")
def city_house(b, rng, story=None):
    d1, d2 = HIGH_DROP[7] if PROFILE == "C" else (0.0, 0.0)  # C: lower storeys

    def up(z):  # a height on the upper floor (and the porch's top, which meets it)
        return z + d1

    def top(z):  # a height at the roof
        return z + d1 + d2
    D.footing(b, 0.25, 0.25, 3.5, 2.5, h=0.55, rng=rng)
    _room(b, rng, 0.3, 0.3, 3.4, 2.4, 0.55, up(2.4), thick=0.42, part="stone", block=(0.58, 0.3), wreath=d1 == 0,
          doors=[("s", 1.25, 0.95, 2.0 if d1 == 0 else 1.85)], windows=[("e", 0.9, 0.4, 1.3, 0.5), ("w", 0.9, 0.4, 1.3, 0.5)])
    _room(b, rng, 0.3, 0.3, 3.4, 2.4, up(2.95), 2.2 + d2, plaster="plaster", windows=[("s", 0.6), ("s", 1.5), ("s", 2.4),
          ("e", 0.9), ("w", 0.9), ("n", 1.5)], stage=(0.5, 0.68), ornate=True)
    b.box(t(0.27), t(0.27), t(up(2.85)), t(3.46), t(2.46), t(0.16), "cloth", stage=0.5, plaster=True)  # red frieze
    if b.detail:
        D.meander(b, 0.32, 2.745, 3.36, up(2.865), along_x=True, outward=1, unit=0.06, part="ochre", stage=0.52)
        D.cornice(b, 0.3, 0.3, 3.4, 2.4, top(5.0), stage=0.66)
    b.box(t(0.3), t(2.7), 0, t(3.4), t(1.1), t(0.55), "stone", stage=0.05)  # the porch's floor and steps
    b.solid("porch floor", t(0.3), t(2.7), 0, t(3.7), t(3.8), t(0.55), block="porch")
    b.box(t(1.0), t(3.8), 0, t(2.0), t(0.15), t(0.28), "stone", stage=0.06)
    for x in (0.75, 1.9, 3.05):
        D.doric_column(b, x, 3.45, 0.55, up(2.25), radius=0.15, stage=0.45)
    b.box(t(0.4), t(3.2), t(up(2.8)), t(3.2), t(0.5), t(0.16), "stone", stage=0.5)  # the architrave
    b.solid("architrave", t(0.4), t(3.2), t(up(2.8)), t(3.6), t(3.7), t(up(2.96)), block="porch")
    if b.detail:  # the Doric frieze over it: triglyphs (three grooved bars) and painted metopes between
        n = 7
        for i in range(n):
            fx = 0.4 + i * 3.2 / (n - 1) - 0.08
            for g in range(3):
                b.box(t(fx + g * 0.055), t(3.66), t(up(2.96)), t(0.04), t(0.04), t(0.2), "stone", stage=0.52)
            if i < n - 1:
                b.box(t(fx + 0.17), t(3.68), t(up(2.98)), t(3.2 / (n - 1) - 0.18), t(0.02), t(0.16), "blue_paint", stage=0.52,
                      tint=0.4)
        b.box(t(0.36), t(3.18), t(up(3.16)), t(3.28), t(0.56), t(0.05), "stone", stage=0.53)
    else:
        b.box(t(0.4), t(3.2), t(up(2.96)), t(3.2), t(0.5), t(0.2), "stone", stage=0.52)
    D.shed_roof(b, 0.3, 2.7, 3.4, 1.0, up(3.21), up(3.62), rng, falls="s", overhang=0.12, gable="stone", stage=(0.62, 0.7),
                antefix=True, group="porch")
    if story == "gardeners":  # the vine thrown over the porch roof, hanging down its eave
        S.roof_vine(b, 0.3, 3.7, 2.95, 3.8, rng)
    ridge = D.tiled_roof(b, 0.3, 0.3, 3.4, 2.4, top(5.15), 1.0, rng, along_x=True, overhang=0.16, antefix=True)
    D.chimney_pot(b, 2.65, 1.5, ridge(2.65), rng, scale=1.15)
    if b.detail:
        _acroteria(b, 0.3, 0.3, 3.4, 2.4, top(5.15), 1.0 * D.ROOF_STEEP, 0.16, size=0.42)
    with D.goods(b, "olive_oil"):
        D.pithos(b, 1.32, 3.0, scale=0.5, rng=rng, z=0.55, lid=True)
        D.amphora(b, 2.8, 3.0, scale=0.6, rng=rng, z=0.55)
    with D.goods(b, "food"):
        D.basket(b, 0.65, 2.98, "figs", scale=0.55, rng=rng, z=0.55)
    with D.goods(b, "fleece"):
        D.wool_bale(b, 3.25, 2.98, scale=0.55, rng=rng, z=0.55)
    D.potted_plant(b, 0.6, 3.9, scale=0.35, rng=rng)  # on the ground before the porch, clear of its floor
    D.potted_plant(b, 3.4, 3.9, scale=0.35, rng=rng, name="plant 2")
    if story == "shepherds":  # dyed cloths hung between the columns under the architrave
        for i, (a, c) in enumerate(((0.75, 1.9), (1.9, 3.05))):
            S.swag(b, (a + 0.2, 3.73, up(2.78)), (c - 0.2, 3.73, up(2.78)), 0.4, S.DYES[i * 2], rng, name="swag %d" % (i + 1))
    elif story == "potters":  # pots and plates hung between the columns, a striped awning out from the
        for i, (a, c) in enumerate(((0.75, 1.9), (1.9, 3.05))):  # architrave over the steps, two kraters
            S.hanging_pots(b, a + 0.24, c - 0.24, 3.45, up(2.8), rng, name="hung pots %d" % (i + 1))
        S.awning(b, 0.45, 3.55, 3.68, up(2.78), 0.29, 0.2, rng, pots=6, name="porch awning")  # on the architrave's face
        for i, x in enumerate((0.40, 3.60)):  # in the gap between the end columns and the porch's edges
            S.krater(b, x, 3.45, 0.55, rng, scale=0.55, name="krater %d" % (i + 1))
    elif story == "gardeners":  # the vine up the outer columns, along the architrave and over the porch roof
        for x in (0.75, 3.05):
            S.column_vine(b, x, 3.45, 0.6, up(2.75), 0.15, rng)
            S.vine_stem(b, [(x + 0.12, 3.6, up(2.75)), (x + 0.1, 3.76, up(2.95)), (x + 0.08, 3.8, up(3.2))], rng)
        S.garland(b, 0.4, 3.6, 3.72, up(2.92), rng)
    if story:
        b.mirror = S.STORIES[story]["mirror"]
    for i, at in enumerate((0.6, 1.5, 2.4)):
        D.flower_box(b, 0.3 + at - 0.03, 2.7, up(3.78), 0.51, rng, name="flower box %d" % (i + 1))
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
    b.prism(0.2, 0.2, 3.9 * M, 3.6, 3.5, 1.6 * M, "roof_patina", along_x=True)  # stores: copper-patina tiles (R-018)
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
    b.prism(0.1, 0.1, 2.75 * M, w - 0.2, 1.3, 0.9 * M, "roof_patina", along_x=True)  # stores: copper-patina tiles (R-018)
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


def flat_bold(b, rng, story=None):
    """A trial (owner, 2026-11-28: bolder massing, as the reference's stepped houses): the flat as three volumes
    side by side, each its own height and roof: a one-storey west room under a lean-to, a three-storey middle
    house with a steep gable and deep eaves, a two-storey east wing under a lean-to against it; a wooden
    balcony on the middle house hung from its wall. Exported only with BO_BOLD=1, not in the game."""
    # West room: one storey.
    D.footing(b, 0.25, 0.55, 1.2, 2.8, h=0.3, rng=rng, group="west")
    _room(b, rng, 0.3, 0.6, 1.1, 2.7, 0.3, 2.0, plaster="plaster", doors=[("s", 0.15, 0.75, 1.65)],
          windows=[("w", 0.8)], stage=(0.1, 0.5), group="west")
    D.shed_roof(b, 0.3, 0.6, 1.1, 2.7, 2.3, 2.85, rng, falls="w", overhang=0.3, group="west")
    # Middle house: three storeys, the tallest part.
    D.footing(b, 1.35, 0.35, 1.5, 3.0, h=0.3, rng=rng)
    _room(b, rng, 1.4, 0.4, 1.4, 2.9, 0.3, 2.0, thick=0.42, part="stone", block=(0.55, 0.3),
          doors=[("s", 0.27, 0.85, 1.75)], wreath=True)
    _room(b, rng, 1.4, 0.4, 1.4, 2.9, 2.3, 1.8, plaster="plaster", doors=[("s", 0.32, 0.75, 1.65)],
          windows=[("n", 0.5)], stage=(0.45, 0.6))
    _room(b, rng, 1.4, 0.4, 1.4, 2.9, 4.1, 1.3, plaster="plaster", windows=[("s", 0.48, 0.42, 0.5, 0.45)],
          stage=(0.6, 0.66), ornate=True)
    b.box(t(1.37), t(0.37), t(4.05), t(1.46), t(2.96), t(0.12), "cloth", stage=0.6, plaster=True)  # a red band
    ridge = D.tiled_roof(b, 1.4, 0.4, 1.4, 2.9, 5.4, 0.8, rng, along_x=False, overhang=0.34, antefix=True)
    D.chimney_pot(b, 2.1, 1.2, ridge(1.2), rng, scale=1.1)
    # East wing: two storeys.
    D.footing(b, 2.85, 0.85, 0.9, 2.2, h=0.3, rng=rng, group="east")
    _room(b, rng, 2.85, 0.9, 0.85, 2.1, 0.3, 1.9, plaster="plaster", doors=[("s", 0.05, 0.7, 1.6)],
          windows=[("e", 0.7)], stage=(0.15, 0.5), group="east")
    _room(b, rng, 2.85, 0.9, 0.85, 2.1, 2.2, 1.6, plaster="plaster", windows=[("s", 0.2, 0.4), ("e", 0.6)],
          stage=(0.45, 0.6), group="east")
    D.shed_roof(b, 2.85, 0.9, 0.85, 2.1, 3.8, 4.25, rng, falls="e", overhang=0.22, group="east")
    # The balcony before the middle house's upper door, hung from its wall by iron ties.
    fx0, fx1, fy0, fy1, fz = 1.45, 2.75, 3.3, 3.75, 2.35
    b.box(t(fx0), t(fy0), t(fz - 0.05), t(fx1 - fx0), t(fy1 - fy0), t(0.05), "wood", stage=0.62)
    b.solid("balcony", t(fx0), t(fy0 + 0.0), t(fz - 0.05), t(fx1), t(fy1), t(fz), block="balcony")
    top = fz + 0.8
    for px in (fx0 + 0.04, fx1 - 0.04):
        for py in (fy0 + 0.04, fy1 - 0.04):
            b.cbox(t(px - 0.035), t(py - 0.035), t(fz), t(0.07), t(0.07), t(top - fz + 0.04), t(0.01), "dark_wood", stage=0.63)
            b.solid("balcony post", t(px - 0.035), t(py - 0.035), t(fz), t(px + 0.035), t(py + 0.035), t(top + 0.04), block="balcony")
    for rx, ry, rw, rd in ((fx0, fy1 - 0.07, fx1 - fx0, 0.07), (fx0, fy0, 0.07, fy1 - fy0), (fx1 - 0.07, fy0, 0.07, fy1 - fy0)):
        b.cbox(t(rx), t(ry), t(top - 0.02), t(rw), t(rd), t(0.06), t(0.01), "dark_wood", stage=0.64)
        b.box(t(rx + 0.01), t(ry + 0.01), t(fz), t(rw - 0.02), t(rd - 0.02), t(0.04), "dark_wood", stage=0.63)
    b.solid("balcony rail", t(fx0), t(fy1 - 0.07), t(top - 0.02), t(fx1), t(fy1), t(top + 0.04), block="balcony")
    if b.detail:
        for i in range(9):
            D.baluster(b, fx0 + 0.15 + i * (fx1 - fx0 - 0.3) / 8, fy1 - 0.035, fz + 0.04, top - fz - 0.06, stage=0.63)
        for py in (fy0 + 0.16, fy0 + 0.3):
            for px in (fx0 + 0.035, fx1 - 0.035):
                D.baluster(b, px, py, fz + 0.04, top - fz - 0.06, stage=0.63)
    for tx in (fx0 + 0.04, fx1 - 0.04):
        b.tube_path([(t(tx), t(3.33), t(3.95)), (t(tx), t(fy1 - 0.04), t(top + 0.02))], t(0.012), "iron", sides=6, stage=0.64)
        b.box(t(tx - 0.04), t(3.3), t(3.9), t(0.08), t(0.04), t(0.1), "iron", stage=0.64)
    if b.detail:
        D.potted_plant(b, fx0 + 0.25, fy0 + 0.22, scale=0.45, rng=rng, z=fz, name="balcony plant")
    # Life by the house: the level's goods, water, greenery.
    with D.goods(b, "food"):
        D.sack(b, 0.55, 3.72, scale=0.6, rng=rng)
        D.basket(b, 1.0, 3.72, "olives", scale=0.6, rng=rng)
    with D.goods(b, "fleece"):
        D.wool_bale(b, 3.2, 3.6, scale=0.6, rng=rng)
    D.hydria(b, 3.6, 3.62, scale=0.55, rng=rng)
    D.flower_box(b, 2.85 + 0.2 - 0.03, 3.0, 3.13, 0.46, rng, name="flower box")
    D.ivy(b, 3.7, 1.0, 0.6, 0.3, 2.0, rng, along_x=False, out=1)
    D.grass(b, rng, 26)
    b.scaffold(height=6.2 * M)


if os.environ.get("BO_BOLD"):
    MODELS["common_housing_5_bold"] = (flat_bold, "common_housing", "mieszkanie, próba śmielszej bryły (poza grą)")


def _stories():
    """Every level in each family's version: common_housing_<level>_<story> (boot.gd picks it by the id)."""
    for level, fn in ((1, shelter), (2, hut), (3, cabin), (4, homestead), (5, flat), (6, row_house), (7, city_house)):
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
            level = int(name.split("_")[2]) if name.startswith("common_housing_") else 0
            D.ROOF_STEEP = 1.0 if PROFILE in ("B", "C") and 1 <= level <= 5 else 1.4  # B, C: low roofs up to level 5
            b = A.Builder(name + suffix, w, h, detail=detail)
            fn(b, random.Random(name))  # the same bricks every run
            A.export(b.finish(), out_dir / (name + suffix + ".glb"))
            provenance[name + suffix + ".glb"] = {"building": building, "footprint": [w, h], "lod": 0 if detail else 1,
                                                  "source": "tools/blender/models.py",
                                                  "author": "Bogowie Olimpu (model ze skryptu)",
                                                  "license": "własny (R-017)", "note": note}
        if re.fullmatch(r"common_housing_\d+", name):  # the house of each level fallen (bo_collapse.py)
            A.reset()
            b = A.Builder(name + "_ruin", w, h, detail=True)
            fn(b, random.Random(name))
            b.check_props()
            A.export(C.ruin(b, out_dir, name), out_dir / (name + "_ruin.glb"))
            provenance[name + "_ruin.glb"] = {"building": "common_housing", "ruin": True, "footprint": [w, h], "lod": 0,
                                              "source": "tools/blender/bo_collapse.py",
                                              "author": "Bogowie Olimpu (model ze skryptu)",
                                              "license": "własny (R-017)", "note": "ruina po zawaleniu, " + note}
    if out_dir.resolve() == (A.REPO / "game" / "models").resolve():
        provenance_path.write_text(json.dumps(dict(sorted(provenance.items())), ensure_ascii=False, indent=1) + "\n")


main()
