"""A house falling down (owner, 2026-11-28: "realistic physics of collapse; the ruins look like that, with the
animation"). Every piece the model is built of keeps its shape (a brick, a tile, a beam, a column); pieces of a
material bonded together fall together (a wall section of bricks with its plaster, a few tiles in their mortar),
a thin coat goes with what it lies on, a thatch or canvas tears into ragged shards (MATERIALS); the footing and what lies on the ground stay. Each chunk falls under gravity from the
moment the walls give way, from the foot up, wall sections tipping outward and the way the house leaned, tumbling; it hits the
ground or the rubble already lying there (a height map of 10 cm cells that grows as chunks come to rest),
bounces a little, loses its speed, rolls onto a face and stays. Nothing leaves the plot. The last frame is the
ruin.

Output: <name>_ruin.glb, the mesh at rest with each vertex's chunk in UV1.x, and <name>_ruin.bin, each chunk's
transform per frame (Godot's axes: x, y up, z), read by building.gdshader (the `collapse` instance uniform, the
seconds since it began) through a float texture: width chunks x 3 (the matrix's rows), height frames.
bin: "BORU", int32 frames, int32 chunks, int32 fps, then float32 [frame][chunk][3 rows][4].
Deterministic: the same chunks and the same fall every run (seeded by the model's name).
"""
import math
import os
import pathlib
import random
import struct

import bmesh
import mathutils
import mathutils.kdtree
import bo_asset as A
import numpy
from mathutils import Matrix, Quaternion, Vector


GROUND = 0.16  # tiles (32 cm): pieces lower than this stay (the footing, the paving, the grass)
G = 9.81 / 2.0  # tiles/s² (1 tile = 2 m)
FPS = 24
SECONDS = 2.5
HM = 0.05  # tiles (10 cm): the rubble's height map
COAT = 0.02  # tiles (4 cm): a sheet thinner than this is a coat over other pieces (plaster, mortar, underlay)
PACK = 0.5  # of a piece's height above its bed that the heap grows by (the rest goes into the gaps)
SLOPE = 0.06  # tiles (12 cm): a drop under a piece that makes it slide off
HOLD = 0.06  # tiles (12 cm): a coat this close to a piece goes with it

# How each material of the model (its part, the palette in bo_asset.py) breaks and falls. Its pieces (a brick, a
# tile, a beam) never break; bond: how far apart the points are that hold pieces together (tiles; 0: each piece
# alone, a beam falls as a beam), a wall falls in sections of its bricks with their plaster; bounce: the share of
# the speed it keeps upward on a hit; keep: of the speed along the ground; spin: how fast it tumbles; drag: air
# resistance (1/s: cloth and thatch drift down, stone does not); topple: a tall section tips over outward from
# its foot.
MATERIALS = {
    "masonry": dict(bond=0.30, bounce=0.08, keep=0.45, spin=0.6, drag=0.0, topple=1.0),
    "tile": dict(bond=0.07, bounce=0.30, keep=0.60, spin=1.6, drag=0.0, topple=0.0),
    "wood": dict(bond=0.0, bounce=0.22, keep=0.70, spin=0.7, drag=0.0, topple=0.4),
    "soft": dict(bond=0.22, bounce=0.00, keep=0.25, spin=0.5, drag=2.5, topple=0.0),
    "metal": dict(bond=0.0, bounce=0.35, keep=0.60, spin=1.2, drag=0.0, topple=0.0),
}
PARTS = {
    "tile": ("roof_tile", "roof_slate", "roof_gold", "roof_brown", "roof_patina"),
    "wood": ("wood", "dark_wood", "bark", "end_grain"),
    "soft": ("thatch", "cloth", "linen", "hide", "sky_cloth", "sage_cloth", "rope", "leaves", "grass", "flower_red",
             "flower_yellow", "flower_white"),
    "metal": ("bronze", "iron"),
}  # the rest (plaster, stone, mudbrick, marble, mortar, paints) is masonry


def _islands(bm):
    parent = list(range(len(bm.verts)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    for f in bm.faces:
        a = find(f.verts[0].index)
        for v in f.verts[1:]:
            b = find(v.index)
            if a != b:
                parent[b] = a
    groups = {}
    for f in bm.faces:
        groups.setdefault(find(f.verts[0].index), []).append(f)
    return list(groups.values())


def ruin(b, out_dir, name):
    """Turns the Builder's finished model (before finish()) into the ruin's two files."""
    rng = random.Random("ruin " + name)
    b._mirror()
    b._wonk()
    bm = b.bm
    # Nothing of the building site and nothing the family owned: the scaffold and the goods go.
    dead = [f for f in bm.faces if f.loops[0][b.col][0] > 0.995 or f.loops[0][b.pivot].uv[0] < -0.5]
    bmesh.ops.delete(bm, geom=dead, context="FACES")
    kind_of = {i: k for k, names in PARTS.items() for i, (n, _) in enumerate(A.PALETTE) if n in names}

    def kind(f):
        return kind_of.get(int(f.loops[0][b.uv].uv[0] * A.PALETTE_SIZE), "masonry")
    # A sheet of thatch or canvas tears along its own weave: its big faces are cut fine first, so that a torn
    # shard has a ragged edge and is not a cut-out triangle.
    for _ in range(4):
        long_edges = [e for e in bm.edges if e.calc_length() > 0.1 and any(kind(f) == "soft" for f in e.link_faces)]
        if not long_edges:
            break
        bmesh.ops.subdivide_edges(bm, edges=long_edges, cuts=1, use_grid_fill=True)
    bm.verts.ensure_lookup_table()
    bm.verts.index_update()
    W, H = b.width, b.height  # tiles; the mesh's y runs from 0 to -H
    # The walls break off at an uneven height: their stumps (30 cm to 1.2 m above the footing) stand.
    grid = [[rng.uniform(0.15, 0.6) for _ in range(int(H / 0.5) + 2)] for _ in range(int(W / 0.5) + 2)]

    def stump(x, y):
        gx, gy = min(max(x / 0.5, 0.0), W / 0.5), min(max(-y / 0.5, 0.0), H / 0.5)
        i, j = int(gx), int(gy)
        fx, fy = gx - i, gy - j
        a = grid[i][j] * (1 - fx) + grid[i + 1][j] * fx
        c = grid[i][j + 1] * (1 - fx) + grid[i + 1][j + 1] * fx
        return GROUND + a * (1 - fy) + c * fy
    top_all = max(v.co.z for v in bm.verts)
    # Where the pieces were bonded: points scattered through the house, each gathers the pieces nearest to it
    # (a wall section of bricks in their mortar, a few tiles stuck together). Irregular, as a break is.
    seeds = {}

    def seed_of(k, co):
        spacing = MATERIALS[k]["bond"]
        if k not in seeds:
            n = max(1, int(W * H * top_all / spacing ** 3))
            tree = mathutils.kdtree.KDTree(n)
            for i in range(n):
                tree.insert((rng.uniform(0, W), rng.uniform(-H, 0), rng.uniform(0, top_all)), i)
            tree.balance()
            seeds[k] = tree
        return seeds[k].find(co)[1]
    # Every piece the model is built of keeps its shape: a brick, a tile, a board, a beam, a stone. Only a coat
    # (a thin sheet laid over the others: plaster, a mortar bed, a thatch's underlay, a canvas) has no shape of
    # its own: it goes with what it lies on, or tears into shards where nothing holds it.
    statics, pieces, coats = [], [], []
    for faces in _islands(bm):
        vs = list({v for f in faces for v in f.verts})
        c = sum((v.co for v in vs), Vector()) / len(vs)
        if max(v.co.z for v in vs) < max(GROUND, stump(c.x, c.y)):
            statics.append(faces)
            continue
        _, he, _ = _obb(vs)
        k = kind(faces[0])
        bond = MATERIALS[k]["bond"]
        if 2 * min(he) < COAT and 2 * max(he) > 0.25:
            coats.append(faces)
        elif k in ("soft", "masonry") and 2 * max(he) > 3 * bond:  # a whole thatch, a canvas, a slab: it tears
            coats.append(faces)
        else:
            pieces.append((faces, vs, c, k))
    chunks = {}  # key -> faces
    owner = []  # (vertex position, chunk key or None for what stands)
    for faces in statics:
        for f in faces:
            owner.extend((v.co.copy(), None) for v in f.verts)
    soft_bits = []
    for faces, vs, c, k in pieces:
        if k == "soft":  # thatch's straws, a rope, leaves: with the sheet they belong to (below)
            soft_bits.append((faces, c))
            continue
        key = (k, seed_of(k, c)) if MATERIALS[k]["bond"] else (k, "piece", len(chunks))
        chunks.setdefault(key, []).extend(faces)
        owner.extend((v.co.copy(), key) for v in vs)
    tree = mathutils.kdtree.KDTree(max(1, len(owner)))
    for i, (co, _) in enumerate(owner):
        tree.insert(co, i)
    tree.balance()
    torn = []
    for faces in coats:
        for f in faces:
            c = f.calc_center_median()
            co, i, d = tree.find(c) if owner else (None, None, 1e9)
            if d is not None and d < HOLD:  # plaster on its bricks, a mortar bed under its tiles
                key = owner[i][1]
                if key is None:
                    statics.append([f])
                else:
                    chunks[key].append(f)
            else:  # nothing under it: it tears into shards
                if max(v.co.z for v in f.verts) < max(GROUND, stump(c.x, c.y)):
                    statics.append([f])
                    continue
                k = kind(f)
                key = (k, "shard", seed_of(k if MATERIALS[k]["bond"] else "soft", c))
                chunks.setdefault(key, []).append(f)
                torn.append((c, key))
    if soft_bits:
        near = mathutils.kdtree.KDTree(max(1, len(torn)))
        for i, (c, _) in enumerate(torn):
            near.insert(c, i)
        near.balance()
        for faces, c in soft_bits:
            _, i, d = near.find(c) if torn else (None, None, 1e9)
            key = torn[i][1] if d is not None and d < 0.12 else ("soft", seed_of("soft", c))
            chunks.setdefault(key, []).extend(faces)
    bodies = []
    for key in sorted(chunks, key=str):
        faces = chunks[key]
        vs = list({v for f in faces for v in f.verts})
        c0, he, axes = _obb(vs)
        bodies.append({"faces": faces, "c0": c0, "he": he, "axes": axes, "mat": MATERIALS[key[0]], "kind": key[0],
                       "pts": numpy.array([tuple(v.co - c0) for v in vs])})
    # Chunk ids into UV1.x (0 = still), the animated mask cleared.
    for f in bm.faces:
        for loop in f.loops:
            loop[b.pivot].uv = (0.0, 0.0)
            c = loop[b.col]
            loop[b.col] = (c[0], c[1], c[2], 0.0)
    for i, body in enumerate(bodies, start=1):
        for f in body["faces"]:
            for loop in f.loops:
                loop[b.pivot].uv = (float(i), 0.0)
    frames = _fall(bodies, statics, W, H, rng)
    _write_bin(pathlib.Path(out_dir) / (name + "_ruin.bin"), frames, len(bodies) + 1)
    return b.finish_mesh()


def _obb(vs):
    """The box along a set of vertices' own (principal) axes: its centre, half extents and axes (columns)."""
    pts = numpy.array([tuple(v.co) for v in vs])
    mean = pts.mean(axis=0)
    _, vecs = numpy.linalg.eigh(numpy.cov((pts - mean).T) if len(vs) > 3 else numpy.eye(3))
    axes = Matrix([tuple(vecs[:, k]) for k in range(3)]).transposed()
    if axes.determinant() < 0:
        axes.col[2] *= -1
    local = pts @ numpy.array(axes)
    lo, hi = local.min(axis=0), local.max(axis=0)
    return axes @ Vector((lo + hi) / 2), Vector((hi - lo) / 2), axes


def _fall(bodies, statics, W, H, rng):
    nx, ny = int(math.ceil(W / HM)), int(math.ceil(H / HM))
    hm = [[0.0] * ny for _ in range(nx)]

    def cells(x0, x1, y0, y1):  # y in the mesh's negative range
        i0, i1 = max(0, int(x0 / HM)), min(nx - 1, int(x1 / HM))
        j0, j1 = max(0, int(-y1 / HM)), min(ny - 1, int(-y0 / HM))
        return [(i, j) for i in range(i0, i1 + 1) for j in range(j0, j1 + 1)]
    for faces in statics:  # what stays is the floor the rubble lands on
        for f in faces:
            for v in f.verts:
                for i, j in cells(v.co.x, v.co.x, v.co.y, v.co.y):
                    hm[i][j] = max(hm[i][j], v.co.z)
    if not bodies:
        return [[]]
    if os.environ.get("BO_RUIN_DEBUG"):  # the heap's height (median, 90 %, 99 %, top), before and after
        flat = sorted(v for row in hm for v in row)
        print("HM0", [round(flat[int(len(flat) * q)], 2) for q in (0.5, 0.9, 0.99)], round(flat[-1], 2))
    top = max(bd["c0"].z + bd["he"].z for bd in bodies)
    centre = sum((bd["c0"] for bd in bodies), Vector()) / len(bodies)
    lean = Vector((1.0, 0.0, 0.0))  # the way the house already leans (building.gdshader, damage 3)
    for bd in bodies:
        h = bd["c0"].z / top
        out = Vector((bd["c0"].x - centre.x, bd["c0"].y - centre.y, 0.0))
        out = out.normalized() if out.length > 1e-6 else Vector((1.0, 0.0, 0.0))
        bd["pos"] = bd["c0"].copy()
        bd["q"] = Quaternion()
        bd["v"] = (out * rng.uniform(0.05, 0.35) + lean * rng.uniform(0.1, 0.45)) * h * 1.6
        axis = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-0.3, 0.3)))
        bd["w"] = (axis.normalized() if axis.length > 1e-6 else Vector((1, 0, 0))) * rng.uniform(0.8, 4.5) * (0.3 + h) \
            * bd["mat"]["spin"]
        # A wall section tips over outward from its foot (and the higher, the further): it turns about the
        # horizontal line along the wall, its top moving out first.
        tip = Vector((0.0, 0.0, 1.0)).cross(out) * rng.uniform(1.2, 2.6) * (0.4 + h) * bd["mat"]["topple"]
        bd["w"] = bd["w"] * (1.0 - 0.7 * bd["mat"]["topple"]) + tip
        bd["v"] += out * bd["mat"]["topple"] * 0.25 * h
        # The walls give way at their foot and what stands on them comes down after: the lower a piece, the
        # sooner it goes (a piece still inside the heap when it starts would be lifted onto it).
        bd["delay"] = rng.uniform(0.0, 0.08) + h * 0.35
        bd["done"] = False
        bd["hits"] = 0
        bd["slides"] = 0
        he = bd["he"]
        bd["corners"] = [bd["axes"] @ Vector((sx * he.x, sy * he.y, sz * he.z)) for sx in (-1, 1) for sy in (-1, 1)
                         for sz in (-1, 1)]
    steps = 4
    dt = 1.0 / FPS / steps
    frames = []
    t = 0.0
    for _f in range(int(SECONDS * FPS) + 1):
        frames.append([_matrix(bd) for bd in bodies])
        for _s in range(steps):
            t += dt
            for bd in bodies:
                if bd["done"] or t < bd["delay"]:
                    continue
                bd["v"].z -= G * dt
                drag = bd["mat"]["drag"]
                if drag:  # cloth and thatch: the air holds them, they drift and sway as they come down
                    bd["v"] -= bd["v"] * min(drag * dt, 1.0)
                    bd["v"].x += math.sin(t * 7.0 + bd["c0"].x * 9.0) * 0.6 * dt
                    bd["v"].y += math.cos(t * 6.0 + bd["c0"].y * 9.0) * 0.6 * dt
                bd["pos"] += bd["v"] * dt
                if bd["w"].length > 1e-6:
                    bd["q"] = Quaternion(bd["w"].normalized(), bd["w"].length * dt) @ bd["q"]
                world = [bd["pos"] + bd["q"] @ c for c in bd["corners"]]
                # The plot's edges: the rubble stays on its own tiles.
                xs, ys = [p.x for p in world], [p.y for p in world]
                if min(xs) < 0.01:
                    bd["pos"].x += 0.01 - min(xs)
                    bd["v"].x = abs(bd["v"].x) * 0.2
                if max(xs) > W - 0.01:
                    bd["pos"].x -= max(xs) - (W - 0.01)
                    bd["v"].x = -abs(bd["v"].x) * 0.2
                if max(ys) > -0.01:
                    bd["pos"].y -= max(ys) + 0.01
                    bd["v"].y = -abs(bd["v"].y) * 0.2
                if min(ys) < -H + 0.01:
                    bd["pos"].y += (-H + 0.01) - min(ys)
                    bd["v"].y = abs(bd["v"].y) * 0.2
                world = [bd["pos"] + bd["q"] @ c for c in bd["corners"]]
                under = cells(min(p.x for p in world), max(p.x for p in world), min(p.y for p in world),
                              max(p.y for p in world))
                heights = [hm[i][j] for i, j in under] or [0.0]
                # A piece lands on what is under most of it (the rubble's upper quarter is uneven: it sinks a little).
                support = sorted(heights)[int(len(heights) * 0.6)]
                low = min(p.z for p in world)
                if low >= support:
                    continue
                bd["pos"].z += support - low
                lower = min(heights)
                if support - lower > SLOPE and bd["slides"] < 30:
                    # On the side of the heap or half on a stump: it tips and slides down to the lower side, as
                    # loose rubble does until its slope is gentle enough to hold it (its angle of repose).
                    hi = [(i, j) for i, j in under if hm[i][j] >= support]
                    hx = (sum(i for i, _ in hi) / len(hi) + 0.5) * HM
                    hy = -(sum(j for _, j in hi) / len(hi) + 0.5) * HM
                    away = Vector((bd["pos"].x - hx, bd["pos"].y - hy, 0.0))
                    away = away.normalized() if away.length > 1e-6 else Vector((1.0, 0.0, 0.0))
                    bd["v"] = away * 0.6
                    bd["w"] = away.cross(Vector((0, 0, -1))) * 2.5
                    bd["slides"] += 1
                    continue
                if bd["v"].z < 0:
                    bd["v"].z = -bd["v"].z * bd["mat"]["bounce"]
                bd["v"].x *= bd["mat"]["keep"]
                bd["v"].y *= bd["mat"]["keep"]
                bd["w"] *= 0.45
                bd["hits"] += 1
                _towards_a_face(bd, 0.35)
                if bd["v"].length < 0.3 and bd["hits"] >= 2:
                    _flat(bd)  # it comes to rest on its broadest side
                    world = [bd["pos"] + bd["q"] @ c for c in bd["corners"]]
                    under = cells(min(p.x for p in world), max(p.x for p in world), min(p.y for p in world),
                                  max(p.y for p in world)) or [(0, 0)]
                    support = sorted(hm[i][j] for i, j in under)[int(len(under) * 0.6)]
                    bd["pos"].z += support - min(p.z for p in world)
                    # It lies where it lies: the rubble grows by its real shape, the height of each of its
                    # vertices in the cell under it (not by the box around it, mostly air for a wall's corner).
                    rot = numpy.array(bd["q"].to_matrix())
                    world = bd["pts"] @ rot.T + numpy.array(tuple(bd["pos"]))
                    for x, y, z in world:
                        i, j = min(max(int(x / HM), 0), nx - 1), min(max(int(-y / HM), 0), ny - 1)
                        # Loose rubble settles into its own gaps: a piece's height over what it lies on counts
                        # for PACK of it.
                        hm[i][j] = max(hm[i][j], support + (float(z) - support) * PACK)
                    bd["done"] = True
                    bd["w"] = Vector()
                    bd["v"] = Vector()
    if os.environ.get("BO_RUIN_DEBUG"):
        flat = sorted(v for row in hm for v in row)
        print("HM1", [round(flat[int(len(flat) * q)], 2) for q in (0.5, 0.9, 0.99)], round(flat[-1], 2))
    return frames


def _towards_a_face(bd, share):
    """Turns the chunk so that the side nearest to the ground lies flat on it (a stone rolls onto a face)."""
    q = bd["q"]
    best, sign = None, 1.0
    for k in range(3):
        w = q @ bd["axes"].col[k].to_3d()
        if best is None or abs(w.z) > abs(best.z):
            best, sign = w, (1.0 if w.z >= 0 else -1.0)
    turn = best.rotation_difference(Vector((0, 0, sign)))
    bd["q"] = q.slerp(turn @ q, share)


def _flat(bd):
    """Lays the chunk with its thinnest side up, turned the least from how it landed."""
    he = bd["he"]
    k = min(range(3), key=lambda a: he[a])
    w = bd["q"] @ bd["axes"].col[k].to_3d()
    turn = w.rotation_difference(Vector((0, 0, 1.0 if w.z >= 0 else -1.0)))
    bd["q"] = turn @ bd["q"]


def _matrix(bd):
    m = Matrix.Translation(bd["pos"]) @ bd["q"].to_matrix().to_4x4() @ Matrix.Translation(-bd["c0"])
    # Blender's mesh (x, y, z up; the plot's y is -y) to Godot's (x, y up, z): glTF's y-up export.
    p = Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, -1, 0, 0), (0, 0, 0, 1)))
    return p @ m @ p.inverted()


def _write_bin(path, frames, chunks):
    path.parent.mkdir(parents=True, exist_ok=True)
    out = bytearray(b"BORU")
    out += struct.pack("<iii", len(frames), chunks, FPS)
    ident = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0]
    for f in frames:
        out += struct.pack("<12f", *ident)  # chunk 0: what stays
        for m in f:
            out += struct.pack("<12f", *[m[r][c] for r in range(3) for c in range(4)])
    path.write_bytes(bytes(out))
    print("RUIN", path, len(frames), "frames", chunks, "chunks")
