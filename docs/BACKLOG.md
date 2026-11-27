# Backlog

Bez sekcji "Zrobione": zamknięte pozycje przenosimy do `docs/archiwum/BACKLOG-ARCHIWUM.md` z datą. Częściowo zrobione mają dopisek "Zostaje:".

## Następne

1. **Dodatek (2026-11-28)**: zrobione A (zagroda, sad pomarańczy, rydwan jako koń domu elity; L-132) , B (jednostki Atlantydy, fregaty z Atlantean Fire, wieża z orichalkiem, Oceanidy; L-133; czwarte demo wojska) C (Hera, Atlas, cytadela Posejdona, ofiary z bydła, cztery potwory, wymagania herosów w Atlantydzie; L-134, B-025; piąte demo bogów Atlantydy) i D (wstrzymanie budowy pomnika, pomost do drogi, czarny marmur, 12 budynków estetycznych; L-128, zapis v49; w demo bogów Atlantydy). Zostaje: dzietność Hery (brak mechanizmu urodzeń). Dług demo spłacony 2026-11-28: włócznicy, rydwannicy i stoisko z rydwanami (szóste demo, elita Atlantydy), Atalanta i Bellerofont przeciw Sfinksowi i Echidnie (siódme demo, herosi, B-026), warstwy pomników w obrazie i przycisk wstrzymania budowy na karcie.
2. **Moduł E**: interfejs gracza i modele. Zrobione: panel budowy, przeciąganie, karta budynku, prędkości, katalogi PL/EN, raport miasta, zapis i autozapis, panele wojska i świata, grupy kompanii 1-5 i zaznaczanie prostokątem, mapa świata z położeniami ze scenariusza i panelem miasta z historią interakcji (L-135 (położenie miast na mapie świata), zapis v50), widoki specjalne z legendą i wyborem na panelu (`SILNIK.md` E6). Zostaje: pomiar E4 na telefonie właściciela, potem LOD i cienie; modele z Blendera w miejsce brył.
3. **Skrypt instalacji narzędzi** w środowisku chmurowym (Godot, Blender, Vulkan lavapipe, Xvfb, Android SDK i NDK, szablony eksportu). Zostaje: potwierdzić w świeżej sesji.
4. **Uzupełnienia szkieletu**: deterministyczny zrzut (zamrożona symulacja zamiast czasu klatki), clang-tidy w CI, zadanie CI budujące APK.
5. **Prototyp tłumu E4** w Godocie: 3 000 postaci (MultiMesh + VAT) i 2 000 budynków, scena `bench_crowd`, 19 wywołań rysowania w kontenerze. Zostaje: pomiar na Xiaomi 15T Pro (APK 0.0.42 u właściciela).
6. **Konwencje potoku Blendera** z `docs/ASSETY.md` jako skrypty `tools/blender/` i rejestr pochodzenia assetów. Inwentarz gotowy.
7. **Pomiary w oryginale**: liczby przyjęte jako [ZAŁOŻENIE] bez źródła liczbowego, do zmierzenia w grze referencyjnej (jeśli dostępna) albo w kolejnym źródle. Taktyki kompanii (L-110 (zasięg i wybór celu w walce), `data/military.json` `tactics_rules`, Z:138-139 opisuje je tylko słowami): obronna walczy do 2 kafli (miotający na zasięg broni) z pancerzem +2, ofensywna i specjalna ruszają na wroga do 6 kafli, motłoch w harcach odskakuje o 3 kafle od wroga obok, klin hoplitów daje 125 % ataku, jeździec po ciosie wraca pod sztandar. Organizacja igrzysk (L-123 (igrzyska panhelleńskie), `data/games.json`, Z:111 mówi o wzroście opinii i popularności bez liczb): +10 opinii każdego miasta i +10 zadowolenia każdego zamieszkanego domu. Do pomiaru: odległość, z której kompania rusza na wroga w każdej taktyce, obrażenia od tego samego wroga w szyku obronnym i ofensywnym, zmiana opinii miasta i nastroju po paradzie. Osobista obrona miasta przez Atenę (L-131 (stałe korzyści bogów), 10-mitologia.md: „osobiście broni miasta”, brak danych w instrukcji i w S-008): kiedy bogini się pojawia, jak walczy, jak długo zostaje; do pomiaru w oryginale, do tego czasu niezaimplementowana.
8. **Konflikty źródeł** otwarte (K-003, K-005, K-009, K-020, K-022, K-024 do K-029) do rozstrzygnięcia testem w oryginalnej grze, jeśli jest dostępna.

## Później

- **Plan odporności** (`docs/PLAN-ODPORNOSCI.md`, wersja 2): etapy E1-E6 odłożone decyzją właściciela 2026-11-27 („na razie lecimy po staremu”). Do tego czasu R-016 działa doraźnie: problem znaleziony sondą dostaje kontrolę w tym samym zadaniu.

- Ścieżka domów atlantydzkich (L-080) i reszta luk dodatku.
- Przeniesienie skryptów generujących `building_stats.json` i `housing.json` do repo (`tools/`), gdy powstanie struktura kodu; manifest z adresami źródeł.
- Edytor scenariuszy.
- Projekt UX dotykowego (makiety A/B/C do wyboru właściciela).
- Lista dopuszczonych licencji assetów.

## Ryzyka

W `docs/00-PLAN.md`.
