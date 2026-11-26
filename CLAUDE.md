# Zasady pracy w repozytorium Bogowie Olimpu

Ten plik czytają wszyscy, którzy pracują w repo, w tym agenci AI. Ma pierwszeństwo przed domyślnymi nawykami.

## Język i styl

- Dokumentacja projektowa po polsku. Kod, identyfikatory i komentarze w kodzie po angielsku.
- Pisz konkretnie i twierdząco. Bez myślników typu "—", bez konstrukcji "to nie X, to Y", bez frazesów marketingowych.
- Liczby podawaj z jednostką i źródłem (np. "zasięg chodzika: 26 kafli, źródło S-003" albo "założenie robocze").

## Źródła i reguły

- Każde nowe źródło dostaje wpis w `docs/09-ZRODLA.md` z identyfikatorem `S-NNN`.
- Każda reguła wydestylowana ze źródła trafia do `docs/10-REGULY.md` z identyfikatorem `R-NNN` i odnośnikiem do źródła.
- Reguła o statusie "obowiązuje" jest wiążąca dla kodu i dalszych dokumentów. Zmiana reguły wymaga wpisu w jej historii.
- Konflikt między regułami albo między regułą a dokumentem rozstrzyga autor projektu. Do tego czasu konflikt trafia do `docs/11-OTWARTE-PYTANIA.md`.

## Oryginalność

- Obowiązują granice z `docs/02-ORYGINALNOSC.md`. Mechaniki gatunkowe wolno odtwarzać. Nazw własnych, grafiki, dźwięków, tekstów, map i kampanii gry referencyjnej nie kopiujemy.
- Mitologia grecka jest domeną publiczną: bogów, herosów, potwory i miejsca z mitów używamy swobodnie, w naszej własnej interpretacji.

## Architektura (gdy powstanie kod)

- Symulacja jest odseparowana od renderingu i deterministyczna (patrz `docs/05-SYMULACJA.md`).
- Dane balansu (koszty, zasięgi, czasy) żyją w plikach danych, poza kodem.
- Każda mechanika ma test symulacyjny odtwarzający scenariusz z dokumentacji.

## Git

- Małe, opisowe commity. Dokumentacja i kod w osobnych commitach, gdy to możliwe.
