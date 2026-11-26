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

Zgłoszone przez agenta weryfikującego (2026-11-26, przebieg demo 10 lat, test walki, przygoda przykładowa); naprawa po decyzjach właściciela.

### B-002 [S3] Infirmeria w demo bez dostępu do drogi
- Status: naprawione: nowe demo z `tools/make_demo.py`; każde postawienie z kolejności budowy sprawdza bramka `core/tests/test_demo.cpp` (odmowa = błąd testu)
- Miejsce: `game/scenes/boot.gd`, infirmeria na (9, 9).
- Dowód: w 7. miesiącu 1. roku się zawala (brak pracowników i utrzymania).
- Przyczyna: błąd rozmieszczenia w skrypcie demo.

### B-003 [S2] Demo nie da się wygrać
- Status: naprawione: demo budowane etapami przez autopilota (`scenarios/demo_build.json`), dystrybucja przed hodowlą; bramka "demo gate" wygrywa przed 7. rokiem i sprawdza każdą mechanikę
- Miejsce: `game/scenes/boot.gd` (priorytety sektorów), `scenarios/demo.json` (cel 300 mieszkańców).
- Dowód: przez 10 lat ludność 200 (25 chat po 8), farmy, zgrzeblarnia, stadnina, gimnazjon i wieże z obsadą 0; bez żywności domy zostają chatami; 0 kompanii; wieże nie strzelają.
- Przyczyna: 84 pracowników idzie na dystrybucję, utrzymanie i mitologię (priorytet 5), na żywność nie zostaje nikt.

Zgłoszone przez agenta przeglądu referencji (2026-11-26, instrukcje Zeusa i Poseidona względem kodu; commit bazowy 90bbc24).

### B-008 [S2] Spacerujący dwufazowi (filozof, aktor, zawodnik) nigdy nie wychodzą
- Status: naprawione: szkoła wysyła wykonawcę do sceny, scena wysyła krążących w trakcie występu (L-120); premia stadionu i muzeum dla miasta; testy "B-008" w `core/tests/test_culture.cpp` (padają bez poprawki)
- Miejsce: `core/src/systems.cpp:73` wysyła tylko `WalkerKind::Roaming`; `TwoPhase` ustawia jedynie `core/src/data.cpp:157`.
- Dowód: mównica, teatr i stadion nie dają kultury; w mieście greckim kulturę daje tylko gimnazjon. Bez kultury domy nie dochodzą do poziomu z wymaganiem rozrywki 35 ani do elity (Z:105-109), więc nie ma też hoplitów i jeźdźców. Testy domów ustawiają pokrycie ręcznie (`core/tests/test_housing.cpp:62`).
- Test "co dziś pada": do napisania (mapa z kolegium i mównicą, pokrycie domu kulturą filozofa).
- Przyczyna: kurs szkoła → miejsce występu nie został zaimplementowany.

### B-009 [S2] Brak limitu jednego pałacu i jednego stadionu
- Status: naprawione: pole `unique` w rozmieszczeniu (`build.only_one`), hipodrom z segmentów wyłączony; test "B-009" w `core/tests/test_culture.cpp`
- Miejsce: pole `"unique": true` w `data/buildings.json` nieczytane przez rozmieszczenie.
- Dowód: Z:95 (jeden pałac), Z:109 (jeden stadion).
- Test "co dziś pada": do napisania (drugi pałac odrzucony).
- Przyczyna: pole danych bez obsługi w `placement.cpp`.

### B-010 [S2] Miejsca występów działają bez szkół, gimnazjon szkoli bez stadionu
- Status: naprawione razem z B-008: scena bez szkoły nie występuje, gimnazjon bez stadionu nie wysyła zawodników; testy w `core/tests/test_culture.cpp`
- Miejsce: pola `trains_requires` i `target` w danych spacerujących ignorowane.
- Dowód: Z:108 (gimnazjon nie szkoli zawodnika bez stadionu), Z:105-107 (występy wymagają wykonawców ze szkół).
- Test "co dziś pada": razem z B-008.
- Przyczyna: jak B-008.

### B-011 [S3] Cel "sanktuarium" wymaga łaski boga
- Status: naprawione (decyzja właściciela): cel liczy ukończone i stojące sanktuarium bez warunku łaski; test "B-011" w `core/tests/test_gods.cpp` (pada bez poprawki)
- Miejsce: `core/src/scenario.cpp:64` sprawdza cel przez `god_blesses` (`core/src/gods.cpp:24-34`), więc cel przepada także, gdy kapłani nie znajdą ofiar.
- Dowód: instrukcja nie wiąże celu z ofiarami (Z:16, Z:21); eZeus liczy cel z postępu budowy (S-008, `eepisodegoal.cpp:378-401`).
- Przyczyna: użyto warunku błogosławieństwa w miejscu warunku "ukończone i stoi".

Znalezione przy przebudowie demo (autopilot, 2026-11-26).

### B-012 [S2] Rodziny elity nie dostają imigrantów, póki zwykłe domy mają miejsce
- Status: naprawione: dom elity liczy się w pierwszej kolejce zasiedlania (rodzina do 8 osób, L-108); test "B-012" w `core/tests/test_elite.cpp` (pada bez poprawki)
- Miejsce: `core/src/migration.cpp`, `fill_houses`.
- Dowód: w demo 3 domy elity stały puste przez 6 lat przy 2000 mieszkańców.
- Przyczyna: pierwsza kolejka bierze domy z co najmniej 8 wolnymi miejscami, a rezydencja ma 6; rosnące zwykłe domy zawsze mają miejsce.

### B-013 [S2] Nowa rodzina elity wyprowadza się po 3 dniach, zanim dostanie towary
- Status: naprawione: dom elity porzucany tylko po utracie spełnionych raz warunków (bit `established` w zapisie domu); test "B-013" w `core/tests/test_elite.cpp` (pada bez poprawki)
- Miejsce: `core/src/housing.cpp`, `update_houses`.
- Dowód: w demo rodziny wprowadzały się z towarami 0 i po 3 dniach dom był porzucony (atrakcyjność 34-67, kultura 35-60).
- Referencja: wyprowadzka po **utracie** warunków (`docs/gameplay/03-mieszkancy.md`, Z:34); eZeus trzyma nową rodzinę na poziomie poniżej rezydencji (S-008, `eelitehousing.cpp` updateLevel).
- Przyczyna: brak stanu "jeszcze nie rezydencja".

### B-014 [S3] Chata ogrodników zbiera mniej oliwek, niż podają dane
- Status: niepotwierdzone
- Miejsce: `core/src/husbandry.cpp` (zbiór: drzewo daje tyle jednostek, ile ma procent dojrzałości, czyli najwyżej 1 ładunek rocznie).
- Dowód: w demo 10 drzew przy chacie daje po roku 40-99 jednostek oliwek w chacie i ok. 6 ładunków oliwy rocznie z trzech tłoczni; `data/production.json` podaje 20 ładunków rocznie na chatę (S-004), obserwowane do 25.
- Do sprawdzenia: liczba drzew na chatę w pomiarze S-004 i czy drzewo daje więcej niż 1 ładunek; test na mapie z 20 drzewami.

### B-004 [S2] Morale kompanii odbudowuje się podczas walki z potworem
- Status: naprawione: brak odbudowy morale, póki potwór walczy w mieście (decyzja właściciela); test "B-004" w test_monsters.cpp (test walki agenta)
- Miejsce: `core/src/combat.cpp`, `recover_morale` (+5 dziennie, gdy nie trwa inwazja).
- Dowód: kompanie walczące z potworem nigdy nie uciekają; żołnierz ginie co ok. 3 dni (−10), odbudowa +5 dziennie.
- Przyczyna: odbudowa sprawdza tylko inwazję, nie potwory w mieście. Do decyzji właściciela, czy walka z potworem to bitwa.

### B-005 [S3] Brak pomnika za założenie kolonii
- Status: naprawione: pomnik kolonii po epizodzie kolonii; test w test_adventure.cpp
- Miejsce: `core/src/adventure.cpp`; `data/monuments.json` ma wyzwalacz `found_colony`, Z:156 wymienia pomnik kolonii.
- Dowód: po epizodzie kolonii `monuments` w mieście macierzystym jest puste.

### B-006 [S4] Pusty identyfikator scenariusza w kolejnym epizodzie macierzystym
- Status: naprawione: identyfikator `<przygoda>#<krok>`; test w test_adventure.cpp
- Miejsce: `core/src/adventure.cpp`, `start_parent_episode` (podmienia cele i zdarzenia, `id` zostaje z pierwszego kroku pustego epizodu).
- Dowód: `get_scenario()["id"]` w drugim epizodzie przykładowej przygody jest pusty.

### B-007 [S1] Zapis gry w trakcie przygody gubi przygodę i miasto macierzyste
- Status: naprawione: zapis przygody w kontenerze (tekst przygody, krok, kolonie, zapis miasta macierzystego, bieżąca symulacja); `load` rozpoznaje rodzaj zapisu i czyści stary stan; testy "B-007" w test_adventure.cpp i test_simulation.gd
- Miejsce: `gdext/src/bo_simulation.cpp` (`save` zapisuje tylko symulację; `AdventureRun` z zapisem miasta macierzystego jest tylko w pamięci).
- Dowód: po `load()` w nowej instancji `get_adventure()` zwraca {}; `load()` w tej samej instancji zostawia stare `run_`.
- Przyczyna: zapis przebiegu przygody nie powstał (L-119); autozapis z kroku E2 zapisuje tylko demo jednoepizodowe.

## Naprawione

### B-001 [S2] Zapis gry przekłamywał kwarantannę i dni płonięcia budynku
- Status: naprawione (commit z testem "B-001" w `core/tests/test_save.cpp`)
- Miejsce i liczba wystąpień: `core/src/serialize_city.cpp`, `read_risks`; każdy wczytany zapis od formatu v12 z domem w kwarantannie albo płonącym budynkiem.
- Dowód: demo w 412. dniu, zapis i odczyt w tym samym ticku dawały inny hash stanu; dom nr 22 miał kwarantannę 1 dzień przed zapisem i 256 po odczycie. Znalezione przy teście zapisu w interfejsie (krok E).
- Test "co dziś pada": "B-001: burning days and quarantine survive a save" pada bez poprawki, przechodzi z nią.
- Przyczyna: `write_risks` zapisuje cztery liczniki, dni płonięcia (1 bajt), potem kwarantannę; `read_risks` czytał pięć liczb 4-bajtowych, a dni płonięcia na końcu. Zapisy są zgodne wstecz: zmieniła się tylko kolejność czytania, układ bajtów został.
