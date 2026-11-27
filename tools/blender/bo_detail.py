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


def brick_wall(b, wall, rng, part="mudbrick", stage=(0.1, 0.6), plaster=None, block=None, both=False):
    """Lays the wall course by course; `plaster` (a palette part) coats the outer face; `block` (length, height)
    for stone blocks instead of mudbricks."""
    s0, s1 = stage
    bl, bh = block or (BRICK_L, BRICK_H)
    p0 = wall.frame(0, -0.03, 0)
    p1 = wall.frame(wall.length, wall.thick, wall.height + 0.05)
    b.solid("wall", t(p0[0]), t(p0[1]), t(p0[2]), t(p1[0]), t(p1[1]), t(p1[2]))
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
            if b.detail:  # two shutters, open against the wall
                for side, at in ((-1, o["at"] - o["w"] / 2), (1, o["at"] + o["w"])):
                    for i in range(3):
                        wall.put(b, at + i * o["w"] / 6, -0.05, o["z"] + 0.02, o["w"] / 6 - 0.01, 0.03, o["h"] - 0.04,
                                 "ochre", stage=s1, tint=rng.uniform(0.35, 0.65))
            wall.put(b, o["at"], wall.thick * 0.5, o["z"], o["w"], 0.02, o["h"], "dark_wood", stage=s1)  # dark inside
            if b.detail:  # the frame and its cross bar
                for a, w_ in ((o["at"], 0.05), (o["at"] + o["w"] - 0.05, 0.05)):
                    wall.put(b, a, wall.thick * 0.3, o["z"], w_, 0.06, o["h"], "wood", stage=s1)
                wall.put(b, o["at"], wall.thick * 0.3, o["z"] + o["h"] - 0.05, o["w"], 0.06, 0.05, "wood", stage=s1)
                wall.put(b, o["at"], wall.thick * 0.3, o["z"] + o["h"] / 2 - 0.02, o["w"], 0.05, 0.04, "wood", stage=s1)
                wall.put(b, o["at"] + o["w"] / 2 - 0.02, wall.thick * 0.3, o["z"], 0.04, 0.05, o["h"], "wood", stage=s1)


def footing(b, x, y, w, d, h=0.35, stage=0.0, rng=None):
    """A stone socle under the walls: rough blocks in the far view's place."""
    b.solid("footing", t(x), t(y), 0, t(x + w), t(y + d), t(h))
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
                # A Laconian pan tile: a shallow trough, its hollow up, each row lying on the one below.
                rad = (bb - a) / 2
                low = tuple(p_low[i] + normal[i] * (lift + rad * 0.55) for i in range(3))
                high = tuple(p_high[i] + normal[i] * (rad * 0.55) for i in range(3))
                b.shell(tuple(t(v) for v in low), tuple(t(v) for v in high), t(rad), t(0.016), tuple(normal), "roof_tile",
                        sides=10, stage=share, tint=rng.uniform(0.25, 0.75), hollow_up=True)
            for c in range(cols + 1):  # cover tiles over the joints
                along = min(max(c * pan_w, 0.06), run_len - 0.06)
                p_low = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, along, f0)
                p_high = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, along, f1)
                normal = _roof_normal(span, rise, along_x, side)
                lift = thick * 1.2 + 0.075 * 0.55
                start = tuple(t(p_low[i] + normal[i] * lift) for i in range(3))
                end = tuple(t(p_high[i] + normal[i] * lift) for i in range(3))
                b.shell(start, end, t(0.075), t(0.014), tuple(normal), "roof_tile", sides=10, stage=share,
                        tint=rng.uniform(0.2, 0.6))
            if antefix and r == 0:  # an antefix closes each cover tile at the eaves: a palmette of seven leaves
                for c in range(cols + 1):
                    along = min(max(c * pan_w, 0.06), run_len - 0.06)
                    p = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, along, 0.0)
                    palmette(b, p, along_x, side, 0.13, stage=share)
    # The verges: the roof's ends closed by a barge board along each gable slope and a row of cover tiles on
    # its edge (no open edge of the tiles or the boards shows from the side).
    normal_s = {side: _roof_normal(span, rise, along_x, side) for side in (-1, 1)}
    for along in (0.0, run_len):
        for side in (-1, 1):
            n = normal_s[side]
            lo = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, along, 0.0)
            hi = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, along, 1.0)
            out = -1 if along == 0.0 else 1
            board_c = tuple(t((lo[i] + hi[i]) / 2 - n[i] * 0.04) for i in range(3))
            half_up = tuple(t((hi[i] - lo[i]) / 2) for i in range(3))
            half_t = (t(0.025 * out), 0, 0) if along_x else (0, t(0.025 * out), 0)
            half_n = tuple(t(n[i] * 0.09) for i in range(3))
            if along_x:
                board_c = (board_c[0] + t(0.025 * out), board_c[1], board_c[2])
            else:
                board_c = (board_c[0], board_c[1] + t(0.025 * out), board_c[2])
            b.obox(board_c, half_t, half_up, half_n, "dark_wood", stage=s1, tint=0.45)
            edge = 0.05 if along == 0.0 else run_len - 0.05
            rows_v = max(2, rows)
            for r in range(rows_v):
                f0, f1 = r / rows_v, min(1.0, (r + 1.15) / rows_v)
                pa = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, edge, f0)
                pb = _roof_point(x0, y0, x1, y1, z, rise, along_x, side, edge, f1)
                lift = thick * 1.2 + 0.085 * 0.55
                b.shell(tuple(t(pa[i] + n[i] * lift) for i in range(3)), tuple(t(pb[i] + n[i] * lift) for i in range(3)),
                        t(0.085), t(0.014), tuple(n), "roof_tile", sides=10, stage=s0 + (s1 - s0) * r / rows_v,
                        tint=rng.uniform(0.2, 0.5))
    ridge_n = max(1, int(run_len / 0.5))
    for i in range(ridge_n):  # the ridge
        a0 = run_len * i / ridge_n
        a1 = run_len * (i + 1) / ridge_n + 0.03
        p0 = _roof_point(x0, y0, x1, y1, z, rise, along_x, 1, a0, 1.0)
        p1 = _roof_point(x0, y0, x1, y1, z, rise, along_x, 1, min(a1, run_len), 1.0)
        b.shell(tuple(t(v) for v in (p0[0], p0[1], p0[2] - 0.02)), tuple(t(v) for v in (p1[0], p1[1], p1[2] - 0.02)),
                t(0.12), t(0.016), (0, 0, 1), "roof_tile", sides=12, stage=s1, tint=rng.uniform(0.3, 0.5))
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


def potted_plant(b, x, y, scale=1.0, stage=0.95, rng=None, name="plant", z=0.0):
    """A terracotta pot with a herb or a young laurel: leaves on short stems round a stalk."""
    hp, rp = 0.32 * scale, 0.16 * scale
    b.prop(name, t(x - rp * 1.8), t(y - rp * 1.8), t(z), t(x + rp * 1.8), t(y + rp * 1.8), t(z + hp + 0.55 * scale))
    if not b.detail:
        return
    b.lathe(t(x), t(y), t(z), [(t(rp * 0.6), 0), (t(rp * 0.8), t(hp * 0.4)), (t(rp), t(hp * 0.9)), (t(rp * 1.12), t(hp)),
                            (t(rp * 1.0), t(hp))], "roof_tile", sides=24, stage=stage, top="dark_wood", smooth=2)
    for i in range(26):
        a = rng.uniform(0, 6.28)
        up = rng.uniform(0.05, 0.5) * scale
        reach = rng.uniform(0.05, 0.26) * scale * (1.0 - up)
        root = (x + 0.02 * math.cos(a), y + 0.02 * math.sin(a), z + hp + up * 0.6)
        tip = (x + reach * math.cos(a), y + reach * math.sin(a), z + hp + up + 0.08)
        b.strand(tuple(t(v) for v in root), tuple(t(v) for v in tip), t(rng.uniform(0.03, 0.05)), "leaves", stage=stage,
                 tint=rng.uniform(0.2, 0.8))


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


def doric_column(b, x, y, z, height, radius=0.16, stage=0.5):
    """A Doric column: a fluted shaft narrowing upwards with a slight swelling (entasis), the echinus and the
    abacus."""
    b.solid("column", t(x - radius * 1.4), t(y - radius * 1.4), t(z), t(x + radius * 1.4), t(y + radius * 1.4), t(z + height))
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
