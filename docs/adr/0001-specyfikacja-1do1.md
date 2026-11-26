# ADR-0001: Specyfikacja rozgrywki 1:1 jako dane i dokumenty

- Data: 2026-11-26
- Status: przyjęty

## Kontekst

Właściciel wymaga mechaniki 1:1 z grą referencyjną i dodatkiem, z późniejszą rozbudową. Dostarczył instrukcje obu gier (S-001, S-002). Instrukcje opisują reguły jakościowo i nie podają większości liczb.

## Decyzja

1. Specyfikacja ma dwie warstwy: dokumenty `docs/gameplay/*.md` (nadrzędne, z cytatami stron) i dane `data/*.json` (do odczytu przez symulację i testy).
2. Każda brakująca wartość to luka `L-NNN` z priorytetem; w danych ma `null`.
3. Odstępstwa od specyfikacji idą do `docs/ODSTEPSTWA.md`.
4. Zmiana mechaniki (rozbudowa) wymaga nowej reguły w `docs/10-REGULY.md` i wpisu w rejestrze odstępstw.

## Wzorzec

Type Object (Nystrom, "Game Programming Patterns"): zachowanie w systemach, typy w danych. Rejestr odstępstw od referencji z wytycznych właściciela (S-003, rozdział 9). Różnica: referencją jest gra, a nie dane GIS; luki w referencji traktujemy jak brakujące pomiary i oznaczamy jawnie.

## Konsekwencje

- Symulacja nie ma warunków na konkretne budynki ani bogów; wszystko z danych.
- Test spójności danych i test zgodności ze specyfikacją są obowiązkowe od pierwszego kodu.
- Prototyp fazy 1 wymaga zamknięcia luk P0 albo jawnych założeń roboczych strojonych audytem.

## Wyniki

Brak (przed implementacją).
