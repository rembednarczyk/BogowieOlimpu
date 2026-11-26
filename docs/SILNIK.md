# Silnik: Godot 4 rozbudowany pod założenia gry

Decyzja właściciela (2026-11-26): **Godot 4**. Assety robimy i przerabiamy w **Blenderze w kontenerze**. **Przed treścią gry rozbudowujemy silnik pod założenia** ze specyfikacji (`docs/gameplay/`, `data/`) i praktyk (`docs/PRAKTYKI.md`).

Zakres "rozbudowy silnika" (potwierdzony przez właściciela 2026-11-26): budujemy warstwę silnika gry na Godocie, czyli moduły ogólne, z których potem złożymy grę: symulację, logistykę, spacerujących, renderowanie tłumu, nakładki, narzędzia i potok assetów. Godota nie forkujemy, dopóki pomiar nie pokaże, że rozszerzenie (GDExtension) nie wystarcza.

## Środowisko w kontenerze (sprawdzone 2026-11-26)

| Narzędzie | Wersja | Status |
|---|---|---|
| Godot | 4.7.2-stable (najnowsza stabilna) | działa bez okna (`--headless`): import, skrypty, testy |
| Zrzuty z Godota | Xvfb + Mesa lavapipe, **Vulkan 1.4**, renderery Mobile i Forward+ | działa, zrzuty sprawdzone (R-015) |
| Renderer Compatibility | OpenGL przez llvmpipe | działa, ale nie jest celem (R-015) |
| Blender | 4.5.9 LTS | działa bez okna (`-b`), eksport glTF sprawdzony |
| Łańcuch Blender → `.glb` → import Godota | | działa |
| Kompilacja C++ | g++, clang, cmake, ninja | dostępne |
| Android | JDK 17, SDK 35, NDK 28.2.13676358, szablony eksportu 4.7.2 | APK debug arm64 budowany i podpisywany w kontenerze; na telefonie nieuruchomiony |
| Rdzenie, pamięć | 4 rdzenie, 15 GB RAM | |

Instalacja jest w `/opt/tools`. Skrypt startowy środowiska chmurowego (zapisany 2026-11-26) instaluje Godota i Blendera przy budowie nowego kontenera, tylko gdy sesja ma repo BogowieOlimpu. Działanie w świeżej sesji jeszcze niepotwierdzone.

## Architektura

```
data/*.json  (specyfikacja, Type Object)
     │
     ▼
core/        rdzeń symulacji: C++17, bez nagłówków Godota      ◄── testy natywne (ctest), złoty hash, audyty
     │        deterministyczny, stały tick, komendy, zdarzenia
     ▼
gdext/       GDExtension: cienkie wiązanie core ↔ Godot         ◄── jedyne miejsce, które zna oba światy
     │
     ▼
game/        projekt Godota: render, kamera, UI, audio, wejście  ◄── testy GDScript (gdUnit4), zrzuty (Xvfb)
     ▲
assets/      wynik potoku Blendera (.glb, tekstury, animacje)   ◄── skrypty Blendera + manifest + rejestr licencji
```

Język rdzenia (decyzja 2026-11-26): **C++17 jako samodzielna biblioteka**, podpięta przez GDExtension. Powody:
- determinizm: arytmetyka stałoprzecinkowa i jawna kolejność bez zależności od maszyny wirtualnej skryptów;
- wydajność dla tysięcy spacerujących na telefonie;
- testy rdzenia działają natywnie w kontenerze, bez uruchamiania Godota (zasada "logika bez renderera");
- działa tak samo na Androidzie, iOS i PC; C# w Godocie ma słabsze wsparcie mobilne.

Prezentacja, UI i narzędzia edytora: GDScript.

## Moduły silnika do zbudowania przed grą

Każdy moduł ma: wzorzec, testy, kryterium odbioru. Kolejność = kolejność prac.

### A. Fundament rdzenia (`core/`): ZROBIONE 2026-11-26

Stan: moduły A1-A6 zaimplementowane w `core/`, 21 przypadków testowych, złoty hash `0x8e4d415da80bd045` zgodny w GCC i Clang (Debug, Release). Strażnik `tools/checks/check_core.py`. Pierwszy scenariusz złotego hasha obejmuje kalendarz, komendy i finanse; scenariusz 32×32 z łańcuchami dojdzie z modułami B i C.

| Moduł | Zakres | Wzorzec | Kryterium odbioru |
|---|---|---|---|
| A1 Czas | tick = klatka, 51 ticków/dzień, 16 dni/miesiąc, kalendarz, harmonogram systemów w ticku dnia | Fix Your Timestep; tabela harmonogramu | test: rok = 9 792 ticki; kolejność systemów stała |
| A2 Losowość | RNG z ziarnem, strumienie per system, stały losowy bajt kafla | PCG / xoshiro; strumienie | ten sam hash dla tego samego ziarna |
| A3 Komendy i zdarzenia | rozdzielnik komend (tabela handlerów), kolejka zdarzeń | Command, Event Queue | log komend + ziarno odtwarza stan |
| A4 Dane | ładowanie `data/*.json`, walidacja schematu i odwołań, wersja danych | Type Object | test spójności przechodzi na całych danych |
| A5 Zapis | zrzut stanu + wersja danych + migracje | Memento, wersjonowanie | zapis/odczyt daje identyczny hash |
| A6 Złoty hash | stały scenariusz, hash ustawiany ręcznie; rozszerzany o mapę 32×32 wraz z modułami B, C | testy charakteryzujące | hash zmieniany tylko ręcznie |

### B. Mapa i ruch: CZĘŚCIOWO (2026-11-26)

Zrobione: B1 (siatka, 12 typów terenu zgodnych z `terrain.json`, bajt losowy kafla, parser mapy tekstowej), B2 (obrysy, kolizje, teren, łąka pod farmy, koszt według trudności, rozbiórka, wycinka lasu), B3 (drogi, Roadblock, sieci dróg, punkt wyjścia budynku od północy zgodnie z zegarem), B4 (A* z remisami po indeksie kafla, tryby: drogi, drogi bez blokad, teren), B5 (spacerujący wędrowny: algorytm rodziny silnika, budżety z `walkers.json`, obsługa 5×5 przez 96 dni, powrót ścieżką), B6 w wersji podstawowej (spacerujący celowy po drogach, przez blokady), B7 w wersji podstawowej (zapytanie o budynki w kwadracie). Systemy dzienne: wysyłanie spacerujących (opóźnienie według obsady, S-007) i zanik zasięgu. Format zapisu v2 z migracją z v1. Złoty hash miasta 32×32.

Zostaje: obrót budynków, undo, brzeg i budynki na agorze lub drodze (komunikat `build.placement_not_implemented`), aleje, bulwary, mosty, wysokość terenu; mapy odległości do magazynów; wyjątek powrotu fontanny i lecznicy; przypadek szerokiej drogi w algorytmie wędrownym; konkretni spacerujący celowi (doręczyciele, wozy, pasterze, myśliwi, rybacy) przychodzą z modułami C.


| Moduł | Zakres | Specyfikacja |
|---|---|---|
| B1 Siatka i teren | kafle, typy terenu, wysokość, złoża, łąka, woda, brzeg prosty | doc 01, `terrain.json` |
| B2 Budowa | obrysy, walidacja (droga, łąka, brzeg), obrót, koszt per trudność, undo | doc 01, 20, `building_stats.json` |
| B3 Graf dróg | drogi, Roadblock, aleje, bulwary, przeprawy i mosty, dostęp do budynku (od północy zgodnie z zegarem) | doc 17, 24 |
| B4 Ścieżki | A* z remisami po numerze kafla, mapy odległości do magazynów, ruch na przełaj | doc 24 |
| B5 Spacerujący losowy | algorytm rodziny silnika, budżety 384/512/640 ticków, wyjątek fontanny | doc 24 |
| B6 Spacerujący celowi | doręczyciele, vendorzy, wozy, pasterze, myśliwi, rybacy | doc 02, 19 |
| B7 Indeks przestrzenny | obsługa w kwadracie 5×5, zapytania o budynki w promieniu | doc 24 |

### C. Systemy miasta

| Moduł | Zakres | Specyfikacja |
|---|---|---|
| C1 Atrakcyjność | pole atrakcyjności przyrostowe, wzór initial/step | doc 20 |
| C2 Domy | ewolucja, regres z histerezą, liczniki usług (96 dni), zużycie, populacja | doc 03, 18, 24 |
| C3 Praca | 8 sektorów, priorytety, przydział, obsada → wydajność | doc 05, 24 |
| C4 Produkcja | tempo, magazyny wewnętrzne, sady, hodowla, łowiectwo, wydobycie | doc 04, 05, 19 |
| C5 Magazyny i dystrybucja | Granary, Storehouse, rozkazy Accept/Get/Empty, Agora, peddler | doc 06, 19 |
| C6 Finanse | podatki, płace, bilans, dług | doc 08, 18 |
| C7 Ryzyka | pożar, zawalenie, choroba, przestępczość | doc 07, 24 |
| C8 Nastroje i migracja | zadowolenie, popularność, imigracja, bezdomni | doc 13, 24 |
| C9 Handel | budynki handlowe, kupcy, limity roczne | doc 06, 24 |

### D. Mit, wojsko, świat, scenariusz

| Moduł | Zakres | Specyfikacja |
|---|---|---|
| D1 Bogowie | sanktuaria, budowa fazami, ofiary, modlitwy, sanktyfikacja, gniew | doc 10, 21 |
| D2 Potwory i herosi | | doc 10, 21 |
| D3 Wojsko | kompanie, rozkazy, walka, morale, flota, fortyfikacje | doc 11, 22, 24 |
| D4 Świat | miasta, relacje, opinie, prośby, handel, trybut, bitwy za granicą | doc 14, 24 |
| D5 Scenariusz | format przygody, cele, zdarzenia, kolonie | doc 23 |

Moduły C i D powstają w fazie 1 ("Prototyp symulacji") jako część silnika; ich odbiór to test zgodności ze specyfikacją (`PRAKTYKI.md` rozdz. 4).

### E. Prezentacja w Godocie (`game/`)

Stan: E1 w wersji minimalnej (klasa `BoSimulation` w `gdext/`, scena `game/scenes/boot.tscn` z datą i hashem; test bez okna `game/tests/test_simulation.gd`; zrzuty `game/tests/screenshot.gd` w rendererze Mobile).

| Moduł | Zakres | Uwagi |
|---|---|---|
| E1 Wiązanie z rdzeniem | interpolacja pozycji między tickami, mapowanie zdarzeń na efekty | GDExtension |
| E2 Teren | siatka terenu z kafli, woda animowana, łąka, skały, las | Blender + shader |
| E3 Budynki | instancing (MultiMesh), stany: wizualna faza budowy, praca, 3 stopnie uszkodzenia z licznika ryzyka, ogień w fazach, ruina per kategoria, sanktyfikacja | O-005: oprawa tylko czyta stan rdzenia |
| E4 Tłum | tysiące postaci: MultiMesh + animacja w teksturach (VAT), LOD | kluczowe ryzyko wydajności |
| E5 Kamera | izometryczna, zoom, obrót (pyt. 7), zakładki lokalizacji | `06-MOBILE-UX.md` |
| E6 Nakładki | widoki "See ..." z danych rdzenia (kolumny, kolory kafli) | jedno źródło prawdy |
| E7 Budowanie dotykiem | podgląd ducha, przeciąganie dróg, precyzyjny kursor | `06-MOBILE-UX.md` |
| E8 UI | jedna skala, katalogi języków PL/EN, panele budynków, raport miasta | |
| E9 Audio | zdarzenia → dźwięki jako dane, muzyka adaptacyjna | |

### F. Narzędzia

| Moduł | Zakres |
|---|---|
| F1 Strona testowa | scena z parametrami (scenariusz, kamera, nakładka, zamrożenie) + zrzuty w Xvfb |
| F2 Autopilot i audyty | build order, raporty liczbowe, soak |
| F3 Potok Blendera | skrypty: modelowanie proceduralne, normalizacja (skala w kaflach, front, paleta), LOD, wypiekanie VAT, eksport `.glb`, manifest, rejestr licencji |
| F4 Edytor scenariuszy | później |
| F5 CI | build rdzenia, testy natywne, testy Godota bez okna, zrzuty, strażnik zależności |

## Potok assetów w Blenderze

- Wszystko skryptami w repo (`tools/blender/`), uruchamiane `blender -b --python ...`. Wynik deterministyczny.
- Assety referencyjne (grafiki z gry oryginalnej) **nie są przerabiane** (R-002). "Przerabiamy" oznacza: modele z otwartych licencji, Twoje modele, nasze wcześniejsze wersje. Każde źródło ma wpis w rejestrze z licencją.
- Konwencje: 1 kafel = 1 m; front budynku w osi −Y; paleta stylu; budżety trójkątów per kategoria i LOD.
- Postacie: szkielet wspólny, animacje (chód, praca, noszenie, walka, śmierć) wypiekane do tekstur VAT dla tłumu.
- Inwentarz potrzebnych assetów: `docs/ASSETY.md` (powstaje na podstawie analizy eZeus).

## Ryzyka

| Ryzyko | Środek |
|---|---|
| Lavapipe różni się od GPU Mali w szczegółach i wydajności | zrzuty w kontenerze do oceny wyglądu; pomiar wydajności i weryfikacja wizualna na Xiaomi 15T Pro |
| Wydajność tłumu na telefonie | prototyp E4 jako pierwszy pomiar w Godocie (3 000 postaci, 2 000 budynków) |
| Złożoność GDExtension na Androidzie | szkielet `gdext/` z buildem Android na samym początku |
