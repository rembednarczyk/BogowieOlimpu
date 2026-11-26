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
- Status: potwierdzone
- Miejsce: `game/scenes/boot.gd`, infirmeria na (9, 9).
- Dowód: w 7. miesiącu 1. roku się zawala (brak pracowników i utrzymania).
- Przyczyna: błąd rozmieszczenia w skrypcie demo.

### B-003 [S2] Demo nie da się wygrać
- Status: potwierdzone
- Miejsce: `game/scenes/boot.gd` (priorytety sektorów), `scenarios/demo.json` (cel 300 mieszkańców).
- Dowód: przez 10 lat ludność 200 (25 chat po 8), farmy, zgrzeblarnia, stadnina, gimnazjon i wieże z obsadą 0; bez żywności domy zostają chatami; 0 kompanii; wieże nie strzelają.
- Przyczyna: 84 pracowników idzie na dystrybucję, utrzymanie i mitologię (priorytet 5), na żywność nie zostaje nikt.

### B-004 [S2] Morale kompanii odbudowuje się podczas walki z potworem
- Status: potwierdzone (test walki agenta)
- Miejsce: `core/src/combat.cpp`, `recover_morale` (+5 dziennie, gdy nie trwa inwazja).
- Dowód: kompanie walczące z potworem nigdy nie uciekają; żołnierz ginie co ok. 3 dni (−10), odbudowa +5 dziennie.
- Przyczyna: odbudowa sprawdza tylko inwazję, nie potwory w mieście. Do decyzji właściciela, czy walka z potworem to bitwa.

### B-005 [S3] Brak pomnika za założenie kolonii
- Status: potwierdzone
- Miejsce: `core/src/adventure.cpp`; `data/monuments.json` ma wyzwalacz `found_colony`, Z:156 wymienia pomnik kolonii.
- Dowód: po epizodzie kolonii `monuments` w mieście macierzystym jest puste.

### B-006 [S4] Pusty identyfikator scenariusza w kolejnym epizodzie macierzystym
- Status: potwierdzone
- Miejsce: `core/src/adventure.cpp`, `start_parent_episode` (podmienia cele i zdarzenia, `id` zostaje z pierwszego kroku pustego epizodu).
- Dowód: `get_scenario()["id"]` w drugim epizodzie przykładowej przygody jest pusty.

### B-007 [S1] Zapis gry w trakcie przygody gubi przygodę i miasto macierzyste
- Status: potwierdzone
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
