#!/usr/bin/env python3
"""Copies data/*.json into game/data/ so the game reads it from res://data (also inside an APK).
game/data/ is generated (gitignored); data/ stays the single source of truth."""
import pathlib
import shutil
import sys

root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '.')
src, dst = root / 'data', root / 'game' / 'data'
if dst.exists():
    shutil.rmtree(dst)
dst.mkdir(parents=True)
n = 0
for f in sorted(src.glob('*.json')):
    shutil.copy2(f, dst / f.name)
    n += 1
print(f"sync_game_data: {n} files -> {dst}")
