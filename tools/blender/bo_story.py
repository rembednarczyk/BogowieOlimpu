"""Houses that tell their family's story (docs/ASSETY.md "Warianty i historie domów", R-018).

The level's function in models.py builds the house by hand (its massing is where the quality is) and declares
the free ground around it as slots; dress() fills the slots with the story's kit for that level, in the kit's
order, each piece at the first spot where it stands clear of every wall and prop (the same test as the grass,
then check_props at export). The stories, their kits and accents are data at the bottom of this file.
Distances in plot metres; a house's plot is 4 x 4 m (2 x 2 tiles).
"""
import math

import bo_detail as D
from bo_detail import t


# ---------------------------------------------------------------- the free ground

def slot(b, name, x0, y0, x1, y1, kind="yard"):
    """Free ground the story may dress: a rectangle in metres; `kind` says what fits (a yard, a strip along a
    wall, the door's front)."""
    if not hasattr(b, "slots"):
        b.slots = []
    b.slots.append((name, (x0, y0, x1, y1), kind))


def _free(b, x0, y0, x1, y1, h):
    """Nothing stands in the box (metres, from the ground up to h): no solid, no prop, and rays from the ground
    meet no geometry below h (walls, paving, steps that are not registered as solids)."""
    import mathutils
    from mathutils.bvhtree import BVHTree
    box = (t(x0), t(y0), 0, t(x1), t(y1), t(h))
    def near(o):
        return all(box[i] < o[i + 3] - 0.002 and o[i] < box[i + 3] - 0.002 for i in range(3))
    if any(near(o) for _, o, _ in b.solids) or any(near(o) for _, o in b.props):
        return False
    if box[0] < 0 or box[1] < 0 or box[3] > b.width or box[4] > b.height:
        return False
    tree = BVHTree.FromBMesh(b.bm)
    for fx in (0.1, 0.5, 0.9):
        for fy in (0.1, 0.5, 0.9):
            px, py = x0 + (x1 - x0) * fx, y0 + (y1 - y0) * fy
            hit = tree.ray_cast(mathutils.Vector((t(px), -t(py), 0.002)), mathutils.Vector((0, 0, 1)), t(h))
            if hit[0] is not None:
                return False
    return True


def dress(b, story, level, rng):
    """Puts the story's pieces for this level where they fit. Returns the names of those that found no room
    (the export goes on; models.py prints them, and a level that loses a piece gets a bigger slot)."""
    left = []
    for piece in STORIES[story]["kit"].get(level, []):
        fn, w, d, h, kinds = KIT[piece]
        placed = False
        for name, (sx0, sy0, sx1, sy1), kind in getattr(b, "slots", []):
            if kind not in kinds:
                continue
            along_x = (sx1 - sx0) >= (sy1 - sy0)
            pw, pd = (w, d) if along_x else (d, w)
            if pw > sx1 - sx0 + 1e-6 or pd > sy1 - sy0 + 1e-6:
                continue
            tries = [(sx0 + (sx1 - sx0 - pw) * k / 6, sy0 + (sy1 - sy0 - pd) * j / 2) for k in range(7) for j in range(3)]
            rng.shuffle(tries)
            for x, y in tries:
                if _free(b, x, y, x + pw, y + pd, h):
                    fn(b, x, y, along_x, rng, piece)
                    placed = True
                    break
            if placed:
                break
        if not placed:
            left.append(piece)
    return left


# ---------------------------------------------------------------- the pieces (x, y the box's corner, metres)

def _p(along_x, x, y, a, o, z):
    """A point a along the piece and o across it, from its corner."""
    return (t(x + a), t(y + o), t(z)) if along_x else (t(x + o), t(y + a), t(z))


def fleece_rack(b, x, y, along_x, rng, name):
    """Two forked posts and a pole, fleeces hung over it to dry, a tuft or two fallen below."""
    w, d, h = 0.9, 0.26, 1.15
    sx, sy = (w, d) if along_x else (d, w)
    b.prop(name, t(x), t(y), 0, t(x + sx), t(y + sy), t(h))
    for a in (0.05, w - 0.05):
        b.log(_p(along_x, x, y, a, d / 2, 0), _p(along_x, x, y, a, d / 2, h - 0.02), t(0.025), sides=8, stage=0.9)
    b.log(_p(along_x, x, y, 0.02, d / 2, h - 0.05), _p(along_x, x, y, w - 0.02, d / 2, h - 0.05), t(0.022), sides=8,
          stage=0.9)
    if not b.detail:
        return
    for k, (a0, a1) in enumerate(((0.1, 0.42), (0.47, 0.82))):
        drop = rng.uniform(0.45, 0.6)
        ph = rng.uniform(0, 6)
        for side in (-1, 1):  # a fleece hangs down both sides of the pole, thick and lumpy
            def fn(u, v, a0=a0, a1=a1, side=side, drop=drop, ph=ph):
                a = a0 + (a1 - a0) * u
                o = d / 2 + side * (0.025 + 0.06 * v + 0.012 * math.sin(9 * u + ph))
                return _p(along_x, x, y, a, o, h - 0.05 - drop * v * (0.85 + 0.15 * math.sin(5 * u + ph)))
            b.surface(fn, 6, 4, "linen", stage=0.9, tint=rng.uniform(0.6, 0.95))


def wattle_fence(b, x, y, along_x, rng, name, length=1.3):
    """A woven fence of hazel: stakes in the ground, withies woven in and out between them."""
    d, h = 0.1, 0.72
    sx, sy = (length, d) if along_x else (d, length)
    b.prop(name, t(x), t(y), 0, t(x + sx), t(y + sy), t(h))
    n = max(3, int(length / 0.22))
    for i in range(n + 1):
        a = 0.03 + (length - 0.06) * i / n
        b.log(_p(along_x, x, y, a, d / 2, 0), _p(along_x, x, y, a, d / 2, h + rng.uniform(-0.03, 0.04)), t(0.018),
              sides=6, stage=0.85, tint=rng.uniform(0.2, 0.6))
    if not b.detail:
        b.box(*(_p(along_x, x, y, 0, d / 2 - 0.02, 0.1)[:2]), t(0.1), t(sx if along_x else 0.04), t(0.04 if along_x else sy),
              t(h - 0.2), "bark", stage=0.85)
        return
    for r in range(7):  # the withies, each over one stake and under the next
        z = 0.12 + r * (h - 0.2) / 6
        pts = []
        for i in range(n * 2 + 1):
            a = 0.03 + (length - 0.06) * i / (2 * n)
            o = d / 2 + (0.022 if (i + r) % 2 else -0.022)
            pts.append(_p(along_x, x, y, a, o, z + rng.uniform(-0.01, 0.01)))
        b.tube_path(pts, t(0.011), "bark", sides=5, stage=0.86, tint=rng.uniform(0.3, 0.7))


def crook(b, x, y, along_x, rng, name):
    """A shepherd's crook stood up with a bell and a goatskin water bag hung on it."""
    b.prop(name, t(x), t(y), 0, t(x + 0.22), t(y + 0.22), t(1.6))
    cx, cy = x + 0.11, y + 0.11
    pts = [(cx, cy, 0.0), (cx, cy, 1.35), (cx + 0.02, cy, 1.48), (cx + 0.07, cy, 1.52), (cx + 0.1, cy, 1.46),
           (cx + 0.08, cy, 1.4)]
    b.tube_path([tuple(t(v) for v in p) for p in pts], t(0.014), "dark_wood", sides=6, stage=0.9)
    b.log((t(cx - 0.08), t(cy), t(0.02)), (t(cx + 0.08), t(cy), t(0.02)), t(0.02), sides=5, stage=0.9)  # its foot stone
    if b.detail:
        b.lathe(t(cx + 0.03), t(cy), t(0.95), [(t(0.004), 0), (t(0.035), t(0.02)), (t(0.03), t(0.07)), (t(0.008), t(0.09))],
                "bronze", sides=10, stage=0.9)
        b.lathe(t(cx - 0.05), t(cy), t(0.62), [(t(0.02), 0), (t(0.07), t(0.08)), (t(0.06), t(0.2)), (t(0.02), t(0.26))],
                "hide", sides=12, stage=0.9, smooth=2)


def shearing_bench(b, x, y, along_x, rng, name):
    """A low bench of a split log on two stumps, shears and a fleece rolled on it."""
    w, d, h = 0.8, 0.28, 0.42
    sx, sy = (w, d) if along_x else (d, w)
    b.prop(name, t(x), t(y), 0, t(x + sx), t(y + sy), t(h))  # the fleece on it is a prop of its own
    for a in (0.12, w - 0.12):
        b.lathe(*(_p(along_x, x, y, a, d / 2, 0)[:2]), 0, [(t(0.1), 0), (t(0.1), t(h - 0.06))], "bark", sides=12,
                stage=0.88, top="end_grain")
    b.log(_p(along_x, x, y, 0.02, d / 2, h - 0.03), _p(along_x, x, y, w - 0.02, d / 2, h - 0.03), t(0.07), sides=12,
          stage=0.88, split="half")
    if b.detail:
        D.wool_bale(b, *(v for v in ((x + w * 0.62, y + d / 2) if along_x else (x + d / 2, y + w * 0.62))), scale=0.35,
                    rng=rng, z=h, name=name + " fleece")


def pot_stack(b, x, y, along_x, rng, name):
    """Pots fresh from the kiln, stacked mouth down in a little pyramid, three, two, one."""
    w, d = 0.48, 0.28
    sx, sy = (w, d) if along_x else (d, w)
    b.prop(name, t(x), t(y), 0, t(x + sx), t(y + sy), t(0.5))
    if not b.detail:
        b.box(t(x), t(y), 0, t(sx), t(sy), t(0.35), "roof_tile", stage=0.9)
        return
    r = 0.075
    for row, n in enumerate((3, 2, 1)):
        for i in range(n):
            a = 0.09 + (i + row * 0.5) * 0.15
            cx, cy = (x + a, y + d / 2) if along_x else (x + d / 2, y + a)
            b.lathe(t(cx), t(cy), t(row * 0.15), [(t(r * 0.95), 0), (t(r * 1.05), t(0.05)), (t(r * 0.9), t(0.11)),
                                                 (t(r * 0.45), t(0.15)), (t(r * 0.05), t(0.155))],
                    "roof_tile", sides=18, stage=0.9, tint=rng.uniform(0.25, 0.8), smooth=2)


def potter_wheel(b, x, y, along_x, rng, name):
    """A kick wheel: a stone flywheel at the foot, a shaft, the wheel head with a pot half thrown on it; a
    lump of wet clay on a board beside."""
    b.prop(name, t(x), t(y), 0, t(x + 0.46), t(y + 0.46), t(0.75))
    cx, cy = x + 0.2, y + 0.2
    b.lathe(t(cx), t(cy), 0, [(t(0.18), 0), (t(0.19), t(0.06)), (t(0.17), t(0.08))], "stone", sides=20, stage=0.9)
    b.log((t(cx), t(cy), t(0.08)), (t(cx), t(cy), t(0.55)), t(0.025), sides=8, stage=0.9)
    b.lathe(t(cx), t(cy), t(0.55), [(t(0.15), 0), (t(0.155), t(0.03)), (t(0.14), t(0.04))], "wood", sides=20, stage=0.9)
    if b.detail:
        b.lathe(t(cx), t(cy), t(0.59), [(t(0.06), 0), (t(0.085), t(0.05)), (t(0.06), t(0.11)), (t(0.045), t(0.14)),
                                       (t(0.05), t(0.15))], "mudbrick", sides=18, stage=0.9, top="dark_wood", smooth=2)
        b.box(t(x + 0.3), t(y + 0.32), 0, t(0.15), t(0.13), t(0.03), "wood", stage=0.9)
        b.lathe(t(x + 0.375), t(y + 0.385), t(0.03), [(t(0.05), 0), (t(0.055), t(0.03)), (t(0.03), t(0.06))], "mudbrick",
                sides=12, stage=0.9, smooth=2)


def kiln(b, x, y, along_x, rng, name):
    """A small updraught kiln: a round firing chamber of mudbrick, plastered with clay, the stoke hole dark
    at the front, a vent at the top, firewood at its side."""
    b.prop(name, t(x), t(y), 0, t(x + 0.62), t(y + 0.62), t(0.9))
    cx, cy = x + 0.31, y + 0.31
    ctrl = [(0.29, 0.0), (0.3, 0.2), (0.28, 0.45), (0.21, 0.65), (0.1, 0.8), (0.07, 0.84)]
    b.lathe(t(cx), t(cy), 0, [(t(a), t(c)) for a, c in ctrl], "mudbrick", sides=24 if b.detail else 8, stage=0.9,
            tint=rng.uniform(0.4, 0.7), top="dark_wood", smooth=3 if b.detail else 0,
            shape=lambda a: 1.0 + 0.03 * math.sin(4 * a))
    if b.detail:
        b.box(*(_p(along_x, x, y, 0.22, 0.6, 0)[:2]), 0, t(0.18 if along_x else 0.04), t(0.04 if along_x else 0.18), t(0.2),
              "dark_wood", stage=0.9)
        b.lathe(t(cx), t(cy), t(0.84), [(t(0.075), 0), (t(0.08), t(0.04))], "stone", sides=12, stage=0.9, top="dark_wood")


def drying_shelf(b, x, y, along_x, rng, name):
    """Two planks on posts against the wall, pots drying on them before the firing."""
    w, d, h = 1.0, 0.26, 0.95
    sx, sy = (w, d) if along_x else (d, w)
    b.prop(name, t(x), t(y), 0, t(x + sx), t(y + sy), t(h + 0.2))
    for a in (0.04, w - 0.04):
        for o in (0.04, d - 0.04):
            b.log(_p(along_x, x, y, a, o, 0), _p(along_x, x, y, a, o, h), t(0.02), sides=6, stage=0.9)
    for z in (0.45, h - 0.02):
        p0 = _p(along_x, x, y, 0, 0.01, z)
        b.box(p0[0], p0[1], t(z), t(sx if along_x else d - 0.02), t(d - 0.02 if along_x else sy), t(0.025), "wood",
              stage=0.9, tint=rng.uniform(0.4, 0.7))
        if not b.detail:
            continue
        for i in range(5):
            a = 0.1 + i * (w - 0.2) / 4
            cx, cy = (x + a, y + d / 2) if along_x else (x + d / 2, y + a)
            r = rng.uniform(0.05, 0.075)
            b.lathe(t(cx), t(cy), t(z + 0.025), [(t(r * 0.6), 0), (t(r), t(r * 1.2)), (t(r * 0.7), t(r * 2.0)),
                                                 (t(r * 0.8), t(r * 2.3))], "mudbrick" if z < 0.5 else "roof_tile",
                    sides=14, stage=0.9, tint=rng.uniform(0.3, 0.8), smooth=2)


def veg_bed(b, x, y, along_x, rng, name):
    """A raised bed edged with boards: rows of cabbages, leeks and lettuces."""
    w, d = 1.2, 0.42
    sx, sy = (w, d) if along_x else (d, w)
    b.prop(name, t(x), t(y), 0, t(x + sx), t(y + sy), t(0.4))
    b.box(t(x), t(y), 0, t(sx), t(sy), t(0.14), "wood", stage=0.85, tint=0.35)
    b.box(t(x + 0.03), t(y + 0.03), t(0.1), t(sx - 0.06), t(sy - 0.06), t(0.05), "dark_wood", stage=0.86)
    if not b.detail:
        return
    for row in range(2):
        for i in range(6):
            a = 0.1 + i * (w - 0.2) / 5
            o = 0.11 + row * 0.2
            cx, cy = (x + a, y + o) if along_x else (x + o, y + a)
            kind = (i + row) % 3
            for k in range(7 if kind != 1 else 4):
                ang = 2 * math.pi * k / 7 + rng.uniform(-0.2, 0.2)
                reach = 0.07 if kind == 0 else 0.03
                tip = (cx + reach * math.cos(ang), cy + reach * math.sin(ang), 0.15 + (0.12 if kind == 1 else 0.08))
                b.strand((t(cx), t(cy), t(0.15)), tuple(t(v) for v in tip), t(0.05 if kind == 0 else 0.02),
                         "leaves" if kind != 2 else "grass", stage=0.9, tint=rng.uniform(0.3, 0.9))


def beehives(b, x, y, along_x, rng, name):
    """Two clay hives lying on a stone stand, as Greek bee-keepers kept them: long tubes, the bees' door a dark
    round at the front, a stone on each lid."""
    w, d, h = 0.72, 0.3, 0.55
    sx, sy = (w, d) if along_x else (d, w)
    b.prop(name, t(x), t(y), 0, t(x + sx), t(y + sy), t(h))
    p0 = _p(along_x, x, y, 0.02, 0.02, 0)
    b.box(p0[0], p0[1], 0, t(sx - 0.04), t(sy - 0.04), t(0.28), "stone", stage=0.9)
    for i in range(2):
        a = 0.2 + i * 0.32
        start = _p(along_x, x, y, a, 0.02, 0.38) if along_x else _p(along_x, x, y, a, 0.02, 0.38)
        end = _p(along_x, x, y, a, d - 0.02, 0.38)
        b.tube_path([start, end], t(0.1), "roof_tile", sides=14 if b.detail else 6, stage=0.9, tint=rng.uniform(0.3, 0.6))
        if b.detail:
            b.tube_path([_p(along_x, x, y, a, d - 0.025, 0.36), _p(along_x, x, y, a, d - 0.005, 0.36)], t(0.03),
                        "dark_wood", sides=8, stage=0.9)
            b.lathe(*(_p(along_x, x, y, a, d / 2, 0.47)[:2]), t(0.47), [(t(0.05), 0), (t(0.055), t(0.03)), (t(0.02), t(0.06))],
                    "stone", sides=8, stage=0.9)


def bean_poles(b, x, y, along_x, rng, name):
    """Three poles tied at the top, beans climbing them."""
    b.prop(name, t(x), t(y), 0, t(x + 0.4), t(y + 0.4), t(1.45))
    cx, cy = x + 0.2, y + 0.2
    for k in range(3):
        ang = 2 * math.pi * k / 3 + 0.4
        foot = (cx + 0.17 * math.cos(ang), cy + 0.17 * math.sin(ang), 0.012)
        b.log(tuple(t(v) for v in foot), (t(cx), t(cy), t(1.42)), t(0.012), sides=5, stage=0.9)
        if not b.detail:
            continue
        for j in range(9):
            f = (j + 0.5) / 9
            px, py, pz = foot[0] + (cx - foot[0]) * f, foot[1] + (cy - foot[1]) * f, 1.42 * f
            tip = (px + rng.uniform(-0.06, 0.06), py + rng.uniform(-0.06, 0.06), pz + 0.06)
            b.strand((t(px), t(py), t(pz)), tuple(t(v) for v in tip), t(0.05), "leaves", stage=0.92,
                     tint=rng.uniform(0.3, 0.9))


def herb_pots(b, x, y, along_x, rng, name):
    """Three pots of herbs in a row."""
    sx, sy = (0.75, 0.26) if along_x else (0.26, 0.75)
    for i in range(3):
        a = 0.13 + i * 0.25
        cx, cy = (x + a, y + 0.13) if along_x else (x + 0.13, y + a)
        D.potted_plant(b, cx, cy, scale=0.42, rng=rng, name="%s %d" % (name, i + 1))


KIT = {  # piece: (function, length along the slot, depth, height, the slots it may take)
    "fleece rack": (fleece_rack, 0.9, 0.26, 1.15, ("strip", "yard")),
    "wattle fence": (wattle_fence, 1.3, 0.1, 0.75, ("strip", "yard")),
    "crook": (crook, 0.22, 0.22, 1.6, ("strip", "yard", "corner")),
    "shearing bench": (shearing_bench, 0.8, 0.28, 0.55, ("strip", "yard")),
    "pot stack": (pot_stack, 0.48, 0.28, 0.5, ("strip", "yard", "corner")),
    "potter's wheel": (potter_wheel, 0.46, 0.46, 0.75, ("yard",)),
    "kiln": (kiln, 0.62, 0.62, 0.9, ("yard",)),
    "drying shelf": (drying_shelf, 1.0, 0.26, 1.15, ("strip", "yard")),
    "vegetable bed": (veg_bed, 1.2, 0.42, 0.4, ("yard",)),
    "beehives": (beehives, 0.72, 0.3, 0.55, ("strip", "yard")),
    "bean poles": (bean_poles, 0.4, 0.4, 1.45, ("yard", "corner")),
    "herb pots": (herb_pots, 0.75, 0.26, 0.6, ("strip", "yard")),
}

# The three families of the common houses (owner, 2026-11-28). `accent`: what the massing takes from the
# story (the level's function reads it); `mirror`: the house built the other way round, door and all.
STORIES = {
    "shepherds": {
        "name": "pasterze",
        "accent": {"tent": ("linen", "hide"), "walls": None},
        "mirror": False,
        "kit": {1: ["fleece rack", "crook", "wattle fence"],
                2: ["fleece rack", "shearing bench", "crook"],
                3: ["fleece rack", "crook", "shearing bench"]},
    },
    "potters": {
        "name": "garncarze",
        "accent": {"tent": ("ochre", "linen"), "walls": "plaster"},
        "mirror": True,
        "kit": {1: ["potter's wheel", "pot stack"],
                2: ["kiln", "pot stack", "potter's wheel"],
                3: ["drying shelf", "pot stack", "pot stack"]},
    },
    "gardeners": {
        "name": "ogrodnicy i pszczelarze",
        "accent": {"tent": ("sky_cloth", "linen"), "walls": None, "pergola": True},
        "mirror": False,
        "kit": {1: ["vegetable bed", "bean poles", "herb pots"],
                2: ["vegetable bed", "beehives", "bean poles"],
                3: ["beehives", "herb pots"]},
    },
}
