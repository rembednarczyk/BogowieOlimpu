# Plan odporności symulacji i demo

Stan na 2026-11-27. Cel: każdy problem, który dziś trzeba wyszukać ręcznie (sondą), zatrzymuje test sam, w dniu i budynku, w którym powstał. Podstawa: reguła R-016 (`docs/10-REGULY.md`), praktyki w `docs/PRAKTYKI.md` rozdz. 4-6.

## 1. Zasada (R-016)

Każde pytanie, na które odpowiedź wymagała sondy, staje się w tym samym zadaniu kontrolą automatyczną. Sonda to: zmienna `BO_DEMO_*`, doraźny program czytający zapis (`BO_DEMO_SAVE_AT`), ręczne przeglądanie raportu albo logu. Kontrola to jedno z pięciu narzędzi:

| Narzędzie | Kiedy | Gdzie |
|---|---|---|
| niezmiennik stanu | coś, co nigdy nie może być prawdą (zombie na liście najeźdźców) | `core/src/invariants.cpp`, sprawdzany co dzień w bramkach i testach losowych |
| próg zdrowia | coś, co może się zdarzyć, ale nie długo i nie często (spichlerz bez pełnej obsady) | `core/src/health.cpp`, progi per demo w bramkach |
| kontrola statyczna | coś, co widać przed symulacją (budynek poza zasięgiem dostaw) | test na plikach `scenarios/` i `data/` |
| test regresji | konkretny błąd rdzenia (B-NNN) | test, który pada bez poprawki |
| zapadka | policzalny dług, którego nie usuniemy od razu | plik bazowy, liczba tylko spada |

Wyjątek (kontrola za droga albo niemożliwa) wymaga wpisu w tabeli z rozdz. 3 z uzasadnieniem.

## 2. Co już mamy

- Determinizm: stały tick, złote hasze (`core/tests/test_golden.cpp`), powtórka z dziennika komend, zapis i odczyt z tym samym haszem, strażnik rdzenia (`tools/checks/check_core.py`: zakazane wywołania, limity długości).
- Bramki demo: Grecja, Atlantyda, bogowie (`core/tests/test_demo*.cpp`): zwycięstwo, lista mechanik z licznikami, odrzucone budowy, połączenie drogą z punktem wejścia (B-021).
- Testy regresji błędów rdzenia: B-015 do B-020, B-022.

Brakuje: niezmienników, progów zdrowia, kontroli statycznych zasięgu, testu losowego, biegu wielu ziaren, testu wytrzymałościowego.

## 3. Inwentarz: problemy z przebiegów 2026-11-27

Każdy wiersz kosztował sondę. Kolumna „Kontrola” mówi, co go złapie w przyszłości.

| # | Problem | Jak znaleziony | Dziś | Kontrola docelowa |
|---|---|---|---|---|
| 1 | Łowiska po skosie od drogi: brak dostępu, 0 % obsady (B-019) | `BO_DEMO_LOG=2` | licznik `made_fish` | statyczna: każdy budynek wymagający drogi ma dostęp; próg: budynek z pracownikami 0 % przez rok |
| 2 | Spichlerze pełne jednej żywności, ryby bez miejsca (B-019) | zrzut magazynów | licznik | próg: produkt stoi w budynku > 60 dni bez odbiorcy |
| 3 | Chata łowców poza zasięgiem 27 kafli od spichlerza (B-019) | odczyt `production.json` | licznik | statyczna: każdy producent ma odbiorcę w swoim zasięgu po drogach |
| 4 | Wybita kompania w polu na zawsze (B-018) | `BO_DEMO_INV` | test regresji | niezmiennik: kompania poza domem ma żołnierzy |
| 5 | Wróg pod pałacem bez końca, kompanie w domach | `BO_DEMO_INV` | Auto-Defend | próg: najazd kończy się w 180 dniach |
| 6 | Awaria przy brakującym żołnierzu (B-020) | `gdb catch throw` | test regresji | niezmiennik: listy żołnierzy wskazują istniejące postacie (po porządkach); test losowy z przekupstwem i rozkazami |
| 7 | Zjedzony przez smoka zostaje na liście (zombie) | sonda na zapisie | test Aresa | niezmiennik: na liście najeźdźców brak postaci z hp ≤ 0 dłużej niż tick |
| 8 | Najazd morski nie startuje, bo trwa inny; bez śladu | ręczny `fprintf` w rdzeniu | brak | cicha porażka → zdarzenie `ScenarioEventFailed{typ, powód}`, bramka wymaga zera |
| 9 | Szybsza wygrana ucina dalszą część scenariusza | raport | brak | próg: każde zdarzenie scenariusza demo zaszło przed końcem bramki |
| 10 | Undo cofa poprzedni dzień (budowa w tym samym ticku) | analiza | CHECK w bramce | test komendy: Undo w tym samym ticku i w kolejnym |
| 11 | Fontanna na drodze odrzucona, Undo cofa inną budowę | bramka | lista odrzuceń | jest (odrzucenia); dodatkowo: Undo bez rekordu daje odrzucenie z powodem |
| 12 | Dar 400 dr = 0 jednostek, odrzucony | `BO_DEMO_REJECT` | brak | próg: komendy autopilota odrzucone z powodem innym niż „czekaj” = błąd bramki |
| 13 | Prośba o wino obniża opinię, znika uderzenie sojusznika | analiza zależności | licznik | cicha porażka (12) + raport opinii miast przy zmianach |
| 14 | Kroki `retry` czekające wiecznie (Zeus bez marmuru) | raport | brak | próg: każdy krok `retry` wykonany do końca bramki |
| 15 | Faktoria pełna jednego towaru, drewno nie wchodzi | sonda na zapisie | brak | próg: budynek handlowy pełny > 90 dni przy otwartych zamówieniach importu |
| 16 | Import bez końca (48 zbroi), skarbiec maleje | sonda | brak | próg: zapas towaru > 4× limitu zamówienia; raport bilansu importu |
| 17 | Spichlerz bez pełnej obsady nie przyjmuje żywności (Z:66) | czytanie kodu | brak | próg: dni spichlerza < 100 % obsady przy żywności w produkcji ≤ 20 % |
| 18 | 30 ruin przed dniem 128 (konserwacja bez ludzi) | sonda zdarzeń | brak | próg: ruin ≤ 5; raport pierwszego zawalenia z budynkiem i obsadą konserwacji |
| 19 | Spalony pałac: brak armii, podatków, najeźdźcy na najbliższy dom | sonda budynków | brak | próg: pałac stoi od postawienia do końca |
| 20 | Kapłan Zeusa nie zdąża po owcę, utrata łaski | sonda tick po ticku | brak | próg: utrata łaski przed końcem pokazu = błąd; raport nieudanych ofiar |
| 21 | Rzemieślnicy na pustym placu (B-022) | sonda placów | test regresji | próg: plac z kompletem materiałów bez postępu > 60 dni |
| 22 | Sektor 0 % obsady przez lata (gildie, stocznia) | sonda pracy | brak | próg: sektor z budynkami ma > 0 % przez ≥ 80 % dni |
| 23 | Stocznia poza zasięgiem dostaw 40 kafli (B-021) | sonda | kontrola dróg | statyczna: zakład ma źródło wkładów w zasięgu po drogach |
| 24 | Droga brzegowa odcięta od sieci (B-021) | sonda | kontrola dróg | jest (`cut_off`) |
| 25 | Znacznik wejścia zasłonięty drogą w generatorze | kontrola dróg | `cut_off` | jest; plus statyczna: znaczniki mapy (E, S, i, f, u, w) istnieją po wczytaniu |
| 26 | Znacznik najazdu zasłonięty drogą | ślad najazdów | brak | jak 25 |
| 27 | Zasięg łowiska czytany jako 300 zamiast 25 (obiekt w `production.json`) | sonda | brak | B-023 do zbadania: dane w formie obiektu po cichu biorą wartość domyślną; statyczna: ładowanie danych bez cichych wartości domyślnych |
| 28 | Demo wrażliwe na każdą zmianę (efekt motyla) | dziesiątki przebiegów | brak | bieg demo na 3 ziarnach; próg wygranej na każdym |
| 29 | Plaga: 30 domów jednego dnia | sonda zdarzeń | licznik `plague` | próg: zdarzenia jednego rodzaju na dzień; raport |
| 30 | Ludność spada o połowę w rok | raport | brak | próg z PRAKTYKI 5: spadek > 20 % bez zdarzenia skryptowego |

## 4. Warstwy

### W1. Niezmienniki stanu (rdzeń)

Funkcja `check_invariants(const WorldState&, const DataCatalog&) -> std::vector<Violation>` (rodzaj, id, opis), bez skutków ubocznych. Wołana co dzień w bramkach, w teście losowym i w teście wytrzymałościowym; w grze tylko w kompilacji debug. Pierwszy zestaw:

- Postacie: kafel w mapie; `path_pos ≤ path.size()`; na liście najeźdźców tylko istniejące postacie z hp > 0 albo w fazie Done; postać z hp ≤ 0 i fazą inną niż Done najdłużej tick.
- Kompanie: poza domem i poza służbą za granicą mają żołnierzy; żołnierze istnieją i mają zadanie Soldier albo wracają; pożyczonych ≤ 20; własnych ≤ 20.
- Flota: statek bez załogi ma 0 rąk; ręce ≤ `crew_workers`; postać statku istnieje, chyba że statek jest za granicą.
- Magazyny: zapas ≤ pojemność; w budynku handlowym ≤ 32 ładunki.
- Domy: mieszkańcy ≤ pojemność poziomu (z przyjeżdżającymi); poziom w zakresie.
- Praca: zatrudnieni ≤ siła robocza; obsada ≤ 100 %; zatrudnieni w sektorze ≤ zapotrzebowanie.
- Sanktuaria: ukończone mają wszystkie potrzeby na 0; sanktuariów bogów ≤ 4.
- Najazd: nieaktywny ⇒ pusta lista; aktywny ⇒ ktoś na lądzie albo w transporcie.
- Mapa: każdy kafel budynku wskazuje jego id i odwrotnie; budynek na drodze tylko typu „nad drogą”.
- Scenariusz: wygrana ⇒ cele spełnione w dniu wygranej.
- Zapis: hasz po zapisie i odczycie równy haszowi (raz w miesiącu w bramkach).

Kryterium odbioru: trzy bramki i test losowy bez naruszeń; każde naruszenie znalezione przy wdrażaniu to błąd B-NNN z testem.

### W2. Ciche porażki jako zdarzenia

- Zdarzenie scenariusza, które nie zaszło (najazd, potwór, katastrofa, prośba): zdarzenie `ScenarioEventFailed{typ, powód}` w miejsce cichego pominięcia.
- Komendy autopilota odrzucone z powodu innego niż oczekiwanie (`build.sanctuary_needs_marble`, `myth.*`): błąd bramki z dniem i powodem.
- Kroki `retry` niewykonane do końca bramki: błąd z typem i kaflem.
- Ładowanie danych: pole o złym typie (obiekt zamiast liczby) daje błąd ładowania, nie wartość domyślną (B-023).

### W3. Progi zdrowia (bramki demo)

Moduł `core/src/health.cpp`: liczniki dzienne (tylko odczyt stanu), raport z pierwszym dniem i budynkiem przekroczenia. Progi w pliku per demo (`scenarios/<demo>_health.json`), żeby zmiana progu była widoczna w historii i miała uzasadnienie (PRAKTYKI 5: próg zmienia się według intencji). Wiersze 2, 5, 9, 14-22, 29, 30 z tabeli inwentarza. Najpierw tryb raportu, po strojeniu tryb CHECK.

### W4. Kontrole statyczne map i autopilotów

Przed symulacją, na każdym pliku `scenarios/`:
- znaczniki mapy (wejście, morze, najazd, łowiska, legowiska) obecne po wczytaniu;
- każdy budynek z planu: właściwy teren, wybrzeże dla nabrzeżnych, dostęp do drogi połączonej z wejściem;
- zasięgi po drogach: producent → odbiorca (`max_storage_distance`), magazyn → zakład (wkłady), łowisko → spichlerz, owce → sanktuaria (droga pieszo);
- cele scenariusza wykonalne (złoża, partnerzy handlowi sprzedający potrzebne towary).

Wykonanie: rozkład budowy symulowany „na sucho” na kopii miasta (jak `note_refusals`), potem analiza grafu dróg.

### W5. Test chaotycznego burmistrza (PRAKTYKI 4)

Na małych mapach, setki ziaren: losowe budowy, rozbiórki, rozkazy magazynów i handlu, podatki, płace, mobilizacje, przekupstwa, kapitulacje, modlitwy, najazdy; niezmienniki z W1 co dzień, zapis i odczyt co miesiąc. Ziarno, które coś złamało, trafia jako powtórka (ziarno + dziennik komend) do katalogu `core/tests/corpus/` i zostaje testem regresji na zawsze. Szybki zestaw (kilkadziesiąt ziaren) w każdym biegu, pełny w biegu nocnym.

### W6. Wiele ziaren i wytrzymałość

- Każde demo na 3 ziarnach: wygrana i lista mechanik na każdym (PRAKTYKI 5: 3 z 3). Łapie kruchość z wiersza 28.
- Test wytrzymałościowy: każde demo 100 lat bez autopilota po jego końcu: niezmienniki, brak zablokowanego najazdu, brak NaN, najgorszy tick w budżecie.
- Oba w biegu nocnym, bo trwają minuty.

### W7. Sondy jako narzędzie stałe

Dzisiejsze sondy (`BO_DEMO_LOG`, `_HOUSE`, `_INV`, `_ARMY`, `_REJECT`, `_SAVE_AT` i doraźne programy w katalogu roboczym) zbieramy w jedno narzędzie `bo_probe` (zapis + zapytanie: budynek z kafla, sektory pracy, spis budynków, oś czasu zawaleń, kompanie, flota, place sanktuariów). Każde nowe zapytanie dodane do narzędzia to sygnał R-016: czy powinno stać się progiem albo niezmiennikiem.

### W8. Zapadki

Pliki bazowe z miejscami (rodzaj, demo, kafel w bloku 8×8): budynki poza zasięgiem, przekroczenia progów zdrowia przyjęte jako wyjątek, luki P0 w zaimplementowanych systemach. Liczba tylko spada (PRAKTYKI 6).

### W9. Determinizm między platformami

Hasz po N dniach demo na Linuksie i na Androidzie (APK w trybie testu) musi być ten sam. Test w powłoce Godota uruchamiany na telefonie albo w emulatorze; w kontenerze najpierw kompilacja arm64 i porównanie w emulatorze, jeśli dostępny.

## 5. Kolejność i koszt

| Etap | Zakres | Kryterium odbioru | Koszt |
|---|---|---|---|
| E1 | W1 (niezmienniki) w bramkach; W2 (ciche porażki, kroki `retry`, odrzucenia autopilota) | trzy bramki bez naruszeń albo z błędami B-NNN i testami | 1 sesja |
| E2 | W3 progi w trybie raportu; wiersze 17-22 jako CHECK | raport zdrowia przy każdej bramce; progi CHECK zielone | 1-2 sesje (strojenie demo) |
| E3 | W4 kontrole statyczne; B-023 (dane bez cichych wartości domyślnych) | każdy plik `scenarios/` przechodzi; test łapie zepsutą mapę | 1 sesja |
| E4 | W5 test losowy z korpusem; zestaw szybki w każdym biegu | 50 ziaren w < 60 s bez naruszeń | 1 sesja |
| E5 | W6 trzy ziarna demo i wytrzymałość w biegu nocnym | 3 z 3 dla każdego demo | 1-2 sesje (strojenie kruchych demo) |
| E6 | W7 narzędzie `bo_probe`; W8 zapadki | sondy z BUILD.md przeniesione do narzędzia | 1 sesja |
| E7 | W9 hasz między platformami | ten sam hasz Linux i arm64 | zależne od emulatora |

Czas bramek dziś: ok. 1-1,5 min na demo. Niezmienniki co dzień dodają szacunkowo 10-20 %; trzy ziarna i wytrzymałość idą do biegu nocnego, żeby zwykły bieg testów nie rósł ponad ok. 5 min.

## 6. Ryzyka

- Fałszywe alarmy progów zdrowia: najpierw tryb raportu, próg ustawiany według intencji, wyjątek z uzasadnieniem.
- Strojenie demo pod progi zajmie czas (doświadczenie z demo bogów). Zysk: każda przyszła zmiana rdzenia, która zepsuje gospodarkę, pokaże przyczynę zamiast „brak zwycięstwa”.
- Test losowy znajdzie błędy w miejscach, których demo nie dotyka; to cel, ale może zatrzymać inne prace na 1-2 sesje.
