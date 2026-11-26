# Plan projektu

Plan jest żywy. Po każdej partii źródeł aktualizujemy fazy, ryzyka i kolejność prac.

## Fazy

### Faza 0: Kontekst i reguły (trwa)

Cel: zebrać źródła, wydestylować reguły, zamknąć kluczowe decyzje.

- [x] Szkielet dokumentacji i zasady pracy (`CLAUDE.md`).
- [ ] Rejestr źródeł wypełniony materiałami od autora (`09-ZRODLA.md`).
- [ ] Reguły projektowe R-001+ (`10-REGULY.md`).
- [ ] Analiza referencji uzupełniona o dane liczbowe (`03-ANALIZA-REFERENCJI.md`).
- [ ] Decyzje: silnik, perspektywa kamery, model biznesowy, platformy startowe (`11-OTWARTE-PYTANIA.md`).
- [ ] Katalog mechanik zamknięty w wersji 1.0 (`04-MECHANIKI.md`).

Kryterium wyjścia: każda mechanika z katalogu ma opis, źródło lub uzasadnienie i priorytet (MVP / v1 / później).

### Faza 1: Prototyp symulacji (bez grafiki)

Cel: udowodnić, że rdzeń symulacji działa i daje dobrą rozgrywkę.

- Siatka kafli, drogi, budowa i wyburzanie.
- Chodziki losowe i celowe, wyszukiwanie ścieżek.
- Domy z ewolucją poziomów, populacja, zatrudnienie.
- Łańcuch żywności: farma, magazyn zboża, agora, dom.
- Jeden łańcuch przemysłowy i handel z jednym miastem.
- Widok debugowy 2D (kolorowe kafle), zapis i odczyt gry, testy deterministyczne.

Kryterium wyjścia: miasto na 2000 mieszkańców działa stabilnie przez 2 godziny gry, a tester rozumie przyczynę każdego problemu miasta.

### Faza 2: Vertical slice

Cel: jeden pełny scenariusz w docelowej jakości na telefonie.

- Docelowa kamera i rendering, pierwsze modele budynków i postaci.
- Pełny HUD dotykowy, nakładki informacyjne (woda, ryzyko pożaru, atrakcyjność).
- Jeden bóg z sanktuarium, błogosławieństwem i gniewem, jeden heros, jeden potwór.
- Podstawowa armia i jedna inwazja.
- Audio: muzyka dynamiczna, ambient miasta, dźwięki budynków.

Kryterium wyjścia: 30 minut grania na telefonie średniej klasy przy 60 FPS (cel) / 30 FPS (minimum), test z 5 graczami spoza zespołu.

### Faza 3: Produkcja treści

- Pełen panteon, herosi, potwory, wszystkie łańcuchy produkcji.
- Mapa świata, dyplomacja, kolonie, wojsko lądowe i morskie.
- Kampania fabularna, tryb swobodny, edytor scenariuszy (opcjonalnie).
- Lokalizacja (PL, EN na start).

### Faza 4: Dopracowanie i wydanie

- Balans na podstawie telemetrii z testów zamkniętych.
- Optymalizacja pod najsłabsze urządzenia z listy wspieranych.
- Wydanie: Android, iOS, PC (kolejność do decyzji).

### Faza 5: Rozbudowa

- Odpowiednik dodatku: nowa kultura/frakcja, nowe łańcuchy, nowe podejście do budowy miasta (patrz `04-MECHANIKI.md`, sekcja "Rozszerzenie").
- Treści społecznościowe: edytor, udostępnianie scenariuszy.

## Ryzyka

| Ryzyko | Skutek | Środek zaradczy |
|---|---|---|
| Złożony UI citybuildera na małym ekranie | Gra niegrywalna na telefonie | Prototyp UX w fazie 1 równolegle z symulacją, testy na 6" ekranie |
| Tysiące chodzików i budynków na mobilnym CPU/GPU | Spadki FPS, grzanie telefonu | Symulacja w ECS/danych, LOD, budżety z `07-TECHNOLOGIA.md` |
| Zbyt bliskie odwzorowanie gry referencyjnej | Ryzyko prawne, brak tożsamości | Granice z `02-ORYGINALNOSC.md`, przegląd nazw i grafik |
| Rozrost zakresu "AAA" | Projekt nigdy się nie kończy | Definicja AAA z `01-WIZJA.md`, priorytety MVP / v1 / później |
| Determinizm psuje się przy optymalizacji | Rozjazd zapisów, błędy trudne do odtworzenia | Testy powtarzalności w CI od fazy 1 |

## Kolejne kroki

1. Autor dostarcza źródła i reguły.
2. Każde źródło: wpis w `09-ZRODLA.md`, destylacja do `10-REGULY.md`, aktualizacja dotkniętych dokumentów.
3. Zamknięcie pytań blokujących z `11-OTWARTE-PYTANIA.md`.
4. Start fazy 1.
