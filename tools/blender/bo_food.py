"""Food as it lies in a granary's bins (owner, 2026-11-28: "every kind of food visible from above in the granary;
8 slots; the player sees at a glance how full it is"). A bin holds one food, up to 4 loads (doc 19: 8 slots x 4
loads); each load fills one of the bin's 4 strips, front to back, so a quarter, a half, three quarters or the
whole bin shows that food from above, each food in its own shape and colour (bo_asset.PALETTE, bo_materials.py).
The order of the foods is the game's (data/goods.json); boot.gd fills the slots in it (owner: a fixed order).
In metres, from the bin strip's north-west corner; every piece whole (R-019).
"""
import math

from bo_detail import t

FOODS = ["wheat", "carrots", "onions", "cheese", "fish", "urchins", "pork", "meat", "oranges"]


def _ball(b, x, y, z, r, part, rng, sides=6, squash=1.0):
    b.lathe(t(x), t(y), t(z), [(t(r * 0.35), 0), (t(r), t(r * 0.8 * squash)), (t(r * 0.35), t(r * 1.7 * squash))],
            part, sides=sides, stage=0.99, tint=rng.uniform(0.3, 0.8))


def _scatter(rng, w, d, n, margin):
    """n points spread over the strip (a jittered grid), so a heap covers it evenly."""
    cols = max(1, round(math.sqrt(n * w / d)))
    rows = max(1, math.ceil(n / cols))
    pts = []
    for i in range(n):
        cx, cy = i % cols, i // cols
        pts.append((margin + (cx + 0.5 + rng.uniform(-0.3, 0.3)) * (w - 2 * margin) / cols,
                    margin + (cy + 0.5 + rng.uniform(-0.3, 0.3)) * (d - 2 * margin) / rows))
    return pts


BED = {"wheat": "grain", "carrots": "carrot", "onions": "onion", "cheese": "grain", "fish": "fish",
       "urchins": "urchin", "pork": "grain", "meat": "grain", "oranges": "orange_fruit"}


def strip(b, food, x, y, z, w, d, rng):
    """One load of `food` over the strip (x, y, w, d) on the bin floor at z: a bed of it over the whole strip
    (read from above at a glance: a quarter of the bin per load, in the food's colour), its pieces on top."""
    bed = 0.05
    b.box(t(x + 0.015), t(y + 0.015), t(z), t(w - 0.03), t(d - 0.03), t(bed), BED[food], stage=0.99,
          tint=0.35 if BED[food] == food else 0.6)
    z += bed
    if food == "wheat":  # loose grain heaped along the strip, a sack at its end
        b.prism(t(x + 0.02), t(y + 0.02), t(z), t(w - 0.04), t(d - 0.04), t(0.12), "grain", along_x=False, stage=0.99)
        return
    if food == "carrots":
        for px, py in _scatter(rng, w, d, 7, 0.05):
            a = rng.uniform(0, math.pi)
            dx, dy = 0.08 * math.cos(a), 0.08 * math.sin(a)  # lying, every way
            b.obox((t(x + px), t(y + py), t(z + 0.025)), (t(dx), t(dy), 0), (t(-dy * 0.25), t(dx * 0.25), 0),
                   (0, 0, t(0.022)), "carrot", stage=0.99, tint=rng.uniform(0.3, 0.8))
        return
    if food == "onions":
        for px, py in _scatter(rng, w, d, 6, 0.06):
            _ball(b, x + px, y + py, z, 0.06, "onion", rng)
        return
    if food == "oranges":
        for px, py in _scatter(rng, w, d, 7, 0.05):
            _ball(b, x + px, y + py, z, 0.055, "orange_fruit", rng, squash=0.9)
        return
    if food == "urchins":
        for px, py in _scatter(rng, w, d, 6, 0.06):
            _ball(b, x + px, y + py, z, 0.06, "urchin", rng, sides=5, squash=0.7)
        return
    if food == "cheese":  # wheels in a row, two high
        n = max(1, int(d / 0.34))
        for i in range(n):
            cy = y + (i + 0.5) * d / n
            for k in range(2):
                b.column(t(x + w / 2), t(cy), t(z + k * 0.1), t(min(w, d / n) * 0.42), t(0.095), "cheese", sides=8,
                         stage=0.99)
        return
    if food == "fish":  # dried fish laid side by side across the strip
        n = max(2, int(d / 0.14))
        for i in range(n):
            cy = y + (i + 0.5) * d / n
            b.obox((t(x + w / 2), t(cy), t(z + 0.02)), (t(w * 0.42), 0, 0), (0, t(0.03), 0), (0, 0, t(0.018)), "fish",
                   stage=0.99, tint=rng.uniform(0.3, 0.8))
        return
    if food in ("meat", "pork"):  # joints and hams on a bed of straw
        b.box(t(x + 0.02), t(y + 0.02), t(z), t(w - 0.04), t(d - 0.04), t(0.02), "grain", stage=0.99)
        n = max(1, int(d / 0.3))
        part = "meat" if food == "meat" else "pork"
        for i in range(n):
            cy = y + (i + 0.5) * d / n
            if food == "meat":
                b.cbox(t(x + 0.06), t(cy - 0.1), t(z + 0.02), t(w - 0.12), t(0.2), t(0.09), t(0.025), part, stage=0.99)
            else:
                b.lathe(t(x + w / 2), t(cy), t(z + 0.02), [(t(0.05), 0), (t(0.12), t(0.06)), (t(0.08), t(0.13))], part,
                        sides=6, stage=0.99, tint=rng.uniform(0.3, 0.8))
        return
    raise ValueError(food)
