# Dane rozgrywki (JSON)

Maszynowa wersja specyfikacji z `docs/gameplay/`. Zasada "dane w tabelach, zachowanie w systemach" (`docs/PRAKTYKI.md`): symulacja i testy czytają te same pliki.

Stan: **referencja 1:1** z instrukcji (S-001, S-002) uzupełniona liczbami z S-004 i S-005 (hierarchia źródeł: R-012, `docs/gameplay/98-konflikty-zrodel.md`). Wartości liczbowe, których instrukcje nie podają, mają `null` i odsyłacz do luki `L-NNN` w polu `gaps`.

## Pliki

| Plik | Zawartość |
|---|---|
| `terrain.json` | typy terenu i złoża |
| `goods.json` | towary: gdzie składowane, kto produkuje, kto zużywa, czy handlowe |
| `buildings.json` | wszystkie budynki: kategoria, kultura, wymagania, wejścia, wyjścia, priorytety dostaw, chodzicy, wpływ na atrakcyjność, sanktyfikacja |
| `housing.json` | poziomy domów zwykłych i elity: wymagania, regres, służba wojskowa |
| `walkers.json` | chodzicy: typ (losowy, celowy, dwufazowy), budynek źródłowy |
| `gods.json` | bogowie: ranga, sanktuarium, korzyści, sanktyfikacja, modlitwa, gniew, potwór |
| `monsters.json` | potwory: pan, heros-pogromca |
| `heroes.json` | herosi: potwory, wymagania znane z instrukcji |
| `military.json` | jednostki, rozkazy, taktyki, priorytety trier, statusy |
| `diplomacy.json` | stany dyplomatyczne, macierz próśb, skutki akcji |
| `games.json` | Igrzyska Panhelleńskie |
| `monuments.json` | pomniki pamiątkowe i piramidy (materiały per poziom) |
| `building_stats.json` | rozmiar, pracownicy, koszt per trudność (B/M/H/T/O), atrakcyjność (initial/step_tiles/step_change/range), ryzyko pożaru i zawalenia; wygenerowane z tabeli S-004 |
| `difficulty.json` | stawki podatków, płace, odsetek pracujących, nastroje, okup |
| `logistics.json` | jednostki, pojemności, doręczyciele, zużycie, limity |
| `production.json` | tempo produkcji i zasięgi dostaw |
| `gods_rules.json` | reguły ogólne bogów (limit 4 sanktuariów, sanktyfikacja, pojawianie się) |
| `quests.json` | zadania bogów |
| `enemies.json` | statystyki jednostek gracza, wrogów i boskich; składy inwazji |
| `hippodrome.json` | długość toru, konie, dochód |

## Konwencje

- `id`: snake_case po angielsku, stały. Nowe byty dopisujemy na końcu tablicy (stabilność identyfikatorów i zapisów).
- `ref_name`: nazwa z instrukcji. `name_pl`: robocza nazwa polska. Finalne nazwy przechodzą przegląd z `docs/02-ORYGINALNOSC.md`.
- `culture`: `greek`, `atlantean` albo `any`.
- `src`: strony instrukcji (`Z:nn`, `P:nn`).
- `gaps`: lista luk z `docs/gameplay/99-luki.md`.
- Pliki katalogowe to tablice obiektów, jeden obiekt w linii, żeby ograniczyć konflikty przy scalaniu. `military.json` i `diplomacy.json` to obiekty reguł.
- W tabelach per trudność `null` oznacza "–" w źródle (brak wartości, np. brak ryzyka na poziomie Beginner).
- `housing.json`: pełny model domu per trudność (`model_by_difficulty`), kolumny opisane w `docs/gameplay/18-domy-i-ekonomia-liczby.md`.
- Spójność odwołań (bogowie, potwory, herosi, sanktyfikowane budynki) ma być sprawdzana testem, gdy powstanie kod.
