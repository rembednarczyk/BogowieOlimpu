# Zasady pracy w repozytorium Bogowie Olimpu

Plik czytany przez każdą sesję i każdego agenta. Pełne praktyki: `docs/PRAKTYKI.md`. Brief dla agenta zawiera tylko zadanie i odstępstwa od tych zasad.

## Start sesji

Przeczytaj w tej kolejności:
1. ten plik,
2. `docs/BACKLOG.md` (sekcja "Następne"),
3. `docs/DECYZJE.md`,
4. `docs/BLEDY.md`,
5. `docs/11-OTWARTE-PYTANIA.md`,
6. otwarte PR-y gałęzi roboczej.

Czerwony main jest pierwszą pracą sesji.

## Czym jest projekt

Citybuilder w mitologii greckiej na telefon (oraz tablet i PC). **Mechanika 1:1 z grą referencyjną i dodatkiem** (R-001), opisana w `docs/gameplay/` i `data/`. Nazwy, grafika, audio, teksty, mapy i kampanie są nasze (R-002, `docs/02-ORYGINALNOSC.md`).

## Mapa repozytorium

| Ścieżka | Zawartość |
|---|---|
| `docs/gameplay/` | specyfikacja rozgrywki 1:1 z cytatami stron instrukcji; `99-luki.md` to brakujące dane |
| `data/` | specyfikacja w JSON, czytana przez symulację i testy |
| `docs/PRAKTYKI.md` | zasady inżynierskie i procesowe |
| `docs/DECYZJE.md`, `docs/adr/` | dziennik decyzji, ADR |
| `docs/BACKLOG.md`, `docs/BLEDY.md`, `docs/ODSTEPSTWA.md` | backlog, rejestr błędów, rejestr odstępstw od referencji |
| `docs/09-ZRODLA.md`, `docs/10-REGULY.md` | rejestr źródeł S-NNN, reguły R-NNN |
| `docs/00-PLAN.md` ... `docs/08-OPRAWA.md` | plan, wizja, oryginalność, analiza, symulacja, UX, technologia, oprawa |
| `docs/SILNIK.md` | plan rozbudowy silnika na Godocie, moduły A-F, potok Blendera |
| `docs/ASSETY.md` | inwentarz assetów do wyprodukowania |
| `core/` | rdzeń symulacji C++17 bez Godota: `include/bo/core/*.h`, `src/`, `tests/` (doctest) |
| `gdext/` | GDExtension: klasa `BoSimulation` (jedyne miejsce łączące rdzeń z Godotem) |
| `game/` | projekt Godota (renderer Mobile): `scenes/`, `tests/` (test bez okna, zrzuty); `game/data/` generowane z `data/` |
| `tools/` | `checks/check_core.py` (strażnik rdzenia), `sync_game_data.py`, `blender/` (potok assetów) |
| `third_party/` | doctest, nlohmann/json, godot-cpp (submoduł); rejestr w `third_party/README.md` |
| `BUILD.md` | polecenia budowania i testów |

## Zasady nadrzędne (skrót `docs/PRAKTYKI.md`)

1. **Specyfikacja jest prawem.** Implementujemy według `docs/gameplay/`. Każde odstępstwo ma wpis w `docs/ODSTEPSTWA.md`. Brakujące dane to luka `L-NNN`, nie pole do zgadywania; założenie robocze oznaczamy `[ZAŁOŻENIE]`.
2. **Wzorzec przed własnym pomysłem.** Brief, komentarz modułu i ADR podają wzorzec i źródło.
3. **Symulacja bez renderera**, wejście tylko komendami, wyjście zdarzeniami, dane w `data/`, kierunek zależności pilnowany w CI.
4. **Determinizm**: stały tick, RNG ze strumieniami przez port, bez `Math.random`/`Date.now` w domenie, iteracja po id, złoty hash na małym scenariuszu zmieniany tylko ręcznie z nazwaną regułą.
5. **Testy**: dowód poprawki (pada bez, przechodzi z), test regresji dla każdego błędu, test zgodności ze specyfikacją z numerem strony, limit czasu ani budżetu nie jest poprawką.
6. **Zapadki**: policzalny dług ma kontrolę z bazą miejsc, która tylko spada.
7. **Teksty dla gracza tylko w katalogach języków** (PL, EN).
8. **Żadnych assetów, tekstów ani nazw z gry referencyjnej.** Instrukcje i zrzuty tylko do analizy, poza repo.

## Narzędzia w kontenerze

- Godot 4.7.2: `godot --headless --path <projekt> ...`; zrzuty: `xvfb-run -a godot --rendering-driver vulkan --rendering-method mobile ...` (Vulkan programowy, lavapipe). Renderer gry: Mobile (R-015).
- Blender 4.5 LTS: `blender -b --factory-startup --python <skrypt>`.
- Jeśli narzędzi brak (nowy kontener), zainstaluj je według `docs/SILNIK.md`, sekcja "Środowisko".

## Źródła i reguły

- Nowe źródło: wpis `S-NNN` w `docs/09-ZRODLA.md`.
- Reguła ze źródła lub decyzji właściciela: `R-NNN` w `docs/10-REGULY.md` z odnośnikiem.
- Konflikt rozstrzyga właściciel; do tego czasu wpis w `docs/11-OTWARTE-PYTANIA.md`.

## Raport dla właściciela

Krótki, po polsku, kończy się trzema listami: **zrobione, sprawdzone, niesprawdzone**. Pytanie zawsze z rekomendacją. Kody z opisem w nawiasie, np. "L-001 (kierunek spacerującego na skrzyżowaniu)".

## Agenci

- Zlecenie: cel słowami właściciela, zakres i czego nie ruszać, co wiadomo, kryterium odbioru, wzorzec, commit bazowy, strony specyfikacji.
- Raport (ok. 30 linii): zrobione, sprawdzone (polecenia i wyniki), niesprawdzone, zmienione pliki, do decyzji.
- Limit rund: po dwóch odrzuceniach nowy agent ze zwięzłym briefem.
- Zakazy na wspólnej maszynie: `git stash`, `pkill` po nazwie, force-push, amend, `rm` bez pełnej ścieżki. Pliki tymczasowe z prefiksem zadania.
- Prywatność: nigdy dane właściciela w zapytaniach sieciowych.

## Git

- Commit i push co ok. 30 min albo po każdym kroku (także `wip`). Trwałe jest tylko to, co wypchnięte.
- Jedna zmiana, jedna gałąź; ryzykowne zmiany (symulacja, zapis, renderer) osobno.
- PR i merge tylko na prośbę właściciela.

## Styl

Dokumentacja po polsku, kod i identyfikatory po angielsku. Wprost i konkretnie, bez patosu, bez myślników em, bez antytezy kontrastowej ("to nie X, to Y"). Liczby z jednostką i źródłem.
