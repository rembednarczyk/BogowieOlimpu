# Rejestr błędów

Błąd: rozjazd między tym, co gra pokazuje albo symuluje, a specyfikacją lub tym, co gracz uzna za realne. Zgłoszenie bez dowodu ma status "niepotwierdzone".

## Ważność

| Poziom | Znaczenie |
|---|---|
| S1 | awaria, utrata zapisu, czerwony main, niedeterminizm |
| S2 | zła reguła rozgrywki, zablokowana gra, błąd widoczny dla każdego gracza |
| S3 | błąd w rzadkiej sytuacji, kosmetyka wpływająca na czytelność |
| S4 | kosmetyka |

Priorytet = ważność × zasięg / koszt. S1 zawsze pierwsze.

## Format

```
### B-NNN [S?] <tytuł>
- Status: niepotwierdzone | potwierdzone | w naprawie | naprawione (commit)
- Miejsce i liczba wystąpień:
- Dowód: <licznik / pomiar / zrzut / ziarno + log komend>
- Test "co dziś pada":
- Przyczyna:
```

## Otwarte

### B-023 [S2] Dane w postaci obiektu po cichu dają wartość domyślną
- Status: niepotwierdzone
- Miejsce i liczba wystąpień: `data/production.json` fishery `max_storage_distance: {"close": 25, "dist20": 40}`; sonda pokazała zasięg 300 (domyślny) zamiast 25 albo 40.
- Dowód: sonda na zapisie demo bogów (`catalog.production.at("fishery").max_storage_distance == 300`).
- Test "co dziś pada": do napisania (E3 planu odporności): ładowanie pola o złym typie daje błąd, nie wartość domyślną.
- Przyczyna: do zbadania; podejrzenie `value(..., default)` przy polu, które nie jest liczbą.


## Naprawione

### B-022 [S2] Rzemieślnicy czekali bez końca na placu bez materiału, gdy inne sanktuarium miało go na miejscu
- Status: naprawione
- Miejsce i liczba wystąpień: demo bogów: Zeus z kompletem materiałów na placu od dnia ok. 400 bez postępu; wcześniej Afrodyta (marmur, drewno i rzeźby na placu) i Demeter.
- Dowód: `BO_DEMO_HOUSE` na placu i gildii: `on_site marble 97 ... sculpture 22 ... wood 28` bez zmian przez lata, rzemieślnik gildii stoi przy innym placu.
- Test "co dziś pada": "B-022: artisans build where the stone is, not idle at the nearest empty site (Z:117)" (`core/tests/test_gods.cpp`), pada bez poprawki.
- Przyczyna: gildia wysyłała rzemieślnika na najbliższy niedokończony plac, a ten czekał tam, aż przyjdzie materiał. Teraz gildia wybiera najbliższy plac z materiałem na bieżącą fazę, a bezczynny rzemieślnik wraca, jeśli inny plac ma pracę (`core/src/sanctuary.cpp` `has_work`) [ZAŁOŻENIE: Z:117 nie opisuje wyboru placu].

### B-021 [S3] Magazyn z rozkazem Get nie zasilał stoczni obok (odrzucone: błąd układu demo, nie rdzenia)
- Status: odrzucone 2026-11-27
- Miejsce: demo bogów, magazyn (55,43) i stocznia (61,47), potem magazyn (49,43) i stocznia (39,47).
- Dowód: testy "B-021 probe" (`core/tests/test_goods.cpp`, `core/tests/test_naval.cpp`): magazyn z rozkazem Get zasila warsztat i stocznię przy wspólnej drodze. W demo droga brzegowa była odcięta od sieci, potem stocznia leżała poza zasięgiem 40 kafli (objazd ok. 47).
- Co zostaje: kontrola `cut_off` w bramkach demo (`core/tests/demo_probes.h`): każdy budynek z dostępem do drogi musi być połączony drogą z punktem wejścia, a mapa musi go mieć; test "B-021: the demo gates find a building whose road does not reach the land entry". Kontrola wykryła, że w obu mapach demo bogów droga zasłaniała znacznik wejścia 'E' (poprawione w `tools/make_demo_gods.py`).

### B-020 [S1] Awaria symulacji przy rozkazie dla kompanii, której żołnierz zniknął tego dnia z mapy
- Status: naprawione
- Miejsce i liczba wystąpień: demo greckie z przekupieniem najazdu (dzień 433) i z Auto-Defend: wyjątek `std::out_of_range` w `soldier_killed` (combat.cpp) i `send_company_home` (military.cpp).
- Dowód: `gdb -ex "catch throw"` na bramce demo, `map::at` dla identyfikatora usuniętego piechura.
- Test "co dziś pada": "B-020: a company breaks ranks while one of its soldiers is already gone from the map (Z:140)" i "B-020: a company is moved and sent home while one of its soldiers is already gone (Z:137-139)" (`core/tests/test_combat.cpp`); pierwszy pada bez poprawki z `map::at`.
- Przyczyna: lista żołnierzy kompanii czyszczona raz dziennie (`update_companies`), a rozkazy i ucieczka sięgały po nieistniejące postacie. Teraz każdy rozkaz czyści listę (`prune_soldiers`), a ucieczka pomija brakujących.

### B-019 [S2] Łowiska, przystań jeżowców i chata łowców dzików w demo nic nie dawały
- Status: naprawione
- Miejsce i liczba wystąpień: demo greckie, przez całe 10 lat; 4 łowiska i przystań z obsadą 0 %, chata z 4 ładunkami bez odbiorcy.
- Dowód: `BO_DEMO_LOG=2`, linie `fishery ... road 0 staff 0`, `boar_hunters_lodge ... out 4`; licznik bramki `made_fish`, `made_urchins`, `made_pork` = 0.
- Test "co dziś pada": bramka demo, mechaniki `made_fish`, `made_urchins`, `made_pork` (`core/tests/test_demo.cpp` kChains).
- Przyczyna: budynki stały po skosie od drogi (dostęp tylko przez krawędź), oba spichlerze pełne marchwi i cebuli, a chata łowców 27+ kafli od spichlerza przy zasięgu dostawy 27 (S-004). Poprawka w `tools/make_demo.py`: przesunięcie pod drogę, limity 8 ładunków marchwi, cebuli i pszenicy w spichlerzach, chata i stado przy spichlerzu (68,14).

### B-018 [S2] Kompania wybita do nogi zostawała w polu na zawsze i blokowała formowanie armii
- Status: naprawione
- Miejsce i liczba wystąpień: demo greckie, najazd Sparty w roku 6: 17 pustych kompanii motłochu w stanie Marching do końca gry, jeźdźcy z majątków nigdy się nie uformowali.
- Dowód: `BO_DEMO_INV=1`, linie `rabble/ st1 ... n0`.
- Test "co dziś pada": "B-018: a company whose soldiers all fell is at home again and forms anew from its houses (Z:136)" (`core/tests/test_combat.cpp`).
- Przyczyna: `update_companies` wracał kompanię do domu tylko ze stanu GoingHome lub Fleeing; kompania bez żołnierzy w każdym stanie poza służbą za granicą jest teraz w domu.

Od najnowszego. Źródła zgłoszeń: B-002 do B-007 agent weryfikujący, B-008 do B-011 agent przeglądu referencji, B-012 do B-015 przebudowa demo i autopilot, B-016 i B-017 testy taktyk i bramka demo.

### B-017 [S2] Rozkaz taktyki ginął przy codziennym przeformowaniu kompanii
- Status: naprawione: kompania zachowuje taktykę, nowa kompania bierze taktykę kompanii tego samego rodzaju; test "B-017" w `core/tests/test_combat.cpp` (pada bez poprawki)
- Miejsce: `core/src/military.cpp` (`form_companies`).
- Dowód: w demo po rozkazie z dnia 1000 żadna kompania nie miała taktyki innej niż obronna (licznik `tactics_set` 0); bramka tego nie złapała, bo liczyła klucz z zerem jako pokazaną mechanikę (poprawione w `core/tests/test_demo.cpp`).
- Referencja: rozkaz taktyki należy do kompanii (Z:138, `docs/gameplay/11-wojsko.md`).

### B-016 [S3] Jeźdźcy w taktyce specjalnej stali przy wrogu zamiast nękać
- Status: naprawione: jeździec po ciosie jedzie do miejsca przy sztandarze, póki ładuje następny cios; test "special tactics: horsemen harass" w `core/tests/test_combat.cpp` (pada bez poprawki)
- Miejsce: `core/src/tactics.cpp` (`fight`).
- Dowód: po ciosie trasa powrotu była kasowana w następnym ticku przez `stand`, bo wróg wciąż stał obok; jeździec zostawał na 1 pole.
- Referencja: nękanie, atak i odwrót jeźdźców (Z:139, `docs/gameplay/11-wojsko.md`).

### B-015 [S2] Pałac padał, gdy najeźdźca do niego doszedł, bez porównania sił
- Status: naprawione: pałac pada tylko przed armią silniejszą od wojska gracza w polu (suma zdrowia razy atak, [ZAŁOŻENIE, L-110]); słabsi najeźdźcy walczą dalej pod pałacem; test "B-015" w `core/tests/test_fortifications.cpp` (pada bez poprawki)
- Miejsce: `core/src/combat.cpp` (`combat_tick`, `invaders_stronger`).
- Dowód: jeden najeźdźca przy pałacu kończył bitwę porażką, nawet gdy 12 hoplitów gracza stało w mieście.
- Referencja: "wróg silniejszy od obrońców, który dotrze do Palace, zdobywa go" (Z:144, `docs/gameplay/11-wojsko.md`).
- Uwaga: wcześniejsza obserwacja "bitwa bez obrońców kończy się od razu" była błędna: w teście relacji najazd 20 żołnierzy szedł 5 dni i zdobył pałac, którego nikt nie bronił, zgodnie z Z:144.

### B-014 [S2] Winogrona bez odbiorcy blokują wysyłkę oliwek z chaty ogrodników
- Status: naprawione: chata próbuje kolejnych owoców, aż któryś ma odbiorcę w zasięgu; test "B-014" w `core/tests/test_husbandry.cpp` (pada bez poprawki)
- Miejsce: `core/src/logistics.cpp` (`next_shipment` zwracał pierwszy owoc z pełnym ładunkiem; dziś `shipments` i `ship_one`).
- Dowód: w demo chata (6, 46) trzymała 369 jednostek oliwek i 236 winogron, a jej dwie tłocznie przez lata nie dostały oliwek; winiarnia była poza zasięgiem chaty (10 kafli).
- Zgłoszenie pierwotne (wydajność poniżej danych) nie potwierdziło się: 20 pielęgnowanych drzew daje 17-20 ładunków w roku (S-004: ok. 20); test wydajności w tym samym pliku. Druga przyczyna w demo: tłocznie nie miały gdzie oddać oliwy, bo magazyny zapchała wełna (poprawione w `tools/make_demo.py`: limit wełny 12, eksport nadwyżki oliwy).
- Przyczyna: jeden dostawca na budynek i wybór towaru bez sprawdzenia odbiorcy.

### B-013 [S2] Nowa rodzina elity wyprowadza się po 3 dniach, zanim dostanie towary
- Status: naprawione: dom elity porzucany tylko po utracie spełnionych raz warunków (bit `established` w zapisie domu); test "B-013" w `core/tests/test_elite.cpp` (pada bez poprawki)
- Miejsce: `core/src/housing.cpp`, `update_houses`.
- Dowód: w demo rodziny wprowadzały się z towarami 0 i po 3 dniach dom był porzucony (atrakcyjność 34-67, kultura 35-60).
- Referencja: wyprowadzka po **utracie** warunków (`docs/gameplay/03-mieszkancy.md`, Z:34); eZeus trzyma nową rodzinę na poziomie poniżej rezydencji (S-008, `eelitehousing.cpp` updateLevel).
- Przyczyna: brak stanu "jeszcze nie rezydencja".

### B-012 [S2] Rodziny elity nie dostają imigrantów, póki zwykłe domy mają miejsce
- Status: naprawione: dom elity liczy się w pierwszej kolejce zasiedlania (rodzina do 8 osób, L-108); test "B-012" w `core/tests/test_elite.cpp` (pada bez poprawki)
- Miejsce: `core/src/migration.cpp`, `fill_houses`.
- Dowód: w demo 3 domy elity stały puste przez 6 lat przy 2000 mieszkańców.
- Przyczyna: pierwsza kolejka bierze domy z co najmniej 8 wolnymi miejscami, a rezydencja ma 6; rosnące zwykłe domy zawsze mają miejsce.

### B-011 [S3] Cel "sanktuarium" wymaga łaski boga
- Status: naprawione (decyzja właściciela): cel liczy ukończone i stojące sanktuarium bez warunku łaski; test "B-011" w `core/tests/test_gods.cpp` (pada bez poprawki)
- Miejsce: `core/src/scenario.cpp:64` sprawdza cel przez `god_blesses` (`core/src/gods.cpp:24-34`), więc cel przepada także, gdy kapłani nie znajdą ofiar.
- Dowód: instrukcja nie wiąże celu z ofiarami (Z:16, Z:21); eZeus liczy cel z postępu budowy (S-008, `eepisodegoal.cpp:378-401`).
- Przyczyna: użyto warunku błogosławieństwa w miejscu warunku "ukończone i stoi".



### B-010 [S2] Miejsca występów działają bez szkół, gimnazjon szkoli bez stadionu
- Status: naprawione razem z B-008: scena bez szkoły nie występuje, gimnazjon bez stadionu nie wysyła zawodników; testy w `core/tests/test_culture.cpp`
- Miejsce: pola `trains_requires` i `target` w danych spacerujących ignorowane.
- Dowód: Z:108 (gimnazjon nie szkoli zawodnika bez stadionu), Z:105-107 (występy wymagają wykonawców ze szkół).
- Test "co dziś pada": razem z B-008.
- Przyczyna: jak B-008.

### B-009 [S2] Brak limitu jednego pałacu i jednego stadionu
- Status: naprawione: pole `unique` w rozmieszczeniu (`build.only_one`), hipodrom z segmentów wyłączony; test "B-009" w `core/tests/test_culture.cpp`
- Miejsce: pole `"unique": true` w `data/buildings.json` nieczytane przez rozmieszczenie.
- Dowód: Z:95 (jeden pałac), Z:109 (jeden stadion).
- Test "co dziś pada": do napisania (drugi pałac odrzucony).
- Przyczyna: pole danych bez obsługi w `placement.cpp`.

### B-008 [S2] Spacerujący dwufazowi (filozof, aktor, zawodnik) nigdy nie wychodzą
- Status: naprawione: szkoła wysyła wykonawcę do sceny, scena wysyła krążących w trakcie występu (L-120); premia stadionu i muzeum dla miasta; testy "B-008" w `core/tests/test_culture.cpp` (padają bez poprawki)
- Miejsce: `core/src/systems.cpp:73` wysyła tylko `WalkerKind::Roaming`; `TwoPhase` ustawia jedynie `core/src/data.cpp:157`.
- Dowód: mównica, teatr i stadion nie dają kultury; w mieście greckim kulturę daje tylko gimnazjon. Bez kultury domy nie dochodzą do poziomu z wymaganiem rozrywki 35 ani do elity (Z:105-109), więc nie ma też hoplitów i jeźdźców. Testy domów ustawiają pokrycie ręcznie (`core/tests/test_housing.cpp:62`).
- Test "co dziś pada": do napisania (mapa z kolegium i mównicą, pokrycie domu kulturą filozofa).
- Przyczyna: kurs szkoła → miejsce występu nie został zaimplementowany.

### B-007 [S1] Zapis gry w trakcie przygody gubi przygodę i miasto macierzyste
- Status: naprawione: zapis przygody w kontenerze (tekst przygody, krok, kolonie, zapis miasta macierzystego, bieżąca symulacja); `load` rozpoznaje rodzaj zapisu i czyści stary stan; testy "B-007" w test_adventure.cpp i test_simulation.gd
- Miejsce: `gdext/src/bo_simulation.cpp` (`save` zapisuje tylko symulację; `AdventureRun` z zapisem miasta macierzystego jest tylko w pamięci).
- Dowód: po `load()` w nowej instancji `get_adventure()` zwraca {}; `load()` w tej samej instancji zostawia stare `run_`.
- Przyczyna: zapis przebiegu przygody nie powstał (L-119); autozapis z kroku E2 zapisuje tylko demo jednoepizodowe.

### B-006 [S4] Pusty identyfikator scenariusza w kolejnym epizodzie macierzystym
- Status: naprawione: identyfikator `<przygoda>#<krok>`; test w test_adventure.cpp
- Miejsce: `core/src/adventure.cpp`, `start_parent_episode` (podmienia cele i zdarzenia, `id` zostaje z pierwszego kroku pustego epizodu).
- Dowód: `get_scenario()["id"]` w drugim epizodzie przykładowej przygody jest pusty.

### B-005 [S3] Brak pomnika za założenie kolonii
- Status: naprawione: pomnik kolonii po epizodzie kolonii; test w test_adventure.cpp
- Miejsce: `core/src/adventure.cpp`; `data/monuments.json` ma wyzwalacz `found_colony`, Z:156 wymienia pomnik kolonii.
- Dowód: po epizodzie kolonii `monuments` w mieście macierzystym jest puste.

### B-004 [S2] Morale kompanii odbudowuje się podczas walki z potworem
- Status: naprawione: brak odbudowy morale, póki potwór walczy w mieście (decyzja właściciela); test "B-004" w test_monsters.cpp (test walki agenta)
- Miejsce: `core/src/combat.cpp`, `recover_morale` (+5 dziennie, gdy nie trwa inwazja).
- Dowód: kompanie walczące z potworem nigdy nie uciekają; żołnierz ginie co ok. 3 dni (−10), odbudowa +5 dziennie.
- Przyczyna: odbudowa sprawdza tylko inwazję, nie potwory w mieście. Do decyzji właściciela, czy walka z potworem to bitwa.

### B-003 [S2] Demo nie da się wygrać
- Status: naprawione: demo budowane etapami przez autopilota (`scenarios/demo_build.json`), dystrybucja przed hodowlą; bramka "demo gate" wygrywa przed 7. rokiem i sprawdza każdą mechanikę
- Miejsce: `game/scenes/boot.gd` (priorytety sektorów), `scenarios/demo.json` (cel 300 mieszkańców).
- Dowód: przez 10 lat ludność 200 (25 chat po 8), farmy, zgrzeblarnia, stadnina, gimnazjon i wieże z obsadą 0; bez żywności domy zostają chatami; 0 kompanii; wieże nie strzelają.
- Przyczyna: 84 pracowników idzie na dystrybucję, utrzymanie i mitologię (priorytet 5), na żywność nie zostaje nikt.



### B-002 [S3] Infirmeria w demo bez dostępu do drogi
- Status: naprawione: nowe demo z `tools/make_demo.py`; każde postawienie z kolejności budowy sprawdza bramka `core/tests/test_demo.cpp` (odmowa = błąd testu)
- Miejsce: `game/scenes/boot.gd`, infirmeria na (9, 9).
- Dowód: w 7. miesiącu 1. roku się zawala (brak pracowników i utrzymania).
- Przyczyna: błąd rozmieszczenia w skrypcie demo.

### B-001 [S2] Zapis gry przekłamywał kwarantannę i dni płonięcia budynku
- Status: naprawione (commit z testem "B-001" w `core/tests/test_save.cpp`)
- Miejsce i liczba wystąpień: `core/src/serialize_city.cpp`, `read_risks`; każdy wczytany zapis od formatu v12 z domem w kwarantannie albo płonącym budynkiem.
- Dowód: demo w 412. dniu, zapis i odczyt w tym samym ticku dawały inny hash stanu; dom nr 22 miał kwarantannę 1 dzień przed zapisem i 256 po odczycie. Znalezione przy teście zapisu w interfejsie (krok E).
- Test "co dziś pada": "B-001: burning days and quarantine survive a save" pada bez poprawki, przechodzi z nią.
- Przyczyna: `write_risks` zapisuje cztery liczniki, dni płonięcia (1 bajt), potem kwarantannę; `read_risks` czytał pięć liczb 4-bajtowych, a dni płonięcia na końcu. Zapisy są zgodne wstecz: zmieniła się tylko kolejność czytania, układ bajtów został.
