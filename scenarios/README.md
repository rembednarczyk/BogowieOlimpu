# Przygody (nasz format)

Pliki JSON czytane przez `bo::core::parse_scenario` (`core/src/scenario.cpp`). Specyfikacja: `docs/gameplay/23-przygody-i-edytor.md`, luki L-093b i L-114. Treść, nazwy i mapy są nasze (R-002).

| Pole | Znaczenie |
|---|---|
| `id`, `name_key` | identyfikator i klucz nazwy w katalogach języków |
| `seed`, `treasury`, `difficulty` | ziarno RNG, skarbiec startowy, poziom (B, M, H, T, O; domyślnie H) |
| `map`, `elevation` | mapa w formacie `parse_text_map` (symbole w `docs/SILNIK.md`; punkty: `E` wejście lądowe, `S` morskie, `i` inwazja, `M` potwór, `w` wilki, `h` dziki, `d` jelenie) i wysokości (cyfra hex na kafel) |
| `cities` | miasta świata w kolejności indeksów: `id`, `relation` (ally, vassal, colony, rival, distant), `by_sea`, `sells` i `buys` (`{towar: [cena, roczny limit]}`), `opinion` 0-100, `military` i `economy` 1-6, `tribute` (dr/rok od wasala lub kolonii) |
| `events` | `type`: `invasion` (`city`, `nation`, `amount` żołnierzy), `request` (`city`, `good`, `amount`, `deadline_months`), `gift` (`city`, `good` lub `drachmas`, `amount`), `relation` (`city`, `relation`), `god_attack` (`god`), `disaster` (`kind`: earthquake, tidal_wave, landslide, lava; `x`, `y`, `radius`), `monster` (`monster` z `data/monsters.json`, `strength`, `target`: food, citizens, military, resources); `year` od 0, `month` 0-11, `repeat`: co rok od tego roku (potwory nigdy) |
| `goals` | `population` (`value`), `treasury` (`value`), `sanctuary` (`target` = bóg), `houses` (`target` = poziom, `value` = liczba domów na nim lub wyżej), `vassal` (`target` = miasto) |

Plik `demo.json` to scena demo z `game/scenes/boot.gd`; narzędzie `tools/sync_game_data.py` kopiuje katalog do `game/scenarios/`.
