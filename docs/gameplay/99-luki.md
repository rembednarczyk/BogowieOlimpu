# 99. Luki w specyfikacji

Dane potrzebne do implementacji 1:1, których źródła nie podają w całości. Priorytet: **P0** blokuje prototyp symulacji, **P1** vertical slice, **P2** wydanie. Status: **zamknięta** (źródło i miejsce), **częściowa**, **otwarta**.

Aktualizacja 2026-11-26 po S-004, S-005 (Zeus Heaven, forum) oraz S-006..S-008 (Julius, Akhenaten, eZeus). Oznaczenia [RODZINA] i [eZeus]: R-014.

## Rdzeń symulacji

| ID | Luka | P | Status |
|---|---|---|---|
| L-001 | Wybór kierunku spacerującego losowego na skrzyżowaniu | P0 | zamknięta [RODZINA]: doc 24 |
| L-002 | Zasięgi spacerujących losowych | P0 | zamknięta: S-004, doc 17 |
| L-003 | Prędkości | P0 | zamknięta: S-004, S-005, doc 17 |
| L-004 | Model czasu | P0 | zamknięta: 51 ticków/dzień, 16 dni/miesiąc, kafel = 15 kroków (S-005, S-007), doc 17, 24 |
| L-005 | Rozmiary budynków | P0 | zamknięta: `building_stats.json` |
| L-006 | Koszty budowy | P0 | zamknięta: `building_stats.json` |
| L-007 | Pracownicy | P0 | zamknięta: `building_stats.json` |
| L-008 | Sektory zatrudnienia i skala priorytetów | P0 | zamknięta: 8 sektorów (S-008 klasa A), przydział [eZeus], doc 24 |
| L-009 | Pojemności domów | P0 | zamknięta: `housing.json` |
| L-010 | Progi atrakcyjności domów | P0 | zamknięta: AppD/AppE per trudność, `housing.json` |
| L-011 | Wymagana kultura | P0 | zamknięta: punkty Ent, doc 18 |
| L-012 | Zużycie dóbr | P0 | zamknięta: doc 18 |
| L-012b | Czas "życia" punktu kultury i wizyty usługi w domu | P0 | zamknięta [RODZINA]: 96 dni, promień obsługi 2, doc 24 |
| L-013 | Atrakcyjność budynków | P0 | zamknięta: algorytm i wartości, doc 20 |
| L-014 | Tempo produkcji | P0 | zamknięta: doc 19 |
| L-015 | Pojemności magazynów | P0 | zamknięta: 32 ładunki, doc 19 |
| L-016 | Próg "Too Distant" dla vendorów | P1 | zamknięta [RODZINA]: 40 kafli (pomiar Zeusa 37), doc 24 |
| L-017 | Formuła migracji | P0 | zamknięta [RODZINA]: doc 24 |
| L-018 | Formuła popularności | P1 | częściowa: struktura [RODZINA] i składniki, doc 24; wagi płac, długu i mobilizacji to założenia |
| L-019 | Kolejność dostaw tam, gdzie instrukcja milczy | P1 | częściowa: K-005 |
| L-020 | Ryzyko zawalenia | P1 | zamknięta: 60 / damage_risk miesięcy |
| L-020b | Próg zapłonu, wzór obniżania ryzyka przez superintendenta | P1 | częściowa: zerowanie w promieniu 2 [RODZINA]; próg pożaru = założenie, doc 24 |
| L-021 | Higiena, zaraza, kwarantanna, śmiertelność | P1 | częściowa: DRI per dom (`housing.json`) |
| L-022 | Niepokoje i eskalacja przestępców | P1 | częściowa: CRI/CRIp per dom |

## Ekonomia

| ID | Luka | P | Status |
|---|---|---|---|
| L-030 | Podatki | P1 | zamknięta: wzór i tabele, doc 18 |
| L-031 | Płace | P1 | zamknięta: doc 18 |
| L-032 | Ceny towarów | P1 | częściowa: przykładowe ceny (doc 19); ceny ustala scenariusz |
| L-033 | Pojemność karawan i statków, częstotliwość | P1 | zamknięta [RODZINA]: 8/12 ładunków, 4/30 dni, doc 24 |
| L-034 | Lata długu do przegranej | P2 | częściowa: założenie 3 lata, doc 24 |
| L-035 | Okno Undo | P2 | częściowa: okno liczy się w czasie gry, "kilka chwil" (S-010; poradnik: cofnięcie wymaga odpauzowania); rozbiórkę da się cofnąć (S-005, średnia pewność). Dokładny czas nieznany; [ZAŁOŻENIE] 1 dzień (51 ticków), cofa komendy budowy jednego ticku |
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
| L-062 | Morale | P1 | częściowa [RODZINA]: doc 24 |
| L-063 | Bitwy za granicą | P1 | częściowa [eZeus]: doc 24 |
| L-064 | Skala opinii przywódców | P1 | częściowa [eZeus]: 0-100, doc 24 |
| L-065 | Trybut, okup | P1 | częściowa: okup zamknięty (doc 22); trybut ustawia scenariusz (doc 23) |
| L-066 | Jednostki atlantydzkie | P2 | częściowa: statystyki wrogów atlantydzkich, poziomy domów jak greckie (S-004) |
| L-067 | Igrzyska: start i szansa | P2 | częściowa [eZeus]: doc 24 |

## Atlantyda

| ID | Luka | P | Status |
|---|---|---|---|
| L-080 | Domy atlantydzkie | P2 | częściowa: wymagania jak greckie, nauka zamiast kultury 1:1 (S-004, S-008); Hut i Shack z greckimi modelami; nazw poziomów brak (nadamy własne, R-002) |
| L-081 | Hipodrom | P2 | zamknięta: doc 18 |
| L-082 | Piramidy | P2 | zamknięta: `monuments.json`, doc 20 |
| L-083 | Nowe struktury estetyczne | P2 | zamknięta: 12 struktur, `buildings.json` |

## Inne

| ID | Luka | P | Status |
|---|---|---|---|
| L-090 | Katastrofy | P2 | zamknięta: zdarzenia skryptowe scenariusza z punktami na mapie (doc 23) |
| L-091 | Poziom trudności | P2 | zamknięta: 5 poziomów i tabele (doc 18, 20) |
| L-092 | Progi pomników Population, Happiness | P2 | częściowa [eZeus]: Population, doc 24 |
| L-093 | Struktura przygód i celów | P1 | częściowa: doc 23; pełna lista typów celów L-093b otwarta |
| L-094 | Budowa przy pustym skarbcu (czy gra blokuje budowę, gdy brakuje drachm, i czy dopuszcza dług) | P1 | otwarta; [ZAŁOŻENIE] w rdzeniu: budowa zawsze dozwolona, skarbiec może zejść poniżej zera |
| L-095 | Przechodniość terenu poza drogą (plaża, las) dla ruchu na przełaj | P2 | otwarta; [ZAŁOŻENIE] plaża przechodnia, las nie (Z:17-18 mówi tylko o bagnie i skałach) |
| L-096 | Ustawienie budynków nabrzeżnych (czy obrys leży na lądzie, czy częściowo na wodzie; co znaczy "prosty brzeg") | P1 | częściowa: prosty, gładki odcinek brzegu, nie w wąskich kanałach i zatoczkach, łodzie potrzebują miejsca (S-001, S-009); rozmiary 2×2 i 3×3 potwierdzone (S-010). Położenie obrysu względem linii brzegu nieznane; [ZAŁOŻENIE] cały obrys na lądzie, jeden pełny bok przy wodzie, pas wody o kafel za oba narożniki |
| L-097 | Przeprawa: szerokość, największa rozpiętość, próg mała/duża | P1 | częściowa: szerokość 1 kafla, koszt za kafel (S-010, średnia pewność); typ dobierany sam: wąska woda daje małą przeprawę, pod którą statki nie przepłyną (S-001); dwie przeprawy nie stoją obok siebie. Rozpiętość maksymalna i próg nieznane; [ZAŁOŻENIE] do 8 kafli wody (jak most w grze z tej samej rodziny), próg mała/duża przy statkach (D) |
| L-098 | Kafel powrotu fontanny i lecznicy | P2 | zamknięta (średnia pewność): diagramy S-004 (`/units/`) pokazują kafel nad górnym narożnikiem budynku, stykający się z nim tylko po przekątnej; w naszej siatce to przekątna od narożnika północno-zachodniego (ten sam narożnik, od którego zaczyna się szukanie wyjścia) |
| L-100 | Dystrybucja w Agorze: ile ładunków bierze vendor, kiedy stoisko wysyła vendora, ile peddler zostawia w domu | P1 | otwarta; [ZAŁOŻENIE] vendor bierze do 4 ładunków (jak doręczyciel magazynu, doc 19), wychodzi poniżej 200 jednostek w stoisku; dom dostaje zapas na 2 miesiące żywności bieżącego albo następnego poziomu (większa wartość), 4 jednostki wełny, oliwy, wina, 1 zbroję, 1 konia, gdy poziom bieżący lub następny ich wymaga |
| L-101 | Hodowla i sady: rozmnażanie stad, przydział zwierząt do budynku, praca ogrodnika, próg dojrzałości do zbioru, zasięg dostaw z magazynu do przetwórni | P1 | otwarta; [ZAŁOŻENIE] zwierzę dołącza do najbliższej zgrzeblarni lub mleczarni z wolnym miejscem w stadzie 8; stado od 2 do 7 sztuk dostaje młode co 32 dni, młode daje pierwszy ładunek po 12 miesiącach; jeden ogrodnik na miesiąc odwiedza najdłużej nieodwiedzane drzewo i wszystkie drzewa w promieniu 2 kafli; zbieracze biorą rośliny dojrzałe w co najmniej 33 %; magazyn wysyła do 4 ładunków do przetwórni w jej zasięgu dostaw; jeden pasterz naraz |
| L-102 | Wydobycie i warsztaty: czas pracy przy złożu, liczba robotników przy niepełnej obsadzie, zapas przy budynku, kamieniarnia, mennica, pojemność zbrojowni | P1 | otwarta; [ZAŁOŻENIE] 3 robotników × obsada, kurs 25 jednostek, 7 dni pracy przy złożu (dobrane testem: 13/8/6 ładunków przy złożu w 2/10/20 kaflach, pomiar S-005: 13/10/7-8), jeden robotnik na kafel złoża, praca staje przy 4 ładunkach czekających; kamieniarnia 12/rok, gdy kamieniołom jest w 20 kaflach (cykl bloków po 8 płyt pominięty); mennica 200 srebra na 100 dr w 24 dni prosto do skarbca (pomiar 600-700/rok, u nas ok. 500); zbrojownia mieści 3 brązy, pracownia rzeźb 4; las się nie wyczerpuje |
| L-099 | Wysokość terenu | P1 | częściowa: poziomy całkowite do 15 nad terenem płaskim; rampy łączą teren różniący się o 1 poziom i stawia je edytor mapy, nie gracz; półspadek rozkłada różnicę na 2 kafle i też może mieć rampę (S-009); droga przeciągnięta przez rampę staje się schodami; po klifie nie da się budować dróg (S-005); skały do przejścia tylko przez przełęcze i schody (S-001). Nieznane: budynki na granicy poziomów, wpływ wysokości na atrakcyjność, wodę, pożar . W rdzeniu: poziomy 0-15, rampy o 1 poziom, klify blokują ruch, drogi i dostęp do drogi; [ZAŁOŻENIE] budynek stoi na jednym poziomie i nie na rampie; półspadek (rampa na 2 kafle) jeszcze niezaimplementowany |

## Kandydaci na kolejne źródła

- Pliki modelu gry (`figure_model`, pliki tekstowe w katalogu `Model`) albo ich opisy: rozstrzygną L-001, L-008, L-017, L-052, L-057b, L-062.
- Instrukcja edytora przygód (PDF z płyty).
- Strona Pecunii o narzędziach i formatach plików (link z ZeusMapper; obecnie niedostępna z tego środowiska).
- Reimplementacje gier z tej rodziny (Julius/Augustus, Akhenaten) jako źródło wiedzy o algorytmie spacerujących (GPL, tylko do lektury).
