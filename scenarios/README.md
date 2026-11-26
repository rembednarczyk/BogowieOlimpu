# Przygody (nasz format)

Pliki JSON czytane przez `bo::core::parse_scenario` (`core/src/scenario.cpp`). Specyfikacja: `docs/gameplay/23-przygody-i-edytor.md`, luki L-093b i L-114. Treść, nazwy i mapy są nasze (R-002).

| Pole | Znaczenie |
|---|---|
| `id`, `name_key` | identyfikator i klucz nazwy w katalogach języków |
| `seed`, `treasury`, `difficulty` | ziarno RNG, skarbiec startowy, poziom (B, M, H, T, O; domyślnie H) |
| `map`, `elevation` | mapa w formacie `parse_text_map` (symbole w `docs/SILNIK.md`; punkty: `E` wejście lądowe, `S` morskie, `i` inwazja, `M` potwór, `w` wilki, `h` dziki, `d` jelenie) i wysokości (cyfra hex na kafel) |
| `cities` | miasta świata w kolejności indeksów: `id`, `relation` (ally, vassal, colony, rival, distant), `by_sea`, `sells` i `buys` (`{towar: [cena, roczny limit]}`), `opinion` 0-100, `military` i `economy` 1-6, `tribute` (dr/rok od wasala lub kolonii) |
| `events` | `type`: `invasion` (`city`, `nation`, `amount` żołnierzy), `request` (`city`, `good`, `amount`, `deadline_months`; `good: "army"`: prośba o `amount` kompanii, L-122), `gift` (`city`, `good` lub `drachmas`, `amount`), `relation` (`city`, `relation`), `god_attack` (`god`), `disaster` (`kind`: earthquake, tidal_wave, landslide, lava; `x`, `y`, `radius`), `monster` (`monster` z `data/monsters.json`, `strength`, `target`: food, citizens, military, resources), `quest` (`god`, `index` 0 lub 1 z `data/quests.json`, opcjonalnie `hero`), `games_host` (`host_year`: rok igrzysk olimpijskich, w którym miasto je organizuje, Z:111); `year` od 0, `month` 0-11, `repeat`: co rok od tego roku (potwory nigdy) |
| `goals` | `population` (`value`), `treasury` (`value`), `sanctuary` (`target` = bóg), `houses` (`target` = poziom, `value` = liczba domów na nim lub wyżej), `vassal` (`target` = miasto), `quest` (`target` = bóg, zadanie wykonane) |

## Przygoda wieloepizodowa (doc 23, L-119)

Pole `steps` zamienia plik w przygodę: lista kroków `{"type": "parent", "goals", "events"}` (kolejny epizod w tym samym mieście macierzystym; zadania, prośby i pomniki przechodzą dalej), `{"type": "colony", "colonies": [...]}` (gracz wybiera jedną z nieużytych kolonii; każda to pełny plik epizodu z własną mapą, a jej `id` to miasto z listy `cities` miasta macierzystego, które po epizodzie dostaje stan `colony`) i `{"type": "victory"}` na końcu. Pierwszy krok musi być `parent`. Plik bez `steps` to przygoda z jednym epizodem. Przykład: `example_adventure.json`.

Plik `demo.json` to scena demo z `game/scenes/boot.gd`; narzędzie `tools/sync_game_data.py` kopiuje katalog do `game/scenarios/`.

## Kolejność budowy (autopilot, `demo_build.json`)

Plik czytany przez `bo::core::parse_build_order` (`core/src/build_order.cpp`): skrypt burmistrza dla demo i dla bramki całej rozgrywki (`core/tests/test_demo.cpp`, `docs/PRAKTYKI.md` rozdz. 4). Plik generuje `tools/make_demo.py` razem z `demo.json`; ręcznie go nie edytujemy.

Każdy krok ma `day` (dzień od startu, 0 = od razu) i jedno polecenie:

| Pole | Znaczenie |
|---|---|
| `place`, `x`, `y`, `rot` | postaw budynek; `repeat: [n, dx, dy]` stawia n sztuk co (dx, dy) |
| `road: [x1, y1, x2, y2]` | droga w linii L: najpierw wzdłuż x, potem y |
| `roadblock: [x, y]` | zapora |
| `priority: [sektor, poziom]` | priorytet sektora pracy (0 hodowla ... 7 wojsko; poziom 0-5) |
| `tax`, `wage` | stawka podatku i płac (indeks jak w `TaxRate`, `WageRate`) |
| `trade_partner`, `trade_order`, `storage_order` z `at: [x, y]` | rozkazy budynku stojącego na kaflu `at` |
| `summon_hero: [x, y]`, `hero_quest: n`, `pray: [x, y]` | mitologia: przyzwij herosa z Sali, wyślij na zadanie n, módl się w sanktuarium |
| `stockpile: towar`, `on` | gromadzenie towaru (Z:80) |
| `not_buying: [x, y]`, `on` | "Nie kupuj" stoiska na kaflu agory `at` (pierwsze stoisko) albo stoiska (Z:72) |
| `dispatch: {"city": n, "raid": bool, "good": towar}` | atak albo rajd wszystkimi kompaniami z domu poza motłochem (Z:177-178) |
| `fulfill_requests: true` | spełnij wszystkie otwarte prośby miast (towar z magazynu, wojsko z domu) |

Modyfikatory: `retry: true` (budynek: próba co dzień, aż stanie, np. dom elity czekający na towary), `when: {towar: ładunki}` (tylko przy takim zapasie w magazynach miasta, bez faktorii, Z:79), `every: n` (polecenie co n dni od `day`).

Kroki są bezstanowe: krok wykonuje się w swoim dniu, więc wczytana gra prowadzi demo dalej od bieżącego dnia (`BoSimulation.load_build_order` po wczytaniu zapisu).
