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
import bo_materials as M
import numpy
from mathutils import Matrix, Quaternion, Vector


GROUND = 0.16  # tiles (32 cm): pieces lower than this stay (the footing, the paving, the grass)
G = 9.81 / 2.0  # tiles/s² (1 tile = 2 m)
FPS = 24
SECONDS = 2.5
HM = 0.05  # tiles (10 cm): the rubble's height map
COAT = 0.02  # tiles (4 cm): a sheet thinner than this is a coat over other pieces (plaster, mortar, underlay)
PACK = 0.4  # of a piece's height above its bed that the heap grows by (the rest goes into the gaps)
SLOPE = 0.06  # tiles (12 cm): a drop under a piece that makes it slide off
HOLD = 0.06  # tiles (12 cm): a coat this close to a piece goes with it

# How each material breaks, how heavy it is and how it moves: the game's material table (bo_materials.py, R-019).
AIR = 22.0  # air resistance per (m² of face / kg of weight), 1/s; at most DRAG_MAX
DRAG_MAX = 4.0  # 1/s: a canvas still comes down from a roof in the time of the fall
MATERIALS = M.CLASSES  # by class: bond, fill, bounce, keep, spin, topple


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
    # Nor the hidden filling (the mortar beds under the tiles): it is there to close the view, not a material.
    name_of = {i: n for i, (n, _) in enumerate(A.PALETTE)}

    def part(f):
        return name_of.get(int(f.loops[0][b.uv].uv[0] * A.PALETTE_SIZE), "plaster")

    def kind(f):
        return M.MATERIALS[part(f)]["cls"]
    # A liquid (water in a basin, oil) drains away: nothing of it falls.
    dead = [f for f in bm.faces if f.loops[0][b.col][0] > 0.995 or f.loops[0][b.pivot].uv[0] < -0.5
            or abs(f.loops[0][b.uv].uv[1] - A.FILL_TINT) < 1e-4 or kind(f) == "liquid"]
    bmesh.ops.delete(bm, geom=dead, context="FACES")
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
        k = kind(faces[0])
        # What stays: the footing and what lies on the ground; of the walls, their stumps (a tent has none).
        if max(v.co.z for v in vs) < (max(GROUND, stump(c.x, c.y)) if k == "masonry" else GROUND):
            statics.append(faces)
            continue
        _, he, _ = _obb(vs)
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
            # Plaster goes with its bricks, a mortar bed with its tiles; a canvas or a thatch is held by nothing
            # once its frame gives way: it comes down on its own, torn, over the poles.
            if d is not None and d < HOLD and kind(f) != "soft":
                key = owner[i][1]
                if key is None:
                    statics.append([f])
                else:
                    chunks[key].append(f)
            else:  # nothing under it: it tears into shards
                k = kind(f)
                if max(v.co.z for v in f.verts) < (max(GROUND, stump(c.x, c.y)) if k == "masonry" else GROUND):
                    statics.append([f])
                    continue
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
        # A piece the model shows as a surface (a plastered gable, a shutter, a sheet of thatch) gets its
        # material's thickness (bo_asset.THICKNESS), backwards, into the house, where nothing sees it yet.
        parts = {}
        for f in faces:
            parts[part(f)] = parts.get(part(f), 0.0) + f.calc_area()
        main = max(parts, key=parts.get)  # the material most of it is of: its thickness and its weight
        solid = [n for n in parts if not M.MATERIALS[n]["coat"]]  # a plastered wall weighs as its bricks
        heavy = max(solid, key=parts.get) if solid else main
        want = M.THICKNESS[main if not M.MATERIALS[main]["coat"] or not solid else heavy] * A.M  # metres to tiles
        if 2 * min(he) < 0.9 * want:  # up to the material's thickness, what it has counted in
            # Extruded along its thinnest axis, towards the middle of the house, kept on the plot.
            k = min(range(3), key=lambda a: he[a])
            axis = axes.col[k].to_3d()
            if axis.dot(Vector((W / 2, -H / 2, c0.z)) - c0) < 0:
                axis = -axis
            have = set(faces)
            res = bmesh.ops.extrude_face_region(bm, geom=faces, use_keep_orig=True)
            new_v = [g for g in res["geom"] if isinstance(g, bmesh.types.BMVert)]
            for v in new_v:
                v.co += axis * (want - 2 * min(he))
                v.co.x = min(max(v.co.x, 0.0), W)
                v.co.y = min(max(v.co.y, -H), 0.0)
                v.co.z = max(v.co.z, 0.0)
            faces = faces + list({f for v in new_v for f in v.link_faces if f not in have})
            chunks[key] = faces
            vs = list({v for f in faces for v in f.verts})
            c0, he, axes = _obb(vs)
        bodies.append({"faces": faces, "c0": c0, "he": he, "axes": axes, "mat": MATERIALS[key[0]], "kind": key[0],
                       "density": M.MATERIALS[heavy]["density"],
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
        mat = bd["mat"]
        h = bd["c0"].z / top
        he_m = [x / A.M for x in bd["he"]]  # metres
        big = sorted(he_m)
        # Its weight, and how much the air holds it back: face (its two larger sizes) over weight.
        bd["mass"] = max(8 * he_m[0] * he_m[1] * he_m[2] * mat["fill"] * bd["density"], 0.05)
        bd["drag"] = min(AIR * 4 * big[1] * big[2] / bd["mass"], DRAG_MAX)
        bd["r"] = sum(bd["he"]) / 3 * 1.1  # its size for colliding with others in the air (tiles)
        out = Vector((bd["c0"].x - centre.x, bd["c0"].y - centre.y, 0.0))
        out = out.normalized() if out.length > 1e-6 else Vector((1.0, 0.0, 0.0))
        bd["pos"] = bd["c0"].copy()
        bd["q"] = Quaternion()
        # The house gives way and comes down where it stood (owner, 2026-11-28: "it does not splash, it drops
        # loosely"): hardly a push outward, a slow turn; a wall section leans out from its foot as it goes.
        bd["v"] = (out * rng.uniform(0.0, 0.06) + lean * rng.uniform(0.0, 0.05)) * h
        axis = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-0.3, 0.3)))
        bd["w"] = (axis.normalized() if axis.length > 1e-6 else Vector((1, 0, 0))) * rng.uniform(0.2, 0.9) \
            * (0.3 + h) * mat["spin"]
        tip = Vector((0.0, 0.0, 1.0)).cross(out) * rng.uniform(0.4, 0.9) * (0.4 + h) * mat["topple"]
        bd["w"] = bd["w"] * (1.0 - 0.7 * mat["topple"]) + tip
        # The walls give way at their foot and what stands on them comes down after.
        bd["delay"] = rng.uniform(0.0, 0.08) + h * 0.35
        if bd["kind"] == "soft":  # a canvas or a thatch sags once its frame has gone, and comes down onto it
            bd["delay"] += 0.35
        bd["done"] = False
        bd["started"] = False
        bd["rest"] = 0.0
        bd["air"] = 0.0
        he = bd["he"]
        bd["corners"] = [bd["axes"] @ Vector((sx * he.x, sy * he.y, sz * he.z)) for sx in (-1, 1) for sy in (-1, 1)
                         for sz in (-1, 1)]
    steps = 4
    dt = 1.0 / FPS / steps
    frames = []
    t = 0.0
    STEP = 0.03  # tiles (6 cm): a piece steps up onto rubble at most this; a higher edge stops it from the side
    LIFT = 0.004  # tiles (8 mm) a substep: how fast a piece that has sunk into the rubble comes to lie on it

    def footprint(bd):
        world = [bd["pos"] + bd["q"] @ c for c in bd["corners"]]
        under = cells(min(p.x for p in world), max(p.x for p in world), min(p.y for p in world),
                      max(p.y for p in world)) or [(0, 0)]
        heights = sorted(hm[i][j] for i, j in under)
        return world, under, heights

    jumps = []  # the largest rise of any piece from one frame to the next (BO_RUIN_DEBUG)
    last_z = [bd["pos"].z for bd in bodies]
    for _f in range(int(SECONDS * FPS) + 1):
        frames.append([_matrix(bd) for bd in bodies])
        jumps.append(max((bd["pos"].z - z, bd["kind"], bd["rest"] > 0, round(bd["drag"], 1), _f)
                         for bd, z in zip(bodies, last_z)))
        last_z = [bd["pos"].z for bd in bodies]
        for _s in range(steps):
            t += dt
            moving = [bd for bd in bodies if not bd["done"] and t >= bd["delay"]]
            for bd in moving:
                mat = bd["mat"]
                if not bd["started"]:  # buried by what came down first: it stays under it
                    bd["started"] = True
                    world, _, heights = footprint(bd)
                    if bd["pos"].z < heights[int(len(heights) * 0.6)]:  # its middle under the rubble
                        bd["done"] = True
                        continue
                bd["v"].z -= G * dt
                # The air: light for its size (canvas, thatch) it drifts down slowly and sways; a stone hardly feels it.
                bd["v"] -= bd["v"] * min(bd["drag"] * dt, 1.0)
                if bd["drag"] > 1.0:
                    sway = min(bd["drag"] / DRAG_MAX, 1.0) * 0.5 * dt
                    bd["v"].x += math.sin(t * 7.0 + bd["c0"].x * 9.0) * sway
                    bd["v"].y += math.cos(t * 6.0 + bd["c0"].y * 9.0) * sway
                old = bd["pos"].copy()
                bd["pos"] += bd["v"] * dt
                if bd["w"].length > 1e-6:
                    turned = Quaternion(bd["w"].normalized(), bd["w"].length * dt) @ bd["q"]
                    if bd["rest"] > 0.0:  # lying on the rubble it turns about where it touches, not into it
                        before = min((bd["pos"] + bd["q"] @ c).z for c in bd["corners"])
                        bd["q"] = turned
                        bd["pos"].z += min(max(before - min((bd["pos"] + bd["q"] @ c).z for c in bd["corners"]), 0.0),
                                           2 * LIFT)
                    else:
                        bd["q"] = turned
                world = [bd["pos"] + bd["q"] @ c for c in bd["corners"]]
                # The plot's edges: the rubble stays on its own tiles (it slides along them, does not bounce).
                xs, ys = [p.x for p in world], [p.y for p in world]
                if min(xs) < 0.01:
                    bd["pos"].x += 0.01 - min(xs)
                    bd["v"].x = 0.0
                if max(xs) > W - 0.01:
                    bd["pos"].x -= max(xs) - (W - 0.01)
                    bd["v"].x = 0.0
                if max(ys) > -0.01:
                    bd["pos"].y -= max(ys) + 0.01
                    bd["v"].y = 0.0
                if min(ys) < -H + 0.01:
                    bd["pos"].y += (-H + 0.01) - min(ys)
                    bd["v"].y = 0.0
                world, under, heights = footprint(bd)
                support = heights[int(len(heights) * 0.6)]
                low = min(p.z for p in world)
                if low >= support:
                    bd["air"] += dt
                    if bd["air"] > 0.1:  # in the air again (a bounce, a fall off an edge)
                        bd["rest"] = 0.0
                    continue
                bd["air"] = 0.0
                moved = Vector((bd["pos"].x - old.x, bd["pos"].y - old.y, 0.0)).length
                if support - low > STEP and moved > 1e-4:
                    here = bd["pos"].copy()
                    bd["pos"].x, bd["pos"].y = old.x, old.y
                    _, _, h_old = footprint(bd)
                    bd["pos"].x, bd["pos"].y = here.x, here.y
                if support - low > STEP and moved > 1e-4 and h_old[int(len(h_old) * 0.6)] < support - STEP:
                    # It ran into the side of the heap: stopped from the side, not lifted onto it.
                    bd["pos"].x, bd["pos"].y = old.x, old.y
                    bd["v"].x *= -0.1
                    bd["v"].y *= -0.1
                    world, under, heights = footprint(bd)
                    support = heights[int(len(heights) * 0.6)]
                    low = min(p.z for p in world)
                    if low >= support:
                        continue
                # It comes to lie on the rubble: a piece just landed is stopped at its surface; one sunk into it
                # (the heap grew under it) rises slowly, never jumps.
                bd["pos"].z += min(support - low, STEP if bd["rest"] == 0.0 else LIFT)
                if bd["v"].z < 0:
                    bd["v"].z = -bd["v"].z * mat["bounce"]
                bd["v"].x *= mat["keep"] ** (dt * 24)
                bd["v"].y *= mat["keep"] ** (dt * 24)
                bd["w"] *= 0.25 ** (dt * 24)  # what touches the rubble stops tumbling
                # On a slope steeper than the rubble holds, it slides down: gravity along the slope, little by little.
                lower = heights[0]
                if support - lower > SLOPE:
                    hi = [(i, j) for i, j in under if hm[i][j] >= support]
                    hx = (sum(i for i, _ in hi) / len(hi) + 0.5) * HM
                    hy = -(sum(j for _, j in hi) / len(hi) + 0.5) * HM
                    away = Vector((bd["pos"].x - hx, bd["pos"].y - hy, 0.0))
                    if away.length > 1e-6:
                        bd["v"] += away.normalized() * G * 0.08 * dt  # loose rubble creeps, it does not run
                # Touching at one end, it tips over onto its broadest side (gravity about where it touches),
                # smoothly, faster the more it leans: a pole falls, a stone settles.
                k = min(range(3), key=lambda a: bd["he"][a])
                flat = abs((bd["q"] @ bd["axes"].col[k].to_3d()).z)
                _settle(bd, 1.0 - (0.85 - 0.25 * (1.0 - flat)) ** (dt * 24))
                bd["rest"] += dt
                # It stays when it lies (flat enough and slow), or held long enough by what is under it.
                if (flat > 0.9 and bd["v"].length < 0.15 and bd["rest"] > 0.15) or bd["rest"] > 1.5:
                    # It lies where it lies: the rubble grows by its real shape, the height of each of its
                    # vertices in the cell under it, a share PACK of it (loose rubble settles into its gaps).
                    rot = numpy.array(bd["q"].to_matrix())
                    pts = bd["pts"] @ rot.T + numpy.array(tuple(bd["pos"]))
                    for x, y, z in pts:
                        i, j = min(max(int(x / HM), 0), nx - 1), min(max(int(-y / HM), 0), ny - 1)
                        hm[i][j] = max(hm[i][j], support + (float(z) - support) * PACK)
                    bd["done"] = True
                    bd["w"] = Vector()
                    bd["v"] = Vector()
            # Pieces that set off together (neighbours in the wall) part on their own first.
            # Only pieces in the air knock each other; what lies on the rubble is the rubble (its height map).
            _collide([bd for bd in moving if not bd["done"] and t - bd["delay"] > 0.1 and bd["rest"] == 0.0])
    if os.environ.get("BO_RUIN_DEBUG"):
        flat = sorted(v for row in hm for v in row)
        print("HM1", [round(flat[int(len(flat) * q)], 2) for q in (0.5, 0.9, 0.99)], round(flat[-1], 2))
        print("JUMP", round(max(jumps)[0] / A.M * 100, 1), "cm in a frame", max(jumps)[1:], ";",
              sum(1 for bd in bodies if not bd["done"]), "still moving",
              sorted({(bd["kind"], round(bd["drag"])) for bd in bodies if not bd["done"]})[:12])
        spread = sorted(Vector((bd["pos"].x - bd["c0"].x, bd["pos"].y - bd["c0"].y)).length / A.M * 100 for bd in bodies)
        print("SPREAD median", round(spread[len(spread) // 2]), "cm, 90 %", round(spread[int(len(spread) * 0.9)]),
              "cm, max", round(spread[-1]), "cm")
        left = [bd for bd in bodies if not bd["done"]]
        print("LEFT started", sum(bd["started"] for bd in left), "rest>0", sum(bd["rest"] > 0 for bd in left),
              "z", sorted(round(bd["pos"].z, 2) for bd in left)[::max(1, len(left) // 10)],
              "v", sorted(round(bd["v"].length, 2) for bd in left)[::max(1, len(left) // 10)])
    return frames


def _settle(bd, share):
    """Turns the chunk a share of the way onto its broadest side (its thinnest axis up), the least turn."""
    k = min(range(3), key=lambda a: bd["he"][a])
    w = bd["q"] @ bd["axes"].col[k].to_3d()
    turn = w.rotation_difference(Vector((0, 0, 1.0 if w.z >= 0 else -1.0)))
    bd["q"] = bd["q"].slerp(turn @ bd["q"], share)


def _collide(bodies):
    """Pieces in the air meet: where two overlap they are pushed apart, the lighter more, and their speeds along
    the line between them even out by their weights (they knock, they do not bounce off)."""
    grid = {}
    shove = [Vector() for _ in bodies]
    for n, bd in enumerate(bodies):
        grid.setdefault(tuple(int(c // 0.3) for c in bd["pos"]), []).append(n)
    for (gx, gy, gz), members in grid.items():
        near = [m for dx in (-1, 0, 1) for dy in (-1, 0, 1) for dz in (-1, 0, 1)
                for m in grid.get((gx + dx, gy + dy, gz + dz), ())]
        for a in members:
            for b in near:
                if b <= a:
                    continue
                p, q = bodies[a], bodies[b]
                d = q["pos"] - p["pos"]
                gap = p["r"] + q["r"] - d.length
                if gap <= 0 or d.length < 1e-6:
                    continue
                n = d.normalized()
                wa, wb = q["mass"] / (p["mass"] + q["mass"]), p["mass"] / (p["mass"] + q["mass"])
                push = min(gap, 0.02) * 0.5
                shove[a] -= n * push * wa
                shove[b] += n * push * wb
                rel = (q["v"] - p["v"]).dot(n)
                if rel < 0:
                    impulse = rel / (1 / p["mass"] + 1 / q["mass"])
                    p["v"] += n * impulse / p["mass"]
                    q["v"] -= n * impulse / q["mass"]
    for bd, d in zip(bodies, shove):  # a little at a time, however many it touches: no piece is thrown
        # In a crowd of falling rubble pieces part mostly up and down (they come to lie on each other), hardly
        # sideways: the heap does not spread out of the house's walls.
        d = Vector((d.x * 0.25, d.y * 0.25, d.z))
        if d.length > 0.002:
            d = d.normalized() * 0.002
        bd["pos"] += d


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
