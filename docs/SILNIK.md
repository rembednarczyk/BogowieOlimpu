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

### B. Mapa i ruch: ZROBIONE 2026-11-26 (wysokość terenu bez półspadków, L-099)

Zrobione:
- B1: siatka, 12 typów terenu zgodnych z `terrain.json`, bajt losowy kafla, parser mapy tekstowej.
- B2: obrysy, kolizje, teren, łąka pod farmy, koszt według trudności, rozbiórka, wycinka lasu; obrót budynków obracalnych (Z:24-25); cofanie ostatniej akcji z pełnym zwrotem (Z:20; komendy jednego ticku to jedna akcja, okno 1 dzień, L-035); budynki przy prostym brzegu z kierunkiem wody (L-096); agory nad istniejącą drogą ze slotami 2×2 i stoiska w slotach (Z:69-71).
- B3: drogi, Roadblock, sieci dróg, punkt wyjścia od północy zgodnie z zegarem; bulwar i aleja nad drogą, budynki za ozdobami mają dostęp do drogi (Z:154); przeprawa i most z własną drogą nad wodą, zakaz przepraw obok siebie (Z:101-103, P:19, L-097).
- B4: A* z remisami po indeksie kafla (drogi, drogi bez blokad, teren); mapa odległości BFS po drogach od wielu źródeł.
- B5: spacerujący wędrowny według rodziny silnika z liczbami Zeusa, reguła szerokiej drogi (S-006), powrót fontanny i lecznicy do kafla na północ (S-004, L-098).
- B6 i B7 w wersji silnikowej: spacerujący celowy po drogach przez blokady; zapytanie o budynki w kwadracie.
- Systemy dzienne: wysyłanie spacerujących (S-007) i zanik zasięgu. Format zapisu v3 z migracjami z v1 i v2. Złoty hash miasta 32×32 z agorą, bulwarem i cofnięciem.

- Wysokość terenu (L-099, S-009, S-005): poziomy 0-15 na kaflu, rampy o jeden poziom, klify blokują kroki, sieci dróg, ścieżki, spacerujących i dostęp do drogi; budynki tylko na jednym poziomie i nie na rampie [ZAŁOŻENIE]; droga może biec po rampie (schody). Brak półspadków.

Przeniesione do modułów, które dają im sens: konkretni spacerujący celowi (doręczyciele, wozy, pasterze, myśliwi, rybacy) i molo (obrys złożony) do C; nabrzeże jeżowców (wymaga jeżowców na mapie) do C; mury, wieże, bramy do D; kładka hipodromu do C; wariant przeprawy mała/duża (statki) do D. Otwarte: wysokość terenu (L-099, brak specyfikacji).

| Moduł | Zakres | Specyfikacja |
|---|---|---|
| B1 Siatka i teren | kafle, typy terenu, wysokość, złoża, łąka, woda, brzeg prosty | doc 01, `terrain.json` |
| B2 Budowa | obrysy, walidacja (droga, łąka, brzeg), obrót, koszt per trudność, undo | doc 01, 20, `building_stats.json` |
| B3 Graf dróg | drogi, Roadblock, aleje, bulwary, przeprawy i mosty, dostęp do budynku (od północy zgodnie z zegarem) | doc 17, 24 |
| B4 Ścieżki | A* z remisami po numerze kafla, mapy odległości do magazynów, ruch na przełaj | doc 24 |
| B5 Spacerujący losowy | algorytm rodziny silnika, budżety 384/512/640 ticków, wyjątek fontanny | doc 24 |
| B6 Spacerujący celowi | doręczyciele, vendorzy, wozy, pasterze, myśliwi, rybacy | doc 02, 19 |
| B7 Indeks przestrzenny | obsługa w kwadracie 5×5, zapytania o budynki w promieniu | doc 24 |

### C. Systemy miasta: W TOKU (2026-11-26)

Zrobione:
- C1: pole atrakcyjności według wzoru S-004 (pierścienie od krawędzi obrysu, sumowanie wkładów, bulwar i aleja emitują z każdego kafla), liczone codziennie; dom widzi najwyższą wartość ze swoich kafli [RODZINA].
- C2: działki domów przy drodze (koszt Hut), ewolucja z histerezą AppE/AppD i wymaganiami z `housing.json` (woda, dobra, punkty kultury z `walkers.json`), awans o jeden poziom dziennie, regres po 3 nieudanych sprawdzeniach do najwyższego spełnionego poziomu, porzucenie domu elity, zużycie dóbr w dniach 0 i 8, nadmiar mieszkańców do licznika bezdomnych. Działka elity odrzucana do czasu magazynów (C5), bo wymaga zapasów miasta.
- C3: pula pracowników z domów ludu według tabeli S-004 (trudność × płace), 8 sektorów, priorytety 6-stopniowe, przydział kolejno od najwyższego priorytetu (K-031), równy ułamek obsady w sektorze, wyłączanie branż, obsada wpływa na wysyłanie spacerujących.
- C6: podatek miesięczny `TRM × mieszkańcy × stawka` tylko dla domów zarejestrowanych przez poborcę (licznik 50 dni), płace roczne wypłacane co miesiąc, ułamki przenoszone w tysięcznych, bilans roku bieżącego i poprzedniego.
- C8: zadowolenie domów dwa razy w miesiącu (podatki S-004, płace, bezrobocie, dług, żywność), miasto poniżej 300 mieszkańców na wartości bazowej, popularność jako średnia, migracja dzienna według tabeli (partie od 4 osób), zasiedlanie najpierw domów z ≥ 8 wolnymi miejscami, emigracja z najniższych poziomów powyżej 100 mieszkańców, bezdomni po regresie i po > 4 dniach bez drogi. Parametry w `data/migration.json`. Imigranci bez spacerujących i bez punktu wejścia (O-006).
- Przełącznik deweloperski `full_staffing` (pełna obsada bez mieszkańców) zostaje dla testów spacerujących; złote miasto i scena demo działają już na prawdziwej migracji i pracy. Nie jest regułą gry.

- C4/C5 etap 1: spichlerz i magazyn (8 pól po 4 ładunki, rozkazy Accept i Don't Accept z limitem, spichlerz przyjmuje tylko przy pełnej obsadzie), farmy z żniwami (8 ładunków/rok przy pełnej obsadzie przez rok), doręczyciel z 1 ładunkiem do najbliższego odbiorcy według `delivery_priority`, vendor stoiska (do 4 ładunków, zasięg 37 kafli), peddler sprzedający domom (L-100), działka elity sprawdza zapasy miasta. Test łańcucha: farmy, spichlerz, agora, fontanna i gimnazjum dają lepianki w 2 lata.

- C4 etap 2: owce i kozy bez własnych kafli (dowolnie wiele na łące), stado do 8 na budynek, strzyża i dojenie po 6 miesiącach i potem co rok, pasterz chodzi przez teren; drzewa oliwne i winorośl dojrzewają od wizyt ogrodnika (połowa tempa po 6 miesiącach bez wizyty, zero po 10), zbieracze (do 3) w miesiącach żniw, niezebrane przepada; chata ogrodników trzyma 5 ładunków każdego owocu; tłocznia i winiarnia (6 ładunków/rok, wejście do 3 ładunków); magazyn odsyła surowiec do przetwórni (L-101). Test: domy dochodzą do Homestead w 3 lata.

- C4/C5 etap 3: rozkazy Get i Empty (tylko przy pełnej obsadzie, do 4 ładunków na kurs), wydobycie z kursami robotników do złoża (tartak, odlewnia, rafineria, mennica; tempo zależy od odległości, L-102), kamieniarnie przy kamieniołomie, mennica do skarbca, zbrojownia i pracownia rzeźb (4 brązy na rzeźbę), rozmnażanie stad (L-101).

- Łowiectwo i rybołówstwo (L-103): punkty odradzania dzików (stado 7, młode co 26 dni) i jeleni (jedno na miesiąc), myśliwi z chat łowców, łódź przystani rybackiej budowana na start i pływająca do łowiska, nabrzeże jeżowców; łowiska i punkty jako cechy kafla mapy.

- C7 ryzyka: liczniki pożaru i zawalenia z `building_stats.json` (60 / ryzyko miesięcy, losowy rozrzut), płonący budynek 32 dni, rozprzestrzenianie z wiatrem, gaszenie przez zarządcę, ruiny; zaraza z kwarantanną (healer zeruje); przestępcy według zadowolenia domu, straż zatrzymuje, złodziej i wandal atakują domy elity (L-104). Parametry w `data/risks.json`.

- C9 handel: partnerzy handlowi w konfiguracji scenariusza (stan, trasa lądowa lub morska, oferty z ceną i rocznym limitem), faktoria lądowa i molo z dokiem na wodzie (najwyżej 10), rozkazy importu i eksportu, karawany co 4 dni (8 ładunków) i statki co 30 dni (12), maks. 1 nowy kupiec dziennie, punkty wejścia na mapę (symbole E i S), rywale nie handlują, towar w budynku handlowym poza zapasami miasta, dostawy produkcji najpierw na eksport, doręczyciele budynku handlowego rozwożą import i zbierają towar na eksport (L-105). Parametry w `data/trade.json`.

- Stadnina i fabryka rydwanów (L-106): stadnina 4×4 z wybiegiem 3×3, 50 jednostek pszenicy na konia, 6 koni rocznie, najwyżej 6 czeka, czekający koń zjada 10 jednostek miesięcznie, pszenica nigdy ze spichlerza; fabryka z drewna i konia, 6 rydwanów rocznie; stajenny przynosi konie (zasięg 44 kafle, S-005); trener koni i sprzedawca rydwanów biorą prosto z budynku; zbroja, konie i rydwany liczone w sztukach.

- Wilki (L-107): legowisko na mapie (symbol `w`), wataha 8, wilk wychodzi średnio raz na 20 dni po najbliższą owcę lub kozę w zasięgu 60 kafli, porywa ją, chyba że pasterz tej zagrody jest w promieniu 5 kafli; bydło chronią byki. Parametry w `data/walkers.json` (wpis `wolf`). Format zapisu v16.

- Elita (L-108): rodziny elity przychodzą od punktu wejścia do domów elity z wolnymi miejscami, poza pulą imigracji ludu (Z:34); zamieszkany dom elity wysyła szlachcica, który zatrzymuje przestępców i zeruje ryzyko przestępczości (Z:88).

Zostaje: premie miasta Stadium i Museum do kultury; wysokość terenu (L-099, dane częściowe).

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

Stan D1: etap 1 zrobiony (L-111, L-052). Sanktuaria 14 bogów z `data/gods.json`, najwyżej 4 w mieście, marmur startowy z magazynów, gildia rzemieślników, wozy z magazynu (marmur i drewno po 4 ładunki, rzeźby po 1), rzemieślnicy wbudowują materiał, zdarzenie ukończenia, ofiary kapłanów (owca lub koza, bez nich żywność ze spichlerza), utrata łaski po roku bez ofiary. Format zapisu v19. Etap 2 zrobiony (L-112, L-053): fazy budowy według S-005, sanktyfikacja z premią 4×, modlitwy z odnowieniem (Demeter, Artemis, Posejdon, Atena, Dionizos, Hades, Apollo), korzyści stałe Hefajstosa, Dionizosa i Afrodyty. Format zapisu v20. Zostaje: pozostałe korzyści stałe (gaje, złoża, Kraken, Cerberus, kompanie Amazonek i Sparti), wizyty boga na mapie, bogowie wrodzy i gniew, potwory, herosi, zadania, wyrocznia.
| D2 Potwory i herosi | | doc 10, 21 |
| D3 Wojsko | kompanie, rozkazy, walka, morale, flota, fortyfikacje | doc 11, 22, 24 |

Stan D3: etap 1 zrobiony (L-109). Bez pałacu nie ma armii; kompanie po 8 żołnierzy formują się codziennie z domów (rabble z domów ludu od Hovel: round(Pop × %S); hoplici z Mansion i Manor po jednej zbroi, jeźdźcy z Estate po zbroi i koniu), najwyżej 20; sztandary wokół pałacu; komendy MusterCompany, MoveCompany, SendCompanyHome; żołnierze idą pieszo do szyku przy sztandarze i wracają do domów; rabble w polu nie pracuje. Format zapisu v17.

Etap 2 zrobiony (L-110): statystyki jednostek z `data/enemies.json`, inwazja komendą StartInvasion (docelowo z modułu świata) w punkcie `i` mapy, skład według procentów nacji zaokrąglany w dół, okup 100/200 dr × procent trudności, kapitulacja, walka co tick (wręcz i pociski, trafienie atak − pancerz), śmierć żołnierza zabiera domowi 6 mieszkańców (rabble) albo zbroję i konia (elita), morale z ucieczką do pałacu i zakazem ponownej mobilizacji w tej bitwie, zdobycie pałacu, kara −2 do zadowolenia za armię w polu bez wroga; rabble w polu to 6 pracowników mniej. Format zapisu v18.

Etap 3 zrobiony (L-117): mury (kafel, bez drogi), wieże na murze podwójnej grubości z pałacem i drogą do niego, strzelające do wrogów, brama na drodze z murami po obu stronach (przepuszcza karawany i imigrantów, zamknięta dla wrogów), wrogowie bez przejścia burzą mury i bramy, stocznia trier buduje trierę z drewna i zbroi (liczy się do wymagań Jazona). Format zapisu v25. Zostaje: walka morska i rozkazy trier, wartownicy patrolujący mury, taktyki kompanii, generałowie (Auto-Defend).
| D4 Świat | miasta, relacje, opinie, prośby, handel, trybut, bitwy za granicą | doc 14, 24 |

Stan D4: etap 1 zrobiony (L-113, L-063, L-064, L-065). Partnerzy scenariusza mają opinię 0-100, siłę militarną i ekonomiczną, trybut; dary z malejącą wartością (+3 / +1,5 za jednostkę, 3 rocznie), prośby gracza z progami według stanu i opinii (rywal tylko pod przymusem siły), trybut wasali i kolonii co rok, trybut dla rywala po zdobyciu miasta, dwie porażki z tym samym rywalem kończą epizod, odparcie inwazji zrywa trybut; atak i rajd za granicą (bez rabble, 200 dni w każdą stronę, siła z doc 24), podbój tworzy wasala, rajd przynosi łup według trudności. Format zapisu v21. Zostaje: prośby miast do gracza i ich terminy (ze zdarzeń scenariusza), bunty, ataki rywali z nienawiści, dary dobrej woli, pakt atlantydzki, pomoc wojskowa sojuszników.
| D5 Scenariusz | format przygody, cele, zdarzenia, kolonie | doc 23 |

Stan D5: etap 1 zrobiony (L-093, L-114). Format przygody JSON w `scenarios/` (opis w `scenarios/README.md`): mapa, wysokości, skarbiec, trudność, miasta świata z dyplomacją, zdarzenia datowane (inwazja, prośba z terminem, dar, zmiana relacji; opcjonalnie co rok) i cele (populacja, skarbiec, sanktuarium, domy na poziomie, wasal) sprawdzane co miesiąc; zwycięstwo epizodu. Prośby miast z terminem, spełnianiem i wycofaniem. Scena demo gra przygodę `scenarios/demo.json`. Format zapisu v22.

Etap 2 zrobiony (L-115, L-054b): zdarzenia `god_attack` (gniew 14 bogów według doc 10 i 21, obrona Zeusa, Hery i Afrodyty, klątwy wyłączające budynki na 45 dni, armie Amazonek i Sparti) i `disaster` (trzęsienie ziemi, fala pływowa, osuwisko, lawa w promieniu wokół punktu). Format zapisu v23.

Etap 3 zrobiony (L-116, L-057b): zdarzenie `monster` (punkt `M` na mapie, siła, cel: żywność, obywatele, wojsko, zasoby), potwór niszczy cele i wraca odpocząć, walczy z żołnierzami tą samą mechaniką co najeźdźcy, Apollo zabija potwory; Hero's Hall tylko przy potworze w mieście, przyzwanie herosa po spełnieniu wymagań z doc 21 (Perseusz pobiera 3000 dr), heros idzie na swojego potwora. Format zapisu v24. Zadania bogów (L-118): zdarzenie `quest` daje zadanie z herosem tradycyjnym lub wskazanym, zadanie jest powodem do Hero's Hall i przyzwania, heros wyrusza do miejsca zaczarowanego i wraca z posągiem boga; pomniki pamiątkowe za zadanie, potwora i podbój; cel `quest`. Format zapisu v26. Zostaje: wymagania herosów zależne od triery, murów, igrzysk i podatków, pomniki, wizyty boga na mapie, kolonie i kolejne epizody, prośby o wojsko.

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
