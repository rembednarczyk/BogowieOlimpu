# Plan odporności symulacji i demo

Wersja 2, 2026-11-27. Cel: każdy problem, który dziś trzeba wyszukać ręcznie (sondą), zatrzymuje test sam, w dniu i budynku, w którym powstał. Podstawa: reguła R-016 (`docs/10-REGULY.md`), praktyki w `docs/PRAKTYKI.md` rozdz. 4-6.

## 1. Zasada i jej granica (R-016)

Kontrolą automatyczną staje się to, co **może wrócić bez ostrzeżenia**: błąd stanu, cicha porażka, błąd mapy albo autopilota. To, co jest **strojeniem jednego demo** (np. dobór priorytetów pracy w demo bogów), zostaje raportem. Sonda to zmienna `BO_DEMO_*`, program na zapisie (`BO_DEMO_SAVE_AT`) albo ręczny przegląd raportu.

| Narzędzie | Na co | Przykład z inwentarza |
|---|---|---|
| niezmiennik stanu | coś, co nigdy nie może być prawdą | zjedzony najeźdźca zostaje na liście |
| próg twardy | zawsze błąd, niezależnie od liczb gry | spalony pałac, najazd bez końca |
| raport zdrowia | zależy od balansu, który jeszcze się zmieni | spichlerz bez pełnej obsady |
| kontrola statyczna | widać przed symulacją | łowisko po skosie od drogi |
| test regresji | konkretny błąd rdzenia B-NNN | B-018, B-020, B-022 |
| zapadka | policzalny dług do spłaty z czasem | budynki poza zasięgiem w mapach |

## 2. Technologie i co już mamy

| Warstwa projektu | Technologia | Co już daje odporność |
|---|---|---|
| rdzeń symulacji | C++17 bez Godota (`core/`), CMake + Ninja, GCC i Clang | stały tick, RNG ze strumieniami, iteracja po id, złote hasze (`test_golden.cpp`), powtórka z dziennika komend, zapis i odczyt z tym samym haszem |
| testy rdzenia | doctest (`core/tests/`), ctest | ok. 290 przypadków; szybki pakiet ok. 5 s, trzy bramki demo ok. 1-1,5 min każda |
| strażnik | Python (`tools/checks/check_core.py`) | zakazane wywołania (czas, losowość spoza RNG), limity długości plików i funkcji |
| dane | JSON w `data/`, ładowane przez nlohmann/json | testy danych (`test_data.cpp`); brak kontroli typów pól (B-023) |
| scenariusze i autopiloty | JSON w `scenarios/`, generatory w Pythonie (`tools/make_demo*.py`) | bramki demo: zwycięstwo, mechaniki, odrzucone budowy, połączenie drogą z wejściem |
| most do gry | GDExtension (`gdext/`, godot-cpp) | testy GDScript bez okna (`game/tests/test_simulation.gd`, `test_ui.gd`) |
| gra | Godot 4.7.2, renderer Mobile (Vulkan; w kontenerze lavapipe przez Xvfb) | zrzuty ekranu możliwe (`game/tests/screenshot.gd`), dziś bez porównań |
| Android | NDK 28, arm64, APK debug | budowa ręczna; brak testów na urządzeniu |
| CI | GitHub Actions (`.github/workflows/ci.yml`) | strażnik, rdzeń GCC i Clang Release (zgodny hasz między kompilatorami), test symulacji w Godocie |

Luki technologiczne:
- CI nie uruchamia `test_ui.gd`.
- Nie ma kompilacji z sanitizerami: B-020 był odczytem poza mapą (`std::out_of_range`), a UB w C++ może przejść testy po cichu.
- Nie ma biegu nocnego.
- Nie ma podziału testów na szybkie i bramki.

## 3. Inwentarz: problemy z przebiegów 2026-11-27

Każdy wiersz kosztował sondę. „Warstwa” mówi, co go złapie w przyszłości (rozdz. 4).

| # | Problem | Znaleziony przez | Dziś pilnuje | Warstwa |
|---|---|---|---|---|
| 1 | Łowiska po skosie od drogi: 0 % obsady (B-019) | `BO_DEMO_LOG=2` | licznik `made_fish` | W4 |
| 2 | Spichlerze pełne jednej żywności, ryby bez miejsca (B-019) | zrzut magazynów | licznik | W3 raport (produkt bez odbiorcy) |
| 3 | Chata łowców poza zasięgiem spichlerza (B-019) | odczyt danych | licznik | W4 |
| 4 | Wybita kompania w polu na zawsze (B-018) | `BO_DEMO_INV` | test regresji | W1 |
| 5 | Wróg pod pałacem bez końca | `BO_DEMO_INV` | Auto-Defend | W3 twardy (najazd ≤ 180 dni) |
| 6 | Awaria przy brakującym żołnierzu (B-020) | `gdb catch throw` | test regresji | W1, W5, sanitizery |
| 7 | Zombie po smoku na liście najeźdźców | sonda na zapisie | test Aresa | W1 |
| 8 | Najazd morski nie startuje, bez śladu | `fprintf` w rdzeniu | brak | W2 |
| 9 | Szybsza wygrana ucina resztę scenariusza | raport | brak | W2 (zdarzenie nie zaszło do końca bramki) |
| 10 | Undo w tym samym ticku cofa poprzedni dzień | analiza | CHECK w bramce | test komendy |
| 11 | Budowa odrzucona, Undo cofa inną | bramka | lista odrzuceń | jest |
| 12 | Dar 400 dr = 0 jednostek, odrzucony | `BO_DEMO_REJECT` | brak | W2 |
| 13 | Prośba obniża opinię, znika uderzenie sojusznika | analiza zależności | licznik | W2 + raport opinii |
| 14 | Kroki `retry` czekające wiecznie | raport | brak | W2 |
| 15 | Faktoria pełna jednego towaru | sonda | brak | W3 raport |
| 16 | Import bez końca, skarbiec maleje | sonda | brak | W3 raport |
| 17 | Spichlerz bez pełnej obsady nie przyjmuje żywności (Z:66) | kod | brak | W3 raport |
| 18 | 30 ruin w 128 dni | sonda zdarzeń | brak | W3 twardy (ruiny) |
| 19 | Spalony pałac | sonda budynków | brak | W3 twardy |
| 20 | Utrata łaski: kapłan nie zdąża po owcę | sonda tick po ticku | brak | W3 twardy (łaska przed końcem pokazu) |
| 21 | Rzemieślnicy na pustym placu (B-022) | sonda placów | test regresji | W3 twardy (plac z materiałem bez postępu) |
| 22 | Sektor 0 % obsady przez lata | sonda pracy | brak | W3 raport |
| 23 | Stocznia poza zasięgiem dostaw (B-021) | sonda | kontrola dróg | W4 |
| 24 | Droga brzegowa odcięta od sieci (B-021) | sonda | `cut_off` | jest |
| 25-26 | Znaczniki 'E' i 'i' zasłonięte drogą w generatorze | kontrola dróg | `cut_off` (dla 'E') | W4 + asercja w generatorze |
| 27 | Zasięg łowiska czytany jako 300 zamiast 25 (B-023) | sonda | brak | W4 (dane) |
| 28 | Demo wrażliwe na każdą zmianę | dziesiątki przebiegów | brak | W6 (później) |
| 29 | 30 domów z zarazą jednego dnia | sonda zdarzeń | licznik | W3 raport |
| 30 | Ludność spada o połowę w rok | raport | brak | W3 raport |

## 4. Warstwy

### W1. Niezmienniki stanu (C++, rdzeń)
- **Co:** `check_invariants(const WorldState&, const DataCatalog&) -> std::vector<Violation>` w `core/src/invariants.cpp`, tylko odczyt.
- **Zestaw:**
  - postacie w mapie, `path_pos ≤ path.size()`;
  - na liście najeźdźców tylko żywi albo w fazie Done;
  - kompania poza domem i poza służbą za granicą ma żołnierzy;
  - żołnierze, statki i kapłani wskazują istniejące postacie;
  - ręce na statku ≤ `crew_workers`, 0 bez załogi;
  - zapas ≤ pojemność, w faktorii ≤ 32;
  - mieszkańcy ≤ pojemność;
  - zatrudnieni ≤ siła robocza, obsada ≤ 100 %;
  - ukończone sanktuarium ma potrzeby 0, sanktuariów bogów ≤ 4;
  - nieaktywny najazd ma pustą listę;
  - kafle budynku i mapa wskazują to samo;
  - wygrana ⇒ cele spełnione.
- **Gdzie:** co dzień w bramkach demo, w teście losowym i w teście wytrzymałościowym; raz w miesiącu hasz po zapisie i odczycie. W grze tylko w kompilacji debug (wywołanie z GDExtension).
- **Technologia:** C++17, doctest; kompilacja Debug z ASan i UBSan (`-fsanitize=address,undefined`) dla szybkiego pakietu w CI.
- **Ryzyko:** koszt czasu bramek (szacunek 10-20 %). Środek: sprawdzanie co dzień, nie co tick; pełny zestaw raz w miesiącu.
- **Ryzyko:** naruszenia znalezione przy wdrażaniu zablokują inne prace na 1-2 sesje. Środek: każde to błąd B-NNN z testem, naprawiany od razu.

### W2. Ciche porażki jako zdarzenia (C++)
- **Co:**
  - zdarzenie `ScenarioEventFailed{typ, powód}` zamiast cichego pominięcia najazdu, potwora, katastrofy, prośby;
  - bramka wymaga zera albo jawnego wyjątku w scenariuszu;
  - komenda autopilota odrzucona z powodu innego niż czekanie zatrzymuje bramkę;
  - krok `retry` niewykonany do końca bramki zatrzymuje bramkę;
  - zdarzenie scenariusza zaplanowane przed końcem bramki, które nie zaszło, zatrzymuje bramkę.
- **Technologia:** nowe zdarzenie w `event.h` (dopisane na koniec wariantu, bez zmiany formatu zapisu), obsługa w `gdext` (`event_to_dict`), komunikat w katalogach PL i EN.
- **Ryzyko:** nowe zdarzenie wymaga każdego odwiedzającego wariant (kompilator to wymusza: tak było z `GodHelped`). Koszt mały.

### W3. Progi twarde i raport zdrowia (C++, bramki)
- **Progi twarde (zatrzymują):**
  - pałac stoi od postawienia do końca;
  - ruin co najwyżej kilka;
  - każdy najazd kończy się w 180 dniach;
  - bóg z sanktuarium nie traci łaski przed końcem pokazu;
  - plac z kompletem materiałów nie stoi bez postępu ponad 60 dni.
- **Raport zdrowia (nie zatrzymuje do przeglądu balansu):**
  - spichlerz bez pełnej obsady;
  - produkt w budynku bez odbiorcy ponad 60 dni;
  - pełna faktoria;
  - zapas powyżej 4× limitu zamówienia;
  - sektor z budynkami bez pracowników;
  - zaraza i pożary dziennie;
  - spadek ludności powyżej 20 % w rok.
- **Forma:** pierwszy dzień i budynek przekroczenia, żeby raport zastąpił sondę.
- **Technologia:** `core/src/health.cpp` (liczniki dzienne, tylko odczyt), progi per demo w `scenarios/<demo>_health.json`, żeby każda zmiana progu była widoczna w historii z uzasadnieniem (PRAKTYKI 5).
- **Ryzyko:** progi ekonomiczne opierają się na liczbach [ZAŁOŻENIE] (L-112, L-130, L-131), które zmienią się po pomiarze w oryginale. Dlatego zostają raportem; staną się zatrzymujące po przeglądzie balansu.

### W4. Kontrole statyczne map, autopilotów i danych
- **Mapy i autopiloty (C++):** dla każdego pliku `scenarios/`:
  - znaczniki (wejście, morze, najazd, łowiska, legowiska) obecne po wczytaniu;
  - każdy budynek z planu na właściwym terenie, nabrzeżne przy wybrzeżu, dostęp do drogi połączonej z wejściem;
  - zasięgi po drogach: producent → odbiorca (`max_storage_distance`), magazyn → zakład, owce → sanktuaria;
  - cele wykonalne: złoża istnieją, partnerzy sprzedają potrzebne towary.
  
  Plan budowy symulowany „na sucho” na kopii miasta (jak dzisiejsze `note_refusals`), potem graf dróg (`road_distances`).
- **Generatory (Python):** asercja w `tools/make_demo*.py` na końcu: każdy znacznik postawiony w siatce jest nadal na swoim miejscu (przypadki 25-26).
- **Dane (C++):** pole o złym typie (obiekt zamiast liczby) daje błąd ładowania, nie wartość domyślną (B-023), w `test_data.cpp` na całym `data/`.
- **Ryzyko:** kontrola zasięgu powieli logikę dostaw. Środek: użyć tych samych funkcji rdzenia (`road_distances`, `receiver_accepts`), nie pisać ich od nowa (PRAKTYKI 4: kontrola liczy niezależnie od poprawki, ale z tych samych reguł danych).

### W5. Test chaotycznego burmistrza (C++, doctest)
- **Co:** na małych mapach (kilka szablonów: ląd, wybrzeże, mur):
  - losowe komendy: budowy, rozbiórki, rozkazy magazynów i handlu, podatki, płace, mobilizacje, przekupstwa, kapitulacje, modlitwy, najazdy, zapis i odczyt;
  - niezmienniki z W1 co dzień.
- **Ziarna:** szybki zestaw (ok. 50) w każdym biegu, pełny (setki) w biegu nocnym.
- **Korpus:** ziarno, które coś złamało, zapisane jako powtórka (ziarno + dziennik komend, już obsługiwany przez `test_golden.cpp`) w `core/tests/corpus/` i powtarzane zawsze.
- **Technologia:** RNG rdzenia ze strumieniem testowym (determinizm testu), doctest, sanitizery w CI.
- **Ryzyko:** znajdzie błędy w miejscach, których demo nie dotyka. To jest cel, ale może zatrzymać inne prace.
- **Ryzyko:** długość testu. Środek: małe mapy, limit dni na ziarno, pełny zestaw nocą.

### W6. Wiele ziaren i wytrzymałość
- **Wytrzymałość teraz:** każde demo po końcu autopilota jeszcze 100 lat. Niezmienniki, brak zablokowanego najazdu, najgorszy dzień w budżecie czasu.
- **Trzy ziarna później:** każde demo na 3 ziarnach z wygraną i listą mechanik, po ustabilizowaniu mechanik i przeglądzie balansu. Dziś każde demo trzeba stroić już na jednym ziarnie.
- **Technologia:** GitHub Actions, osobny przepływ z harmonogramem (bieg nocny), etykiety ctest (`fast`, `gates`, `nightly`).
- **Ryzyko:** trzy ziarna potroją pracę strojenia przy każdej zmianie mechaniki. Dlatego dopiero po przeglądzie balansu.

### W7. Testy gry i Androida
- **Co:**
  - `test_ui.gd` w CI;
  - test przejścia między epizodami przygody w powłoce gry (dziś niesprawdzone);
  - zrzuty ekranu nowych typów budynków porównywane z wzorcem (PRAKTYKI 8);
  - hasz stanu po N dniach demo na arm64 (testy rdzenia skompilowane NDK i uruchomione w emulatorze albo przez qemu-user) zgodny z Linuksem.
- **Technologia:** Godot 4.7.2 bez okna, Xvfb + lavapipe, NDK 28.
- **Ryzyko:** emulator Androida może być niedostępny w kontenerze. Wtedy qemu-user dla testów rdzenia, a test na telefonie ręcznie przy wydaniu.

### W8. Narzędzie sond i zapadki
- **Narzędzie:** zmienne `BO_DEMO_*` i doraźne programy z tej sesji zebrane w `bo_probe` (C++, czyta zapis): budynek z kafla, sektory pracy, spis budynków, oś czasu zawaleń, kompanie, flota, place. Każde nowe zapytanie to sygnał dla R-016.
- **Zapadki:** pliki bazowe z miejscami (rodzaj, demo, kafel w bloku 8×8), liczba tylko spada (PRAKTYKI 6).
- **Ryzyko:** narzędzie zestarzeje się, jeśli nikt go nie używa. Środek: każda nowa sonda dopisywana do niego, nie jako doraźny program.

## 5. Kolejność

| Etap | Zakres | Kryterium odbioru | Koszt |
|---|---|---|---|
| E1 | W1 niezmienniki + W2 ciche porażki; sanitizery dla szybkiego pakietu w CI | trzy bramki bez naruszeń albo z B-NNN i testami; CI z ASan/UBSan zielone | 1-2 sesje |
| E2 | W4 kontrole statyczne + B-023 + asercje generatorów | każdy plik `scenarios/` przechodzi; test łapie celowo zepsutą mapę | 1 sesja |
| E3 | W5 test losowy z korpusem; etykiety ctest, `test_ui.gd` w CI | 50 ziaren w < 60 s bez naruszeń | 1 sesja |
| E4 | W3 progi twarde zatrzymujące, raport zdrowia przy każdej bramce; W6 wytrzymałość w biegu nocnym | twarde progi zielone na trzech demo; raport bez progów ekonomicznych | 1-2 sesje |
| E5 | W8 narzędzie sond i zapadki | sondy z `BUILD.md` przeniesione | 1 sesja |
| E6 | W7 przejście epizodów w grze, zrzuty, hasz arm64 | test powłoki zielony; ten sam hasz Linux i arm64 | 1-2 sesje |
| po przeglądzie balansu | progi ekonomiczne z raportu zatrzymujące; demo na 3 ziarnach | 3 z 3 dla każdego demo | 2+ sesje |

Budżet czasu testów:
- **Zwykły bieg najwyżej ok. 6 min:** szybki pakiet, trzy bramki, test losowy 50 ziaren.
- **Bieg nocny:** sanitizery na pełnym pakiecie, wytrzymałość, pełny test losowy.

## 6. Ryzyka całego planu

| Ryzyko | Skutek | Środek |
|---|---|---|
| plan wyprze prace nad mechanikami | wolniejsze postępy gry przez 5-8 sesji | etapy krótkie; po E1-E3 powrót do mechanik, E4-E6 przeplatane |
| strojenie demo pod progi | czas tracony na liczby, które się zmienią | progi ekonomiczne tylko raportem do przeglądu balansu |
| fałszywe alarmy | ignorowanie testów | najpierw raport, próg według intencji, wyjątek z uzasadnieniem w pliku progów |
| wzrost czasu testów | rzadsze uruchamianie | etykiety `fast`/`gates`/`nightly`, bieg nocny |
| powielenie logiki rdzenia w kontrolach | kontrola zgodna z błędem | kontrole używają funkcji i danych rdzenia, a sprawdzają wynik niezależnie |
| sanitizery spowolnią pakiet | dłuższy CI | tylko szybki pakiet z sanitizerami w zwykłym biegu, pełny nocą |
| brak emulatora Androida | determinizm arm64 niesprawdzony | qemu-user dla testów rdzenia, telefon ręcznie przy wydaniu |
| dług z wdrożenia (znalezione błędy) | E1 i E3 dłuższe od planu | każdy błąd jako B-NNN, naprawa w tej samej sesji, limit czasu na etap z decyzją właściciela przy przekroczeniu |
