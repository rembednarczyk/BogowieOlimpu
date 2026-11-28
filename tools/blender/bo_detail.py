"""Detail for the building models (docs/ASSETY.md, R-017): walls laid brick by brick in a running bond with the
mortar between, a plaster coat over them that falls off to the bricks, roofs laid tile by tile (flat pan tiles,
round cover tiles over the joints, the ridge), thatch in layers of bundles, plank doors, shutters, columns, jars.

Every element carries its own share of the building phase (R): the walls rise course by course, the roof row by
row from the eaves, so the game shows the house going up. With b.detail False the same calls give the far view's
simple shapes (LOD1). Sizes in real metres (bo_asset.M converts), positions in metres from the plot's north-west
corner, y growing south.
"""
import math
import random

import bo_asset as A
from bo_asset import M

BRICK_L = 0.40   # a mudbrick: 40 x 40 x 10 cm (Greek houses, e.g. Olynthus)
BRICK_H = 0.10
JOINT = 0.014    # the mortar joint


def t(v):
    return v * M


def _cut(span, holes):
    """[a, b] minus the intervals in holes: the pieces left."""
    pieces = [span]
    for h0, h1 in holes:
        nxt = []
        for a, b in pieces:
            if h1 <= a or h0 >= b:
                nxt.append((a, b))
                continue
            if h0 > a:
                nxt.append((a, h0))
            if h1 < b:
                nxt.append((h1, b))
        pieces = nxt
    return pieces


def _rects(length, height, openings):
    """The wall's face [0, length] x [0, height] minus its openings, as rectangles (along, up)."""
    rects = [(0.0, length, 0.0, height)]
    for o in openings:
        o0, o1, z0, z1 = o["at"], o["at"] + o["w"], o["z"], o["z"] + o["h"]
        nxt = []
        for a, b, c, d in rects:
            if o1 <= a or o0 >= b or z1 <= c or z0 >= d:
                nxt.append((a, b, c, d))
                continue
            if o0 > a:
                nxt.append((a, o0, c, d))
            if o1 < b:
                nxt.append((o1, b, c, d))
            lo, hi = max(a, o0), min(b, o1)
            if z0 > c:
                nxt.append((lo, hi, c, z0))
            if z1 < d:
                nxt.append((lo, hi, z1, d))
        rects = nxt
    return rects


class Wall:
    """A straight wall: its outer face starts at (x, y) and runs `length` along +x (axis "x") or +y (axis "y");
    `thick` goes inwards (+y for "x", +x for "y") unless `outward` is -1."""

    def __init__(self, x, y, length, axis, z, height, thick=0.40, outward=1):
        self.x, self.y, self.length, self.axis = x, y, length, axis
        self.z, self.height, self.thick, self.outward = z, height, thick, outward
        self.openings = []
        self.ornate = False  # stone surrounds and a small pediment over the windows (the higher houses)

    def door(self, at, w=0.85, h=1.85):
        self.openings.append({"at": at, "w": w, "z": 0.0, "h": h, "kind": "door"})
        return self

    def window(self, at, w=0.45, z=1.05, h=0.50):
        self.openings.append({"at": at, "w": w, "z": z, "h": h, "kind": "window"})
        return self

    def frame(self, along, depth, up):
        """A point of the wall in plot metres: `along` the wall, `depth` into it (0 the outer face), `up`."""
        d = depth * self.outward
        if self.axis == "x":
            return self.x + along, self.y + d, self.z + up
        return self.x + d, self.y + along, self.z + up

    def axes(self, half_along, half_depth, half_up):
        if self.axis == "x":
            return (t(half_along), 0, 0), (0, t(half_depth), 0), (0, 0, t(half_up))
        return (0, t(half_along), 0), (t(half_depth), 0, 0), (0, 0, t(half_up))

    def put_c(self, b, along, depth, up, size_along, size_depth, size_up, chamfer, part, **kw):
        """As put, the edges chamfered (a brick or a block that catches the light)."""
        p0 = self.frame(along, depth, up)
        p1 = self.frame(along + size_along, depth + size_depth, up + size_up)
        x0, x1 = sorted((p0[0], p1[0]))
        y0, y1 = sorted((p0[1], p1[1]))
        return b.cbox(t(x0), t(y0), t(p0[2]), t(x1 - x0), t(y1 - y0), t(p1[2] - p0[2]), t(chamfer), part, **kw)

    def put(self, b, along, depth, up, size_along, size_depth, size_up, part, **kw):
        cx, cy, cz = self.frame(along + size_along / 2, depth + size_depth / 2, up + size_up / 2)
        ax, ay, az = self.axes(size_along / 2, size_depth / 2, size_up / 2)
        return b.obox((t(cx), t(cy), t(cz)), ax, ay, az, part, **kw)


def brick_wall(b, wall, rng, part="mudbrick", stage=(0.1, 0.6), plaster=None, block=None, both=False, group="main"):
    """Lays the wall course by course; `plaster` (a palette part) coats the outer face; `block` (length, height)
    for stone blocks instead of mudbricks."""
    s0, s1 = stage
    bl, bh = block or (BRICK_L, BRICK_H)
    p0 = wall.frame(0, -0.03, 0)
    p1 = wall.frame(wall.length, wall.thick, wall.height + 0.05)
    b.solid("wall", t(p0[0]), t(p0[1]), t(p0[2]), t(p1[0]), t(p1[1]), t(p1[2]), block=group)
    if not b.detail:
        for a, c, z0, z1 in _rects(wall.length, wall.height, wall.openings):
            wall.put(b, a, 0, z0, c - a, wall.thick, z1 - z0, part, stage=s0)
        if plaster:
            for a, c, z0, z1 in _rects(wall.length, wall.height, wall.openings):
                wall.put(b, a, -0.02, z0, c - a, 0.02, z1 - z0, plaster, stage=s1, plaster=True)
        _openings(b, wall, rng, s0, s1)
        return
    courses = max(1, int(round(wall.height / bh)))
    real_h = wall.height / courses
    for k in range(courses):
        z0, z1 = k * real_h, (k + 1) * real_h
        holes = [(o["at"], o["at"] + o["w"]) for o in wall.openings if o["z"] < z1 - 1e-6 and o["z"] + o["h"] > z0 + 1e-6]
        offset = -bl / 2 if k % 2 else 0.0
        s = offset
        while s < wall.length - 1e-6:
            for a, c in _cut((max(s, 0.0), min(s + bl, wall.length)), holes):
                if c - a < 0.06:
                    continue
                jitter = rng.uniform(-0.006, 0.006)
                share = s0 + (s1 - s0) * (k + (a / max(wall.length, 0.01)) * 0.9) / courses
                if plaster:  # under the plaster: plain bricks (they show only where it falls)
                    wall.put(b, a + JOINT / 2, 0.004 + jitter, z0 + JOINT / 2, c - a - JOINT, wall.thick - 0.008,
                             real_h - JOINT, part, stage=share, tint=rng.uniform(0.22, 0.78))
                else:  # bare: each brick or block with its edges worn
                    wall.put_c(b, a + JOINT / 2, 0.004 + jitter, z0 + JOINT / 2, c - a - JOINT, wall.thick - 0.008,
                               real_h - JOINT, 0.03 if block else 0.012, part, stage=share, tint=rng.uniform(0.22, 0.78))
            s += bl
        for a, c in _cut((0.0, wall.length), holes):  # the mortar inside the joints, rising with the course
            share = s0 + (s1 - s0) * k / courses
            wall.put(b, a, 0.012, z0, c - a, wall.thick - 0.024, real_h, "mortar", stage=share)
    if plaster:
        for a, c, z0, z1 in _rects(wall.length, wall.height, wall.openings):
            wall.put(b, a, -0.018, z0, c - a, 0.018, z1 - z0, plaster, stage=s1, plaster=True)
            if both:  # a free-standing wall: plastered on both faces, a coping on top
                wall.put(b, a, wall.thick, z0, c - a, 0.018, z1 - z0, plaster, stage=s1, plaster=True)
        if both:
            wall.put_c(b, -0.02, -0.04, wall.height, wall.length + 0.04, wall.thick + 0.08, 0.06, 0.015, "stone", stage=s1)
    _openings(b, wall, rng, s0, s1)


def _openings(b, wall, rng, s0, s1):
    for o in wall.openings:
        mid = s0 + (s1 - s0) * min(1.0, (o["z"] + o["h"]) / max(wall.height, 0.01))
        wall.put(b, o["at"] - 0.12, -0.02, o["z"] + o["h"], o["w"] + 0.24, wall.thick + 0.04, 0.12, "dark_wood",
                 stage=mid)  # the lintel
        if o["kind"] == "door":
            wall.put(b, o["at"] - 0.05, -0.08, -0.04, o["w"] + 0.10, wall.thick + 0.12, 0.06, "stone", stage=s0)
            boards = max(3, int(o["w"] / 0.14)) if b.detail else 1
            colour = "blue_paint" if rng.random() < 0.5 else "dark_wood"
            for i in range(boards):
                bw = o["w"] / boards
                wall.put(b, o["at"] + i * bw + 0.005, wall.thick * 0.35, 0.02, bw - 0.01, 0.045, o["h"] - 0.04,
                         colour, stage=s1, tint=rng.uniform(0.3, 0.7))
            if b.detail:  # two ledges across the boards, iron straps and a ring for a handle
                for zz in (0.3, o["h"] - 0.4):
                    wall.put(b, o["at"] + 0.04, wall.thick * 0.35 - 0.03, zz, o["w"] - 0.08, 0.03, 0.1, "dark_wood", stage=s1)
                    wall.put(b, o["at"] + 0.02, wall.thick * 0.35 - 0.045, zz + 0.03, o["w"] * 0.6, 0.015, 0.035, "iron",
                             stage=s1)
                wall.put(b, o["at"] + o["w"] * 0.78, wall.thick * 0.35 - 0.05, o["h"] * 0.5, 0.05, 0.02, 0.05, "iron",
                         stage=s1)
                if getattr(wall, "wreath", False):
                    wreath(b, wall, o["at"] + o["w"] / 2, o["h"] + 0.24, radius=0.085, rng=rng, stage=s1)
        else:
            wall.put(b, o["at"] - 0.06, -0.05, o["z"] - 0.06, o["w"] + 0.12, wall.thick + 0.08, 0.06, "stone",
                     stage=mid)  # the sill
            if b.detail and wall.ornate:  # a stone surround and a pediment on two consoles
                for a, w_ in ((o["at"] - 0.08, 0.08), (o["at"] + o["w"], 0.08)):
                    wall.put_c(b, a, -0.04, o["z"] - 0.06, w_, 0.04, o["h"] + 0.12, 0.01, "stone", stage=s1)
                wall.put_c(b, o["at"] - 0.14, -0.07, o["z"] + o["h"] + 0.12, o["w"] + 0.28, 0.07, 0.06, 0.012, "stone", stage=s1)
                for a in (o["at"] - 0.12, o["at"] + o["w"] + 0.06):
                    wall.put_c(b, a, -0.06, o["z"] + o["h"] + 0.02, 0.06, 0.06, 0.10, 0.01, "stone", stage=s1)
                apex = wall.frame(o["at"] + o["w"] / 2, -0.05, o["z"] + o["h"] + 0.36)
                left = wall.frame(o["at"] - 0.14, -0.05, o["z"] + o["h"] + 0.18)
                right = wall.frame(o["at"] + o["w"] + 0.14, -0.05, o["z"] + o["h"] + 0.18)
                tri = [tuple(t(v) for v in left), tuple(t(v) for v in right), tuple(t(v) for v in apex)]
                b.poly(tri, "stone", stage=s1)
                b.poly(list(reversed(tri)), "stone", stage=s1)
            shutter = "blue_paint" if rng.random() < 0.55 else "ochre"  # teal or ochre, as painted by hand
            if b.detail:  # two shutters, open against the wall
                for side, at in ((-1, o["at"] - o["w"] / 2), (1, o["at"] + o["w"])):
                    for i in range(3):
                        wall.put(b, at + i * o["w"] / 6, -0.05, o["z"] + 0.02, o["w"] / 6 - 0.01, 0.03, o["h"] - 0.04,
                                 shutter, stage=s1, tint=rng.uniform(0.35, 0.65))
            wall.put(b, o["at"], wall.thick * 0.5, o["z"], o["w"], 0.02, o["h"], "dark_wood", stage=s1)  # dark inside
            if b.detail:  # the frame and its cross bar
                for a, w_ in ((o["at"], 0.05), (o["at"] + o["w"] - 0.05, 0.05)):
                    wall.put(b, a, wall.thick * 0.3, o["z"], w_, 0.06, o["h"], "wood", stage=s1)
                wall.put(b, o["at"], wall.thick * 0.3, o["z"] + o["h"] - 0.05, o["w"], 0.06, 0.05, "wood", stage=s1)
                wall.put(b, o["at"], wall.thick * 0.3, o["z"] + o["h"] / 2 - 0.02, o["w"], 0.05, 0.04, "wood", stage=s1)
                wall.put(b, o["at"] + o["w"] / 2 - 0.02, wall.thick * 0.3, o["z"], 0.04, 0.05, o["h"], "wood", stage=s1)


def footing(b, x, y, w, d, h=0.35, stage=0.0, rng=None, group="main"):
    """A stone socle under the walls: rough blocks in the far view's place."""
    b.solid("footing", t(x), t(y), 0, t(x + w), t(y + d), t(h), block=group)
    if not b.detail or rng is None:
        b.box(t(x), t(y), 0, t(w), t(d), t(h), "stone", stage=stage)
        return
    for side in range(4):
        length = w if side % 2 == 0 else d
        s = 0.0
        while s < length - 1e-6:
            bl = min(rng.uniform(0.45, 0.8), length - s)
            if side == 0:
                bx, by, sx, sy = x + s, y, bl, 0.42
            elif side == 2:
                bx, by, sx, sy = x + s, y + d - 0.42, bl, 0.42
            elif side == 1:
                bx, by, sx, sy = x + w - 0.42, y + s, 0.42, bl
            else:
                bx, by, sx, sy = x, y + s, 0.42, bl
            b.box(t(bx + 0.01), t(by + 0.01), 0, t(sx - 0.02), t(sy - 0.02), t(h * rng.uniform(0.92, 1.0)), "stone",
                  stage=stage)
            s += bl
    b.box(t(x + 0.3), t(y + 0.3), 0, t(w - 0.6), t(d - 0.6), t(h * 0.6), "mortar", stage=stage)  # the floor's bed


def shed_roof(b, x, y, w, d, z_low, z_high, rng, falls="s", overhang=0.2, stage=(0.65, 0.95), gable="plaster",
              antefix=False, group="main", gap=0.045):
    """A lean-to roof of one slope against a higher wall: its high edge stops at the wall's face (`gap` off it,
    flashed with a board), it falls towards `falls` ("n", "s", "e", "w"). Over the rectangle x..x+w, y..y+d
    (the high edge on the side opposite `falls`)."""
    rise = z_high - z_low
    if falls == "s":
        tiled_roof(b, x, y + gap - d, w, 2 * d - gap, z_low, rise, rng, along_x=True, overhang=overhang, stage=stage,
                   gable=gable, antefix=antefix, sides=(1,), group=group, sag=0.0)
    elif falls == "n":
        tiled_roof(b, x, y, w, 2 * d - gap, z_low, rise, rng, along_x=True, overhang=overhang, stage=stage, gable=gable,
                   antefix=antefix, sides=(-1,), group=group, sag=0.0)
    elif falls == "e":
        tiled_roof(b, x + gap - w, y, 2 * w - gap, d, z_low, rise, rng, along_x=False, overhang=overhang, stage=stage,
                   gable=gable, antefix=antefix, sides=(1,), group=group, sag=0.0)
    else:
        tiled_roof(b, x, y, 2 * w - gap, d, z_low, rise, rng, along_x=False, overhang=overhang, stage=stage, gable=gable,
                   antefix=antefix, sides=(-1,), group=group, sag=0.0)


ROOF_STEEP = 1.4  # the town's style (R-018): gable roofs a good deal steeper than the plans' rise


def tiled_roof(b, x, y, w, d, z, rise, rng, along_x=True, overhang=0.35, stage=(0.65, 0.95), gable="plaster",
               antefix=False, sides=(-1, 1), group="main", sag=0.08, curve=None, steep=None):
    """A gable roof of terracotta: pan tiles in rows from the eaves, cover tiles over their joints, a ridge;
    the gables walled in. `sides` (-1, 1) both slopes, one of them for a lean-to (then the high edge, where the
    ridge would be, gets a flashing board); `sag` lets the ridge dip a little in its middle (a roof of some age,
    drawn by hand); `curve` bows each slope down between the eaves and the ridge (owner, 2026-11-28: arched
    roofs): the eaves lie flatter, the slope steepens to the ridge, as a roof bent by its own weight over the
    years. `steep` scales the rise (ROOF_STEEP for a gable roof, 1 for a lean-to, whose rise is fixed by its
    walls); `curve` defaults to 7.5% of the slope's length. Returns the ridge's height at a point along it
    (plot metres), for what stands on the ridge (a chimney pot)."""
    s0, s1 = stage
    shed = len(sides) == 1
    rise *= (1.0 if shed else ROOF_STEEP) if steep is None else steep
    x0, y0, x1, y1 = x - overhang, y - overhang, x + w + overhang, y + d + overhang
    run_len = (x1 - x0) if along_x else (y1 - y0)
    span = (y1 - y0) / 2 if along_x else (x1 - x0) / 2
    slope = math.hypot(span, rise)
    if curve is None:
        curve = 0.075 * slope

    def rp(side, along, f):  # a point on a slope; the ridge dips by `sag` in the middle of its run
        p = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, along, f)
        bow = sag * math.sin(math.pi * min(max(along / run_len, 0.0), 1.0)) * f
        return (p[0], p[1], p[2] - bow - curve * math.sin(math.pi * min(max(f, 0.0), 1.0)))

    # Points on the slopes for the check that no roof goes into the walls of another part (bo_asset.check_props).
    pts = []
    for side in sides:
        n = _roof_normal(span, rise, along_x, side)
        for ia in range(11):
            for jf in range(8):
                p = rp(side, run_len * ia / 10, jf / 7)
                pts.append(tuple(t(p[k] + n[k] * 0.03) for k in range(3)))
    b.roof_points(group, pts)

    def slab(side, part, lift, thickness, share):  # one slope as a board: the far view, or the boards under the tiles
        lo_a = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, run_len / 2, 0.0)
        hi_a = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, run_len / 2, 1.0)
        n = _roof_normal(span, rise, along_x, side)
        centre = tuple(t((lo_a[k] + hi_a[k]) / 2 + n[k] * lift) for k in range(3))
        half_run = (t(run_len / 2), 0, 0) if along_x else (0, t(run_len / 2), 0)
        half_up = tuple(t((hi_a[k] - lo_a[k]) / 2) for k in range(3))
        b.obox(centre, half_run, half_up, tuple(t(n[k] * thickness / 2) for k in range(3)), part, stage=share)

    def ridge(at):
        return rp(sides[-1], min(max(at - (x0 if along_x else y0), 0.0), run_len), 1.0)[2]

    if not b.detail:
        for side in sides:
            slab(side, "roof_tile", 0.05, 0.1, s0)
        _gables(b, x, y, w, d, z, rise, along_x, gable, s0, sides)
        return ridge
    pan_w, exposed, tile_len, thick = 0.46, 0.34, 0.44, 0.022
    rows = max(2, int(math.ceil(slope / exposed)))
    cols = max(2, int(round(run_len / pan_w)))
    pan_w = run_len / cols
    for side in sides:  # the bed under the tiles, row by row along the bow, deep enough not to show
        for r in range(rows):
            f0, f1 = r * exposed / slope, min(1.0, (r + 1) * exposed / slope + 0.02)
            n = _roof_normal(span, rise, along_x, side)
            lo_a, hi_a = rp(side, run_len / 2, f0), rp(side, run_len / 2, f1)
            centre = tuple(t((lo_a[k] + hi_a[k]) / 2 - n[k] * 0.14) for k in range(3))
            half_run = (t(run_len / 2 - 0.02), 0, 0) if along_x else (0, t(run_len / 2 - 0.02), 0)
            half_up = tuple(t((hi_a[k] - lo_a[k]) / 2) for k in range(3))
            b.obox(centre, half_run, half_up, tuple(t(n[k] * 0.0125) for k in range(3)), "roof_tile",
                   stage=s0, tint=0.1)
    for side in sides:  # the slopes
        for r in range(rows):
            f0 = r * exposed / slope
            f1 = min(1.0, (r * exposed + tile_len) / slope)
            share = s0 + (s1 - s0) * r / rows
            for c in range(cols):
                lift = thick * 1.2 if r else 0.0
                a = c * pan_w + 0.012
                bb = (c + 1) * pan_w - 0.012
                p_low = rp(side, (a + bb) / 2, f0)
                p_high = rp(side, (a + bb) / 2, f1)
                normal = _roof_normal(span, rise, along_x, side)
                # A Laconian pan tile: a shallow trough, its hollow up, each row lying on the one below.
                rad = (bb - a) / 2
                low = tuple(p_low[i] + normal[i] * (lift + rad * 0.55) for i in range(3))
                high = tuple(p_high[i] + normal[i] * (rad * 0.55) for i in range(3))
                b.shell(tuple(t(v) for v in low), tuple(t(v) for v in high), t(rad), t(0.016), tuple(normal), "roof_tile",
                        sides=10, stage=share, tint=rng.uniform(0.25, 0.75), hollow_up=True)
            for c in range(cols + 1):  # cover tiles over the joints
                along = min(max(c * pan_w, 0.06), run_len - 0.06)
                p_low = rp(side, along, f0)
                p_high = rp(side, along, f1)
                normal = _roof_normal(span, rise, along_x, side)
                lift = thick * 1.2 + 0.075 * 0.55
                start = tuple(t(p_low[i] + normal[i] * lift) for i in range(3))
                end = tuple(t(p_high[i] + normal[i] * lift) for i in range(3))
                b.shell(start, end, t(0.075), t(0.014), tuple(normal), "roof_tile", sides=10, stage=share,
                        tint=rng.uniform(0.2, 0.6))
            # Mortar beds close the troughs' open ends: at the eaves a little behind the rims, under the ridge
            # (or the flashing of a lean-to) at the top row, so no hollow and no sky shows through the tiles.
            beds = ([0.035] if r == 0 else []) + ([0.975] if r == rows - 1 else [])
            for fb in beds:
                normal = _roof_normal(span, rise, along_x, side)
                for c in range(cols):
                    along = (c + 0.5) * pan_w
                    p = rp(side, along, fb)
                    rad = pan_w / 2 + 0.012  # overlapping the next one: the joints under the cover tiles too
                    centre = tuple(t(p[i] + normal[i] * 0.03) for i in range(3))
                    half_run = (t(rad), 0, 0) if along_x else (0, t(rad), 0)
                    dirv = tuple(p[i] - rp(side, along, fb - 0.035)[i] for i in range(3))
                    ln = math.sqrt(sum(v * v for v in dirv)) or 1.0
                    half_up = tuple(t(dirv[i] / ln * 0.02) for i in range(3))
                    b.obox(centre, half_run, half_up, tuple(t(normal[i] * 0.13) for i in range(3)), "roof_tile",
                           stage=share, tint=0.15)
            if antefix and r == 0:  # an antefix closes each cover tile at the eaves: a palmette of seven leaves
                for c in range(cols + 1):
                    along = min(max(c * pan_w, 0.06), run_len - 0.06)
                    p = rp(side, along, 0.0)
                    palmette(b, p, along_x, side, 0.13, stage=share)
    # The verges: the roof's ends closed by a barge board along each gable slope and a row of cover tiles on
    # its edge (no open edge of the tiles or the boards shows from the side).
    normal_s = {side: _roof_normal(span, rise, along_x, side) for side in sides}
    for along in (0.0, run_len):
        for side in sides:
            n = normal_s[side]
            lo = rp(side, along, 0.0)
            hi = rp(side, along, 1.0)
            out = -1 if along == 0.0 else 1
            half_t = (t(0.025 * out), 0, 0) if along_x else (0, t(0.025 * out), 0)
            half_n = tuple(t(n[i] * 0.09) for i in range(3))
            segs = 8  # the mortared verge follows the slope's bow in short pieces (a straight one would stand
            for k in range(segs):  # out of the tiles in the middle)
                lo = rp(side, along, k / segs)
                hi = rp(side, along, (k + 1) / segs + 0.01)
                board_c = tuple(t((lo[i] + hi[i]) / 2 - n[i] * 0.04) for i in range(3))
                half_up = tuple(t((hi[i] - lo[i]) / 2) for i in range(3))
                if along_x:
                    board_c = (board_c[0] + t(0.025 * out), board_c[1], board_c[2])
                else:
                    board_c = (board_c[0], board_c[1] + t(0.025 * out), board_c[2])
                b.obox(board_c, half_t, half_up, half_n, "mortar", stage=s1, tint=0.45)
            edge = 0.05 if along == 0.0 else run_len - 0.05
            rows_v = max(2, rows)
            for r in range(rows_v):
                f0, f1 = r / rows_v, min(1.0, (r + 1.15) / rows_v)
                pa = rp(side, edge, f0)
                pb = rp(side, edge, f1)
                lift = thick * 1.2 + 0.085 * 0.55
                b.shell(tuple(t(pa[i] + n[i] * lift) for i in range(3)), tuple(t(pb[i] + n[i] * lift) for i in range(3)),
                        t(0.085), t(0.014), tuple(n), "roof_tile", sides=10, stage=s0 + (s1 - s0) * r / rows_v,
                        tint=rng.uniform(0.2, 0.5))
    ridge_n = max(1, int(run_len / 0.5))
    if shed:  # the high edge against the wall: a flashing board over the top row
        side = sides[0]
        n = _roof_normal(span, rise, along_x, side)
        for i in range(ridge_n):
            a0, a1 = run_len * i / ridge_n, run_len * (i + 1) / ridge_n
            p0, p1 = rp(side, a0, 1.0), rp(side, a1, 1.0)
            centre = tuple(t((p0[k] + p1[k]) / 2 + n[k] * 0.06) for k in range(3))
            half_run = tuple(t((p1[k] - p0[k]) / 2) for k in range(3))
            half_thick = (0, t(0.03), 0) if along_x else (t(0.03), 0, 0)
            b.obox(centre, half_run, (0, 0, t(0.07)), half_thick, "roof_tile", stage=s1, tint=0.15)
        ridge_n = 0
    for i in range(ridge_n):  # the ridge
        a0 = run_len * i / ridge_n
        a1 = run_len * (i + 1) / ridge_n + 0.03
        p0 = rp(1, a0, 1.0)
        p1 = rp(1, min(a1, run_len), 1.0)
        b.shell(tuple(t(v) for v in (p0[0], p0[1], p0[2] - 0.02)), tuple(t(v) for v in (p1[0], p1[1], p1[2] - 0.02)),
                t(0.12), t(0.016), (0, 0, 1), "roof_tile", sides=12, stage=s1, tint=rng.uniform(0.3, 0.5))
    for side in sides:  # the rafters' ends under the eaves
        for c in range(cols + 1):
            along = c * pan_w
            p = rp(side, min(max(along, 0.05), run_len - 0.05), 0.04)
            half_w = (t(0.04), 0, 0) if along_x else (0, t(0.04), 0)
            n = _roof_normal(span, rise, along_x, side)
            b.obox((t(p[0] - n[0] * 0.06), t(p[1] - n[1] * 0.06), t(p[2] - 0.07)), half_w,
                   (0, t(0.08), 0) if along_x else (t(0.08), 0, 0), (0, 0, t(0.04)), "dark_wood", stage=s0)
    # The gables walled up to the underside of the bowed slopes, so no gap shows under the tiles and no wall
    # pokes through them.
    span_full = span
    f_wall = overhang / span_full
    for along in (overhang, run_len - overhang):
        outline = []
        for side in sorted(sides):
            fs = [f_wall + (1.0 - f_wall) * k / 10 for k in range(11)]
            pts = []
            for f in fs:
                p = rp(side, along, f)
                pts.append((p[0], p[1], max(z, p[2] - 0.15)))
            if side > 0:
                pts.reverse()
            outline.append(pts)
        if shed:
            pts = outline[0]
            foot_hi = (pts[-1][0], pts[-1][1], z) if sides[0] < 0 else (pts[0][0], pts[0][1], z)
            if sides[0] < 0:
                poly = [(pts[0][0], pts[0][1], z)] + pts + [foot_hi]
            else:
                poly = [foot_hi] + pts + [(pts[-1][0], pts[-1][1], z)]
        else:
            a, c = outline
            poly = [(a[0][0], a[0][1], z)] + a + c[1:] + [(c[-1][0], c[-1][1], z)]
        poly = [q for i, q in enumerate(poly) if i == 0 or (abs(q[0] - poly[i - 1][0]) + abs(q[1] - poly[i - 1][1]) + abs(q[2] - poly[i - 1][2])) > 1e-6]
        # A fan of triangles from the middle of the gable's foot: the bowed outline is concave, one n-gon of it
        # would not show (2026-11-28, a hole in the gables); both windings kept as built (seen from both sides).
        hub = tuple((poly[0][k] + poly[-1][k]) / 2 for k in range(3))
        for qa, qb in zip(poly, poly[1:]):
            tri = [tuple(t(v) for v in q) for q in (hub, qa, qb)]
            if sum(abs(tri[1][k] - tri[0][k]) + abs(tri[2][k] - tri[0][k]) for k in range(3)) < 1e-6:
                continue
            faces = b.poly(tri, gable, stage=s0, plaster=True) + b.poly(list(reversed(tri)), gable, stage=s0, plaster=True)
            b.two_sided.update(faces)
    return ridge


def _roof_point(x0, y0, x1, y1, z, rise, along_x, side, along, f):
    """A point on a slope: `along` the ridge's direction from the roof's start, `f` 0 at the eaves, 1 at the ridge;
    side -1 the north (or west) slope, 1 the south (or east)."""
    if along_x:
        mid = (y0 + y1) / 2
        eave = y0 if side < 0 else y1
        return (x0 + along, eave + (mid - eave) * f, z + rise * f)
    mid = (x0 + x1) / 2
    eave = x0 if side < 0 else x1
    return (eave + (mid - eave) * f, y0 + along, z + rise * f)


def _roof_normal(span, rise, along_x, side):
    length = math.hypot(span, rise)
    out = side * rise / length
    up = span / length
    return (0.0, out, up) if along_x else (out, 0.0, up)


def _gables(b, x, y, w, d, z, rise, along_x, part, stage, sides=(-1, 1)):
    """The triangles under a gable roof's ends (a half of each for a lean-to), faced both ways (the material
    culls back faces)."""
    def tri(pts):
        b.poly(pts, part, stage=stage, plaster=True)
        b.poly(list(reversed(pts)), part, stage=stage, plaster=True)
    if along_x:
        mid, top = y + d / 2, z + rise * d / (d + 0.7)
        for gx in (x, x + w):
            if len(sides) == 2:
                tri([(t(gx), t(y), t(z)), (t(gx), t(y + d), t(z)), (t(gx), t(mid), t(top))])
            else:
                eave = y if sides[0] < 0 else y + d
                tri([(t(gx), t(eave), t(z)), (t(gx), t(mid), t(z)), (t(gx), t(mid), t(top))])
    else:
        mid, top = x + w / 2, z + rise * w / (w + 0.7)
        for gy in (y, y + d):
            if len(sides) == 2:
                tri([(t(x), t(gy), t(z)), (t(x + w), t(gy), t(z)), (t(mid), t(gy), t(top))])
            else:
                eave = x if sides[0] < 0 else x + w
                tri([(t(eave), t(gy), t(z)), (t(mid), t(gy), t(z)), (t(mid), t(gy), t(top))])


def thatch_roof(b, x, y, w, d, z, rise, rng, hip=0.3, along_x=True, overhang=0.35, stage=(0.65, 0.95), layers=6):
    """Thatch: layers of bundles, each a little smaller and higher, a ragged edge of straw at the eaves."""
    s0, s1 = stage
    x0, y0, x1, y1 = x - overhang, y - overhang, x + w + overhang, y + d + overhang
    if not b.detail:
        b.prism(t(x0), t(y0), t(z), t(x1 - x0), t(y1 - y0), t(rise), "thatch", along_x=along_x, hip=hip, stage=s0)
        return
    for k in range(layers):
        f = k / layers
        inset = f * min(x1 - x0, y1 - y0) * 0.42
        lift = f * rise * 0.82
        b.prism(t(x0 + inset), t(y0 + inset), t(z + lift), t(x1 - x0 - 2 * inset), t(y1 - y0 - 2 * inset),
                t(rise - lift + 0.05), "thatch", along_x=along_x, hip=hip, stage=s0 + (s1 - s0) * f,
                )
    # The eaves: straw cut ragged, blade by blade, in two rows (the outer longer), each blade along the slope.
    perim = [((x0, y0), (x1, y0), (0, -1)), ((x1, y0), (x1, y1), (1, 0)), ((x1, y1), (x0, y1), (0, 1)),
             ((x0, y1), (x0, y0), (-1, 0))]
    for (ax, ay), (bx, by), (ox, oy) in perim:
        length = math.hypot(bx - ax, by - ay)
        n = int(length / 0.022)
        for row, (reach, drop) in enumerate(((0.06, 0.10), (0.13, 0.20))):
            for i in range(n):
                f = (i + rng.uniform(0.0, 1.0)) / n
                px, py = ax + (bx - ax) * f, ay + (by - ay) * f
                pz = z + 0.03 + row * 0.04
                ln = rng.uniform(0.6, 1.0)
                tip = (px + ox * reach * ln + rng.uniform(-0.02, 0.02), py + oy * reach * ln + rng.uniform(-0.02, 0.02),
                       pz - drop * ln)
                b.strand((t(px - ox * 0.06), t(py - oy * 0.06), t(pz + 0.02)), tuple(t(v) for v in tip),
                         t(rng.uniform(0.012, 0.022)), "thatch", stage=s0 + (s1 - s0) * 0.1, tint=rng.uniform(0.1, 0.9))
    # The ridge: a roll of straw bound with rope every half metre.
    if along_x:
        r0 = (x0 + hip * (x1 - x0), (y0 + y1) / 2)
        r1 = (x1 - hip * (x1 - x0), (y0 + y1) / 2)
    else:
        r0 = ((x0 + x1) / 2, y0 + hip * (y1 - y0))
        r1 = ((x0 + x1) / 2, y1 - hip * (y1 - y0))
    zr = z + rise - 0.02
    b.tube_path([(t(r0[0]), t(r0[1]), t(zr)), (t(r1[0]), t(r1[1]), t(zr))], t(0.13), "thatch", sides=12, stage=s1, tint=0.3)
    ridge_len = math.hypot(r1[0] - r0[0], r1[1] - r0[1])
    bands = max(2, int(ridge_len / 0.5))
    for k in range(bands + 1):
        f = k / bands
        cx, cy = r0[0] + (r1[0] - r0[0]) * f, r0[1] + (r1[1] - r0[1]) * f
        dx, dy = (r1[0] - r0[0]) / max(ridge_len, 1e-6), (r1[1] - r0[1]) / max(ridge_len, 1e-6)
        ring = []
        for i in range(13):
            a = 2 * math.pi * i / 12
            ring.append((t(cx - dy * 0.145 * math.cos(a)), t(cy + dx * 0.145 * math.cos(a)), t(zr + 0.145 * math.sin(a))))
        b.tube_path(ring, t(0.012), "rope", sides=5, stage=s1, caps=False)


class goods:
    """Props of a good the house keeps (food, fleece, olive_oil, wine): they show only while it holds it."""

    def __init__(self, b, good):
        self.b, self.good = b, good

    def __enter__(self):
        self.b.good = self.good

    def __exit__(self, *exc):
        self.b.good = None


def _painted_band(b, x, y, z, h, r, ctrl, f0, f1, part, stage, steps=6):
    """A painted zone on a jar (the black glaze of a frieze, a red band): a thin skin over the body's profile
    between the heights f0 and f1 (shares of h), standing 6 mm off it."""
    def radius(f):
        for (a0, c0), (a1, c1) in zip(ctrl, ctrl[1:]):
            if c0 <= f <= c1 and c1 > c0:
                return r * (a0 + (a1 - a0) * (f - c0) / (c1 - c0))
        return r * ctrl[-1][0]
    prof = [(t(radius(f0 + (f1 - f0) * i / steps) + 0.006), t(h * (f0 + (f1 - f0) * i / steps))) for i in range(steps + 1)]
    prof = [(prof[0][0] * 0.97, prof[0][1])] + prof + [(prof[-1][0] * 0.97, prof[-1][1])]
    b.lathe(t(x), t(y), t(z), prof, part, sides=40, stage=stage, tint=0.5)


def pithos(b, x, y, scale=1.0, stage=0.9, rng=None, name="pithos", z=0.0, lid=False):
    """A storage jar: tall and round-bellied on a narrow foot, a thick rolled lip, two lugs on the shoulder,
    three bands round the belly; the oil dark in its mouth or a lid on it. A smooth body of revolution."""
    h, r = 1.05 * scale, 0.30 * scale
    b.prop(name, t(x - r * 1.05), t(y - r * 1.05), t(z), t(x + r * 1.05), t(y + r * 1.05), t(z + h + 0.04))
    if not b.detail:
        b.column(t(x), t(y), t(z), t(r * 0.85), t(h * 0.95), "roof_tile", sides=8, stage=stage)
        return
    ctrl = [(0.36, 0.0), (0.42, 0.04), (0.60, 0.12), (0.82, 0.24), (0.97, 0.38), (1.0, 0.48), (0.96, 0.58),
            (0.86, 0.68), (0.70, 0.77), (0.54, 0.84), (0.45, 0.89), (0.44, 0.93), (0.50, 0.95), (0.57, 0.97),
            (0.58, 0.995), (0.52, 1.0), (0.44, 0.99)]
    b.lathe(t(x), t(y), t(z), [(t(r * a), t(h * c)) for a, c in ctrl], "roof_tile", sides=48, stage=stage,
            tint=rng.uniform(0.35, 0.65) if rng else 0.5, top="dark_wood" if lid else "oil", smooth=4)
    for zz, k in ((0.26, 0.90), (0.48, 1.0), (0.66, 0.88)):  # three raised bands
        b.lathe(t(x), t(y), t(z + h * zz), [(t(r * k + 0.003), 0), (t(r * k + 0.012), t(0.012)), (t(r * k + 0.003), t(0.024))],
                "roof_tile", sides=48, stage=stage, tint=0.25)
    for side in (-1, 1):  # two lugs on the shoulder
        b.tube_path([(t(x + side * r * 0.62), t(y), t(z + h * 0.80)), (t(x + side * r * 0.78), t(y), t(z + h * 0.79)),
                     (t(x + side * r * 0.80), t(y), t(z + h * 0.74)), (t(x + side * r * 0.70), t(y), t(z + h * 0.72))],
                    t(0.018 * scale), "roof_tile", sides=10, stage=stage)
    if lid:
        b.lathe(t(x), t(y), t(z + h * 0.99), [(t(r * 0.52), 0), (t(r * 0.50), t(0.03)), (t(r * 0.15), t(0.05)),
                                              (t(r * 0.10), t(0.08)), (t(r * 0.02), t(0.08))], "dark_wood", sides=32,
                stage=stage, smooth=3)


def amphora(b, x, y, scale=1.0, stage=0.9, rng=None, name="amphora", z=0.0, stopper=True):
    """A transport amphora: a slim egg of a body, a knob of a toe, a long neck, two handles from neck to
    shoulder; a stopper of cork and pitch; standing in a ring of stones."""
    h, r = 0.75 * scale, 0.17 * scale
    b.prop(name, t(x - r * 1.5), t(y - r * 1.5), t(z), t(x + r * 1.5), t(y + r * 1.5), t(z + h + 0.03))
    if not b.detail:
        return
    ctrl = [(0.06, 0.0), (0.16, 0.03), (0.20, 0.07), (0.40, 0.14), (0.70, 0.26), (0.92, 0.40), (1.0, 0.52),
            (0.94, 0.63), (0.70, 0.73), (0.42, 0.79), (0.32, 0.84), (0.30, 0.93), (0.36, 0.965), (0.38, 0.99),
            (0.32, 1.0)]
    b.lathe(t(x), t(y), t(z), [(t(r * a), t(h * c)) for a, c in ctrl], "roof_tile", sides=40, stage=stage,
            tint=rng.uniform(0.4, 0.8) if rng else 0.6, top="oil", smooth=4)
    _painted_band(b, x, y, z, h, r, ctrl, 0.40, 0.62, "dark_wood", stage)  # the black-glazed frieze
    for f in (0.36, 0.66):
        _painted_band(b, x, y, z, h, r, ctrl, f, f + 0.025, "cloth", stage, steps=1)
    for side in (-1, 1):
        pts = []
        for i in range(9):
            f = i / 8
            ang = math.pi * f
            pts.append((x + side * r * (0.33 + 0.62 * math.sin(ang)), y, z + h * (0.93 - 0.24 * f + 0.06 * math.sin(ang))))
        b.tube_path([tuple(t(v) for v in p) for p in pts], t(0.017 * scale), "roof_tile", sides=12, stage=stage)
    if stopper:
        b.lathe(t(x), t(y), t(z + h * 0.99), [(t(r * 0.30), 0), (t(r * 0.33), t(0.02)), (t(r * 0.25), t(0.04))], "dark_wood",
                sides=20, stage=stage)
    b.lathe(t(x), t(y), t(z), [(t(r * 0.95), 0), (t(r * 0.95), t(0.05)), (t(r * 0.6), t(0.06))], "stone", sides=20,
            stage=stage)


def hydria(b, x, y, scale=1.0, stage=0.9, rng=None, name="hydria", z=0.0):
    """A water jar: a round belly, a flat shoulder, two side handles to lift it and one at the back to pour."""
    h, r = 0.55 * scale, 0.20 * scale
    b.prop(name, t(x - r * 1.3), t(y - r * 1.3), t(z), t(x + r * 1.3), t(y + r * 1.3), t(z + h + 0.02))
    if not b.detail:
        return
    ctrl = [(0.5, 0.0), (0.56, 0.04), (0.80, 0.18), (0.98, 0.38), (1.0, 0.52), (0.94, 0.64), (0.70, 0.72),
            (0.40, 0.76), (0.32, 0.84), (0.34, 0.94), (0.46, 0.99), (0.40, 1.0)]
    b.lathe(t(x), t(y), t(z), [(t(r * a), t(h * c)) for a, c in ctrl], "roof_tile", sides=40, stage=stage,
            tint=rng.uniform(0.3, 0.6) if rng else 0.45, top="water", smooth=4)
    _painted_band(b, x, y, z, h, r, ctrl, 0.42, 0.60, "dark_wood", stage)
    _painted_band(b, x, y, z, h, r, ctrl, 0.66, 0.70, "ochre", stage, steps=1)
    for side in (-1, 1):  # the lifting handles, level, on the belly
        pts = [(x + side * r * 0.95, y - r * 0.35, z + h * 0.50), (x + side * r * 1.22, y - r * 0.2, z + h * 0.53),
               (x + side * r * 1.22, y + r * 0.2, z + h * 0.53), (x + side * r * 0.95, y + r * 0.35, z + h * 0.50)]
        b.tube_path([tuple(t(v) for v in p) for p in pts], t(0.014 * scale), "roof_tile", sides=10, stage=stage)
    pts = [(x, y - r * 0.34, z + h * 0.92), (x, y - r * 0.85, z + h * 0.92), (x, y - r * 1.02, z + h * 0.74), (x, y - r * 0.86, z + h * 0.62)]
    b.tube_path([tuple(t(v) for v in p) for p in pts], t(0.016 * scale), "roof_tile", sides=10, stage=stage)


def sack(b, x, y, scale=1.0, stage=0.9, rng=None, name="sack", z=0.0, part="linen"):
    """A sack of grain: a slumped body, the neck gathered and tied with rope."""
    h, r = 0.62 * scale, 0.22 * scale
    b.prop(name, t(x - r), t(y - r), t(z), t(x + r), t(y + r), t(z + h))
    if not b.detail:
        return
    lumps = rng.uniform(0, 6.28) if rng else 0
    ctrl = [(0.55, 0.0), (0.90, 0.05), (1.0, 0.18), (0.97, 0.42), (0.88, 0.62), (0.62, 0.78), (0.30, 0.86),
            (0.26, 0.90), (0.36, 0.96), (0.20, 1.0)]
    b.lathe(t(x), t(y), t(z), [(t(r * a), t(h * c)) for a, c in ctrl], part, sides=32, stage=stage,
            tint=rng.uniform(0.3, 0.7) if rng else 0.5, smooth=3, shape=lambda a: 1.0 + 0.05 * math.sin(5 * a + lumps))
    b.lathe(t(x), t(y), t(z + h * 0.86), [(t(r * 0.30), 0), (t(r * 0.30), t(0.02))], "rope", sides=16, stage=stage)


def basket(b, x, y, produce, scale=1.0, stage=0.9, rng=None, name="basket", z=0.0):
    """A woven basket of produce (onions, figs, carrots' tops): ribs round its side, a rolled rim, the fruit
    heaped above it."""
    h, r = 0.26 * scale, 0.24 * scale
    b.prop(name, t(x - r * 1.1), t(y - r * 1.1), t(z), t(x + r * 1.1), t(y + r * 1.1), t(z + h + 0.12))
    if not b.detail:
        return
    b.lathe(t(x), t(y), t(z), [(t(r * 0.7), 0), (t(r * 0.95), t(h * 0.5)), (t(r), t(h))], "wood", sides=36, stage=stage,
            tint=0.65, smooth=3, shape=lambda a: 1.0 + 0.03 * abs(math.sin(18 * a)), top="dark_wood")
    b.lathe(t(x), t(y), t(z + h - 0.01), [(t(r * 1.03), 0), (t(r * 1.06), t(0.015)), (t(r * 1.03), t(0.03))], "wood", sides=36,
            stage=stage, tint=0.4)
    colour = {"onions": "ochre", "figs": "dark_wood", "carrots": "ochre", "olives": "leaves"}[produce]
    n = 9 if produce != "olives" else 16
    for i in range(n):  # the heap: small round bodies
        a = 2 * math.pi * i / n + (rng.uniform(0, 0.4) if rng else 0)
        rr = r * (0.55 if i % 2 else 0.25)
        fx, fy = x + rr * math.cos(a), y + rr * math.sin(a)
        fr = 0.045 if produce != "olives" else 0.022
        b.lathe(t(fx), t(fy), t(z + h - 0.02 + (0.03 if i % 2 == 0 else 0.0)),
                [(t(fr * 0.3), 0), (t(fr), t(fr * 0.8)), (t(fr * 0.9), t(fr * 1.4)), (t(fr * 0.2), t(fr * 2.0))],
                colour, sides=10, stage=stage, tint=rng.uniform(0.3, 0.8) if rng else 0.5, smooth=2)


def wool_bale(b, x, y, scale=1.0, stage=0.9, rng=None, name="wool", z=0.0):
    """Fleece rolled into a bale: lumpy, tied twice with rope."""
    h, r = 0.40 * scale, 0.24 * scale
    b.prop(name, t(x - r * 1.1), t(y - r * 1.1), t(z), t(x + r * 1.1), t(y + r * 1.1), t(z + h))
    if not b.detail:
        return
    phase = rng.uniform(0, 6.28) if rng else 0
    ctrl = [(0.6, 0.0), (0.96, 0.08), (1.0, 0.35), (0.98, 0.65), (0.88, 0.9), (0.4, 1.0), (0.0, 1.0)]
    b.lathe(t(x), t(y), t(z), [(t(r * a), t(h * c)) for a, c in ctrl], "linen", sides=36, stage=stage, tint=0.85, smooth=3,
            shape=lambda a: 1.0 + 0.07 * math.sin(7 * a + phase) * math.sin(3 * a))
    for zz in (0.3, 0.68):
        b.lathe(t(x), t(y), t(z + h * zz), [(t(r * 1.01), 0), (t(r * 1.03), t(0.012)), (t(r * 1.01), t(0.024))], "rope",
                sides=36, stage=stage)


def loom(b, x, y, along_x=True, stage=0.9, rng=None, name="loom"):
    """A warp-weighted loom leaning on a wall: two uprights, the cloth beam, the warp threads, the clay weights
    hanging in a row, a part of the cloth woven."""
    w, d, h = (1.1, 0.35, 1.7)
    sx, sy = (w, d) if along_x else (d, w)
    b.prop(name, t(x), t(y), 0, t(x + sx), t(y + sy), t(h))
    if not b.detail:
        return
    def p(a, dd, zz):
        return (t(x + a), t(y + dd), t(zz)) if along_x else (t(x + dd), t(y + a), t(zz))
    for a in (0.04, w - 0.04):
        b.log(p(a, 0.28, 0.03), p(a, 0.06, h), t(0.03), sides=10, stage=stage)
    b.log(p(0.0, 0.07, h - 0.08), p(w, 0.07, h - 0.08), t(0.035), sides=12, stage=stage)  # the cloth beam
    b.log(p(0.02, 0.18, 0.55), p(w - 0.02, 0.18, 0.55), t(0.02), sides=8, stage=stage)  # the shed rod
    for i in range(22):
        a = 0.1 + (w - 0.2) * i / 21
        b.strand(p(a, 0.075, h - 0.1), p(a, 0.2, 0.38), t(0.006), "linen", stage=stage)
        b.lathe(*(p(a, 0.205, 0.24)[:2]), t(0.24), [(t(0.012), 0), (t(0.03), t(0.04)), (t(0.012), t(0.10))], "roof_tile",
                sides=8, stage=stage, tint=0.4)
    cloth = lambda u, v: p(0.1 + (w - 0.2) * u, 0.075 + 0.02 * v, h - 0.12 - 0.45 * v)
    b.surface(cloth, 8, 3, "cloth", stage=stage, tint=0.5)


def blossom(b, p, r, colour, stage=0.95, tint=0.5):
    """A small open flower at p (metres): five round petals in a shallow cup, a yellow heart."""
    x, y, z = p
    b.lathe(t(x), t(y), t(z), [(t(r * 0.15), 0), (t(r), t(r * 0.35)), (t(r * 0.25), t(r * 0.4))], colour, sides=10,
            stage=stage, tint=tint, top="flower_yellow" if colour != "flower_yellow" else "ochre",
            shape=lambda a: 0.62 + 0.38 * abs(math.cos(2.5 * a)))


def chimney_pot(b, x, y, z, rng, scale=1.0, stage=0.8):
    """A terracotta smoke pot on the ridge (metres; z the ridge top): a squat plastered base sunk into the
    tiles, a round-bellied pot drawn up to a narrow neck, a little hat on three legs to keep the rain out.
    Greek houses had them; ours are a size too big, on purpose."""
    s_ = scale
    b.box(t(x - 0.16 * s_), t(y - 0.16 * s_), t(z - 0.22), t(0.32 * s_), t(0.32 * s_), t(0.3), "mortar", stage=stage,
          tint=0.6)
    if not b.detail:
        b.column(t(x), t(y), t(z + 0.08), t(0.12 * s_), t(0.4 * s_), "roof_tile", sides=6, stage=stage)
        return
    b.box(t(x - 0.18 * s_), t(y - 0.18 * s_), t(z + 0.06), t(0.36 * s_), t(0.36 * s_), t(0.035), "stone", stage=stage)
    ctrl = [(0.55, 0.0), (0.85, 0.15), (1.0, 0.38), (0.92, 0.6), (0.6, 0.78), (0.45, 0.88), (0.5, 0.96), (0.42, 1.0)]
    h, r = 0.48 * s_, 0.14 * s_
    b.lathe(t(x), t(y), t(z + 0.095), [(t(r * a), t(h * c)) for a, c in ctrl], "roof_tile", sides=28, stage=stage,
            tint=rng.uniform(0.3, 0.6), top="dark_wood", smooth=3,
            shape=lambda a: 1.0 + 0.04 * math.sin(3 * a + 1.0))  # thrown by hand, not quite round
    for i in range(3):
        a = 2 * math.pi * i / 3
        b.log((t(x + 0.05 * s_ * math.cos(a)), t(y + 0.05 * s_ * math.sin(a)), t(z + 0.095 + h)),
              (t(x + 0.07 * s_ * math.cos(a)), t(y + 0.07 * s_ * math.sin(a)), t(z + 0.095 + h + 0.07)), t(0.008), sides=5,
              stage=stage)
    b.lathe(t(x), t(y), t(z + 0.095 + h + 0.065), [(t(0.02), 0), (t(0.13 * s_), t(0.02)), (t(0.08 * s_), t(0.06)),
                                                   (t(0.01), t(0.09))], "roof_tile", sides=20, stage=stage, tint=0.35)


def wreath(b, wall, at, up, radius=0.1, rng=None, stage=0.9):
    """A laurel wreath hung over a door (in the wall's frame: `at` along, `up` above its foot), tied with a red
    ribbon whose two ends hang down: what a Greek house put up for a wedding or a victory, here on every
    house that can afford it."""
    leaves = 26
    for i in range(leaves):
        a = 2 * math.pi * i / leaves
        ca, sa = math.cos(a), math.sin(a)
        root = wall.frame(at + radius * ca, -0.045, up + radius * sa)
        tip = wall.frame(at + (radius + 0.035) * math.cos(a + 0.35), -0.06, up + (radius + 0.035) * math.sin(a + 0.35))
        b.strand(tuple(t(v) for v in root), tuple(t(v) for v in tip), t(0.035), "leaves", stage=stage,
                 tint=(rng.uniform(0.2, 0.8) if rng else 0.5))
    ring = [tuple(t(v) for v in wall.frame(at + radius * math.cos(2 * math.pi * k / 16), -0.04,
                                           up + radius * math.sin(2 * math.pi * k / 16))) for k in range(17)]
    b.tube_path(ring, t(0.012), "bark", sides=6, stage=stage, caps=False)
    for side in (-1, 1):  # the ribbon's ends
        pts = [wall.frame(at, -0.05, up - radius), wall.frame(at + side * 0.05, -0.05, up - radius - 0.12),
               wall.frame(at + side * 0.03, -0.05, up - radius - 0.24)]
        b.tube_path([tuple(t(v) for v in p) for p in pts], t(0.01), "cloth", sides=4, stage=stage)


def potted_plant(b, x, y, scale=1.0, stage=0.95, rng=None, name="plant", z=0.0, bloom="random"):
    """A terracotta pot with a herb or a geranium: leaves on short stems round a stalk, flowers among them."""
    hp, rp = 0.32 * scale, 0.16 * scale
    b.prop(name, t(x - rp * 1.8), t(y - rp * 1.8), t(z), t(x + rp * 1.8), t(y + rp * 1.8), t(z + hp + 0.55 * scale))
    if bloom == "random":
        bloom = rng.choice(["flower_red", "flower_red", "flower_white", "flower_yellow", None])
    if not b.detail:
        return
    b.lathe(t(x), t(y), t(z), [(t(rp * 0.6), 0), (t(rp * 0.8), t(hp * 0.4)), (t(rp), t(hp * 0.9)), (t(rp * 1.12), t(hp)),
                            (t(rp * 1.0), t(hp))], "roof_tile", sides=24, stage=stage, top="dark_wood", smooth=2)
    tips = []
    for i in range(26):
        a = rng.uniform(0, 6.28)
        up = rng.uniform(0.05, 0.5) * scale
        reach = rng.uniform(0.05, 0.26) * scale * (1.0 - up)
        root = (x + 0.02 * math.cos(a), y + 0.02 * math.sin(a), z + hp + up * 0.6)
        tip = (x + reach * math.cos(a), y + reach * math.sin(a), z + hp + up + 0.08)
        tips.append(tip)
        b.strand(tuple(t(v) for v in root), tuple(t(v) for v in tip), t(rng.uniform(0.045, 0.075) * max(scale, 0.7)), "leaves", stage=stage,
                 tint=rng.uniform(0.2, 0.8))
    if bloom:
        for tip in tips[:14]:
            blossom(b, tip, 0.05 * max(scale, 0.7), bloom, stage=stage, tint=rng.uniform(0.35, 0.75))


def flower_box(b, x, y, z, length, rng, along_x=True, out=1, name="flower box"):
    """A wooden box of flowers under a window (metres): x, y the wall's outer face at the box's start, out the
    side the wall faces (+1 or -1 along y for a box along x, along x otherwise). The box stands off the sill on
    two iron brackets; geraniums and trailing leaves over its front."""
    gap, d, h = 0.035, 0.16, 0.14  # against the plaster (the wall solid reaches 0.03 out)
    def at(a, o, zz):  # a along the wall, o out from its face
        return (x + a, y + out * o, zz) if along_x else (x + out * o, y + a, zz)
    lo, hi = at(0, gap, z), at(length, gap + d, z + 0.42)
    b.prop(name, t(min(lo[0], hi[0])), t(min(lo[1], hi[1])), t(z - 0.08), t(max(lo[0], hi[0])), t(max(lo[1], hi[1])), t(z + 0.42))
    c0, c1 = at(0, gap, z), at(length, gap + d, z)
    b.box(t(min(c0[0], c1[0])), t(min(c0[1], c1[1])), t(z), t(abs(c1[0] - c0[0])), t(abs(c1[1] - c0[1])), t(h), "wood",
          stage=0.9, tint=0.6)
    if not b.detail:
        return
    for a in (0.08, length - 0.08):  # the brackets
        p0, p1 = at(a - 0.015, 0.0, z - 0.07), at(a + 0.015, gap + d * 0.8, z)
        b.box(t(min(p0[0], p1[0])), t(min(p0[1], p1[1])), t(z - 0.07), t(abs(p1[0] - p0[0])), t(abs(p1[1] - p0[1])), t(0.07),
              "iron", stage=0.9)
    s0, s1 = at(0.02, gap + 0.02, z + h - 0.03), at(length - 0.02, gap + d - 0.02, z + h)
    b.box(t(min(s0[0], s1[0])), t(min(s0[1], s1[1])), t(z + h - 0.03), t(abs(s1[0] - s0[0])), t(abs(s1[1] - s0[1])),
          t(0.035), "dark_wood", stage=0.9)  # the soil
    colour = rng.choice(["flower_red", "flower_red", "flower_white", "flower_yellow"])
    n = max(8, int(length / 0.03))
    for i in range(n):
        a = 0.05 + (length - 0.1) * (i + rng.uniform(-0.3, 0.3)) / max(n - 1, 1)
        root = at(a, gap + d * rng.uniform(0.3, 0.7), z + h)
        tip = at(a + rng.uniform(-0.05, 0.05), gap + d * rng.uniform(0.2, 0.9), z + h + rng.uniform(0.1, 0.24))
        b.strand(tuple(t(v) for v in root), tuple(t(v) for v in tip), t(rng.uniform(0.05, 0.075)), "leaves", stage=0.95,
                 tint=rng.uniform(0.2, 0.8))
        if i % 3 != 2:
            blossom(b, tip, rng.uniform(0.04, 0.055), colour, stage=0.95, tint=rng.uniform(0.35, 0.75))
        if i % 3 == 1:  # a leaf trailing over the front
            r2 = at(a, gap + d, z + h - 0.01)
            b.strand(tuple(t(v) for v in r2), tuple(t(v) for v in at(a + 0.02, gap + d + 0.02, z - 0.06)), t(0.035),
                     "leaves", stage=0.95, tint=rng.uniform(0.2, 0.6))


def ivy(b, x, y, length, z0, z1, rng, along_x=True, out=1, name="ivy"):
    """Ivy climbing a wall (metres): stems wandering up from the ground, leaves standing off the face. x, y the
    wall's outer face at the patch's start, out the side the wall faces."""
    def at(a, o, zz):
        return (x + a, y + out * o, zz) if along_x else (x + out * o, y + a, zz)
    lo, hi = at(0, 0.036, z0), at(length, 0.1, z1)  # off the plaster (the wall solid reaches 0.03 out)
    b.prop(name, t(min(lo[0], hi[0])), t(min(lo[1], hi[1])), t(z0), t(max(lo[0], hi[0])), t(max(lo[1], hi[1])), t(z1))
    if not b.detail:
        return
    for s_ in range(max(2, int(length / 0.35))):
        a = (s_ + 0.5) * length / max(2, int(length / 0.35))
        pts, zz = [], z0
        while zz < z1 - 0.05:
            pts.append(at(min(max(a, 0.02), length - 0.02), 0.046, zz))
            zz += rng.uniform(0.12, 0.2)
            a += rng.uniform(-0.12, 0.12)
        if len(pts) < 2:
            continue
        b.tube_path([tuple(t(v) for v in p) for p in pts], t(0.008), "bark", sides=5, stage=0.95)
        for p in pts:
            top = 1.0 - (p[2] - z0) / max(z1 - z0, 0.01)  # thicker near the ground, sparse at the top
            for _ in range(int(3 + 5 * top)):
                da, dz = rng.uniform(-0.14, 0.14), rng.uniform(-0.08, 0.1)
                aa = min(max((p[0] - x if along_x else p[1] - y) + da, 0.03), length - 0.03)
                zl = min(max(p[2] + dz, z0 + 0.02), z1 - 0.04)
                root = at(aa, 0.048, zl)
                tip = at(aa + rng.uniform(-0.03, 0.03), rng.uniform(0.06, 0.09), zl + rng.uniform(0.04, 0.07))
                b.strand(tuple(t(v) for v in root), tuple(t(v) for v in tip), t(rng.uniform(0.05, 0.07)),
                         "leaves" if rng.random() < 0.75 else "grass", stage=0.95, tint=rng.uniform(0.15, 0.7))


def laundry_line(b, p0, p1, height, rng, name="laundry"):
    """Washing on a line between two posts (metres): the rope sags, the cloths hang from it in a breeze."""
    (x0, y0), (x1, y1) = p0, p1
    for i, (px, py) in enumerate((p0, p1)):
        b.prop("%s post %d" % (name, i + 1), t(px - 0.04), t(py - 0.04), 0, t(px + 0.04), t(py + 0.04), t(height + 0.05))
    ux, uy = (x1 - x0) / math.hypot(x1 - x0, y1 - y0), (y1 - y0) / math.hypot(x1 - x0, y1 - y0)
    a0, a1 = (x0 + ux * 0.09, y0 + uy * 0.09), (x1 - ux * 0.09, y1 - uy * 0.09)  # between the posts
    b.prop(name, t(min(a0[0], a1[0]) - 0.04), t(min(a0[1], a1[1]) - 0.04), t(height - 0.8), t(max(a0[0], a1[0]) + 0.04),
           t(max(a0[1], a1[1]) + 0.04), t(height))
    for px, py in (p0, p1):
        b.log((t(px), t(py), 0), (t(px), t(py), t(height + 0.05)), t(0.03), sides=8 if b.detail else 4, stage=0.9, tint=0.4)
    if not b.detail:
        return
    sag = 0.1
    line = lambda f: (x0 + (x1 - x0) * f, y0 + (y1 - y0) * f, height - 0.04 - sag * 4 * f * (1 - f))
    b.tube_path([tuple(t(v) for v in line(k / 12)) for k in range(13)], t(0.006), "rope", sides=5, stage=0.9)
    f = 0.08
    for colour, w in (("linen", 0.34), ("sky_cloth", 0.42), ("flower_white", 0.3), ("cloth", 0.26)):
        span = w / math.hypot(x1 - x0, y1 - y0)
        if f + span > 0.94:
            break
        drop = rng.uniform(0.4, 0.62)
        fa = f
        phase = rng.uniform(0, 6.28)
        def cloth(u, v, fa=fa, span=span, drop=drop, phase=phase):
            px, py, pz = line(fa + span * u)
            nx, ny = -(y1 - y0), (x1 - x0)
            n = math.hypot(nx, ny)
            billow = 0.035 * math.sin(math.pi * u) * v + 0.012 * math.sin(6 * u + phase)
            return (t(px + nx / n * billow), t(py + ny / n * billow), t(pz - 0.01 - drop * v))
        b.surface(cloth, 6, 4, colour, stage=0.9, tint=rng.uniform(0.4, 0.7))
        for u in (0.05, 0.95):  # the pegs
            px, py, pz = line(fa + span * u)
            b.box(t(px - 0.008), t(py - 0.008), t(pz - 0.05), t(0.016), t(0.016), t(0.06), "wood", stage=0.9, tint=0.7)
        f += span + rng.uniform(0.04, 0.08)


def grass(b, rng, n=40, name="grass"):
    """Tufts of grass and wild flowers where the ground is free: round the footing, along the walls, at the
    plot's edge. A tuft goes only where nothing stands within its reach (rays up from the ground hit nothing
    below knee height), so it never grows through a wall or a prop. Call last."""
    import mathutils
    from mathutils.bvhtree import BVHTree
    if not b.detail:
        return
    tree = BVHTree.FromBMesh(b.bm)
    w, h = b.width / M, b.height / M  # metres
    placed = 0
    for _ in range(n * 6):
        if placed >= n:
            break
        x, y = rng.uniform(0.08, w - 0.08), rng.uniform(0.08, h - 0.08)
        r = rng.uniform(0.05, 0.09)
        box = (t(x - r), t(y - r), 0, t(x + r), t(y + r), t(0.3))
        near = lambda o: all(box[i] < o[i + 3] + 0.004 and o[i] < box[i + 3] + 0.004 for i in range(3))
        free = not any(near(o) for _, o, _ in b.solids) and not any(near(o) for _, o in b.props)
        for dx, dy in ((0, 0), (r, 0), (-r, 0), (0, r), (0, -r)) if free else ():
            hit = tree.ray_cast(mathutils.Vector((t(x + dx), -t(y + dy), 0.002)), mathutils.Vector((0, 0, 1)), t(0.45))
            if hit[0] is not None:
                free = False
                break
        if not free:
            continue
        b.prop("%s %d" % (name, placed + 1), t(x - r), t(y - r), 0, t(x + r), t(y + r), t(0.3))
        k = rng.randint(6, 10)
        for i in range(k):
            a = 2 * math.pi * i / k + rng.uniform(-0.3, 0.3)
            rr = r * rng.uniform(0.2, 0.9)
            tip = (x + rr * 1.2 * math.cos(a), y + rr * 1.2 * math.sin(a), rng.uniform(0.1, 0.24))
            b.strand((t(x + rr * 0.3 * math.cos(a)), t(y + rr * 0.3 * math.sin(a)), 0), tuple(t(v) for v in tip),
                     t(rng.uniform(0.02, 0.035)), "grass" if rng.random() < 0.7 else "leaves", stage=0.95,
                     tint=rng.uniform(0.2, 0.9))
        if rng.random() < 0.35:
            blossom(b, (x, y, rng.uniform(0.16, 0.24)), 0.025, rng.choice(["flower_white", "flower_yellow", "flower_red"]),
                    tint=rng.uniform(0.4, 0.8))
        placed += 1


def woodpile(b, x, y, rng, along_x=True, length=0.9, depth=0.42, height=0.55, stage=0.9, name="woodpile", block=True):
    """Firewood stacked as it would lie: the bottom row on two bearers, round logs and halves (flat side down)
    side by side, every log above resting on the two below it at the points where they touch; a chopping block
    with an axe in it beside the pile. Bark round the logs, end grain on the cuts."""
    sx, sy = (length, depth) if along_x else (depth, length)
    b.prop(name, t(x), t(y), 0, t(x + sx), t(y + sy), t(height + 0.06))
    if not b.detail:
        b.box(t(x), t(y), 0, t(sx), t(sy), t(height), "bark", loose=True, stage=stage)
        return
    base = 0.06
    for dd in (0.06, depth - 0.06):  # the bearers keep the wood off the ground
        a, e = ((x + 0.02, y + dd, 0.03), (x + sx - 0.02, y + dd, 0.03)) if along_x else ((x + dd, y + 0.02, 0.03), (x + dd, y + sy - 0.02, 0.03))
        b.log(tuple(t(v) for v in a), tuple(t(v) for v in e), t(0.03), sides=8, stage=stage, tint=0.3)
    # The bottom row: (centre along the row, radius, centre height, piece).
    row = []
    s = 0.02
    while True:
        r = rng.uniform(0.045, 0.085)
        half = rng.random() < 0.3
        if s + 2 * r > length - 0.02:
            break
        row.append((s + r, r, base if half else base + r, "half" if half else None))
        s += 2 * r + rng.uniform(0.0, 0.01)
    layers = [row]
    while len(layers[-1]) >= 2:
        below, above = layers[-1], []
        for (c1, r1, z1, _), (c2, r2, z2, _) in zip(below, below[1:]):
            r = rng.uniform(0.04, 0.08)
            # The centre at distance r1 + r from one and r2 + r from the other: where it rests on both.
            dx, dz = c2 - c1, z2 - z1
            dist = math.hypot(dx, dz)
            if dist > r1 + r2 + 2 * r - 0.005 or dist < 1e-6:
                continue  # the gap is too wide: it would fall through
            a_ = ((r1 + r) ** 2 - (r2 + r) ** 2 + dist ** 2) / (2 * dist)
            hgt = math.sqrt(max((r1 + r) ** 2 - a_ ** 2, 0.0))
            mx, mz = c1 + a_ * dx / dist, z1 + a_ * dz / dist
            cx, cz = mx - hgt * dz / dist, mz + hgt * dx / dist
            if cz + r > height + base:
                continue
            above.append((cx, r, cz, None))
        if not above:
            break
        layers.append(above)
    for layer in layers:
        for c, r, cz, piece in layer:
            l0, l1 = rng.uniform(0.0, 0.05), depth - rng.uniform(0.0, 0.05)
            skew = rng.uniform(-0.015, 0.015)
            if along_x:
                a, e = (x + c - skew, y + l0, cz), (x + c + skew, y + l1, cz)
            else:
                a, e = (x + l0, y + c - skew, cz), (x + l1, y + c + skew, cz)
            b.log(tuple(t(v) for v in a), tuple(t(v) for v in e), t(r), sides=14, stage=stage, split=piece,
                  tint=rng.uniform(0.2, 0.8), grain_tint=rng.uniform(0.3, 0.8))
    if block:  # the chopping block and its axe, beside the pile
        bx, by = (x + length + 0.2, y + depth / 2) if along_x else (x + depth / 2, y + length + 0.2)
        hx, hy = (1, 0) if along_x else (0, 1)  # the axe's handle points away from the pile, along it
        b.prop(name + " block", t(bx - 0.17), t(by - 0.17), 0, t(bx + 0.17 + hx * 0.2), t(by + 0.17 + hy * 0.2), t(0.5))
        b.log((t(bx), t(by), 0), (t(bx), t(by), t(0.34)), t(0.16), sides=18, stage=stage, tint=0.5)
        b.tube_path([(t(bx + hx * 0.02), t(by + hy * 0.02), t(0.34)), (t(bx + hx * 0.18 + hy * 0.03), t(by + hy * 0.18 + hx * 0.03), t(0.42)),
                     (t(bx + hx * 0.34 + hy * 0.05), t(by + hy * 0.34 + hx * 0.05), t(0.47))], t(0.014), "wood", sides=8,
                    stage=stage)
        b.obox((t(bx), t(by), t(0.34)), (t(0.05), 0, 0), (0, t(0.012), 0), (0, 0, t(0.04)), "iron", stage=stage)


def palmette(b, p, along_x, side, size, stage=0.9, part="ochre"):
    """An antefix or an acroterion: a fan of leaves standing on the eave, faced outwards, seen from both sides."""
    for i in range(7):
        a = math.pi * (0.15 + 0.7 * i / 6)
        reach = size * (1.0 if i == 3 else 0.82 if i in (2, 4) else 0.66)
        base = (p[0], p[1], p[2] + 0.02)
        if along_x:
            tip = (p[0] + reach * math.cos(a), p[1] + side * 0.01, p[2] + 0.02 + reach * math.sin(a))
        else:
            tip = (p[0] + side * 0.01, p[1] + reach * math.cos(a), p[2] + 0.02 + reach * math.sin(a))
        b.strand(tuple(t(v) for v in base), tuple(t(v) for v in tip), t(size * 0.28), part, stage=stage, tint=0.4 + 0.05 * i)


def cornice(b, x, y, w, d, z, steps=((0.06, 0.05), (0.10, 0.04), (0.14, 0.05)), part="stone", stage=0.6):
    """A moulded cornice round the top of the walls: steps standing out further as they rise."""
    zz = z
    for out, hh in steps:
        b.box(t(x - out), t(y - out), t(zz), t(w + 2 * out), t(d + 2 * out), t(hh), part, stage=stage)
        zz += hh
    return zz


def meander(b, x0, y0, length, z, along_x=True, outward=-1, unit=0.10, part="ochre", stage=0.6):
    """A meander (Greek key) in low relief along a facade: its strokes as small raised bars."""
    n = int(length / (unit * 2))
    strokes = [(0, 0, 1, 0), (1, 0, 1, 1), (1, 1, 0.33, 1), (0.33, 1, 0.33, 0.33), (0.33, 0.33, 0.66, 0.33),
               (0.66, 0.33, 0.66, 0.66)]
    for k in range(n):
        o = k * unit * 2
        for a0, z0, a1, z1 in strokes + [(1, 0, 2, 0), (0, 1.15, 2, 1.15), (0, -0.15, 2, -0.15)]:
            la, lb = sorted((a0, a1))
            za, zb = sorted((z0, z1))
            lw = max((lb - la) * unit, 0.018)
            zh = max((zb - za) * unit, 0.018)
            if along_x:
                b.box(t(x0 + o + la * unit), t(y0 + (0.012 if outward > 0 else -0.012) - 0.012), t(z + za * unit), t(lw),
                      t(0.012), t(zh), part, stage=stage)
            else:
                b.box(t(x0 + (0.012 if outward > 0 else -0.012) - 0.012), t(y0 + o + la * unit), t(z + za * unit), t(0.012),
                      t(lw), t(zh), part, stage=stage)


def baluster(b, x, y, z, height, stage=0.6):
    """A turned wooden baluster."""
    b.lathe(t(x), t(y), t(z), [(t(0.03), 0), (t(0.03), t(height * 0.08)), (t(0.018), t(height * 0.2)),
                              (t(0.034), t(height * 0.45)), (t(0.016), t(height * 0.75)), (t(0.024), t(height * 0.9)),
                              (t(0.03), t(height))], "wood", sides=12, stage=stage, smooth=2, top="wood")


def doric_column(b, x, y, z, height, radius=0.16, stage=0.5, group="porch"):
    """A Doric column: a fluted shaft narrowing upwards with a slight swelling (entasis), the echinus and the
    abacus."""
    b.solid("column", t(x - radius * 1.4), t(y - radius * 1.4), t(z), t(x + radius * 1.4), t(y + radius * 1.4), t(z + height),
            block=group)
    if not b.detail:
        b.column(t(x), t(y), t(z), t(radius), t(height * 0.93), "stone", sides=8, stage=stage)
        b.box(t(x - radius * 1.3), t(y - radius * 1.3), t(z + height * 0.93), t(radius * 2.6), t(radius * 2.6),
              t(height * 0.07), "stone", stage=stage)
        return
    shaft = [(t(radius * (1.0 - 0.18 * f + 0.05 * math.sin(math.pi * f))), t(height * 0.82 * f)) for f in
             [i / 10 for i in range(11)]]
    b.lathe(t(x), t(y), t(z), shaft, "stone", sides=40, stage=stage, shape=lambda a: 1.0 - 0.06 * abs(math.sin(10 * a)))
    b.lathe(t(x), t(y), t(z + height * 0.82), [(t(radius * 0.84), 0), (t(radius * 0.86), t(height * 0.02)),
                                                (t(radius * 1.05), t(height * 0.05)), (t(radius * 1.28), t(height * 0.10)),
                                                (t(radius * 1.30), t(height * 0.11))], "stone", sides=32, stage=stage)
    b.box(t(x - radius * 1.4), t(y - radius * 1.4), t(z + height * 0.93), t(radius * 2.8), t(radius * 2.8),
          t(height * 0.07), "stone", stage=stage)
