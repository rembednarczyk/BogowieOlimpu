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

    def put(self, b, along, depth, up, size_along, size_depth, size_up, part, **kw):
        cx, cy, cz = self.frame(along + size_along / 2, depth + size_depth / 2, up + size_up / 2)
        ax, ay, az = self.axes(size_along / 2, size_depth / 2, size_up / 2)
        return b.obox((t(cx), t(cy), t(cz)), ax, ay, az, part, **kw)


def brick_wall(b, wall, rng, part="mudbrick", stage=(0.1, 0.6), plaster=None, block=None):
    """Lays the wall course by course; `plaster` (a palette part) coats the outer face; `block` (length, height)
    for stone blocks instead of mudbricks."""
    s0, s1 = stage
    bl, bh = block or (BRICK_L, BRICK_H)
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
                wall.put(b, a + JOINT / 2, 0.004 + jitter, z0 + JOINT / 2, c - a - JOINT, wall.thick - 0.008,
                         real_h - JOINT, part, stage=share, tint=rng.uniform(0.22, 0.78))
            s += bl
        for a, c in _cut((0.0, wall.length), holes):  # the mortar inside the joints, rising with the course
            share = s0 + (s1 - s0) * k / courses
            wall.put(b, a, 0.012, z0, c - a, wall.thick - 0.024, real_h, "mortar", stage=share)
    if plaster:
        for a, c, z0, z1 in _rects(wall.length, wall.height, wall.openings):
            wall.put(b, a, -0.018, z0, c - a, 0.018, z1 - z0, plaster, stage=s1, plaster=True)
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
        else:
            wall.put(b, o["at"] - 0.06, -0.05, o["z"] - 0.06, o["w"] + 0.12, wall.thick + 0.08, 0.06, "stone",
                     stage=mid)  # the sill
            if b.detail:  # two shutters, open against the wall
                for side, at in ((-1, o["at"] - o["w"] / 2), (1, o["at"] + o["w"])):
                    for i in range(3):
                        wall.put(b, at + i * o["w"] / 6, -0.05, o["z"] + 0.02, o["w"] / 6 - 0.01, 0.03, o["h"] - 0.04,
                                 "ochre", stage=s1, tint=rng.uniform(0.35, 0.65))
            wall.put(b, o["at"], wall.thick * 0.5, o["z"], o["w"], 0.02, o["h"], "dark_wood", stage=s1)  # dark inside


def footing(b, x, y, w, d, h=0.35, stage=0.0, rng=None):
    """A stone socle under the walls: rough blocks in the far view's place."""
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


def tiled_roof(b, x, y, w, d, z, rise, rng, along_x=True, overhang=0.35, stage=(0.65, 0.95), gable="plaster",
               antefix=False):
    """A gable roof of terracotta: pan tiles in rows from the eaves, cover tiles over their joints, a ridge;
    the gables walled in."""
    s0, s1 = stage
    x0, y0, x1, y1 = x - overhang, y - overhang, x + w + overhang, y + d + overhang
    if not b.detail:
        if along_x:
            b.prism(t(x0), t(y0), t(z), t(x1 - x0), t(y1 - y0), t(rise), "roof_tile", along_x=True, stage=s0)
        else:
            b.prism(t(x0), t(y0), t(z), t(x1 - x0), t(y1 - y0), t(rise), "roof_tile", along_x=False, stage=s0)
        _gables(b, x, y, w, d, z, rise, along_x, gable, s0)
        return
    run_len = (x1 - x0) if along_x else (y1 - y0)
    span = (y1 - y0) / 2 if along_x else (x1 - x0) / 2
    slope = math.hypot(span, rise)
    pan_w, exposed, tile_len, thick = 0.46, 0.34, 0.44, 0.022
    rows = max(2, int(math.ceil(slope / exposed)))
    cols = max(2, int(round(run_len / pan_w)))
    pan_w = run_len / cols
    # The boards under the tiles show through the joints.
    b.prism(t(x0 + 0.02), t(y0 + 0.02), t(z - 0.03), t(x1 - x0 - 0.04), t(y1 - y0 - 0.04), t(rise - 0.02), "dark_wood",
            along_x=along_x, stage=s0)
    for side in (-1, 1):  # the two slopes
        for r in range(rows):
            f0 = r * exposed / slope
            f1 = min(1.0, (r * exposed + tile_len) / slope)
            share = s0 + (s1 - s0) * r / rows
            for c in range(cols):
                lift = thick * 1.2 if r else 0.0
                a = c * pan_w + 0.012
                bb = (c + 1) * pan_w - 0.012
                p_low = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, (a + bb) / 2, f0)
                p_high = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, (a + bb) / 2, f1)
                normal = _roof_normal(span, rise, along_x, side)
                low = tuple(p_low[i] + normal[i] * (lift + thick) for i in range(3))
                high = tuple(p_high[i] + normal[i] * thick for i in range(3))
                centre = tuple(t((low[i] + high[i]) / 2) for i in range(3))
                half_up = tuple(t((high[i] - low[i]) / 2) for i in range(3))
                half_w = (t((bb - a) / 2), 0, 0) if along_x else (0, t((bb - a) / 2), 0)
                half_n = tuple(t(normal[i] * thick / 2) for i in range(3))
                b.obox(centre, half_w, half_up, half_n, "roof_tile", stage=share, tint=rng.uniform(0.25, 0.75))
            for c in range(cols + 1):  # cover tiles over the joints
                along = min(max(c * pan_w, 0.06), run_len - 0.06)
                p_low = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, along, f0)
                p_high = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, along, f1)
                normal = _roof_normal(span, rise, along_x, side)
                lift = thick * 2.2
                start = tuple(t(p_low[i] + normal[i] * lift) for i in range(3))
                end = tuple(t(p_high[i] + normal[i] * lift) for i in range(3))
                b.half_tube(start, end, t(0.075), tuple(normal), "roof_tile", sides=4, stage=share,
                            tint=rng.uniform(0.2, 0.6))
            if antefix and r == 0:  # upright tiles at the eaves, a palmette's shape left to the light
                for c in range(cols + 1):
                    along = min(max(c * pan_w, 0.06), run_len - 0.06)
                    p = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, along, 0.0)
                    centre = (t(p[0]), t(p[1]), t(p[2] + 0.10))
                    half_w = (t(0.07), 0, 0) if along_x else (0, t(0.07), 0)
                    half_d = (0, t(0.015), 0) if along_x else (t(0.015), 0, 0)
                    b.obox(centre, half_w, half_d, (0, 0, t(0.09)), "ochre", stage=share)
    ridge_n = max(1, int(run_len / 0.5))
    for i in range(ridge_n):  # the ridge
        a0 = run_len * i / ridge_n
        a1 = run_len * (i + 1) / ridge_n + 0.03
        p0 = _roof_point(x0, y0, x1, y1, z, rise, along_x, 1, a0, 1.0)
        p1 = _roof_point(x0, y0, x1, y1, z, rise, along_x, 1, min(a1, run_len), 1.0)
        b.half_tube(tuple(t(v) for v in (p0[0], p0[1], p0[2] + 0.02)), tuple(t(v) for v in (p1[0], p1[1], p1[2] + 0.02)),
                    t(0.11), (0, 0, 1), "roof_tile", sides=5, stage=s1, tint=rng.uniform(0.3, 0.5))
    for side in (-1, 1):  # the rafters' ends under the eaves
        for c in range(cols + 1):
            along = c * pan_w
            p = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, min(max(along, 0.05), run_len - 0.05), 0.04)
            half_w = (t(0.04), 0, 0) if along_x else (0, t(0.04), 0)
            n = _roof_normal(span, rise, along_x, side)
            b.obox((t(p[0] - n[0] * 0.06), t(p[1] - n[1] * 0.06), t(p[2] - 0.07)), half_w,
                   (0, t(0.08), 0) if along_x else (t(0.08), 0, 0), (0, 0, t(0.04)), "dark_wood", stage=s0)
    _gables(b, x, y, w, d, z, rise, along_x, gable, s0)


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


def _gables(b, x, y, w, d, z, rise, along_x, part, stage):
    """The triangles under a gable roof's ends, faced both ways (the material culls back faces)."""
    if along_x:
        for gx in (x, x + w):
            pts = [(t(gx), t(y), t(z)), (t(gx), t(y + d), t(z)), (t(gx), t(y + d / 2), t(z + rise * d / (d + 0.7)))]
            b.poly(pts, part, stage=stage, plaster=True)
            b.poly(list(reversed(pts)), part, stage=stage, plaster=True)
    else:
        for gy in (y, y + d):
            pts = [(t(x), t(gy), t(z)), (t(x + w), t(gy), t(z)), (t(x + w / 2), t(gy), t(z + rise * w / (w + 0.7)))]
            b.poly(pts, part, stage=stage, plaster=True)
            b.poly(list(reversed(pts)), part, stage=stage, plaster=True)


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
    # the eaves' straw ends
    perim = [((x0, y0), (x1, y0)), ((x1, y0), (x1, y1)), ((x1, y1), (x0, y1)), ((x0, y1), (x0, y0))]
    for (ax, ay), (bx, by) in perim:
        length = math.hypot(bx - ax, by - ay)
        n = int(length / 0.12)
        for i in range(n):
            f = (i + 0.5) / n
            px, py = ax + (bx - ax) * f, ay + (by - ay) * f
            drop = rng.uniform(0.04, 0.12)
            b.box(t(px - 0.05), t(py - 0.05), t(z - drop), t(0.10), t(0.10), t(drop + 0.06), "thatch", stage=s0,
                  tint=rng.uniform(0.15, 0.85))
    # the ridge: a bound roll of straw
    if along_x:
        rx0, rx1 = x0 + hip * (x1 - x0), x1 - hip * (x1 - x0)
        b.half_tube((t(rx0), t((y0 + y1) / 2), t(z + rise)), (t(rx1), t((y0 + y1) / 2), t(z + rise)), t(0.14), (0, 0, 1),
                    "thatch", sides=6, stage=s1, tint=0.3)
    else:
        ry0, ry1 = y0 + hip * (y1 - y0), y1 - hip * (y1 - y0)
        b.half_tube((t((x0 + x1) / 2), t(ry0), t(z + rise)), (t((x0 + x1) / 2), t(ry1), t(z + rise)), t(0.14), (0, 0, 1),
                    "thatch", sides=6, stage=s1, tint=0.3)


def pithos(b, x, y, scale=1.0, stage=0.9, rng=None):
    """A storage jar: a lathe of rings (detail only)."""
    if not b.detail:
        return
    profile = [(0.10, 0.06), (0.20, 0.10), (0.25, 0.14), (0.26, 0.14), (0.22, 0.12), (0.15, 0.08), (0.12, 0.04), (0.14, 0.04)]
    zz = 0.0
    for r, h in profile:
        b.column(t(x), t(y), t(zz * scale), t(r * scale), t(h * scale), "roof_tile", sides=10, stage=stage)
        zz += h


def woodpile(b, x, y, logs=7, rng=None, stage=0.9, along_x=True):
    if not b.detail:
        b.box(t(x), t(y), 0, t(0.8 if along_x else 0.3), t(0.3 if along_x else 0.8), t(0.35), "wood", loose=True,
              stage=stage)
        return
    for i in range(logs):
        row, col = divmod(i, 3)
        cx = x + (0.13 + col * 0.24) if along_x else x + 0.15
        cy = y + 0.15 if along_x else y + (0.13 + col * 0.24)
        start = (t(cx), t(cy - 0.0), t(0.1 + row * 0.18))
        if along_x:
            a, e = (t(cx), t(y), t(0.1 + row * 0.18)), (t(cx), t(y + 0.32), t(0.1 + row * 0.18))
        else:
            a, e = (t(x), t(cy), t(0.1 + row * 0.18)), (t(x + 0.32), t(cy), t(0.1 + row * 0.18))
        b.half_tube(a, e, t(0.09), (0, 0, 1), "wood", sides=5, stage=stage, tint=rng.uniform(0.3, 0.8))
        b.half_tube(a, e, t(0.09), (0, 0, -1), "wood", sides=5, stage=stage, tint=rng.uniform(0.3, 0.8))


def doric_column(b, x, y, z, height, radius=0.16, stage=0.5):
    """A Doric column: a shaft narrowing upwards in three drums, the echinus and the abacus."""
    sides = 16 if b.detail else 8
    drums = 3 if b.detail else 1
    for i in range(drums):
        r = radius * (1.0 - 0.08 * i / drums)
        b.column(t(x), t(y), t(z + height * 0.85 * i / drums), t(r), t(height * 0.85 / drums), "stone", sides=sides,
                 stage=stage)
    b.column(t(x), t(y), t(z + height * 0.85), t(radius * 1.25), t(height * 0.08), "stone", sides=sides, stage=stage)
    b.box(t(x - radius * 1.4), t(y - radius * 1.4), t(z + height * 0.93), t(radius * 2.8), t(radius * 2.8),
          t(height * 0.07), "stone", stage=stage)
