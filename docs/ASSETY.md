# Inwentarz assetów do wyprodukowania

Źródło kategorii i liczności: analiza kodu ładującego grafikę i dźwięk w eZeus (S-008), który używa oryginalnych plików gry. **Z oryginału bierzemy wyłącznie listę kategorii i liczby** (ile animacji, ile wariantów, ile ścieżek). Wszystkie assety tworzymy sami w Blenderze lub z otwartych źródeł (R-002, `docs/02-ORYGINALNOSC.md`). Pełne notatki z cytatami (plik:linia, 364 szablony tekstur) leżą w katalogu roboczym sesji.

## Jak przekładamy oryginał 2D na nasz potok 3D

| Oryginał (sprite'y izometryczne) | Nasz odpowiednik (Godot 4, 3D) |
|---|---|
| 4 poziomy zoomu (kafel 30×15 ... 120×60 px) | jeden model 3D z LOD (2-3 poziomy) |
| 8 kierunków, 5 rysowanych + 3 lustrzane | model obraca się dowolnie; animacja jedna dla wszystkich kierunków |
| klatki animacji (np. chód 12 klatek) | klip animacji; liczba klatek oryginału = wskazówka długości klipu |
| tłum: każdy spacerujący to sprite | tłum: MultiMesh + animacje wypiekane do tekstur (VAT) |
| nakładki animowane na budynek (6-60 klatek) | animowane części modelu, cząsteczki, shader |
| ~55 300 sprite'ów na poziom zoomu | ok. 150 modeli budynków i ich części, ok. 110 typów postaci, 14 bogów, 8 herosów, 17 potworów, ok. 35 jednostek wojskowych |

Wariant awaryjny (gdyby 3D w czasie rzeczywistym było za drogie na telefonie): renderujemy te same modele z Blendera do sprite'ów izometrycznych w 5 kierunkach. Potok musi to umożliwiać od początku.

## Konwencje potoku (wiążące dla `tools/blender/`)

- 1 kafel = 1 m; wysokość kondygnacji i proporcje z architektury greckiej (`PRAKTYKI.md` rozdz. 9: kształt realistyczny, stylizacja w materiale i świetle).
- Front budynku w osi −Y; punkt odniesienia w narożniku obrysu; obrys zgodny z `data/building_stats.json`.
- Postacie: wspólny szkielet humanoida; jeden zestaw "spacerujący standardowy" (chód + śmierć) dla ok. 30 typów, różnice w ubiorze i rekwizytach.
- Towary: osobne modele ładunków (skrzynie, amfory, bele, bloki, sztaby) wspólne dla wozów, magazynów i stosów przy budynkach.
- Każdy asset ma wpis w rejestrze pochodzenia (autor, źródło, licencja, zmiany) przed wejściem do repo.

## Parametry budynku: wiek, stan, praca (plan 2026-11-27)

Jeden model budynku i jeden shader (`game/shaders/building.gdshader`) dają wszystkie stany z O-005 i wiek. Oprawa tylko czyta stan rdzenia; parametry liczy `boot.gd` co dzień gry i podaje jako `instance uniform` (przy przejściu na MultiMesh, moduł E3: te same cztery liczby w `INSTANCE_CUSTOM`).

| Parametr | Zakres | Z czego (rdzeń, `get_buildings`) | Co widać |
|---|---|---|---|
| `build` | 0-1 | `age_days` / 4 dni (ok. 3,4 s przy 1x) | fundament, mury, dach po kolei; rusztowanie do końca budowy (tylko obraz, budynek działa od pierwszego ticku, O-005) |
| `age` | 0-1 | `age_days` / 1 920 dni (10 lat) | patyna: przygaszony tynk, zabrudzenie przy ziemi, mech na dachu; [ZAŁOŻENIE] liniowo |
| `damage` | 0-3 | `damage_risk` / próg 960: od 40 %, 70 %, 90 % | 1 pęknięcia na tynku, 2 odpadający tynk (kamień pod spodem), 3 przechylone luźne części |
| `fire` | 0-3 | `burning_days` z 32: faza 1 do 1/3, 2 do 2/3, 3 reszta | 1 sadza i żar, 2 płomienie (cząsteczki, etap E), 3 zwęglenie |
| `working` | 0/1 | obsada > 0 i brak klątwy | ruchome części w ruchu (np. kopuła obserwatorium, prasa, krosno, woda w fontannie) |
| wariant | | poziom domu (`house_level`), kategoria ruiny | osobny model na poziom domu; ruina osobnym modelem na kategorię |

Maski części w kolorach wierzchołków modelu (eksport glTF `COLOR_0`), ustawiane w skryptach Blendera:

| Kanał | Znaczenie |
|---|---|
| R | etap budowy części: 0 fundament, 1/3 mury, 2/3 dach, 1 rusztowanie |
| G | tynk lub farba: tu działają patyna, pęknięcia i odpadanie tynku |
| B | część luźna: przechyla się przy `damage` 3 |
| A | część ruchoma: obraca się wokół osi pionowej przez punkt z `UV2` (x, z w metrach), gdy `working` |

Kolor bazowy (stylizowany realizm, etap 1a) idzie w drugim zestawie kolorów materiału według części (tynk, kamień, dachówka, drewno); tekstury CC0 (etap 1b) dojdą w tym samym shaderze jako mapy kafelkowane po świecie, bez zmiany modeli.

## Animacje postaci (etap 2, Mixamo od właściciela)

Spis czynności wyprowadzamy z kodu eZeus i instrukcji (nazwy animacji i liczby klatek, S-008, rozdz. "2. Postacie" niżej), bez oglądania plików graficznych oryginału (R-017). Właściciel dostarcza klipy z Mixamo, my je wypiekamy do tekstur (VAT, jak w prototypie E4) na wspólnym szkielecie. Wstępny podział: chód i śmierć dla każdego spacerującego; praca w miejscu (budowa, kopanie, zbiór, noszenie wody, gaszenie); występy (aktor w teatrze, filozof na podium, zawodnik na stadionie i gimnazjonie, muzyk); wojsko (marsz, atak wręcz, rzut, strzał, śmierć); bogowie, herosi i potwory po kilka klipów walki i pojawienia. Lista nazw z odpowiednikami Mixamo powstanie na początku etapu 2.

## Budynki interaktywne

Części ruchome sterowane `working` (kanał A): obserwatorium atlantydzkie obraca kopułę i lunetę, fontanna ma płynącą wodę, warsztaty mają ruchome narzędzia (prasa oliwy, krosno, miech kuźni), stadion i hipodrom ruch na trybunach (etap postaci). Ruch tylko w obrazie, symulacji nie dotyczy.

## 1. Budynki

### Mieszkalne
| Asset | Liczba |
|---|---|
| Dom ludu grecki | 7 poziomów × 2 warianty + pusta działka |
| Dom ludu atlantydzki | 5 poziomów (Hovel-Townhouse) × 2; Hut i Shack wspólne z greckimi |
| Dom elity grecki i atlantydzki | po 6 stanów (pusty, 4 poziomy, opuszczony), 4×4 |
| Konie przy Estate | 4 warianty (liczba koni w stajni) |
| Ikony potrzeb domu | 6 par (ma / brak): żywność, wełna, oliwa, wino, broń, konie |

### Produkcja i rolnictwo
| Asset | Zawartość |
|---|---|
| Farmy (pszenica, marchew, cebula) | baza + 5 poletek; uprawa w **6 fazach wzrostu** |
| Drzewo oliwne, winorośl, drzewo pomarańczowe | 6 faz dojrzewania |
| Growers' Lodge, Orange Tenders' Lodge, Dairy, Carding Shed, Hunting Lodge, Corral | budynek + animacja pracy; Corral: ubój i bydło w zagrodzie |
| Fishery, Urchin Quay | 4 orientacje na brzegu; budowa łodzi, rozładunek |
| Granary | budynek + wypełnienie: 8 rodzajów żywności widocznych w otworach |
| Storehouse | budynek + **19 towarów × 4 poziomy wypełnienia** na polu + rzeźba |
| Stosy towarów przy budynkach | 4 poziomy dla 10 towarów |
| Warsztaty i wydobycie | Timber Mill, Masonry Shop (stos kamienia 8 stanów, kamieniarze), Black Marble Workshop, Mint, Foundry, Refinery, Olive Press, Winery, Sculpture Studio, Armory, Artisans' Guild, Chariot Factory, Trireme Wharf (4 orientacje), Horse Ranch: każdy z animacją pracy |

### Dystrybucja, administracja, kultura, nauka
| Asset | Zawartość |
|---|---|
| Agora zwykła i wielka | podstawa + odcinki drogi; 4 orientacje |
| Stoiska (7) | jedzenie, wełna, oliwa, broń, wino, konie, rydwany; każde z animacją |
| Trading Post, Pier, Palace (2 orientacje), Tax Office, Watchpost, Maintenance Office, Fountain, Infirmary | budynek + animacja |
| College, Gymnasium, Drama School, Podium, Theater, Stadium (2 orientacje, wykonawcy i publiczność) | budynek + animacja |
| Bibliotheke, University, Observatory, Laboratory, Inventors' Workshop, Museum | budynek + animacja |
| Hipodrom | ok. 14 segmentów toru, trybuny, publiczność, linia mety, sprzedawcy |

### Wojskowe i ozdoby
| Asset | Zawartość |
|---|---|
| Mur | elementy łączące się automatycznie (~36 przypadków) |
| Wieża, brama (2 orientacje), wieża atlantydzka z orichalkiem, Roadblock | |
| Sztandary kompanii | drzewce, flagi, zwieńczenia (grecki, atlantydzki) |
| Ozdoby greckie | park (3 rozmiary), ławka, ogród, altana, labirynt, staw z rybami, 3 porządki kolumn + łuk łączący, aleja i bulwar (zestawy elementów), 9 pomników pamiątkowych |
| Ozdoby atlantydzkie | 12: park wodny (8 wariantów), poidełko, 2 obeliski, ogród z muszli, zegar słoneczny, delfiny, planetarium, źródło, ogród topiarny, łaźnie, krąg kamienny |

### Sanktuaria, herosi, piramidy
| Asset | Zawartość |
|---|---|
| Elementy sanktuarium | płytki, schody, ołtarz, ogień ofiarny, poświata; **świątynia w 3 fazach budowy × 4 orientacje** |
| Posągi bogów | 14 bogów × 4 orientacje; posąg nagrody za zadanie: 14 |
| Ofiary | animacje ofiary z byka, owcy, towarów |
| Układy sanktuariów | 14 planów (płytki, posąg, świątynia, ołtarz, pola specjalne: miedź, srebro, drzewa) |
| Hero's Hall | grecki, atlantydzki + 8 posągów herosów |
| Piramidy | elementy ścian, szczytów, płytek w białym i czarnym marmurze na każdy poziom; świątynia, ołtarz, obserwatorium, muzeum; 1-3 fazy budowy |

W oryginale zwykłe budynki nie mają faz budowy ani stanów uszkodzenia; zawalony budynek to ogólny gruz. **U nas (O-005, decyzja właściciela):**
- każdy budynek ma wizualną fazę budowy (rusztowanie, mury, dach), odtwarzaną bez wpływu na symulację;
- 3 stany uszkodzenia sterowane licznikiem ryzyka zawalenia, pożar w 2-3 fazach;
- własna ruina dla każdej kategorii budynku.

Konsekwencja dla produkcji: na każdy model budynku dochodzą warianty "budowa" (3 etapy, najlepiej generowane proceduralnie z modelu gotowego), "uszkodzenie" (3 stopnie, przez materiał i deformację) i "ruina".

## 2. Postacie

| Grupa | Liczba typów | Animacje (długość w klatkach oryginału) |
|---|---|---|
| Spacerujący standardowy | ok. 30 | chód 12, śmierć 8 |
| Spacerujący z pracą | ok. 20 | np. strażak: chód, noszenie wody, gaszenie 40; rzemieślnik: budowa 9; górnik: kopanie 10; ogrodnik: praca i zbiór przy oliwkach i winoroślach; zbieracz jeżowców: pływanie, zbiór 40 |
| Wozy i łodzie | | wóz z ładunkiem dla 16 towarów (2-3 poziomy załadunku), wóz z wołami (marmur, drewno, rzeźba, czarny marmur), łódź rybacka, statek handlowy, triera, fregata, okręty wroga |
| Zwierzęta | 11 | dzik, koza, wilk, jeleń, owca (ostrzyżona i nie), koń, byk, bydło (3 umaszczenia), wół, konie wyścigowe (4 maści), ryby i jeżowce jako znaczniki |
| Jednostki gracza | 7 | rabble (rzut 12), hoplita, jeździec, łucznik na wieży; atlantydzkie: hoplita, łucznik, rydwan |
| Wrogowie | 11 nacji, ok. 30 jednostek | Grecy, Trojanie, Persowie, centaury, Amazonki, Egipcjanie, Majowie, Fenicjanie, Oceanidy, Atlantydzi; wojownicy Aresa |
| Bogowie | 14 | chód 13-31, walka 16-33, pojawienie/zniknięcie 16-40; 5 bogów z błogosławieństwem |
| Herosi | 8 | chód 15-32, walka 15-35, śmierć 20-31 |
| Potwory | 17 | chód, śmierć, 2 ataki; Kraken i Scylla wodne |

## 3. Teren

- Ląd suchy, zarośla, łąka kwietna, przejścia między nimi.
- Las (grecki i atlantydzki), las wycięty.
- Woda animowana (oryginał: 15 typów), plaża, bagno (płytkie i głębokie), lawa.
- Skały i złoża w 3 rozmiarach (1×1, 2×2, 3×3): kamień, miedź, srebro, orichalk.
- Kamieniołom marmuru i czarnego marmuru z pogłębianiem.
- Wzniesienia i klify (pojedyncze, podwójne, połówkowe).
- Drogi: zwykła, **ładna (zależna od atrakcyjności)**, przy plaży, aleja; mosty grecki i atlantydzki.
- Pęknięcia po trzęsieniu ziemi.
- Nakładki: atrakcyjność (10 stopni), podstawa budynku, kafel wybrany i nieprawidłowy.
- Znaczniki edytora: 13 punktów (wejście, wyjście, katastrofy, zwierzęta, potwory).

## 4. Efekty

Pożar budynku (3 warianty), ogień ofiarny, aura błogosławieństwa i klątwy, magia boga, chmura zarazy, fala pływowa, wypływ lawy (10 animacji), pył przy zawaleniu i osuwisku, wybuch orichalku, pociski: kamień, włócznia, strzała (oryginał: 32 kierunki), pociski bogów (6 kolorów), ogień atlantydzki.

## 5. Interfejs

- Panele i ramki, przyciski 12 kategorii budowy (populacja, rolnictwo, przemysł, dystrybucja, higiena i bezpieczeństwo, administracja, kultura, nauka, mitologia, wojsko, estetyka, przegląd), ok. 45 podprzycisków.
- 19 ikon alertów, 23 ikony towarów + drachma, 14 ikon zadań bogów, ikony rozkazów magazynów, panel wojska (taktyki, rozkazy).
- Mapa świata: tła, znaczniki miast dla 10 nacji, kolonie, ruiny, strzałki dalekich miast, znaczniki armii i herosów, flagi, ikony siły i buntu.
- Portrety: doradcy i postacie (oryginał ok. 166), bogowie (32).
- Ekrany: ładowanie, menu, wybór gry, wprowadzenie do misji, zwycięstwo, porażka, ilustracje kampanii.
- Czcionka: własna, w stylu greckim, z pełnym zestawem polskich znaków.

Projekt UI robimy pod dotyk (`docs/06-MOBILE-UX.md`); lista mówi, **jakie informacje** muszą mieć swoje grafiki.

## 6. Audio

| Kategoria | Oryginał | Nasz plan |
|---|---|---|
| Muzyka | 12 utworów miejskich, 6 bitewnych, menu, 3 dżingle | muzyka adaptacyjna z warstwami (`08-OPRAWA.md`) |
| Narracja kampanii | per epizod | własne teksty, lektor PL i EN |
| Ambient | ok. 240 plików w 2 warstwach, ok. 60 kategorii (budynki, zwierzęta, teren, katastrofy) | dźwięk z pozycji budynku, natężenie z zoomu |
| Głosy spacerujących | ok. 45 typów, 7 komentarzy o stanie miasta + kwestia bezczynności + kwestie zdarzeń; ok. 400 plików | ta sama struktura kwestii; teksty piszemy sami |
| Bogowie | ok. 25 plików na boga (zazdrość, zaloty, inwazja, potwór, zadanie, pomoc, pojawienie, klątwa, sanktyfikacja, ataki) | |
| Herosi | ok. 13 na herosa | |
| Potwory | ok. 7 na potwora | |
| Walka i UI | ok. 60 | |

## Kolejność produkcji (propozycja)

1. Zestaw testowy do prototypu tłumu (E4): 1 postać standardowa z chodem i śmiercią, 5 budynków różnej wielkości, kafle terenu.
2. Vertical slice (faza 2): łańcuch żywności, domy 7 poziomów, Agora, fontanna, 1 sanktuarium, 1 bóg, 1 heros, 1 potwór, podstawowe UI.
3. Reszta greckiej treści, potem atlantydzka.
