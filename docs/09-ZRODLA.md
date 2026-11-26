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

## Kandydaci na kolejne źródła

Potrzebne do zamknięcia luk P0 (`docs/gameplay/99-luki.md`):
- tabele wartości budynków, domów i chodzików (społeczność graczy, pliki danych gry),
- instrukcja edytora przygód dodatku,
- nagrania rozgrywki do pomiaru zasięgów i prędkości.
