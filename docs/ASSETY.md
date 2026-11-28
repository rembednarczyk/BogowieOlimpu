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

- 1 kafel = 1 jednostka = 2 m rzeczywiste (człowiek 1,8 m to 0,9 kafla, jak spacerujący w grze; działka domu 2×2 kafle to dom ok. 4×4 m); wysokość kondygnacji i proporcje z architektury greckiej (`PRAKTYKI.md` rozdz. 9: kształt realistyczny, stylizacja w materiale i świetle). Skrypty podają wymiary w metrach przez stałą `M` (`tools/blender/bo_asset.py`).
- Front budynku w osi −Y; punkt odniesienia w narożniku obrysu; obrys zgodny z `data/building_stats.json`.
- Postacie: wspólny szkielet humanoida; jeden zestaw "spacerujący standardowy" (chód + śmierć) dla ok. 30 typów, różnice w ubiorze i rekwizytach.
- Towary: osobne modele ładunków (skrzynie, amfory, bele, bloki, sztaby) wspólne dla wozów, magazynów i stosów przy budynkach.
- Każdy asset ma wpis w rejestrze pochodzenia `game/models/provenance.json` (autor, źródło, licencja, zmiany) przed wejściem do repo (R-017).
- Jeden budynek to jedna siatka i jeden materiał (jedno wywołanie rysowania): kolor części z palety (UV0), maski parametrów w kolorach wierzchołków, punkt obrotu części ruchomej w UV1. Pilnuje tego `tools/checks/check_models.py` (bez Blendera, w CI) razem z obrysem z `data/building_stats.json` i zgodnością palety ze shaderem.

Stan (2026-11-27): `tools/blender/models.py` buduje siedem poziomów domu ludu (chata, szałas, lepianka, zagroda chłopska, kamienica czynszowa, mieszkanie, dom miejski), fontannę, spichlerz, magazyn i obserwatorium atlantydzkie; gra wczytuje je w miejsce brył (`boot.gd` `_model`, `_update_looks`). Domy powstają cegła po cegle i dachówka po dachówce (`tools/blender/bo_detail.py`: cegła 40×40×10 cm w wiązaniu z przesunięciem, spoina rosnąca z warstwą, tynk jako powłoka nad cegłą, dachówki płaskie i nakrywające, kalenica, strzecha warstwami, drzwi z desek, okiennice, kolumny doryckie, pitosy); każdy element ma własny próg budowy, więc w grze dom rośnie warstwa po warstwie przez 4 dni gry (`_grow` co klatkę). Dwa widoki: z bliska (LOD0, 25-82 tys. trójkątów, budżet 120 tys.) i z daleka (`_lod1`, 0,3-2,0 tys., budżet 3 tys.), przełączane przy rozmiarze kamery 26 kafli (`LOD_CLOSE_SIZE`). Zrzuty: `screenshot.gd ... build_view=7` (stany spichlerza), `build_view=8` (siedem poziomów domu i budowa domu miejskiego). Przeglądarka 3D dla właściciela: artefakt „Domy Bogów Olimpu”.

Dobra przy domu (uwaga właściciela 2026-11-27): każdy poziom ma przy sobie dobra, których potrzebuje do rozwoju (`data/housing.json`): żywność od szałasu (worki ze zbożem, kosze z cebulą, figami, marchwią, oliwkami), woda od lepianki (hydria z trzema uchami, studnia w mieszkaniu), wełna od zagrody (bele runa, krosno pionowe z ciężarkami w kamienicy), oliwa od mieszkania (pitosy z pokrywą, amfory z korkiem). Rekwizyty żywności, wełny i oliwy niosą w UV1 numer dobra (`-1 - HouseGood`) i shader pokazuje je tylko, gdy dom ma to dobro w zapasie (`goods`, maska z `house_goods` rdzenia); woda to zasięg, nie zapas, więc hydria stoi zawsze. Zdobienia wyższych poziomów: gzyms, meander w reliefie, antefiksy z palmetą, akroteria, obramienia okien z daszkiem na konsolach, fryz z tryglifami i metopami, toczone tralki balkonu, donice z roślinami, studnia z żurawiem. Odsłonięta cegła i kamienne bloki mają fazowane krawędzie; dachówki to pełne bryły z grubością; naczynia gładkie z interpolacji profilu (48 boków); drewno ułożone fizycznie: każda kłoda leży na dwóch pod sobą w punkcie styku, połówki płaską stroną w dół, obok pieniek z siekierą.

Uwagi właściciela 2026-11-27, wdrożone: rekwizyty nie wchodzą w budynek ani w siebie i mieszczą się na działce (`Builder.solid`, `Builder.prop`, `check_props`: eksport przerywa się z nazwą kolizji); rekwizyty to zamknięte bryły; pitos wysoki i pękaty jako jedna bryła obrotowa z 32 bokami, z wywiniętym wylewem, opaskami i oliwą w środku, amfora z dwoma uchami; sąg z kłód pnia i gałęzi, połówek i ćwiartek z łupania, z korą i jaśniejszym przekrojem, gałęzie na wierzchu; dachy lakońskie (wklęsła dachówka pod spodem, wypukła nakrywająca na łączeniu) ze szczytami osłoniętymi deską wiatrową i rzędem dachówek nakrywających; strzecha z okapem z pojedynczych źdźbeł w dwóch rzędach i kalenicą związaną sznurem; szałas (poziom 2) z drewnianego szkieletu obciągniętego płótnem i skórami, z fałdami, zwiniętą klapą wejścia, linkami do kołków i paleniskiem; kolumny doryckie żłobkowane (40 boków) z entazą; drzwi z okuciami, okna w ramach ze szczeblinami.

Uwagi właściciela 2026-11-28, wdrożone:
- Dachy nie wchodzą w obrys innej części budynku: ściany, cokoły i kolumny należą do grup (`group`), `check_props` sprawdza punkty każdego dachu względem ścian innych grup. Przybudówki (skrzydło zagrody, skrzydła mieszkania, portyk domu miejskiego) mają dach jednospadowy (`shed_roof`) opadający od wyższej ściany.
- Balkon kamienicy: drzwi z piętra na balkon, balustrada zamknięta z trzech stron (słupki narożne i przyścienne, poręcz, dolny rygiel, tralki), każda tralka sprawdzana w obrysie płyty.
- Trochę stylu Ghibli: paleta cieplejsza i nasycona (28 pozycji, nowe: trawa, trzy kolory kwiatów, pranie); okiennice turkusowe albo ochrowe; w `building.gdshader` funkcja `light()` z miękkim przejściem światła w cień (smoothstep od -0.05 do 0.35), cień chłodny, niebieskawy, strona słoneczna ciepła, cienka ciepła poświata na krawędziach, lekkie plamy koloru jak od pędzla. Zieleń z `bo_detail.py`: kępy trawy i polnych kwiatów tylko na wolnym gruncie (promienie w górę i kolizje z bryłami), bluszcz na ścianach (lepianka, mieszkanie), skrzynki z kwiatami pod oknami (kamienica, mieszkanie, dom miejski), kwitnące donice, pranie na sznurze w zagrodzie. Kalenice lekko się uginają (`sag` 0.05 m).

Uwagi właściciela 2026-11-28 (druga runda), wdrożone:
- Zamiana modeli: poziom 1 to namiot (szkielet z płótnem i skórami, bez dóbr, bo poziom 1 ich nie potrzebuje), poziom 2 to chata z gliny ze strzechą, przy niej worki zboża i kosz cebuli (żywność od poziomu 2, `data/housing.json` Fd). Nazwy poziomów w danych bez zmian (do decyzji, `docs/11-OTWARTE-PYTANIA.md`).
- Nieregularność ręcznej roboty we wszystkich budowlach (`Builder._wonk` w `bo_asset.py`): jedno gładkie pole wygina cały model razem z rekwizytami, więc nic nie wchodzi w ścianę: budynek lekko się przechyla (do 1,2 cm na metr wysokości), ściany pęcznieją w dwóch trzecich wysokości (2-3,5%), linia dachu faluje (do 3,6 cm). Ziemia i działka zostają na miejscu, ziarno z nazwy modelu. Długie płaskie ściany dostają przed wygięciem więcej wierzchołków, żeby tynk wyginał się razem z cegłami. Widok z daleka (LOD1) ma tylko przechył, bez zagęszczania, i mieści się w budżecie 3000 trójkątów.
- Przerysowana greckość: terakotowe kominy z pękatą donicą i daszkiem na trzech nóżkach na kalenicach poziomów 3-7, laurowe wieńce z czerwoną wstążką nad drzwiami zagrody, mieszkania i domu miejskiego, czarnofigurowe pasy z czerwonymi liniami na amforach i hydriach, akroteria o połowę większe.

Uwagi właściciela 2026-11-28 (trzecia runda), wdrożone:
- Dachy łukowe: każda połać dachówkowa ugina się między okapem a kalenicą (`curve` 0,09 m: okap płaski, połać stromieje ku kalenicy), kalenica wygina się w środku (`sag` 0,08 m). Szczyty ścian idą pod spodem łuku, więc nie ma szpary ani tynku przebijającego dachówkę. Wzorzec: charakterystyczne wygięte dachy gry referencyjnej, oglądane tylko jako proporcja (zrzut właściciela, poza repo); kształt i detal nasze.
- Dach przestał wyglądać jak drewniany: podkład pod dachówkami leży 14 cm pod połacią, w kolorze ciemnej terakoty, i nie wychodzi między gąsiorami; deska wiatrowa na szczycie zastąpiona murowaną obróbką w materiale szczytu, deska przy wyższej ścianie dachu jednospadowego zastąpiona zaprawionym rzędem dachówek.
- Kamienica bez lewitujących części: belki balkonu podparte zastrzałami z kamiennych wsporników w ścianie, zamiast wiszących haków. Od 2026-11-28 (uwaga właściciela: zastrzały odchylały się na zewnątrz) balkon wisi na czterech żelaznych ściągach od płyt w ścianie pod fryzem do przedniej poręczy, bez zastrzałów pod spodem.

## Warianty i historie domów (plan 2026-11-28, przyjęty)

Stan 2026-11-28: kroki (a), (b) i (d) zrobione.
- Generator: `tools/blender/bo_story.py`. Funkcja poziomu deklaruje wolny grunt (`slot`: podwórze, pas przy ścianie, róg), `dress` stawia zestaw historii w kolejności, każdy element w pierwszym wolnym miejscu (bryły, rekwizyty i promienie z ziemi jak przy trawie), na końcu `check_props`. Element, który się nie zmieści, eksport wypisuje jako `NO ROOM`.
- Historie poziomów 1-3 (`common_housing_<poziom>_<historia>.glb` z LOD1): pasterze (żerdź z runem, kij pasterski z dzwonkiem i bukłakiem, płot z wikliny, ława do strzyżenia z runem), garncarze (koło garncarskie, piramidy garnków, piec z gliny, półka z naczyniami; dom lustrzany, ściany chaty bielone, namiot ochrowy), ogrodnicy i pszczelarze (grządka, tyczki z fasolą, donice z ziołami, gliniane ule na kamieniu; w lepiance pergola z winoroślą nad drzwiami; namiot z płótna w zgaszonej zieleni (od 2026-11-28; błękit zostaje dla dachów budynków publicznych)).
- Gra (`boot.gd`): historia z id budynku (`house_story`), trwała przy awansie i po wczytaniu; dom obrócony o 0, 90, 180 albo 270 stopni (`house_turn`) i w skali 0,92-1,00 (`house_scale`), zawsze w obrysie działki. Test w `test_ui.gd`: trzy historie, cztery obroty, domy na działkach, historia ta sama na każdym poziomie.
- Poziomy 4-7 (2026-11-28, krok c): każda historia ma na każdym poziomie jeden duży element od frontu, czytelny z kamery gry. Pasterze: farbowana wełna na sznurze w zagrodzie, na balustradzie kamienicy, na drągu między skrzydłami mieszkania, kolorowe tkaniny między kolumnami domu miejskiego. Garncarze: pasiasta markiza z wiszącymi garnkami nad drzwiami zagrody, nad balkonem kamienicy (amfory na balkonie) i na dwóch słupach nad dziedzińcem mieszkania; garnki i talerze między kolumnami i dwa kratery na portyku. Ogrodnicy: winorośl na ramie nad drzwiami zagrody i nad balkonem kamienicy, pergola nad dziedzińcem mieszkania, winorośl na kolumnach i wzdłuż architrawu domu miejskiego. Garncarze mają dom lustrzany na każdym poziomie. Razem 21 modeli historii z LOD1 (`bo_story.py`: `skeins`, `awning`, `vine_canopy`, `swag`, `column_vine`, `garland`, `hanging_pots`, `krater`).
- Szałasy (poziom 1, 2026-11-28, uwaga właściciela: mniejsze, różnej wielkości i kształtu, do połowy działki, bez wrażenia obozu wojskowego): pasterze niska kopulasta chata z wygiętych tyczek pod skórami (średnica 2,6 m, wys. 1,95 m), garncarze wiata jednospadowa otwarta od frontu z ławą i garnkami (2,8 x 1,6 m), ogrodnicy mały namiot z kalenicą (2,2 x 1,8 m), wersja bez historii namiot 2,4 x 2,0 m; każdy przesunięty na działce, obrys z okapem 26-37 % działki, reszta na rekwizyty historii i zieleń.
- Wzmocnienie domu miejskiego (2026-11-28): garncarze dostali pasiastą markizę z sześcioma garnkami pod architrawem, nad schodami; ogrodnicy winorośl od kolumn przez architraw na dach portyku (liście leżą na dachówkach tam, gdzie trafiają promienie z góry, zwisają z okapu, winogrona na szypułkach).
- Zamiana 2026-11-28: poziom 5 to mieszkanie, poziom 6 kamienica (z oliwą; ponad mieszkanie także w detalu: czerwony malowany fryz pod gzymsem, meander na froncie, antefiksy, akroteria); wpisy niżej i wyżej o "kamienicy (poziom 5)" dotyczą dziś poziomu 6.
- Mieszkanie: skrzydła stały 30 cm nad ziemią bez cokołu; dostały własne cokoły (grupy skrzydeł).
- Drzwi do drogi: obrót domu wybierany spośród tych, przy których drzwi patrzą na najbliższą drogę (do 4 kafelków, `door_turns` w `boot.gd`); bez drogi w zasięgu dowolny.
- Do zrobienia: pozostałe budowle; elementy ziemne historii (kij, garnki) dla poziomów 4-7, gdzie zostanie miejsca.

Cel właściciela: każdy dom opowiada swoją historię, miasto jest bardziej zróżnicowane; każdy budynek ma 2-3 warianty.

1. **Historia trwa przez poziomy.** Historia to rodzina, która rośnie razem z domem: namiot pasterzy staje się chatą z zagrodą dla owiec, potem kamienicą z krosnem, w końcu domem miejskim z warsztatem tkackim. Dom dostaje historię przy postawieniu i zachowuje ją przy awansie.
2. **Wybór bez rdzenia.** Historia jest czystą oprawą: `boot.gd` liczy ją z id budynku (mieszanie z solą, modulo liczba historii), więc ten sam dom ma tę samą historię po wczytaniu zapisu, a sąsiedzi rzadko są tacy sami. Rdzeń, zapis i złoty hash bez zmian. Model: `common_housing_<poziom>_<historia>.glb`, gdy go brak, `common_housing_<poziom>.glb`.
3. **Trzy historie domów zwykłych (propozycja):**
   - pasterze: suszące się runo, płot z wikliny, dzwonek, ława do strzyżenia, potem krosno i bele wełny;
   - garncarze: piramidy garnków, mały kopulasty piec, półka z suszącymi się naczyniami, malowane amfory i kratery, w domu miejskim lada z naczyniami;
   - ogrodnicy i pszczelarze: grządki, gliniane ule leżące poziomo, pergola z winoroślą nad wejściem, gołębnik na dachu w wyższych poziomach.
4. **Co się zmienia w wariancie:** zestaw rekwizytów historii, akcenty kolorów (drzwi, okiennice, tkaniny), układ (drzwi z lewej lub z prawej, lustrzane skrzydła), jedna cecha bryły (pergola, schody zewnętrzne na dach, loggia zamiast balkonu, gołębnik) i własny detal grecki historii (herma przy bramie, ołtarzyk domowy, malowany fronton).
5. **Generator, częściowo.** Bryła zostaje pisana ręcznie w `models.py`, z przełącznikami wariantu (`story`), bo to ona daje jakość. Rekwizyty rozstawia generator w nowym `tools/blender/bo_story.py`: funkcja poziomu deklaruje wolne miejsca (`b.slot(nazwa, prostokąt, rodzaj)`: podwórze, ściana, parapet, kalenica), a `dress(b, story, level, rng)` wypełnia je zestawem historii według priorytetu i pomija to, co by kolidowało (ta sama kontrola co dla trawy i `check_props`). Opis historii (zestawy, kolory, cechy na poziom) w `tools/blender/stories.py` jako dane. Nieregularność ma ziarno z poziomu i historii.
6. **Pozostałe budowle:** ten sam mechanizm, 2-3 warianty każdej, gdy dostaną pełny model (np. fontanna z wylewką w kształcie lwa, delfina albo amfory).
7. **Przeglądarka:** przyciski historii obok poziomów.
8. **Kolejność:** (a) sloty i `dress` bez zmiany wyglądu obecnych modeli; (b) trzy historie poziomów 1-3; (c) poziomy 4-7; (d) wybór w `boot.gd` i test w `test_ui.gd`: różne id dają różne historie, awans zachowuje historię; (e) pozostałe budowle.
9. **Koszt:** 21 modeli domów i 21 widoków z daleka. Teraz 18 plików waży 45 MB, z historiami około 130 MB. Przed krokiem (b) trzeba zdecydować o przechowywaniu (Git LFS albo generowanie przy budowaniu).

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

Stany domów dopracowane 2026-11-28 (uwaga właściciela: „dopracuj wszystkie stany domów”):
- Budowa: prawdziwe rusztowanie (stojaki, poziome rygle co metr, deski na każdym poziomie, zastrzały, drabina od frontu); szałas z płótna bez rusztowania.
- Wiek: dodatkowo smugi deszczu na murach i więcej mchu na dachówkach.
- Uszkodzenie: 1 pęknięcia przez wszystkie mury (dwie skale, wyraźne); 2 odpada tynk, zsuwają się dachówki (10 %), rwą się płótno i strzecha; 3 więcej tego wszystkiego (25 % dachówek) i cały dom osiada krzywo (pochylenie 3,5 %, luźne części 25 %).
- Pożar: 1 tli się (osmolenie, migoczący żar nisko); 2 płomienie wspinają się po ścianach i dachu (ruchome języki, mocniej na strzesze, płótnie i dachówkach); 3 zgliszcza (czerń, strzecha i płótno spalone do szkieletu, połowa dachówek spadła, żar w ruinie).

Kolejność budowy i zawalenie (uwaga właściciela 2026-11-28: „budowa w realistycznej kolejności”, „realistyczna fizyka zawalania się, tak wyglądają ruiny, z animacją”):
- Budowa: cokół, mury warstwa po warstwie, belki i więźba, zaprawa pod dachówki rząd po rzędzie, dachówki od okapu do kalenicy; stolarka (drzwi, okiennice) od 96,5 % budowy, a wykończenie i dobytek (markizy, rośliny, pranie, dzbany, krosno, sągi, wieńce, kominy) dopiero w ostatnich 2,5 % budowy, gdy dom stoi już pod dachem (`finishing` w `bo_asset.py`, dekorator w `bo_detail.py` i `bo_story.py`).
- Zawalenie, fizyka i pył: zasady dla każdego budynku w `docs/STYL-BUDOWLI.md` rozdz. 13 (i materiały w rozdz. 12); tutaj tylko to, co dotyczy domów.
- Pliki: `common_housing_N_ruin.glb` (kawałek w `UV2.x`, 0 to część stojąca) i `common_housing_N_ruin.bin` („BORU”, klatki, kawałki, fps, potem macierze 3×4 w osiach Godota). Shader czyta je przez teksturę (`ruin_frames`, `ruin_len`, parametr instancji `collapse` w sekundach, -1 to gruz w spoczynku).
- Gra: gdy rdzeń zamienia dom w ruiny (`ruin_building`, to samo miejsce w tym samym ticku), `boot.gd` stawia ruinę tego domu (własną szałasu według historii, `common_housing_1_<historia>_ruin`, inaczej ruinę poziomu) w jego obrocie i skali, odtwarza upadek i pył; po pożarze ruina jest zwęglona (`fire` 3). Ruiny z zapisu (bez domu przed nimi) leżą w spoczynku, poziom 2-4 z id. Ruiny innych budynków zostają na razie płaską bryłą.
- Bez fizyki w czasie gry: upadek jest wypieczony w Blenderze, powtarzalny (ziarno z nazwy modelu), koszt w grze to jedna tekstura na poziom i odczyt w shaderze wierzchołków.

## Animacje postaci (etap 2, Mixamo od właściciela)

Spis czynności wyprowadzamy z kodu eZeus i instrukcji (nazwy animacji i liczby klatek, S-008, rozdz. "2. Postacie" niżej) (R-017: grafikę oryginału wolno analizować, nie kopiować). Właściciel dostarcza klipy z Mixamo, my je wypiekamy do tekstur (VAT, jak w prototypie E4) na wspólnym szkielecie. Wstępny podział: chód i śmierć dla każdego spacerującego; praca w miejscu (budowa, kopanie, zbiór, noszenie wody, gaszenie); występy (aktor w teatrze, filozof na podium, zawodnik na stadionie i gimnazjonie, muzyk); wojsko (marsz, atak wręcz, rzut, strzał, śmierć); bogowie, herosi i potwory po kilka klipów walki i pojawienia. Lista nazw z odpowiednikami Mixamo powstanie na początku etapu 2.

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
