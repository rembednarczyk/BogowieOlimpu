# Backlog

Bez sekcji "Zrobione": zamknięte pozycje przenosimy do `docs/archiwum/BACKLOG-ARCHIWUM.md` z datą. Częściowo zrobione mają dopisek "Zostaje:".

## Następne

1. **Decyzje blokujące** z `docs/11-OTWARTE-PYTANIA.md`: silnik (pyt. 11), urządzenie odniesienia (pyt. 12), limit 20 kompanii (pyt. 16), nazewnictwo (pyt. 17).
2. **Zamknięcie luk P0** z `docs/gameplay/99-luki.md`. Zostaje: L-001 (kierunek chodzika na skrzyżowaniu), L-008 (sektory zatrudnienia), L-012b (czas życia wizyty usługi w domu), L-017 (migracja). Kandydaci: opis plików modelu gry, reimplementacje gier z tej rodziny, nagrania rozgrywki.
3. **Konflikty źródeł do rozstrzygnięcia przez właściciela** albo testem w oryginalnej grze: K-003, K-004, K-005, K-009, K-021, K-022, K-023 (`docs/gameplay/98-konflikty-zrodel.md`).
4. **Prototyp pomiarowy renderera**: 3 000 animowanych postaci i 2 000 budynków na siatce, pomiar na telefonie odniesienia (Babylon.js vs Three.js). Kryterium: stałe 30 FPS na telefonie odniesienia.
5. **Szkielet kodu fazy 1**: monorepo, pakiet `domain` bez zależności, strażnik zależności w CI, linter z zakazem losowości i czasu, limity rozmiaru, RNG ze strumieniami, tick, komendy, zdarzenia, ładowanie `data/*.json`, test spójności odwołań w danych.
6. **Scenariusz testowy 32×32 i złoty hash** (patrz `docs/PRAKTYKI.md`, rozdział 3).

## Później

- Ścieżka domów atlantydzkich (L-080) i reszta luk dodatku.
- Przeniesienie skryptów generujących `building_stats.json` i `housing.json` do repo (`tools/`), gdy powstanie struktura kodu; manifest z adresami źródeł.
- Edytor scenariuszy.
- Projekt UX dotykowego (makiety A/B/C do wyboru właściciela).
- Lista dopuszczonych licencji assetów.

## Ryzyka

W `docs/00-PLAN.md`.
