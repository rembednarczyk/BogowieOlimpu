# Potok assetów Blendera

Skrypty uruchamiane bez okna: `blender -b --factory-startup --python tools/blender/<skrypt>.py -- <argumenty>`.
Konwencje (`docs/ASSETY.md`): 1 kafel = 2 m rzeczywiste (stała `M`), front w osi −Y, punkt odniesienia w narożniku obrysu,
eksport `.glb`, wynik deterministyczny, każdy asset z wpisem w rejestrze pochodzenia.

| Skrypt | Zadanie |
|---|---|
| `bo_asset.py` | wspólny moduł: jednostki (`M`, 1 kafel = 2 m), paleta części, maski parametrów, prostopadłościan, dach, kolumna, rusztowanie, eksport `.glb` |
| `models.py` | modele budynków gry do `game/models/` z wpisem w `provenance.json`: `blender -b --factory-startup --python tools/blender/models.py [-- game/models granary ...]` |
| `placeholder_building.py` | prostopadłościan o obrysie z `data/building_stats.json` (zastępczy model do testów potoku) |
