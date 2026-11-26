# Dane rozgrywki (JSON)

Maszynowa wersja specyfikacji z `docs/gameplay/`. Zasada "dane w tabelach, zachowanie w systemach" (`docs/PRAKTYKI.md`): symulacja i testy czytają te same pliki.

Stan: **referencja 1:1 z instrukcji** (S-001, S-002). Wartości liczbowe, których instrukcje nie podają, mają `null` i odsyłacz do luki `L-NNN` w polu `gaps`.

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
| `monuments.json` | pomniki pamiątkowe i piramidy |

## Konwencje

- `id`: snake_case po angielsku, stały. Nowe byty dopisujemy na końcu tablicy (stabilność identyfikatorów i zapisów).
- `ref_name`: nazwa z instrukcji. `name_pl`: robocza nazwa polska. Finalne nazwy przechodzą przegląd z `docs/02-ORYGINALNOSC.md`.
- `culture`: `greek`, `atlantean` albo `any`.
- `src`: strony instrukcji (`Z:nn`, `P:nn`).
- `gaps`: lista luk z `docs/gameplay/99-luki.md`.
- Pliki katalogowe to tablice obiektów, jeden obiekt w linii, żeby ograniczyć konflikty przy scalaniu. `military.json` i `diplomacy.json` to obiekty reguł.
- Wspólne luki wszystkich budynków: rozmiar (L-005), koszt (L-006), liczba pracowników (L-007), tempo produkcji (L-014). Nie powtarzamy ich w każdym wpisie.
- `housing.json`: liczby rodzajów kultury (`culture:1/2/3`, `culture:n+k`) są `[ZAŁOŻENIE]` do czasu zamknięcia L-011; pojemności `null` (L-009); progi atrakcyjności domów zwykłych L-010.
- Spójność odwołań (bogowie, potwory, herosi, sanktyfikowane budynki) ma być sprawdzana testem, gdy powstanie kod.
