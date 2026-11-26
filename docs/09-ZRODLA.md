# Rejestr źródeł

Każde źródło dostarczone przez właściciela ma wpis. Reguły wydestylowane ze źródła trafiają do `10-REGULY.md` i odwołują się do jego identyfikatora.

## Format wpisu

```
### S-NNN: <krótki tytuł>
- Typ: dokument / strona WWW / wideo / zrzut ekranu / notatka właściciela / dane liczbowe
- Lokalizacja: <link lub ścieżka>
- Dostarczone: RRRR-MM-DD
- Zakres:
- Wiarygodność: wysoka / średnia / niska (uzasadnienie)
- Wydestylowane: R-NNN, dokumenty
- Notatki:
```

Materiały chronione prawem autorskim (instrukcje, zrzuty z gry referencyjnej) trzymamy poza repo i używamy wyłącznie do analizy.

## Źródła

### S-001: Instrukcja gry podstawowej (Zeus: Master of Olympus)
- Typ: dokument PDF, 102 strony (numeracja drukowana do ok. 200), wersja "post-release".
- Lokalizacja: poza repo (przesłane przez właściciela w sesji 2026-11-26).
- Dostarczone: 2026-11-26
- Zakres: wszystkie systemy gry podstawowej: teren, budowa, chodzicy, domy, żywność, przemysł, dystrybucja, handel, higiena, bezpieczeństwo, administracja, kultura, mitologia, wojsko, estetyka, przegląd, świat, interfejs; notatki projektantów.
- Wiarygodność: wysoka dla reguł jakościowych; instrukcja sama zaznacza, że w razie konfliktu rozstrzyga plik README gry (Z:14). Brak większości wartości liczbowych.
- Wydestylowane: `docs/gameplay/01`-`16`, `data/*.json`, R-005 do R-010.
- Notatki: cytujemy jako `Z:nn` (strona drukowana).

### S-002: Instrukcja dodatku (Poseidon)
- Typ: dokument PDF, 17 stron (numeracja drukowana do ok. 33), z diagramem łańcuchów atlantydzkich.
- Lokalizacja: poza repo.
- Dostarczone: 2026-11-26
- Zakres: kultura atlantydzka (rolnictwo, przemysł, nauka, hipodrom, wojsko, fregaty), Hera i Atlas, nowe potwory i herosi, piramidy, wstrzymywanie budowy, edytor (tylko wzmianka).
- Wiarygodność: wysoka dla reguł; brak ścieżki ewolucji domów atlantydzkich i liczb.
- Wydestylowane: `docs/gameplay/15-atlantyda.md` i wpisy w 04, 05, 10, 11, 12, 14; `data/*.json`.
- Notatki: cytujemy jako `P:nn`. Osobna instrukcja edytora (PDF na płycie dodatku) nie została dostarczona.

### S-003: Wytyczne "Dobre praktyki tworzenia gier" właściciela
- Typ: dokument Markdown, 904 linie, stan na 25.11.2026.
- Lokalizacja: poza repo (przesłane w sesji 2026-11-26).
- Dostarczone: 2026-11-26
- Zakres: architektura, determinizm, testy, audyty, zapadki, wydajność, weryfikacja wizualna, świat, rozgrywka, assety, UI i mobile, audio, praca z agentami, ciągłość, git, błędy, dokumentacja, współpraca z właścicielem.
- Wiarygodność: wysoka (praktyka z trzech projektów właściciela).
- Wydestylowane: `docs/PRAKTYKI.md`, `CLAUDE.md`, `docs/DECYZJE.md`, `docs/BACKLOG.md`, `docs/BLEDY.md`, `docs/ODSTEPSTWA.md`, `docs/adr/`, R-011.

### S-004: Portal Zeus Heaven (zeus.heavengames.com)
- Typ: strony WWW, pobrane skryptem 2026-11-26 (200 stron: gameinfo, buildings, units, scendesign, walkthroughs, history, archives, interview).
- Lokalizacja: https://zeus.heavengames.com/ (kopia robocza poza repo).
- Dostarczone: 2026-11-26 (linki od właściciela: gameinfo, buildings, units, blocks/legend, downloads fileid=598).
- Zakres: tabela modelu budynków (rozmiar, pracownicy, koszty per trudność, atrakcyjność, ryzyka), modele domów per trudność, podatki, płace, odsetek pracujących, zasięgi i prędkości chodzików, produkcja, sanktuaria, herosi, zadania, potwory, statystyki jednostek i wrogów, okup, piramidy, hipodrom, porady projektowania scenariuszy.
- Wiarygodność: wysoka dla tabel "Building Model Data" i "Housing Models" (pochodzą z plików modelu gry); średnia dla opisów i pomiarów (różnice między podstronami, patrz `docs/gameplay/98-konflikty-zrodel.md`).
- Wydestylowane: `docs/gameplay/17`-`23`, `98`, `99`; `data/building_stats.json`, `housing.json`, `difficulty.json`, `logistics.json`, `production.json`, `gods.json`, `gods_rules.json`, `heroes.json`, `quests.json`, `enemies.json`, `monuments.json`, `hippodrome.json`, `walkers.json`; R-012.
- Notatki: plik fileid=598 to ZeusMapper 1.2 (narzędzie do minimap z plików .pak/.map/.sav); strona autora z opisem formatów jest niedostępna z tego środowiska (archiwum zablokowane przez politykę sieci). Opisy przejść kampanii (walkthroughs) traktujemy tylko jako źródło obserwacji mechanik; treść kampanii nie trafia do gry (R-002).

### S-005: Wątki forum City Builders (caesar3.heavengames.com, dział Zeus)
- Typ: 9 wątków wskazanych przez S-004 jako najważniejsze ("Ok Folks - Let's Crunch Numbers", "Hungry? So are your people", "Everything You Ever Wanted to Know About Industries in Zeus", "On the Building of Sanctuaries", "Odds & Ends", "How To Do Colonies", "The Tower Switch of Sparta", "Balanced Block", "My Favorite Housing Blocks").
- Dostarczone: 2026-11-26 (pobrane w ramach skanu portalu).
- Zakres: model czasu i prędkości z plików `figure_model`, tempo produkcji, pojemności, zachowanie doręczycieli, fazy budowy sanktuariów, ciekawostki edytora.
- Wiarygodność: średnia (pomiary graczy, część na wersji 1.0 gry; rozbieżności w 98-konflikty).
- Wydestylowane: `docs/gameplay/17`, `19`, `21`, `23`; `data/production.json`, `logistics.json`.

## Kandydaci na kolejne źródła

Potrzebne do zamknięcia pozostałych luk P0 (`docs/gameplay/99-luki.md`: L-001, L-008, L-012b, L-017):
- opis plików modelu gry albo reimplementacje gier z tej rodziny (algorytm chodzików, migracja),
- instrukcja edytora przygód dodatku,
- nagrania rozgrywki do pomiarów.
