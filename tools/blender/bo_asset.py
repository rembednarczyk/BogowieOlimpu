"""Shared building blocks of the Blender asset pipeline (docs/ASSETY.md, R-017). Imported by the model scripts in
tools/blender/models/; runs inside Blender (bpy), headless.

Conventions:
- units: 1 tile = 1 unit = 2 real metres (a person of 1.8 m is 0.9 tile, as the walkers in the game); the
  scripts give sizes in real metres through M;
- the footprint's north-west corner is the origin; the building covers x 0..w and y 0..-h in Blender (glTF and
  Godot: x 0..w, z 0..h), the front faces -Y (south, towards the camera);
- one object, one material, one draw call: the part's colour is an index into PALETTE (UV0.x) and a shade of it
  per element (UV0.y, 0.5 = as is: bricks and tiles each a little different), the masks for the building's
  parameters are vertex colours (R the share of the building phase after which the element shows, 1 = the
  scaffold; G plaster; B loose; A animated), the pivot of an animated part is UV1 (x, z in tiles);
  game/shaders/building.gdshader reads them;
- the export is deterministic: the same script gives the same bytes.
"""
import json
import math
import random
import pathlib

import bmesh
import bpy

REPO = pathlib.Path(__file__).resolve().parents[1].parent
M = 0.5  # one real metre in tiles

# The parts' colours: stylized realism, stage 1a (docs/08-OPRAWA.md). The same order as PALETTE in
# game/shaders/building.gdshader (tools/checks/check_models.py compares them).
PALETTE = [  # warm and saturated, the light of a summer afternoon (stage 1a, 2026-11-28)
    ("plaster", (0.96, 0.91, 0.80)),  # lime plaster, whitewash
    ("stone", (0.82, 0.77, 0.66)),  # limestone
    ("roof_tile", (0.82, 0.40, 0.26)),  # terracotta tiles
    ("wood", (0.60, 0.42, 0.26)),
    ("mudbrick", (0.80, 0.64, 0.46)),
    ("thatch", (0.80, 0.68, 0.40)),
    ("water", (0.32, 0.62, 0.78)),
    ("marble", (0.95, 0.94, 0.90)),
    ("bronze", (0.66, 0.48, 0.24)),
    ("cloth", (0.78, 0.26, 0.20)),  # red ochre awnings
    ("dark_wood", (0.36, 0.25, 0.16)),
    ("grain", (0.90, 0.76, 0.40)),
    ("mortar", (0.70, 0.63, 0.52)),  # the lime and clay between the bricks
    ("ochre", (0.88, 0.56, 0.26)),  # painted bands and shutters
    ("blue_paint", (0.22, 0.52, 0.56)),  # teal paint on doors and shutters
    ("leaves", (0.38, 0.58, 0.26)),
    ("end_grain", (0.84, 0.70, 0.48)),  # a log's cut face
    ("bark", (0.40, 0.31, 0.22)),
    ("linen", (0.90, 0.85, 0.72)),  # undyed cloth
    ("hide", (0.62, 0.47, 0.32)),  # tanned hides
    ("rope", (0.70, 0.60, 0.40)),
    ("oil", (0.30, 0.26, 0.08)),  # olive oil in a jar's mouth
    ("iron", (0.30, 0.30, 0.34)),
    ("grass", (0.50, 0.70, 0.30)),  # tufts round the walls
    ("flower_red", (0.90, 0.30, 0.34)),
    ("flower_yellow", (0.98, 0.82, 0.30)),
    ("flower_white", (0.97, 0.95, 0.90)),
    ("sky_cloth", (0.56, 0.72, 0.88)),  # washing on the line
]
PALETTE_INDEX = {name: i for i, (name, _) in enumerate(PALETTE)}
PALETTE_SIZE = 32  # UV0.x = (index + 0.5) / PALETTE_SIZE

FOUNDATION, WALLS, ROOF, SCAFFOLD = 0.0, 1.0 / 3.0, 2.0 / 3.0, 1.0
STILL, FLOW, ROTATE = 0.0, 0.5, 1.0  # kinds of animated part (alpha)


def footprint(building_id):
    """(width, height) in tiles from data/building_stats.json; common housing is a 2x2 plot (S-004)."""
    if building_id == "common_housing":
        return 2, 2
    stats = {o["id"]: o for o in json.loads((REPO / "data" / "building_stats.json").read_text())}
    w, h = stats[building_id]["size"].replace("×", "x").split("x")[:2]
    return int(w), int("".join(c for c in h if c.isdigit()))


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = "METRIC"


class Builder:
    """Collects the parts of one building into one mesh."""

    def __init__(self, name, width, height, detail=True):
        self.detail = detail  # False: the far view's simple shapes (LOD1)
        self.name = name
        self.width = width
        self.height = height
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new("UVMap")
        self.pivot = self.bm.loops.layers.uv.new("Pivot")
        self.col = self.bm.loops.layers.float_color.new("Mask")
        self.solids = []  # (name, aabb, part of the house) of the building's walls and footings, in tiles
        self.roofs = []   # (part of the house, points on its slopes): a roof must not go into another part's walls
        self.props = []   # (name, aabb) of the things around it: jars, wood, carts
        self.two_sided = set()  # faces drawn from both sides on purpose: their normals stay as built

    # Props must not go into the building or into each other (the owner's rule, 2026-11-27); finish() checks.
    def solid(self, name, x0, y0, z0, x1, y1, z1, block="main"):
        self.solids.append((name, (min(x0, x1), min(y0, y1), min(z0, z1), max(x0, x1), max(y0, y1), max(z0, z1)), block))

    def roof_points(self, block, points):
        self.roofs.append((block, points))

    def prop(self, name, x0, y0, z0, x1, y1, z1):
        self.props.append((name, (min(x0, x1), min(y0, y1), min(z0, z1), max(x0, x1), max(y0, y1), max(z0, z1))))

    def check_props(self, eps=0.004):
        def hit(a, b):
            return all(a[i] < b[i + 3] - eps and b[i] < a[i + 3] - eps for i in range(3))
        bad = []
        for i, (pn, pa) in enumerate(self.props):
            if pa[0] < -eps or pa[1] < -eps or pa[3] > self.width + eps or pa[4] > self.height + eps:
                bad.append(f"{pn} leaves the plot")
            for sn, sa, _ in self.solids:
                if hit(pa, sa):
                    bad.append(f"{pn} goes into {sn}")
            for qn, qa in self.props[i + 1:]:
                if hit(pa, qa):
                    bad.append(f"{pn} goes into {qn}")
        e = 0.006  # a roof of one part must not pass through the walls of another (the owner's rule, 2026-11-28)
        for block, pts in self.roofs:
            for sn, sa, sb in self.solids:
                if sb == block:
                    continue
                for p in pts:
                    if all(sa[i] + e < p[i] < sa[i + 3] - e for i in range(3)):
                        bad.append(f"the roof of {block} goes into the {sn} of {sb} at {tuple(round(v, 3) for v in p)}")
                        break
        if bad:
            raise ValueError(self.name + ": " + "; ".join(bad))

    # A good the house keeps (data.h HouseGood): its props show only while the house holds it. Stored in UV1.x as
    # -1 - index (the pivot of animated parts is >= 0), read by building.gdshader.
    GOODS = {"food": 0, "fleece": 1, "olive_oil": 2, "wine": 3}
    good = None

    def _tag(self, faces, part, stage, plaster, loose, anim, pivot, tint=0.5):
        if self.good is not None and pivot == (0, 0):
            pivot = (-1.0 - self.GOODS[self.good], 0.0)
        u = (PALETTE_INDEX[part] + 0.5) / PALETTE_SIZE
        for f in faces:
            for loop in f.loops:
                loop[self.uv].uv = (u, tint)
                loop[self.pivot].uv = pivot
                loop[self.col] = (stage, 1.0 if plaster else 0.0, 1.0 if loose else 0.0, anim)

    def box(self, x, y, z, sx, sy, sz, part, stage=WALLS, plaster=False, loose=False, anim=STILL, pivot=(0, 0),
            tint=0.5):
        """An axis-aligned box: (x, y) its north-west corner in tiles from the footprint's corner (y grows
        south), z its bottom; sizes in tiles."""
        verts = [self.bm.verts.new((x + dx * sx, -(y + dy * sy), z + dz * sz))
                 for dz in (0, 1) for dy in (0, 1) for dx in (0, 1)]
        idx = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
        faces = [self.bm.faces.new([verts[i] for i in q]) for q in idx]
        self._tag(faces, part, stage, plaster, loose, anim, pivot, tint)
        return faces

    def cbox(self, x, y, z, sx, sy, sz, chamfer, part, stage=WALLS, plaster=False, tint=0.5):
        """An axis-aligned box with its edges chamfered by `chamfer` (tiles): a brick or a stone block that
        catches the light on its edges. 26 faces."""
        c = min(chamfer, sx / 3, sy / 3, sz / 3)
        hx, hy, hz = sx / 2, sy / 2, sz / 2
        cx, cy, cz = x + hx, y + hy, z + hz
        verts = {}
        for s_x in (-1, 1):
            for s_y in (-1, 1):
                for s_z in (-1, 1):
                    k = (s_x, s_y, s_z)
                    verts[k] = [self.bm.verts.new(self._b((cx + s_x * hx, cy + s_y * (hy - c), cz + s_z * (hz - c)))),
                                self.bm.verts.new(self._b((cx + s_x * (hx - c), cy + s_y * hy, cz + s_z * (hz - c)))),
                                self.bm.verts.new(self._b((cx + s_x * (hx - c), cy + s_y * (hy - c), cz + s_z * hz)))]
        faces = []
        order = {0: [(-1, -1), (1, -1), (1, 1), (-1, 1)]}
        for axis in range(3):  # the six main faces
            for sgn in (-1, 1):
                quad = []
                for a, b2 in order[0]:
                    k = [0, 0, 0]
                    k[axis] = sgn
                    others = [i for i in range(3) if i != axis]
                    k[others[0]], k[others[1]] = a, b2
                    quad.append(verts[tuple(k)][axis])
                faces.append(self.bm.faces.new(quad))
        for a1 in range(3):  # the twelve chamfers along the edges
            for a2 in range(a1 + 1, 3):
                free = 3 - a1 - a2
                for s1 in (-1, 1):
                    for s2 in (-1, 1):
                        k0 = [0, 0, 0]; k1 = [0, 0, 0]
                        k0[a1] = k1[a1] = s1
                        k0[a2] = k1[a2] = s2
                        k0[free], k1[free] = -1, 1
                        faces.append(self.bm.faces.new((verts[tuple(k0)][a1], verts[tuple(k0)][a2], verts[tuple(k1)][a2],
                                                        verts[tuple(k1)][a1])))
        for k, (fx, fy, fz) in verts.items():  # the eight corners
            faces.append(self.bm.faces.new((fx, fy, fz)))
        self._tag(faces, part, stage, plaster, False, STILL, (0, 0), tint)
        return faces

    def shell(self, start, end, radius, thick, up, part, sides=10, stage=ROOF, tint=0.5, hollow_up=False):
        """A curved tile with its thickness: a half pipe from `start` to `end` (tiles), closed on every side.
        `up` the side its round back faces; `hollow_up` turns it into a trough (a pan tile)."""
        import mathutils
        a = mathutils.Vector(start)
        e = mathutils.Vector(end)
        axis = (e - a).normalized()
        u = mathutils.Vector(up)
        u = (u - axis * u.dot(axis)).normalized()
        if hollow_up:
            u = -u
        side = axis.cross(u)
        def ring(p, r):
            return [self.bm.verts.new(self._b(p + side * (r * math.cos(math.pi * i / sides)) +
                                               u * (r * math.sin(math.pi * i / sides)))) for i in range(sides + 1)]
        oa, ia = ring(a, radius), ring(a, radius - thick)
        oe, ie = ring(e, radius), ring(e, radius - thick)
        faces = []
        for i in range(sides):
            faces.append(self.bm.faces.new((oa[i], oe[i], oe[i + 1], oa[i + 1])))   # the outer face
            faces.append(self.bm.faces.new((ia[i], ia[i + 1], ie[i + 1], ie[i])))   # the inner face
            faces.append(self.bm.faces.new((oa[i], oa[i + 1], ia[i + 1], ia[i])))   # the end at start
            faces.append(self.bm.faces.new((oe[i], ie[i], ie[i + 1], oe[i + 1])))   # the end at end
        for j in (0, sides):  # the two long edges
            faces.append(self.bm.faces.new((oa[j], ia[j], ie[j], oe[j])))
        self._tag(faces, part, stage, False, False, STILL, (0, 0), tint)
        return faces

    def obox(self, center, ax, ay, az, part, stage=WALLS, plaster=False, loose=False, tint=0.5):
        """A box of any orientation: `center` (x, y, z) in tiles (y grows south), `ax`, `ay`, `az` its half-axis
        vectors in the same frame (their lengths are the half sizes)."""
        cx, cy, cz = center
        verts = []
        for dz in (-1, 1):
            for dy in (-1, 1):
                for dx in (-1, 1):
                    x = cx + dx * ax[0] + dy * ay[0] + dz * az[0]
                    y = cy + dx * ax[1] + dy * ay[1] + dz * az[1]
                    z = cz + dx * ax[2] + dy * ay[2] + dz * az[2]
                    verts.append(self.bm.verts.new((x, -y, z)))
        idx = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
        faces = [self.bm.faces.new([verts[i] for i in q]) for q in idx]
        self._tag(faces, part, stage, plaster, loose, STILL, (0, 0), tint)
        return faces

    def half_tube(self, start, end, radius, up, part, sides=4, stage=ROOF, tint=0.5, two_sided=False):
        """A half cylinder from `start` to `end` (tiles), its round side towards `up` (a cover tile)."""
        import mathutils
        a = mathutils.Vector(start)
        e = mathutils.Vector(end)
        axis = (e - a).normalized()
        u = mathutils.Vector(up)
        u = (u - axis * u.dot(axis)).normalized()
        side = axis.cross(u)
        rings = []
        for p in (a, e):
            ring = []
            for i in range(sides + 1):
                t = math.pi * i / sides
                q = p + side * (radius * math.cos(t)) + u * (radius * math.sin(t))
                ring.append(self.bm.verts.new((q.x, -q.y, q.z)))
            rings.append(ring)
        faces = [self.bm.faces.new((rings[0][i], rings[0][i + 1], rings[1][i + 1], rings[1][i])) for i in range(sides)]
        if two_sided:  # a trough seen from inside (a pan tile): both windings, normals kept as built
            back = [self.bm.faces.new([self.bm.verts.new(v.co) for v in reversed(f.verts)]) for f in faces]
            faces += back
            self.two_sided.update(faces)
        self._tag(faces, part, stage, False, False, STILL, (0, 0), tint)
        return faces

    def lathe(self, cx, cy, z, profile, part, sides=28, stage=WALLS, tint=0.5, top=None, shape=None, smooth=0):
        """A closed body of revolution about the vertical through (cx, cy): `profile` (radius, height) from the
        bottom up, in tiles; the bottom is capped, the top capped with `top` (a part) or left to the profile."""
        if smooth and len(profile) >= 3:  # Catmull-Rom through the control points: a smooth body
            pts = [profile[0]] + list(profile) + [profile[-1]]
            dense = []
            for k in range(1, len(pts) - 2):
                p0, p1, p2, p3 = pts[k - 1], pts[k], pts[k + 1], pts[k + 2]
                for s in range(smooth):
                    f = s / smooth
                    dense.append(tuple(0.5 * ((2 * p1[i]) + (-p0[i] + p2[i]) * f + (2 * p0[i] - 5 * p1[i] + 4 * p2[i] - p3[i]) * f * f +
                                              (-p0[i] + 3 * p1[i] - 3 * p2[i] + p3[i]) * f ** 3) for i in range(2)))
            dense.append(profile[-1])
            profile = dense
        rings = []
        for r, h in profile:
            ring = []
            for i in range(sides):
                a = 2 * math.pi * i / sides
                rr = r * (shape(a) if shape else 1.0)  # e.g. a column's flutes
                ring.append(self.bm.verts.new((cx + rr * math.cos(a), -(cy + rr * math.sin(a)), z + h)))
            rings.append(ring)
        faces = []
        for a, b in zip(rings, rings[1:]):
            for i in range(sides):
                faces.append(self.bm.faces.new((a[i], a[(i + 1) % sides], b[(i + 1) % sides], b[i])))
        faces.append(self.bm.faces.new(list(reversed(rings[0]))))
        self._tag(faces, part, stage, False, False, STILL, (0, 0), tint)
        cap = self.bm.faces.new(rings[-1])
        self._tag([cap], top or part, stage, False, False, STILL, (0, 0), tint)
        return faces

    def tube_path(self, points, radius, part, sides=8, stage=WALLS, tint=0.5, caps=True):
        """A closed tube along `points` (x, y, z in tiles, y grows south): handles, ropes, branches."""
        import mathutils
        pts = [mathutils.Vector(p) for p in points]
        rings = []
        for k, p in enumerate(pts):
            d = (pts[min(k + 1, len(pts) - 1)] - pts[max(k - 1, 0)]).normalized()
            ref = mathutils.Vector((0, 0, 1)) if abs(d.z) < 0.9 else mathutils.Vector((1, 0, 0))
            u = d.cross(ref).normalized()
            v = d.cross(u).normalized()
            r = radius(k / max(len(pts) - 1, 1)) if callable(radius) else radius
            ring = []
            for i in range(sides):
                a = 2 * math.pi * i / sides
                q = p + u * (r * math.cos(a)) + v * (r * math.sin(a))
                ring.append(self.bm.verts.new((q.x, -q.y, q.z)))
            rings.append(ring)
        faces = []
        for a, b in zip(rings, rings[1:]):
            for i in range(sides):
                faces.append(self.bm.faces.new((a[i], a[(i + 1) % sides], b[(i + 1) % sides], b[i])))
        if caps:
            faces.append(self.bm.faces.new(list(reversed(rings[0]))))
            faces.append(self.bm.faces.new(rings[-1]))
        self._tag(faces, part, stage, False, False, STILL, (0, 0), tint)
        return faces

    def log(self, start, end, radius, sides=10, stage=WALLS, tint=0.5, split=None, grain_tint=0.5):
        """A log from `start` to `end` (tiles): bark round it, the cut faces in end grain; `split` "half" or
        "quarter" cuts it along its length, the split face in end grain too."""
        import mathutils
        a = mathutils.Vector(start)
        e = mathutils.Vector(end)
        d = (e - a).normalized()
        ref = mathutils.Vector((0, 0, 1)) if abs(d.z) < 0.9 else mathutils.Vector((1, 0, 0))
        u = d.cross(ref).normalized()
        v = d.cross(u).normalized()
        if v.z < 0:  # a split piece lies with its flat face down: its arc rises from the axis
            v = -v
            u = -u
        span = {None: 2 * math.pi, "half": math.pi, "quarter": math.pi / 2}[split]
        steps = sides if split is None else max(3, int(sides * span / (2 * math.pi)) + 1)
        def ring(p):
            pts = []
            for i in range(steps if split is None else steps + 1):
                ang = span * i / steps
                q = p + u * (radius * math.cos(ang)) + v * (radius * math.sin(ang))
                pts.append(self.bm.verts.new((q.x, -q.y, q.z)))
            if split is not None:
                pts.append(self.bm.verts.new((p.x, -p.y, p.z)))  # the axis: the split faces meet here
            return pts
        r0, r1 = ring(a), ring(e)
        n = len(r0)
        bark, grain = [], []
        for i in range(n):
            j = (i + 1) % n
            f = self.bm.faces.new((r0[i], r0[j], r1[j], r1[i]))
            (bark if split is None or i < n - 2 else grain).append(f)
        grain.append(self.bm.faces.new(list(reversed(r0))))
        grain.append(self.bm.faces.new(r1))
        self._tag(bark, "bark", stage, False, False, STILL, (0, 0), tint)
        self._tag(grain, "end_grain", stage, False, False, STILL, (0, 0), grain_tint)
        return bark + grain

    def surface(self, fn, nu, nv, part, stage=WALLS, tint=0.5, both=True):
        """A grid surface: fn(u, v) in 0..1 gives (x, y, z) in tiles; cloth, hides, mats. Both sides drawn."""
        grid = [[self.bm.verts.new(self._b(fn(i / nu, j / nv))) for i in range(nu + 1)] for j in range(nv + 1)]
        faces = []
        for j in range(nv):
            for i in range(nu):
                q = (grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i])
                faces.append(self.bm.faces.new(q))
        if both:
            back = [[self.bm.verts.new(v.co) for v in row] for row in grid]
            for j in range(nv):
                for i in range(nu):
                    faces.append(self.bm.faces.new((back[j][i], back[j + 1][i], back[j + 1][i + 1], back[j][i + 1])))
        self._tag(faces, part, stage, False, False, STILL, (0, 0), tint)
        self.two_sided.update(faces)
        return faces

    def strand(self, root, tip, width, part, stage=WALLS, tint=0.5):
        """A blade of straw or grass: a thin quad from `root` to `tip` (tiles), seen from both sides."""
        import mathutils
        r = mathutils.Vector(root)
        t_ = mathutils.Vector(tip)
        d = (t_ - r)
        side = d.cross(mathutils.Vector((0, 0, 1)))
        if side.length < 1e-6:
            side = mathutils.Vector((1, 0, 0))
        side = side.normalized() * (width / 2)
        pts = [r - side, r + side, t_ + side * 0.4, t_ - side * 0.4]
        vs = [self.bm.verts.new(self._b(p)) for p in pts]
        faces = [self.bm.faces.new(vs), self.bm.faces.new([self.bm.verts.new(v.co) for v in reversed(vs)])]
        self._tag(faces, part, stage, False, False, STILL, (0, 0), tint)
        self.two_sided.update(faces)
        return faces

    @staticmethod
    def _b(p):
        return (p[0], -p[1], p[2])

    def poly(self, points, part, stage=WALLS, plaster=False, tint=0.5):
        """One flat face through `points` (tiles, y grows south), e.g. a gable's triangle."""
        face = self.bm.faces.new([self.bm.verts.new((x, -y, z)) for x, y, z in points])
        self._tag([face], part, stage, plaster, False, STILL, (0, 0), tint)
        return [face]

    def prism(self, x, y, z, sx, sy, rise, part, along_x=True, hip=0.0, stage=ROOF, loose=False):
        """A pitched roof over the rectangle: ridge along x (or y), height `rise`; `hip` > 0 pulls the ridge's
        ends in (a hipped roof, 0..0.5 of the length)."""
        def p(px, py, pz):
            return self.bm.verts.new((px, -py, pz))
        if along_x:
            a = p(x, y, z); b = p(x + sx, y, z); c = p(x + sx, y + sy, z); d = p(x, y + sy, z)
            r0 = p(x + hip * sx, y + sy / 2, z + rise); r1 = p(x + sx - hip * sx, y + sy / 2, z + rise)
            quads = [(a, b, r1, r0), (c, d, r0, r1)]
            tris = [(b, c, r1), (d, a, r0)]
        else:
            a = p(x, y, z); b = p(x + sx, y, z); c = p(x + sx, y + sy, z); d = p(x, y + sy, z)
            r0 = p(x + sx / 2, y + hip * sy, z + rise); r1 = p(x + sx / 2, y + sy - hip * sy, z + rise)
            quads = [(b, c, r1, r0), (d, a, r0, r1)]
            tris = [(a, b, r0), (c, d, r1)]
        faces = [self.bm.faces.new(q) for q in quads] + [self.bm.faces.new(t) for t in tris]
        self._tag(faces, part, stage, False, loose, STILL, (0, 0))
        return faces

    def column(self, cx, cy, z, radius, height, part="stone", sides=8, stage=WALLS, anim=STILL, pivot=(0, 0),
               tint=0.5):
        """A column (or post) centred on (cx, cy)."""
        bottom = [self.bm.verts.new((cx + radius * math.cos(2 * math.pi * i / sides),
                                     -(cy + radius * math.sin(2 * math.pi * i / sides)), z)) for i in range(sides)]
        top = [self.bm.verts.new((v.co.x, v.co.y, z + height)) for v in bottom]
        faces = [self.bm.faces.new((bottom[i], bottom[(i + 1) % sides], top[(i + 1) % sides], top[i]))
                 for i in range(sides)]
        faces.append(self.bm.faces.new(list(reversed(bottom))))
        faces.append(self.bm.faces.new(top))
        self._tag(faces, part, stage, False, False, anim, pivot, tint)
        return faces

    def scaffold(self, margin=0.06, height=None):
        """Poles at the corners and the middle of each side, shown only while the building goes up."""
        top = height if height is not None else 2.4 * M
        for px in (-margin, self.width / 2, self.width + margin):
            for py in (-margin, self.height / 2, self.height + margin):
                if px == self.width / 2 and py == self.height / 2:
                    continue
                cx = min(max(px, 0.03), self.width - 0.03)
                cy = min(max(py, 0.03), self.height - 0.03)
                self.column(cx, cy, 0, 0.03, top, part="wood", sides=4, stage=SCAFFOLD)

    # Hand-made irregularity (owner, 2026-11-28, "Ghibli"): one smooth field bends the whole model, props and all,
    # so nothing that was clear of a wall goes into it: the building leans a little, its walls swell out at mid
    # height like a soft loaf, the roofline waves. Fixed at the ground (the plot and the footing stay), seeded by
    # the model's name, the same every run. In tiles; at most a few centimetres.
    wonk = 1.0

    def _wonk(self):
        if not self.wonk or not self.bm.verts:
            return
        # Long flat faces (a plaster coat, a slab) get more vertices first, or they would stay flat while the
        # bricks under them bend, and the bricks would show through.
        for _ in range(4 if self.detail else 0):  # the far view keeps its budget: only the lean, no swell
            long_edges = [e for e in self.bm.edges if e.calc_length() > 0.3 and all(len(f.verts) == 4 for f in e.link_faces)]
            if not long_edges:
                break
            bmesh.ops.subdivide_edges(self.bm, edges=long_edges, cuts=1, use_grid_fill=True)
        rnd = random.Random("wonk " + self.name.replace("_lod1", ""))
        top = max(v.co.z for v in self.bm.verts) or 1.0
        lean = (rnd.uniform(-1, 1) * 0.012 * self.wonk, rnd.uniform(-1, 1) * 0.012 * self.wonk)
        swell = rnd.uniform(0.02, 0.035) * self.wonk * (1.0 if self.detail else 0.0)
        wave, ph = rnd.uniform(0.010, 0.018) * self.wonk * (1.0 if self.detail else 0.0), rnd.uniform(0, 6.28)
        cx, cy = self.width / 2, -self.height / 2
        for v in self.bm.verts:
            x, y, z = v.co
            f = min(max(z / top, 0.0), 1.0)
            bulge = swell * math.sin(math.pi * min(f * 1.3, 1.0))  # most at about two thirds of the height
            nx, ny = x + lean[0] * z + (x - cx) * bulge, y - lean[1] * z + (y - cy) * bulge
            v.co.x = min(max(nx, 0.0), self.width)
            v.co.y = min(max(ny, -self.height), 0.0)
            v.co.z = z + wave * f * f * math.sin(x * 2.1 + ph) * math.cos(y * 1.7 - ph)

    mirror = False  # the house built the other way round (a story's variant, bo_story.py)

    def _mirror(self):
        if not self.mirror:
            return
        for v in self.bm.verts:  # the windings flip with it: finish() recalculates the normals, and the faces
            v.co.x = self.width - v.co.x  # seen from both sides have both windings anyway

    def finish(self):
        self.check_props()
        self._mirror()
        self._wonk()
        bmesh.ops.recalc_face_normals(self.bm, faces=[f for f in self.bm.faces if f not in self.two_sided])
        mesh = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        mesh.color_attributes.active_color = mesh.color_attributes["Mask"]
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        mat = bpy.data.materials.new("building")  # Godot replaces it with building.gdshader
        mesh.materials.append(mat)
        for poly in mesh.polygons:
            poly.use_smooth = False
        return obj


def export(obj, out):
    out = pathlib.Path(out)
    out.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.gltf(filepath=str(out), export_format="GLB", use_selection=True, export_yup=True,
                              export_apply=True, export_texcoords=True, export_normals=True,
                              export_vertex_color="NAME", export_vertex_color_name="Mask", export_all_vertex_colors=False,
                              export_materials="PLACEHOLDER", export_animations=False, export_extras=False)
    print("EXPORTED", out)
