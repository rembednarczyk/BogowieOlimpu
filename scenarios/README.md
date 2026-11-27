# Przygody (nasz format)

Pliki JSON czytane przez `bo::core::parse_scenario` (`core/src/scenario.cpp`). Specyfikacja: `docs/gameplay/23-przygody-i-edytor.md`, luki L-093b i L-114. Treść, nazwy i mapy są nasze (R-002).

| Pole | Znaczenie |
|---|---|
| `id`, `name_key` | identyfikator i klucz nazwy w katalogach języków |
| `seed`, `treasury`, `difficulty` | ziarno RNG, skarbiec startowy, poziom (B, M, H, T, O; domyślnie H) |
| `map`, `elevation` | mapa w formacie `parse_text_map` (symbole w `docs/SILNIK.md`; punkty: `E` wejście lądowe, `S` morskie, `i` inwazja, `M` potwór, `w` wilki, `h` dziki, `d` jelenie) i wysokości (cyfra hex na kafel) |
| `cities` | miasta świata w kolejności indeksów: `id`, `relation` (ally, vassal, colony, rival, distant), `by_sea`, `sells` i `buys` (`{towar: [cena, roczny limit]}`), `opinion` 0-100, `military` i `economy` 1-6, `tribute` (dr/rok od wasala lub kolonii) |
| `events` | `type`: `invasion` (`city`, `nation`, `amount` żołnierzy, opcjonalnie `by_sea: true`: transporty z eskortą z wejścia morskiego `S`, Z:144, L-124), `request` (`city`, `good`, `amount`, `deadline_months`; `good: "army"`: prośba o `amount` kompanii, L-122), `gift` (`city`, `good` lub `drachmas`, `amount`), `relation` (`city`, `relation`), `god_attack` (`god`), `disaster` (`kind`: earthquake, tidal_wave, landslide, lava; `x`, `y`, `radius`), `monster` (`monster` z `data/monsters.json`, `strength`, `target`: food, citizens, military, resources), `quest` (`god`, `index` 0 lub 1 z `data/quests.json`, opcjonalnie `hero`), `games_host` (`host_year`: rok igrzysk olimpijskich, w którym miasto je organizuje, Z:111); `year` od 0, `month` 0-11, `repeat`: co rok od tego roku (potwory nigdy) |
| `culture` | `greek` (domyślnie) albo `atlantean`: miasto Atlantydy, budynki atlantydzkie tylko tam, greckie nigdy (P:16-17) |
| `goals` | `population` (`value`), `treasury` (`value`), `sanctuary` (`target` = bóg), `houses` (`target` = poziom, `value` = liczba domów na nim lub wyżej), `vassal` (`target` = miasto), `quest` (`target` = bóg, zadanie wykonane); z edytora (S-008, L-093b): `support` (`target` = rabble, hoplite, horseman albo trireme, `value` = żołnierzy z domów lub trier), `slay` (`target` = potwór albo pusty i `value` = liczba), `set_aside_goods` (`target` = towar, `value` = ładunki w magazynach), `survive_until` (`value` = rok od 0), `complete_before` (`value` = rok; po nim epizod przegrany), `trading_partners` (`value`), `yearly_production` (`target` = towar, `value` = ładunki dostarczone przez producentów w jednym roku, najlepszym albo bieżącym), `yearly_profit` (`value` = podatki minus płace i budowa w zeszłym roku), `hippodrome` (`value` = stadiów działającego toru), `monument` (`target` = pomnik dodatku albo pusty, `value` = liczba ukończonych) |

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
| `dispatch: {"city": n, "raid": bool, "good": towar, "ships": bool}` | atak albo rajd wszystkimi kompaniami z domu poza motłochem (Z:177-178) |
| `tactics: n` | taktyka wszystkich kompanii: 0 obronna, 1 ofensywna, 2 specjalna (Z:138-139) |
| `fulfill_requests: true` | spełnij wszystkie otwarte prośby miast (towar z magazynu, wojsko z domu) |
| `ships: n` | rozkaz dla każdej triery: 0-3 rozkazy (obrona, atak, naprawa, patrol), 4 wezwij załogę (Z:141-142) |
| `ship_order: [triera, rozkaz, x, y]` | rozkaz albo posterunek jednej triery (Z:142) |
| `demolish: [x, y]`, `oracle: [x, y]` | rozbiórka budynku z kafla; zapytaj wyrocznię sanktuarium z kafla (Z:124) |
| `gift: {"city": n, "good": towar, "amount": n}` | dar dla miasta (`drachmas`: pieniądze, Z:176-177) |
| `military_help: {"city": n, "strike": bool, "target": m}` | prośba o pomoc obronną (tylko w czasie najazdu) albo o uderzenie miasta n na miasto m (Z:174, L-125) |
| `ask_city: {"city": n, "good": towar}` | prośba do miasta o towar, który wytwarza, albo o pieniądze (Z:174-175) |
| `man_towers: bool`, `auto_defend: bool` | Man All (Z:146); samoczynna obrona, włączana tylko poza walką (Z:145, L-129) |
| `bribe: true`, `surrender: n` | przekup trwający najazd; poddaj się atakującemu miastu n, tylko raz (rywal bierze trybut co rok, Z:137, Z:172) |
| `clear: [x, y]`, `undo: true` | wytnij las z kafla (Z:19); cofnij ostatnią budowę w oknie jednego dnia (Z:20, L-035) |
| `industry: {"type": t, "on": bool}` | wyłącz albo włącz branżę (Z:62-63) |

Modyfikatory: `retry: true` (budynek: próba co dzień, aż stanie, np. dom elity czekający na towary), `when: {towar: ładunki}` (tylko przy takim zapasie w magazynach miasta, bez faktorii, Z:79), `every: n` (polecenie co n dni od `day`), `until: dzień` (z `every`: ostatni dzień), `later: true` (tick po pozostałych krokach dnia, np. Undo po budowie z tego samego dnia).

Kroki są bezstanowe: krok wykonuje się w swoim dniu, więc wczytana gra prowadzi demo dalej od bieżącego dnia (`BoSimulation.load_build_order` po wczytaniu zapisu).
