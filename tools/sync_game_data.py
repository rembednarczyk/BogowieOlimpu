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
# Building names for the language catalogs (E8): game/i18n/buildings.csv from our names in data/buildings.json
# (Polish) and the ids (working English names until a translator writes them; R-002: never reference names).
import csv
import json
with open(root / 'data' / 'buildings.json', encoding='utf-8') as fh:
    buildings = json.load(fh)
(root / 'game' / 'i18n').mkdir(exist_ok=True)
with open(root / 'game' / 'i18n' / 'buildings.csv', 'w', encoding='utf-8', newline='') as fh:
    w = csv.writer(fh, lineterminator='\n')
    w.writerow(['keys', 'pl', 'en'])
    for b in buildings:
        pl = b.get('name_pl', b['id'])
        en = b['id'].replace('_', ' ')
        if b.get('god'):
            pl = f"{pl} ({b['god']})"
        w.writerow([f"building.{b['id']}", pl, en])
    # Goods and house levels: our Polish names, working English from the ids.
    for name, prefix in (('goods.json', 'good'), ('housing.json', 'house')):
        with open(root / 'data' / name, encoding='utf-8') as gh:
            for g in json.load(gh):
                w.writerow([f"{prefix}.{g['id']}", g.get('name_pl', g['id']), g['id'].replace('_', ' ')])
# Adventures (our content, scenarios/*.json) -> game/scenarios/ (res://scenarios).
sdst = root / 'game' / 'scenarios'
if sdst.exists():
    shutil.rmtree(sdst)
sdst.mkdir(parents=True)
for f in sorted((root / 'scenarios').glob('*.json')):
    shutil.copy2(f, sdst / f.name)
