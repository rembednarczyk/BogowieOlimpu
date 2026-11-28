# 19. Produkcja i logistyka: liczby

Źródła: S-004 (`industry-production`, `buildings`), S-005 (Caesar Alan, JonN "Everything You Ever Wanted to Know About Industries in Zeus", Moquel, Plebus, MarvL). Dane: `data/production.json`, `data/logistics.json`, `data/building_stats.json`.

## Jednostki i pojemności (zamyka L-015)

- Storehouse i Granary: 8 pól × 4 ładunki = **32 ładunki**; rzeźba zajmuje całe pole (1 na pole).
- Doręczyciel budynku produkcyjnego: 1 ładunek; magazynu, spichlerza, budynku handlowego: 4 ładunki; rzeźba: 1.
- Zasięg dostaw przy danym tempie produkcji (doręczyciel musi wrócić przed kolejnym ładunkiem):

| Ładunki/rok | Maks. odległość do odbiorcy (kafle) |
|---|---|
| 6 | 54 |
| 9 | 36 |
| 12 | 27 |
| 16 | 20 |

Gdy doręczyciel nie wróci na czas, produkcja staje na 100% i **nie pokazuje się to w widoku problemów** (S-005). To zachowanie referencyjne; widoczność problemu poprawimy jako rozbudowę (kandydat na odstępstwo).

## Tempo produkcji (zamyka L-014, L-036, L-037, L-039, L-040)

| Budynek | Tempo | Uwagi |
|---|---|---|
| Farmy (pszenica, marchew, cebula) | 8 ładunków/rok (800 jedn.) | jednorazowo w miesiącu żniw przy pełnej obsadzie przez rok; odbiorca może być dowolnie daleko |
| Dairy, Carding Shed | 7-8/rok | maks. 8 zwierząt; zwierzę daje 1 ładunek raz w roku; nowe: po 6 mies., młode: po 12 mies.; młode rodzą się tylko poniżej 8; budynek mieści 5 ładunków |
| Corral | 12-14/rok | 8 sztuk (7 krów, 1 byk); sztuka po roku daje 2 ładunki; ubój 6-7 sztuk/rok; po uboju rodzi się cielę |
| Growers' Lodge | ~20/rok (do 25) | 1 pielęgnujący, 3 zbieracze w żniwa; mieści 5 ładunków każdego owocu; magazyn musi stać obok (5-10 kafli) |
| Orange Tenders' Lodge | ~15/rok | jak Growers' Lodge |
| Boar Hunter's Lodge, Hunting Lodge | ~12/rok | stado 7 dzików; poniżej 7 rodzi się dzik co 26 e-dni; 1 lodge na stado |
| Fishery, Urchin Quay | 13/rok blisko, 8-10 przy 20 kaflach | cykl: 16 e-dni połowu + 10 e-dni rozładunku; Fishery mieści 1 ładunek (nadmiar przepada), Urchin Quay 5 |
| Foundry, Refinery, Timber Mill | 13 blisko, 10 przy 10 kaflach, 7-8 przy 20 kaflach | 3 robotników po 25 jednostek na kurs; 1 górnik na skałę naraz; Foundry wysyła ładunek przy 100 jednostkach |
| Mint | 600-700 dr blisko, 500 przy 10, 300-400 przy 20 kaflach | przetapia 200 srebra na 100 dr w 1-2 mies.; mieści 300 srebra |
| Masonry Shop, Black Marble Workshop | 12/rok | blok na 8 płyt kopany 2 mies.; 1 płyta/mies.; nowy blok przy 3 płytach |
| Olive Press, Winery | 6/rok | 1 owoc na 1 produkt; mieści 3 jednostki wejścia |
| Armory | 6/rok | 1 brąz na zbroję |
| Sculpture Studio | 6/rok | **4 brązu na rzeźbę**; nie rusza przy mniej niż 4 |
| Horse Ranch | 6 koni/rok | 50 jedn. pszenicy na konia; koń czekający w stadninie zjada 10 jedn./mies.; maks. 6 koni; odbiorca do 40 kafli |
| Chariot Factory | 6 rydwanów/rok | drewno + koń; odbiorca do 40 kafli |

## Sady: dojrzewanie

- Roślina dojrzewa od wizyt pielęgnującego: brak wizyty przez ≥ 6 mies. połowi tempo, ≥ 10 mies. zatrzymuje dojrzewanie (S-005, Kraken).
- Procent dojrzałości = liczba jednostek do zebrania.
- Żniwa: winogrona X-XII, oliwki I-III (3 miesiące). Niezebrane spada do 0%; roślina poniżej ok. 33% dojrzałości przechodzi na kolejny rok.
- Zbieracze idą do najbliższej rośliny do zebrania, nawet na drugim końcu mapy; zatrzymują się, gdy Lodge jest pełny.

## Hodowla

- Pasterz idzie do zwierzęcia gotowego do strzyży lub dojenia, nawet przez całą mapę (trzymać stado w jednym miejscu).
- Kapłani zabierają zwierzęta na ofiary, wilki porywają: realna produkcja 700-800 jednostek/rok.
- Dowolna liczba owiec i kóz zmieści się na jednym kaflu łąki (S-004 trivia).

## Magazyny: zachowanie

- Nowy Storehouse przyjmuje wszystko poza pszenicą.
- Para magazynów Accept/Get przenosi do 48 ładunków rocznie na 54 kafle (2 doręczycieli Get × 4 ładunki).
- Empty: magazyn wysyła doręczyciela Empty i doręczyciela produkcyjnego.
- Pier i Trading Post mają doręczyciela rozwożącego i pobierającego, po 4 ładunki; tylko drogami. Towar oddany do sprzedaży nie wraca.
- Maks. **10 Pierów handlowych** w mieście (S-005, obserwacja).

## Materiały budowlane

Wóz z marmurem, drewnem i orichalkiem czeka na 4 ładunki w jednym magazynie (Z:118). Rzeźby po 1. Szczegóły faz budowy sanktuarium: `21-mitologia-liczby.md`.

## Dochodowość eksportu (orientacyjnie, ceny ze scenariusza "Economic Sandbox")

| Produkt | Cena za ładunek |
|---|---|
| brąz | 110 |
| drewno | 75 |
| marmur | 84 |
| oliwki | 38 |
| winogrona | 42 |
| oliwa | 145 |
| wino | 160 |
| zbroja | 200 |
| rzeźba | 640 |
| wełna | 60 |

Ceny ustala scenariusz; tabela jest punktem wyjścia do balansu (L-032 częściowo).

## Utrzymanie

- Superintendent pokrywa odcinek drogi do 43 kafli albo pętlę do 80 kafli (S-005, JonN).
- Ryzyko zawalenia: `60 / damage_risk = miesiące do zawalenia bez wizyty superintendenta` (S-004 legenda). Wartości per budynek i trudność w `data/building_stats.json` (zamyka L-020 dla zawaleń; pożar L-020b: wartości przyrostu znane, próg zapłonu nie).
