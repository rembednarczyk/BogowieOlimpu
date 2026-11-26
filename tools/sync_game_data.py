#!/usr/bin/env python3
"""Copies data/*.json into game/data/ and scenarios/*.json into game/scenarios/ so the game reads them from
res:// (also inside an APK). Both are generated (gitignored); data/ and scenarios/ stay the source of truth."""
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
# Adventures (our content, scenarios/*.json) -> game/scenarios/ (res://scenarios).
sdst = root / 'game' / 'scenarios'
if sdst.exists():
    shutil.rmtree(sdst)
sdst.mkdir(parents=True)
for f in sorted((root / 'scenarios').glob('*.json')):
    shutil.copy2(f, sdst / f.name)
