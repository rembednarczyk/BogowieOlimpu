# Silnik: Godot 4 rozbudowany pod założenia gry

Decyzja właściciela (2026-11-26): **Godot 4**. Assety robimy i przerabiamy w **Blenderze w kontenerze**. **Przed treścią gry rozbudowujemy silnik pod założenia** ze specyfikacji (`docs/gameplay/`, `data/`) i praktyk (`docs/PRAKTYKI.md`).

Interpretacja "rozbudowy silnika" (do potwierdzenia, `11-OTWARTE-PYTANIA.md` pyt. 22): budujemy warstwę silnika gry na Godocie, czyli moduły ogólne, z których potem złożymy grę: symulację, logistykę, chodziców, renderowanie tłumu, nakładki, narzędzia i potok assetów. Godota nie forkujemy, dopóki pomiar nie pokaże, że rozszerzenie (GDExtension) nie wystarcza.

## Środowisko w kontenerze (sprawdzone 2026-11-26)

| Narzędzie | Wersja | Status |
|---|---|---|
| Godot | 4.7.2-stable (najnowsza stabilna) | działa bez okna (`--headless`): import, skrypty, testy |
| Zrzuty z Godota | Xvfb + Mesa llvmpipe, renderer Compatibility (OpenGL 4.5) | działa, zrzut 640×360 sprawdzony |
| Renderery Forward+ i Mobile (Vulkan) | brak programowego sterownika Vulkan w kontenerze | **nie działa**; zrzuty tylko w Compatibility |
| Blender | 4.5.9 LTS | działa bez okna (`-b`), eksport glTF sprawdzony |
| Łańcuch Blender → `.glb` → import Godota | | działa |
| Kompilacja C++ | g++, cmake | dostępne; scons do doinstalowania przez pip |
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

Rekomendacja języka rdzenia: **C++ jako samodzielna biblioteka**, podpięta przez GDExtension (pyt. 23). Powody:
- determinizm: arytmetyka stałoprzecinkowa i jawna kolejność bez zależności od maszyny wirtualnej skryptów;
- wydajność dla tysięcy chodzików na telefonie;
- testy rdzenia działają natywnie w kontenerze, bez uruchamiania Godota (zasada "logika bez renderera");
- działa tak samo na Androidzie, iOS i PC; C# w Godocie ma słabsze wsparcie mobilne.

Prezentacja, UI i narzędzia edytora: GDScript.

## Moduły silnika do zbudowania przed grą

Każdy moduł ma: wzorzec, testy, kryterium odbioru. Kolejność = kolejność prac.

### A. Fundament rdzenia (`core/`)

| Moduł | Zakres | Wzorzec | Kryterium odbioru |
|---|---|---|---|
| A1 Czas | tick = klatka, 51 ticków/dzień, 16 dni/miesiąc, kalendarz, harmonogram systemów w ticku dnia | Fix Your Timestep; tabela harmonogramu | test: rok = 9 792 ticki; kolejność systemów stała |
| A2 Losowość | RNG z ziarnem, strumienie per system, stały losowy bajt kafla | PCG / xoshiro; strumienie | ten sam hash dla tego samego ziarna |
| A3 Komendy i zdarzenia | rozdzielnik komend (tabela handlerów), kolejka zdarzeń | Command, Event Queue | log komend + ziarno odtwarza stan |
| A4 Dane | ładowanie `data/*.json`, walidacja schematu i odwołań, wersja danych | Type Object | test spójności przechodzi na całych danych |
| A5 Zapis | zrzut stanu + wersja danych + migracje | Memento, wersjonowanie | zapis/odczyt daje identyczny hash |
| A6 Złoty hash i scenariusz 32×32 | mały scenariusz ze wszystkimi łańcuchami | testy charakteryzujące | hash zmieniany tylko ręcznie |

### B. Mapa i ruch

| Moduł | Zakres | Specyfikacja |
|---|---|---|
| B1 Siatka i teren | kafle, typy terenu, wysokość, złoża, łąka, woda, brzeg prosty | doc 01, `terrain.json` |
| B2 Budowa | obrysy, walidacja (droga, łąka, brzeg), obrót, koszt per trudność, undo | doc 01, 20, `building_stats.json` |
| B3 Graf dróg | drogi, Roadblock, aleje, bulwary, przeprawy i mosty, dostęp do budynku (od północy zgodnie z zegarem) | doc 17, 24 |
| B4 Ścieżki | A* z remisami po numerze kafla, mapy odległości do magazynów, ruch na przełaj | doc 24 |
| B5 Chodzik losowy | algorytm rodziny silnika, budżety 384/512/640 ticków, wyjątek fontanny | doc 24 |
| B6 Chodzicy celowi | doręczyciele, vendorzy, wozy, pasterze, myśliwi, rybacy | doc 02, 19 |
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

| Moduł | Zakres | Uwagi |
|---|---|---|
| E1 Wiązanie z rdzeniem | interpolacja pozycji między tickami, mapowanie zdarzeń na efekty | GDExtension |
| E2 Teren | siatka terenu z kafli, woda animowana, łąka, skały, las | Blender + shader |
| E3 Budynki | instancing (MultiMesh), stany: budowa, praca, ogień, ruina, sanktyfikacja | |
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
| Renderer Mobile (Vulkan) nie do sprawdzenia w kontenerze | zrzuty i testy wizualne w Compatibility; pomiar wydajności na telefonie właściciela; ewentualnie gra na rendererze Compatibility, jeśli wystarczy |
| Wydajność tłumu na telefonie | prototyp E4 jako pierwszy pomiar w Godocie (3 000 postaci, 2 000 budynków) |
| Złożoność GDExtension na Androidzie | szkielet `gdext/` z buildem Android na samym początku |
