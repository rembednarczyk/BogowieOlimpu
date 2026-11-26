# 99. Luki w specyfikacji

Dane potrzebne do implementacji 1:1, których źródła nie podają w całości. Priorytet: **P0** blokuje prototyp symulacji, **P1** vertical slice, **P2** wydanie. Status: **zamknięta** (źródło i miejsce), **częściowa**, **otwarta**.

Aktualizacja 2026-11-26 po S-004 (Zeus Heaven) i S-005 (wątki forum).

## Rdzeń symulacji

| ID | Luka | P | Status |
|---|---|---|---|
| L-001 | Wybór kierunku chodzika losowego na skrzyżowaniu | P0 | otwarta (znane: limit kafli, powrót najkrótszą drogą, punkty wyjścia; doc 17) |
| L-002 | Zasięgi chodzików losowych | P0 | zamknięta: S-004, doc 17 |
| L-003 | Prędkości | P0 | zamknięta: S-004, S-005, doc 17 |
| L-004 | Model czasu | P0 | zamknięta: miesiąc = 16 dni × 51 klatek, kafel = 15 kroków (S-005), doc 17 |
| L-005 | Rozmiary budynków | P0 | zamknięta: `building_stats.json` |
| L-006 | Koszty budowy | P0 | zamknięta: `building_stats.json` |
| L-007 | Pracownicy | P0 | zamknięta: `building_stats.json` |
| L-008 | Sektory zatrudnienia i skala priorytetów | P0 | otwarta |
| L-009 | Pojemności domów | P0 | zamknięta: `housing.json` |
| L-010 | Progi atrakcyjności domów | P0 | zamknięta: AppD/AppE per trudność, `housing.json` |
| L-011 | Wymagana kultura | P0 | zamknięta: punkty Ent, doc 18 |
| L-012 | Zużycie dóbr | P0 | zamknięta: doc 18 |
| L-012b | Czas "życia" punktu kultury i wizyty usługi w domu | P0 | otwarta |
| L-013 | Atrakcyjność budynków | P0 | zamknięta: algorytm i wartości, doc 20 |
| L-014 | Tempo produkcji | P0 | zamknięta: doc 19 |
| L-015 | Pojemności magazynów | P0 | zamknięta: 32 ładunki, doc 19 |
| L-016 | Próg "Too Distant" dla vendorów | P1 | częściowa: food vendor 37 kafli; pozostali nieznani |
| L-017 | Formuła migracji | P0 | otwarta |
| L-018 | Formuła popularności | P1 | częściowa: wpływ stawki podatku (doc 18) |
| L-019 | Kolejność dostaw tam, gdzie instrukcja milczy | P1 | częściowa: K-005 |
| L-020 | Ryzyko zawalenia | P1 | zamknięta: 60 / damage_risk miesięcy |
| L-020b | Próg zapłonu, wzór obniżania ryzyka przez superintendenta | P1 | otwarta |
| L-021 | Higiena, zaraza, kwarantanna, śmiertelność | P1 | częściowa: DRI per dom (`housing.json`) |
| L-022 | Niepokoje i eskalacja przestępców | P1 | częściowa: CRI/CRIp per dom |

## Ekonomia

| ID | Luka | P | Status |
|---|---|---|---|
| L-030 | Podatki | P1 | zamknięta: wzór i tabele, doc 18 |
| L-031 | Płace | P1 | zamknięta: doc 18 |
| L-032 | Ceny towarów | P1 | częściowa: przykładowe ceny (doc 19); ceny ustala scenariusz |
| L-033 | Pojemność karawan i statków, częstotliwość | P1 | otwarta |
| L-034 | Lata długu do przegranej | P2 | otwarta |
| L-035 | Okno Undo | P2 | otwarta |
| L-036 | Wydajność Mint | P1 | zamknięta: doc 19 (próg K-009 otwarty) |
| L-037 | Zwierzęta na budynek | P1 | zamknięta: 8 |
| L-038 | Odrost wypasionej łąki | P2 | otwarta |
| L-039 | Wzrost drzew, żniwa | P1 | zamknięta: doc 19 |
| L-040 | Wpływ odległości od złoża | P1 | zamknięta: doc 19 |

## Religia i mit

| ID | Luka | P | Status |
|---|---|---|---|
| L-050 | Materiały i koszty sanktuariów | P1 | zamknięta: doc 21 |
| L-051 | Rozmiary sanktuariów | P1 | zamknięta (K-021 otwarty) |
| L-052 | Częstotliwość ofiar, czas do utraty łaski | P1 | otwarta |
| L-053 | Szansa modlitwy, cooldowny | P1 | częściowa: kolejność bogów (doc 21) |
| L-054 | Sanktyfikacja | P1 | zamknięta: 4× produkcji przy kolejnym kursie, Apollo 2× wykonawców |
| L-054b | Czas trwania klątw per trudność | P1 | otwarta |
| L-055 | Co wywołuje wrogość boga | P1 | częściowa: wrogość i ataki ustawia scenariusz (doc 23) |
| L-056 | Wymagania herosów | P1 | zamknięta: doc 21 |
| L-057 | Potwory w scenariuszu | P1 | częściowa: ustawienia scenariusza (doc 21, 23) |
| L-057b | Statystyki walki potworów, herosów, bogów | P1 | otwarta |
| L-058 | Treść przepowiedni | P2 | otwarta (treść piszemy sami) |

## Wojsko i świat

| ID | Luka | P | Status |
|---|---|---|---|
| L-060 | Statystyki jednostek | P1 | zamknięta: doc 22, `enemies.json` |
| L-061 | Odsetek służących | P1 | zamknięta: %S i #S w `housing.json` |
| L-062 | Morale | P1 | otwarta |
| L-063 | Bitwy za granicą | P1 | otwarta |
| L-064 | Skala opinii przywódców | P1 | otwarta |
| L-065 | Trybut, okup | P1 | częściowa: okup zamknięty (doc 22); trybut ustawia scenariusz (doc 23) |
| L-066 | Jednostki atlantydzkie | P2 | częściowa: statystyki wrogów atlantydzkich, poziomy domów jak greckie (S-004) |
| L-067 | Igrzyska: start i szansa | P2 | otwarta |

## Atlantyda

| ID | Luka | P | Status |
|---|---|---|---|
| L-080 | Domy atlantydzkie | P2 | częściowa: S-004 traktuje poziomy jak greckie, z nauką w miejsce kultury i jednostkami archer/spearman/charioteer; nazw poziomów brak |
| L-081 | Hipodrom | P2 | zamknięta: doc 18 |
| L-082 | Piramidy | P2 | zamknięta: `monuments.json`, doc 20 |
| L-083 | Nowe struktury estetyczne | P2 | zamknięta: 12 struktur, `buildings.json` |

## Inne

| ID | Luka | P | Status |
|---|---|---|---|
| L-090 | Katastrofy | P2 | zamknięta: zdarzenia skryptowe scenariusza z punktami na mapie (doc 23) |
| L-091 | Poziom trudności | P2 | zamknięta: 5 poziomów i tabele (doc 18, 20) |
| L-092 | Progi pomników Population, Happiness | P2 | otwarta |
| L-093 | Struktura przygód i celów | P1 | częściowa: doc 23; pełna lista typów celów L-093b otwarta |

## Kandydaci na kolejne źródła

- Pliki modelu gry (`figure_model`, pliki tekstowe w katalogu `Model`) albo ich opisy: rozstrzygną L-001, L-008, L-017, L-052, L-057b, L-062.
- Instrukcja edytora przygód (PDF z płyty).
- Strona Pecunii o narzędziach i formatach plików (link z ZeusMapper; obecnie niedostępna z tego środowiska).
- Reimplementacje gier z tej rodziny (Julius/Augustus, Akhenaten) jako źródło wiedzy o algorytmie chodzików (GPL, tylko do lektury).
