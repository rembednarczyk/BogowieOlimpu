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
import pathlib

import bmesh
import bpy

REPO = pathlib.Path(__file__).resolve().parents[1].parent
M = 0.5  # one real metre in tiles

# The parts' colours: stylized realism, stage 1a (docs/08-OPRAWA.md). The same order as PALETTE in
# game/shaders/building.gdshader (tools/checks/check_models.py compares them).
PALETTE = [
    ("plaster", (0.93, 0.89, 0.80)),     # lime plaster, whitewash
    ("stone", (0.78, 0.74, 0.66)),       # limestone
    ("roof_tile", (0.72, 0.36, 0.24)),   # terracotta tiles
    ("wood", (0.52, 0.36, 0.22)),
    ("mudbrick", (0.74, 0.60, 0.44)),
    ("thatch", (0.70, 0.60, 0.36)),
    ("water", (0.30, 0.55, 0.72)),
    ("marble", (0.94, 0.93, 0.90)),
    ("bronze", (0.62, 0.45, 0.22)),
    ("cloth", (0.70, 0.22, 0.18)),       # red ochre awnings
    ("dark_wood", (0.32, 0.22, 0.14)),
    ("grain", (0.86, 0.72, 0.38)),
    ("mortar", (0.66, 0.60, 0.50)),      # the lime and clay between the bricks
    ("ochre", (0.78, 0.48, 0.24)),       # painted bands and shutters
    ("blue_paint", (0.30, 0.42, 0.56)),  # painted doors and frames
    ("leaves", (0.40, 0.48, 0.26)),
]
PALETTE_INDEX = {name: i for i, (name, _) in enumerate(PALETTE)}
PALETTE_SIZE = 16  # UV0.x = (index + 0.5) / PALETTE_SIZE

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

    def _tag(self, faces, part, stage, plaster, loose, anim, pivot, tint=0.5):
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

    def half_tube(self, start, end, radius, up, part, sides=4, stage=ROOF, tint=0.5):
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
        self._tag(faces, part, stage, False, False, STILL, (0, 0), tint)
        return faces

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

    def finish(self):
        bmesh.ops.recalc_face_normals(self.bm, faces=self.bm.faces)
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
