# Potok assetów Blendera

Skrypty uruchamiane bez okna: `blender -b --factory-startup --python tools/blender/<skrypt>.py -- <argumenty>`.
Konwencje (`docs/ASSETY.md`): 1 kafel = 1 m, front w osi −Y, punkt odniesienia w narożniku obrysu,
eksport `.glb`, wynik deterministyczny, każdy asset z wpisem w rejestrze pochodzenia.

| Skrypt | Zadanie |
|---|---|
| `placeholder_building.py` | prostopadłościan o obrysie z `data/building_stats.json` (zastępczy model do testów potoku) |
