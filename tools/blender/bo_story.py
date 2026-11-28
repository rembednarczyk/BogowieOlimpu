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
        "accent": {"tent": ("sage_cloth", "linen"), "walls": None, "pergola": True},  # blue is for public roofs
        "mirror": False,
        "kit": {1: ["vegetable bed", "bean poles", "herb pots"],
                2: ["vegetable bed", "beehives", "bean poles"],
                3: ["beehives", "herb pots"]},
    },
}


# ---------------------------------------------------------------- the big pieces at the front (levels 4-7)
# One per story and level, big enough to read from the game's camera: what the family does, shown on the
# house's face (owner, 2026-11-28). Points in plot metres.

DYES = ["cloth", "ochre", "blue_paint", "flower_yellow", "sky_cloth", "flower_red"]


def _pt(p):
    return tuple(t(v) for v in p)


def skeins(b, p0, p1, rng, name="skeins", pole=True, drop=0.42, out=(0.0, 0.0), pad=0.05):
    """Skeins of dyed wool hung over a pole (or over a rail when `pole` is False) from p0 to p1: each a long
    loop of a few strands, red, ochre, blue, yellow, sky; `out` shifts the loops off the line (over a rail)."""
    (x0, y0, z), (x1, y1, _) = p0, p1
    ox, oy = out
    px_, py_ = (pad if abs(x1 - x0) < 1e-6 else 0.0), (pad if abs(y1 - y0) < 1e-6 else 0.0)  # across the line only:
    b.prop(name, t(min(x0, x1) - px_ + min(ox, 0)), t(min(y0, y1) - py_ + min(oy, 0)), t(z - drop - 0.06),  # its ends
           t(max(x0, x1) + px_ + max(ox, 0)), t(max(y0, y1) + py_ + max(oy, 0)), t(z + 0.04))  # may meet a wall
    if pole:
        b.log(_pt(p0), _pt(p1), t(0.022), sides=8, stage=0.9)
    if not b.detail:
        return
    length = math.hypot(x1 - x0, y1 - y0)
    n = max(3, int(length / 0.13))
    for i in range(n):
        f = (i + 0.5) / n
        cx, cy = x0 + (x1 - x0) * f + ox, y0 + (y1 - y0) * f + oy
        colour = DYES[(i + int(rng.random() * 2)) % len(DYES)]
        d = drop * rng.uniform(0.8, 1.05)
        for k in range(3):  # three strands of the loop, a little apart
            s_ = (k - 1) * 0.014
            ux, uy = (x1 - x0) / length, (y1 - y0) / length
            top = (cx + ux * s_, cy + uy * s_, z + 0.02)
            pts = [top, (top[0], top[1], z - d * 0.5), (top[0] + ux * 0.02, top[1] + uy * 0.02, z - d),
                   (top[0] + ux * 0.04, top[1] + uy * 0.04, z - d * 0.5), (top[0] + ux * 0.04, top[1] + uy * 0.04, z + 0.02)]
            b.tube_path([_pt(q) for q in pts], t(0.012), colour, sides=5, stage=0.9, tint=rng.uniform(0.4, 0.8))


def awning(b, x0, x1, y, z, depth, drop, rng, name="awning", poles=False, pots=4, stripes=("cloth", "linen")):
    """A striped awning out from the wall (its face at y, facing +y) at height z, `depth` out and `drop` lower
    at the front, on iron arms (or two poles at the front when `poles`); its hem cut in scallops; pots on ropes
    hung from the front bar: the potters' sign."""
    yf, zf = y + depth, z - drop
    b.prop(name, t(x0), t(y + 0.035), t(zf - 0.12 - (0.45 if pots else 0)), t(x1), t(yf + 0.03), t(z + 0.03))
    if poles:
        for i, px in enumerate((x0 + 0.04, x1 - 0.04)):
            b.prop("%s pole %d" % (name, i + 1), t(px - 0.03), t(yf - 0.03), 0, t(px + 0.03), t(yf + 0.03),
                   t(zf - 0.12 - (0.45 if pots else 0)))
            b.log((t(px), t(yf), 0), (t(px), t(yf), t(zf + 0.02)), t(0.025), sides=8, stage=0.9)
    for px in (x0 + 0.03, x1 - 0.03):  # the arms
        b.log((t(px), t(y + 0.04), t(z)), (t(px), t(yf), t(zf)), t(0.012), sides=5, stage=0.9)
    b.log((t(x0), t(yf), t(zf)), (t(x1), t(yf), t(zf)), t(0.014), sides=6, stage=0.9)
    n = max(4, int((x1 - x0) / 0.16)) if b.detail else 1
    for i in range(n):
        a0, a1 = x0 + (x1 - x0) * i / n, x0 + (x1 - x0) * (i + 1) / n
        part = stripes[i % 2]
        def fn(u, v, a0=a0, a1=a1):
            px = a0 + (a1 - a0) * u
            sag = 0.03 * math.sin(math.pi * v)
            return (t(px), t(y + 0.04 + (depth - 0.04) * v), t(z + 0.02 - (drop) * v - sag))
        b.surface(fn, 3 if b.detail else 1, 4 if b.detail else 1, part, stage=0.92, tint=0.6)
        if b.detail:  # the scalloped hem
            def hem(u, v, a0=a0, a1=a1):
                px = a0 + (a1 - a0) * u
                return (t(px), t(yf + 0.005), t(zf + 0.02 - 0.12 * v * (0.55 + 0.45 * math.sin(math.pi * u))))
            b.surface(hem, 4, 2, part, stage=0.92, tint=0.55)
    if not b.detail:
        return
    for i in range(pots):
        px = x0 + (x1 - x0) * (i + 0.5) / pots
        hang = rng.uniform(0.15, 0.3)
        b.tube_path([(t(px), t(yf), t(zf)), (t(px), t(yf), t(zf - hang))], t(0.004), "rope", sides=4, stage=0.92)
        r = rng.uniform(0.06, 0.08)
        b.lathe(t(px), t(yf), t(zf - hang - r * 1.8), [(t(r * 0.4), 0), (t(r), t(r * 0.8)), (t(r * 0.8), t(r * 1.5)),
                                                         (t(r * 0.45), t(r * 1.8))], "roof_tile", sides=14, stage=0.92,
                tint=rng.uniform(0.3, 0.8), smooth=2)


def vine_canopy(b, x0, x1, y, z, depth, rng, root, name="vine"):
    """A vine trained over a light frame out from the wall (face at y, facing +y) at height z: two brackets, a
    front rail, a roof of leaves with bunches of grapes; its trunk rising from `root` (x, y) on the ground."""
    rx, ry = root
    b.prop(name, t(x0), t(y + 0.035), t(z - 0.16), t(x1), t(y + depth), t(z + 0.18))
    b.prop(name + " trunk", t(rx - 0.04), t(ry - 0.04), 0, t(rx + 0.04), t(ry + 0.04), t(z - 0.16))
    for px in (x0 + 0.04, x1 - 0.04):
        b.log((t(px), t(y + 0.04), t(z)), (t(px), t(y + depth - 0.02), t(z)), t(0.02), sides=6, stage=0.9)
        b.log((t(px), t(y + 0.04), t(z - 0.15)), (t(px), t(y + depth * 0.6), t(z - 0.01)), t(0.014), sides=5, stage=0.9)
    b.log((t(x0), t(y + depth - 0.02), t(z)), (t(x1), t(y + depth - 0.02), t(z)), t(0.02), sides=6, stage=0.9)
    trunk = [(rx + 0.025 * math.cos(k * 1.4), ry + 0.025 * math.sin(k * 1.4), 0.02 + (z - 0.18) * k / 10) for k in range(11)]
    b.tube_path([_pt(q) for q in trunk], t(0.02), "bark", sides=6, stage=0.88)
    if not b.detail:
        b.box(t(x0), t(y + 0.04), t(z + 0.02), t(x1 - x0), t(depth - 0.06), t(0.08), "leaves", stage=0.9)
        return
    for _ in range(int(70 * (x1 - x0) * depth / 0.5) + 30):
        lx, ly = rng.uniform(x0 + 0.02, x1 - 0.02), rng.uniform(y + 0.06, y + depth - 0.02)
        lz = z + rng.uniform(0.03, 0.12)
        tip = (lx + rng.uniform(-0.07, 0.07), ly + rng.uniform(-0.07, 0.07), lz + rng.uniform(-0.02, 0.04))
        b.strand(_pt((lx, ly, lz)), _pt(tip), t(rng.uniform(0.07, 0.1)), "leaves", stage=0.92, tint=rng.uniform(0.2, 0.9))
    for k in range(3, 10):  # leaves up the trunk
        q = trunk[k]
        b.strand(_pt(q), _pt((q[0] + rng.uniform(-0.08, 0.08), q[1] + rng.uniform(0.0, 0.06), q[2] + 0.06)), t(0.07),
                 "leaves", stage=0.92, tint=rng.uniform(0.3, 0.8))
    for _ in range(max(3, int((x1 - x0) * 3))):
        gx, gy = rng.uniform(x0 + 0.1, x1 - 0.1), rng.uniform(y + 0.12, y + depth - 0.08)
        b.lathe(t(gx), t(gy), t(z - 0.13), [(t(0.005), 0), (t(0.035), t(0.03)), (t(0.045), t(0.08)), (t(0.01), t(0.12))],
                "blue_paint", sides=8, stage=0.92, tint=0.1, smooth=2, shape=lambda a: 1.0 + 0.15 * abs(math.sin(3 * a)))


def swag(b, p0, p1, drop, part, rng, name="swag"):
    """A dyed cloth hung between two points, falling in a curve, its folds down the middle."""
    (x0, y0, z0), (x1, y1, z1) = p0, p1
    b.prop(name, t(min(x0, x1)), t(min(y0, y1) - 0.03), t(min(z0, z1) - drop - 0.02), t(max(x0, x1)), t(max(y0, y1) + 0.03),
           t(max(z0, z1)))
    def fn(u, v):
        x = x0 + (x1 - x0) * u
        sag = drop * math.sin(math.pi * u)
        return (t(x), t(y0 + 0.02 * math.sin(14 * u) * v), t(z0 + (z1 - z0) * u - (sag + 0.06) * v))
    b.surface(fn, 12 if b.detail else 3, 3 if b.detail else 1, part, stage=0.92, tint=rng.uniform(0.45, 0.75))


def column_vine(b, x, y, z0, z1, radius, rng):
    """A vine wound up a column: a thin stem spiralling round it, leaves standing off the shaft."""
    if not b.detail:
        return
    r = radius + 0.03
    pts = [(x + r * math.cos(k * 0.9), y + r * math.sin(k * 0.9), z0 + (z1 - z0) * k / 18) for k in range(19)]
    b.tube_path([_pt(q) for q in pts], t(0.012), "bark", sides=5, stage=0.92)
    for q in pts[1:]:
        for _ in range(3):
            a = math.atan2(q[1] - y, q[0] - x) + rng.uniform(-0.5, 0.5)
            tip = (x + (r + rng.uniform(0.05, 0.09)) * math.cos(a), y + (r + rng.uniform(0.05, 0.09)) * math.sin(a),
                   q[2] + rng.uniform(0.0, 0.07))
            b.strand(_pt(q), _pt(tip), t(0.07), "leaves", stage=0.92, tint=rng.uniform(0.2, 0.9))


def garland(b, x0, x1, y, z, rng, grapes=True):
    """Vine leaves along a beam's face, sagging a little between, bunches of grapes under them."""
    if not b.detail:
        return
    n = int((x1 - x0) / 0.04)
    for i in range(n):
        f = i / max(n - 1, 1)
        px = x0 + (x1 - x0) * f
        pz = z - 0.08 * math.sin(math.pi * ((f * 3) % 1.0))
        tip = (px + rng.uniform(-0.05, 0.05), y + rng.uniform(0.03, 0.07), pz + rng.uniform(-0.06, 0.04))
        b.strand(_pt((px, y, pz)), _pt(tip), t(0.07), "leaves", stage=0.92, tint=rng.uniform(0.2, 0.9))
    if grapes:
        for k in range(4):
            gx = x0 + (x1 - x0) * (k + 0.5) / 4
            b.lathe(t(gx), t(y + 0.04), t(z - 0.2), [(t(0.005), 0), (t(0.035), t(0.03)), (t(0.045), t(0.08)), (t(0.01), t(0.12))],
                    "blue_paint", sides=8, stage=0.92, tint=0.1, smooth=2)


def hanging_pots(b, x0, x1, y, z, rng, n=3, name="hung pots"):
    """Pots and painted plates on ropes from a beam's underside: a potter's shop front."""
    b.prop(name, t(x0), t(y - 0.09), t(z - 0.55), t(x1), t(y + 0.09), t(z))
    if not b.detail:
        return
    for i in range(n):
        px = x0 + (x1 - x0) * (i + 0.5) / n
        hang = rng.uniform(0.12, 0.25)
        b.tube_path([(t(px), t(y), t(z)), (t(px), t(y), t(z - hang))], t(0.004), "rope", sides=4, stage=0.92)
        r = rng.uniform(0.06, 0.08)
        b.lathe(t(px), t(y), t(z - hang - r * 2.2), [(t(r * 0.4), 0), (t(r), t(r * 0.9)), (t(r * 0.75), t(r * 1.7)),
                                                      (t(r * 0.5), t(r * 2.0)), (t(r * 0.6), t(r * 2.2))],
                "roof_tile", sides=16, stage=0.92, tint=rng.uniform(0.3, 0.8), smooth=2)
        D._painted_band(b, px, y, z - hang - r * 2.2, r * 2.2, r, [(0.4, 0), (1, 0.4), (0.75, 0.77), (0.5, 0.9), (0.6, 1)],
                        0.35, 0.6, "dark_wood", 0.92, steps=2)


def krater(b, x, y, z, rng, scale=1.0, name="krater"):
    """A big painted mixing bowl on a foot, two handles: black glaze, a red band, a stone plinth under it."""
    h, r = 0.42 * scale, 0.16 * scale
    b.prop(name, t(x - r * 1.35), t(y - r * 1.35), t(z), t(x + r * 1.35), t(y + r * 1.35), t(z + h + 0.12))
    b.box(t(x - r * 1.1), t(y - r * 1.1), t(z), t(r * 2.2), t(r * 2.2), t(0.12), "stone", stage=0.9)
    if not b.detail:
        b.column(t(x), t(y), t(z + 0.12), t(r * 0.9), t(h), "roof_tile", sides=8, stage=0.9)
        return
    ctrl = [(0.35, 0.0), (0.4, 0.06), (0.22, 0.12), (0.25, 0.2), (0.6, 0.35), (0.85, 0.6), (1.0, 0.9), (1.05, 1.0)]
    b.lathe(t(x), t(y), t(z + 0.12), [(t(r * a), t(h * c)) for a, c in ctrl], "roof_tile", sides=32, stage=0.9,
            top="oil", smooth=3)
    D._painted_band(b, x, y, z + 0.12, h, r, ctrl, 0.5, 0.88, "dark_wood", 0.9)
    D._painted_band(b, x, y, z + 0.12, h, r, ctrl, 0.42, 0.46, "cloth", 0.9, steps=1)
    for side in (-1, 1):
        pts = [(x + side * r * 0.8, y, z + 0.12 + h * 0.55), (x + side * r * 1.25, y, z + 0.12 + h * 0.6),
               (x + side * r * 1.25, y, z + 0.12 + h * 0.8), (x + side * r * 0.95, y, z + 0.12 + h * 0.85)]
        b.tube_path([_pt(q) for q in pts], t(0.016), "roof_tile", sides=8, stage=0.9)


def vine_stem(b, pts, rng):
    """A bare stem of the vine from one point to the next (from a column's top up to the roof)."""
    if not b.detail:
        return
    b.tube_path([_pt(q) for q in pts], t(0.013), "bark", sides=5, stage=0.92)
    for q in pts[1:]:
        b.strand(_pt(q), _pt((q[0] + rng.uniform(-0.06, 0.06), q[1] + 0.05, q[2] + 0.05)), t(0.07), "leaves", stage=0.92,
                 tint=rng.uniform(0.3, 0.8))


def roof_vine(b, x0, x1, y0, y1, rng, density=560):
    """A vine lying over a roof between x0-x1 and y0-y1 (y1 the eave): leaves on the tiles where rays from
    above meet them, sprays hanging down over the eave, grapes under it. Drawn after the roof it lies on."""
    import mathutils
    from mathutils.bvhtree import BVHTree
    if not b.detail:
        return
    tree = BVHTree.FromBMesh(b.bm)
    def top(x, y):
        hit = tree.ray_cast(mathutils.Vector((t(x), -t(y), 50.0)), mathutils.Vector((0, 0, -1)), 100.0)
        return None if hit[0] is None else hit[0].z / t(1.0)
    eave = []
    for _ in range(density):
        # thicker towards the eave and in two patches over the end columns, as a vine would grow
        x = rng.choice([rng.uniform(x0, x0 + 1.2), rng.uniform(x1 - 1.2, x1), rng.uniform(x0, x1)])
        y = y0 + (y1 - y0) * (1.0 - rng.random() ** 2)
        z = top(x, y)
        if z is None:
            continue
        tip = (x + rng.uniform(-0.07, 0.07), y + rng.uniform(-0.07, 0.07), z + rng.uniform(0.03, 0.07))
        b.strand(_pt((x, y, z + 0.02)), _pt(tip), t(rng.uniform(0.07, 0.1)), "leaves", stage=0.93, tint=rng.uniform(0.2, 0.9))
        if y > y1 - 0.12:
            eave.append((x, y, z))
    for x, y, z in eave[::2]:  # sprays over the eave
        drop = rng.uniform(0.15, 0.4)
        b.strand(_pt((x, y1 + 0.01, z)), _pt((x + rng.uniform(-0.04, 0.04), y1 + 0.04, z - drop)), t(0.06), "leaves",
                 stage=0.93, tint=rng.uniform(0.2, 0.8))
    for k in range(5):
        gx = x0 + 0.3 + (x1 - x0 - 0.6) * k / 4
        z = top(gx, y1 - 0.02)
        if z is not None:  # on a stalk from the eave
            b.tube_path([_pt((gx, y1 + 0.01, z - 0.02)), _pt((gx, y1 + 0.03, z - 0.1))], t(0.006), "bark", sides=4, stage=0.93)
            b.lathe(t(gx), t(y1 + 0.03), t(z - 0.2), [(t(0.005), 0), (t(0.035), t(0.03)), (t(0.045), t(0.08)),
                                                     (t(0.01), t(0.12))], "blue_paint", sides=8, stage=0.93, tint=0.1,
                    smooth=2)
