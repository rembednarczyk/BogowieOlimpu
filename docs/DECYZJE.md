# Dziennik decyzji

Pamięć projektu między sesjami. Nowe wpisy na górze sekcji. Nieaktualne oznaczamy dopiskiem **[NIEAKTUALNE: powód, data]**, nigdy nie usuwamy. Wpis: data, decyzja, powód, gdzie wdrożone, cytat właściciela (jeśli jest), odrzucone warianty, czego nie sprawdzono.

## Produkt i zakres

### 2026-11-27: Modele budynków: stylizowany realizm, potem tekstury CC0; animacje z Mixamo; wiek i stan budynku
- Decyzja: modele budujemy skryptami Blendera w stylizowanym realizmie (proporcje z architektury greckiej, kolor według części), tekstury CC0 jako drugi krok; tylko licencje z R-017; animacje postaci z Mixamo od właściciela w etapie 2, spis czynności z kodu eZeus i instrukcji; każdy budynek ma parametry wieku, budowy, uszkodzenia, pożaru i pracy (`ASSETY.md`), część budynków ruchome elementy (obserwatorium).
- Powód: odpowiedzi właściciela „1a i później b, 2 tak CC0, 3 tak” i uwagi o animacjach Mixamo, budowlach interaktywnych i parametryzacji wieku i stanu. Assety oryginału nie są w repozytorium i nie powstaną z nich przeróbki (R-002).
- Wdrożone: R-017, `ASSETY.md` (parametry, maski, animacje), rdzeń: dzień postawienia budynku (zapis v51).

### 2026-11-27: Cienie słońca w grze na stałe
- Decyzja: cienie słońca włączone w grze (budynki i postacie); LOD tłumu dopiero dla słabszych telefonów.
- Powód: pomiar właściciela na Xiaomi 15T Pro (APK 0.0.44): 6 000 postaci z animacją i 2 000 budynków, GPU średnio 4,42 ms, najgorzej 6,76 ms; cienie dodają setne części milisekundy, są widoczne na budynkach i postaciach („Myślę, że mogą wejść”).
- Wdrożone: `game/scenes/boot.gd` `_build_environment` (cień słońca), zakres głębi kamery ortogonalnej według widoku (`_place_camera`), `07-TECHNOLOGIA.md`.
- Niesprawdzone: wygląd cieni w samej grze na telefonie (w kontenerze Vulkan programowy rysuje je źle); kąt słońca do strojenia przy modelach z Blendera.

### 2026-11-27: Skala interfejsu w dp, 48 dp, panel po prawej, tylko poziomo
- Decyzja: interfejs w dp (dpi/160 na telefonie i tablecie, skala systemowa na PC), cel dotyku 48 dp, suwak skali 75-150 %, tylko orientacja pozioma (obie strony), panel kategorii po prawej jak w oryginale; najmniejszy ekran 640 x 360 dp, nadmiar przewijany.
- Powód: odpowiedzi właściciela na pytania 1-5 o skalowanie (podejście 2 z trzech). Dotychczasowe rozciąganie 1280 x 720 dawało 5,3-6,1 mm przycisku na telefonach i 25 mm na PC 24"; na Xiaomi lista Widok i kolumna budynków wychodziły poza ekran.
- Wdrożone: `game/scenes/ui/ui_scale.gd`, `build_ui.gd`, `army_world_ui.gd`, `boot.gd` (`apply_ui_scale`), `project.godot` (orientacja sensor_landscape), `06-MOBILE-UX.md`. Bramka `game/tests/test_layout.gd` w CI.
- Odrzucone: większa skala bazowa (podejście 1, przyciski na PC ok. 30 mm), dwa układy z progami (podejście 3, czeka na ikony kategorii).

### 2026-11-27: Położenie miast na mapie świata w scenariuszu
- Decyzja: miasto scenariusza ma pole `"map": [x, y]` (0-100 od zachodu i od północy); miasto gracza w środku mapy. Razem z nim panel miasta z historią interakcji (Z:171): każda zmiana opinii z powodem, osiem najnowszych. Zapis v50.
- Powód: odpowiedź właściciela "tak" na pytanie o pole położenia (L-135); położenia są cechą naszych map (R-002).
- Wdrożone: `core/src/scenario.cpp`, `core/src/world.cpp` (`change_opinion` z powodem), `core/src/serialize_atlas.cpp`, `gdext` `get_cities`, `game/scenes/ui/world_map.gd`, `army_world_ui.gd`; położenia w generatorach siedmiu dem. Testy `core/tests/test_world_map.cpp`, `game/tests/test_ui.gd`.

### 2026-11-28: Budowa w realistycznej kolejności; zawalenie wypieczone z fizyki
- Decyzja właściciela: dom rośnie w kolejności z placu budowy, a zawalony dom spada z fizyką; ruina to ostatnia klatka upadku, z animacją.
- Wykonanie: wykończenie i dobytek po rusztowaniu; upadek symulowany raz w Blenderze (`tools/blender/bo_collapse.py`), w grze odtwarzany shaderem z tekstury klatek (opis w `docs/ASSETY.md`). Fizyka w czasie gry odrzucona: koszt na telefonie i brak powtarzalności.
- Nie sprawdzono: wydajności upadku na telefonie (7 tekstur do 2 MB, ruina kamienicy 7,5 MB glb).

### 2026-11-28: Głębokie okapy; śmielsze bryły odłożone
- Decyzja: (3) każdy dach dwuspadowy ma okap co najmniej 34 cm (`EAVE` w `bo_detail.py`, przycięty do brzegu działki), wdrożone. (2) Rozbijanie niższych poziomów na bryły o różnej wysokości przyjęte jako kierunek, ale na razie zostają obecne bryły domów; próba `flat_bold` (rozdział 11 przewodnika) poza grą.
- Powód: odpowiedź właściciela z 2026-11-28 („2 i 3 tak, ale póki co zostajemy przy starszych bryłach”).

### 2026-11-28: Proporcje B, mieszkanie i kamienica zamienione
- Decyzja: proporcje B w grze (dachy poziomów 1-5 w nachyleniu z rzutu, wyższe od poziomu 6; `BO_PROPORTION` domyślnie B). Poziom 5 to mieszkanie (id `tenement`, model dawnego poziomu 6), poziom 6 kamienica czynszowa (id `apartment`, dawny poziom 5, pełne kondygnacje); nazwy PL i EN zamienione w `data/housing.json`, mechanika bez zmian. Oliwa (potrzebna od poziomu 6) przeszła z mieszkania do kamienicy: pitos i amfora przy wschodniej ścianie. Pałac 1,6 raza, posąg boga 2,5 raza wysokości domu miejskiego (przy ich modelach).
- Wysokości (do komina): 2,7 / 3,7 / 4,0 / 4,2 / 6,0 / 6,8 / 7,3 m.
- Powód: odpowiedź właściciela z 2026-11-28 („Bierzemy B ... zamieniamy modelami 5 i 6; mieszkania wygląda jak naturalna rewolucja poprzedniego etapu - zmień wraz z nazwą i doskaluj do B”; punkt 2 z rekomendacją).

### 2026-11-28: Cienki kontur
- Decyzja: cienki kontur (1 px, ciepły ciemny brąz, ze skoków głębi powyżej 36 cm) domyślnie w grze; bez konturu w widokach specjalnych.
- Powód: odpowiedź właściciela z 2026-11-28 („Cienki kontur, z rekomendacją”).

### 2026-11-28: Wnioski z zrzutów Zeus Heaven
- Decyzja: dachy magazynów, spichlerzy i handlu w zielonej patynie miedzi (`roof_patina` (0,26; 0,50; 0,46), spichlerz i magazyn przemalowane); ściany domów elity i budynków publicznych z niebiesko-szarego kamienia (`stone_cool`); barwne posadzki placów, alei i sanktuariów z wzorami naszego projektu; gęstość ozdób według zamożności jako punkt R-018.
- Powód: odpowiedź właściciela z 2026-11-28 („Zgadzam się z rekomendacjami”) na pytania po analizie S-011 (`STYL-BUDOWLI.md` rozdział 8).

### 2026-11-28: Dachy i minimapa według funkcji, paleta 64, proporcje do porównania
- Decyzja: (2) magazyny, spichlerze i handel morski dostają kolor dachu osobny od warsztatów (kolor do wyboru); (3) gospodarstwa strzecha, handel terakota z markizą w kolorze towaru, wojsko szary kamień, ozdoby bez dachu; (4) minimapa miasta koloruje budynki tymi samymi grupami co dachy; (5) do porównania dwie wersje proporcji: obecna i niższa (dom przysadzisty do poziomu 5, w górę na 6-7), w niższej wyżej posągi bogów i budynki ważne. Paleta powiększona do 64 miejsc (`PALETTE_SIZE`), modele wyeksportowane ponownie.
- Pliki oryginalnej gry: nie pobieramy ich z sieci (kopia bez licencji); do analizy kolorów tylko zrzuty albo pliki z legalnej kopii właściciela, poza repo.
- Powód: odpowiedzi właściciela z 2026-11-28 („2. B - dzielimy 3. Tak 4. Tak 5. Ok tak”).

### 2026-11-28: Płótno szałasu ogrodników w zgaszonej zieleni
- Decyzja: szałas ogrodników z płótna `sage_cloth` (0,64; 0,70; 0,52) zamiast błękitu; błękit zostaje dla dachów budynków publicznych. Paleta ma 32 kolory, wszystkie miejsca zajęte.
- Powód: odpowiedź właściciela z 2026-11-28 („Zgadzam się z płótnem”).

### 2026-11-28: Referencja z eZeus, poluzowana R-017, kolor dachu według funkcji
- Decyzja: (1) referencją stylu są dane eZeus (rozmiary i liczby klatek sprite'ów, składanie budynków); wnioski w `docs/STYL-BUDOWLI.md` rozdział 7. (2) R-017 poluzowana: grafikę oryginału wolno oglądać i analizować, wnioski słowami i liczbami, bez kopiowania kształtów i bez plików oryginału w repo. (3) Kolor dachu mówi, czym jest budynek: domy terakota, budynki publiczne łupkowy błękit, świątynie złoto, warsztaty i magazyny brąz (`roof_slate`, `roof_gold`, `roof_brown` w palecie, `tiles` w `tiled_roof`); gospodarstwa, handel, wojsko i ozdoby do decyzji.
- Powód: odpowiedzi właściciela z 2026-11-28 („1. Z ezeus 2. Poluzować regułę, może oglądać 3. Tak”). Punkty 4 (kontur, próba na jednym domu) i 5 (proporcje, dwie propozycje) oceniamy osobno.

### 2026-11-28: Historie wszystkich kategorii budynków, wygląd Atlantydy
- Decyzja: (1) tabela historii z `docs/STYL-BUDOWLI.md` (rozdział 5) obowiązuje jako punkt wyjścia; każdą kategorię doprecyzowujemy przy jej pierwszym pełnym modelu i wpisujemy tam. (2) Kultura atlantydzka ma własny wygląd: orichalk, błękit morza, muszle i koralowce, przy tych samych zasadach R-018 (rozdział 6 przewodnika).
- Powód: polecenie właściciela z 2026-11-28 („Tak zrób tak”), pytania 25-26 w `11-OTWARTE-PYTANIA.md`.

### 2026-11-28: Nazwy poziomów 1-2, historie domów, modele w Git LFS
- Decyzja: (1) poziom 1 domu zwykłego nazywa się "szałas" (EN "Shelter") i pokazuje namiot, poziom 2 "chata" (EN "Cottage") i pokazuje chatę z gliny; id `hut` i `shack` bez zmian, mechanika bez zmian. (2) Trzy historie domów zwykłych: pasterze, garncarze, ogrodnicy i pszczelarze, według planu w `docs/ASSETY.md` ("Warianty i historie domów"); historie elity osobno. (3) `game/models/*.glb` w Git LFS (`.gitattributes`), CI pobiera je przy checkout (`lfs: true`); historia sprzed zmiany zostaje bez przepisywania.
- Powód: polecenie właściciela z 2026-11-28 („Robimy z rekomendacją: 1, 2 i 3”), pytania 22-24 w `11-OTWARTE-PYTANIA.md`.

### 2026-11-28: Styl domów z domieszką Ghibli
- Decyzja: miękkie światło z chłodnym cieniem w shaderze budynku, cieplejsza paleta 28 kolorów, zieleń, kwiaty i pranie jako rekwizyty w modelach (szczegóły w `docs/ASSETY.md`). Etap 1a bez tekstur pozostaje; tekstury CC0 (etap 1b) nałożą się na ten sam shader.
- Powód: polecenie właściciela z 2026-11-28 („Spróbujmy nadać trochę stylu Ghibli studio”). Styl dotyczy oprawy, mechanika bez zmian.

### 2026-11-28: Siódme demo, herosi
- Decyzja: Atalanta przeciw Sfinksowi i Bellerofont przeciw Echidnie (P:23-24, Z:130) w demo `scenarios/demo_heroes.json` (`tools/make_demo_heroes.py`, bramka `core/tests/test_demo_heroes.cpp`, siódmy przycisk demo): greckie miasto bez Apolla (zabiłby potwory pierwszy, Z:129), Artemida i Ares, działający stadion, trzy stadniny z farmami, urzędy podatkowe na każdej ulicy, trzy faktorie po grupach towarów, każdy magazyn tylko na swoje.
- Powód: polecenie właściciela z 2026-11-28. Demo greckie ma limit czterech sanktuariów wyczerpany, miasta Atlantydy nie udźwignęły sanktuarium Artemidy i 8 kompanii obok elity.
- Niesprawdzone: herosi przeciw Chimerze i Harpiom w demo (ci sami herosi, test wymagań i test wezwania).

### 2026-11-28: Szóste demo, elita Atlantydy
- Decyzja: włóczników, rydwanników, stoisko z rydwanami i majątek w Atlantydzie (P:15, P:18-19, Z:32-34) pokazuje demo `scenarios/demo_atlantis_elite.json` (`tools/make_demo_atlantis_elite.py`, bramka `core/tests/test_demo_atlantis_elite.cpp`, szósty przycisk demo). Nauka stoi po drugiej stronie ulicy elity, dwie agory z sześcioma stoiskami między działkami, osobna faktoria na pszenicę. Cel: 500 mieszkańców; majątek bramka widzi po drodze (rodziny odchodzą, gdy pokrycie nauką spada, Z:34).
- Powód: polecenie właściciela z 2026-11-28 („Robimy: włócznicy, rydwannicy i stoisko z rydwanami…”). Próba z dzielnicą elity w demo wojska Atlantydy odrzucona: dzielnica i stare miasto dzielą pulę pracowników i losowe ruchy, każda poprawka psuła fregatę albo wieżę.

### 2026-11-28: Piąte demo, bogowie Atlantydy
- Decyzja: Hera, Atlas i cytadela Posejdona (P:20-26) z gajami pomarańczy, sadem, dwoma rzemieślnikami Atlasa, jego modlitwą, ofiarami z bydła i Sfinksem pokazuje demo `scenarios/demo_atlantis_gods.json` (`tools/make_demo_atlantis_gods.py`, bramka `core/tests/test_demo_atlantis_gods.cpp`, w grze piąty przycisk demo), ułożone jak kolonia demo bogów.
- Powód: trzy sanktuaria potrzebują ok. 200 pracowników, więcej niż ma którekolwiek z dotychczasowych miast demo; zasada właściciela (demo pokazuje każdą mechanikę).
- Niesprawdzone: heros przeciw Sfinksowi (Atalanta wymaga stadionu lub muzeum i 8 kompanii; jest test jednostkowy).

### 2026-11-28: Czwarte demo, wojsko Atlantydy; dzielnica gospodarki dodatku w demo Atlantydy
- Decyzja: wojsko dodatku (łucznicy, fregata z Atlantean Fire, wieża z orichalkiem, desant z morza, Oceanidy; P:15-19) pokazuje osobne demo `scenarios/demo_atlantis_army.json` (`tools/make_demo_atlantis_army.py`, bramka `core/tests/test_demo_atlantis_army.cpp`, w grze czwarty przycisk demo). Gospodarka dodatku (zagroda z bydłem, sad pomarańczy; P:7-8) dołączyła do demo Atlantydy jako dzielnica południowa po wygranej; mapa urosła w dół, żeby stare miasto zachowało swoje losowe bajty kafli.
- Powód: zasada właściciela (demo pokazuje każdą mechanikę) i polecenie „lecimy z dodatkiem” z rekomendacją. Małe miasto demo Atlantydy po wygranej traciło siłę roboczą (z 277 do 142), a stocznia fregat potrzebuje 100 ludzi; własne miasto nad morzem z handlem utrzymuje ok. 800 mieszkańców.
- Odrzucone: wojsko jako kolejny etap demo Atlantydy (próba: miasto nie wyżywiło nowych miejsc pracy); poszerzenie mapy demo Atlantydy na wschód (zmieniło losowe bajty wszystkich kafli i przebieg starego miasta).
- Niesprawdzone: włócznicy i rydwannicy w demo (potrzebują domów elity w Atlantydzie; jest test jednostkowy), stoisko z rydwanami w demo.

### 2026-11-27: Trzecie demo, przygoda bogów
- Decyzja: ośmiu bogów, na których demo greckie nie ma miejsca (limit 4 sanktuariów na miasto, `data/gods_rules.json`), pokazuje przygoda `scenarios/demo_gods.json` z dwoma epizodami: miasto macierzyste (Zeus, Hades, Posejdon, Demeter; morze, triera, najazdy lądowy i morski, atak wrogiego boga) i kolonia (Artemida, Hefajstos, Afrodyta, Dionizos; wilki, Amazonki). Generator `tools/make_demo_gods.py`, autopilot dla każdego epizodu, bramka "demo gate: gods" (`core/tests/test_demo_gods.cpp`), wybór w menu gry (Grecja, Atlantyda, bogowie). Niepełną załogę triery (Z:141) pokazuje miasto macierzyste: samoczynna obrona wyprowadza motłoch z chat, motłoch zabiera robotników, załodze brakuje rąk.
- Powód: zasada właściciela (demo pokazuje każdą mechanikę podstawy) i jego zgoda z 2026-11-27 („zgadzam się, robimy te dwa punkty do decyzji”). Przygoda pokazuje przy okazji kolonię i przejście między epizodami (doc 23, L-119).
- Odrzucone: kolonia z Posejdonem i Hadesem (za mało rąk na cztery duże sanktuaria; podział odwrócony).
- Niesprawdzone: przejście do kolonii w samej grze (autopilot wybiera pierwszą kolonię, zapis i przeładowanie sceny); rdzeń sprawdza je w bramce.

### 2026-11-27: Drugie demo dla Atlantydy
- Decyzja: mechaniki atlantydzkie (nauka zamiast kultury, hipodrom, później piramidy i budynki nauki) pokazuje osobne małe demo `scenarios/demo_atlantis.json` z własnym autopilotem (`tools/make_demo_atlantis.py`) i bramką "demo gate: Atlantis" (`core/tests/test_demo_atlantis.cpp`).
- Powód: budynki atlantydzkie wolno stawiać tylko w mieście atlantydzkim (P:16-17), a zasada właściciela wymaga, żeby demo pokazywało każdą mechanikę. Demo greckie zostaje bez zmian.
- Odrzucone: drugi epizod przygody z kolonią atlantydzką (większa praca, przygody wieloepizodowe zostają na później).
- Niesprawdzone: wybór drugiego demo w menu gry (gra startuje demo greckie).

### 2026-11-26: Domyślny poziom trudności Hero, R-012 przyjęta
- Decyzja: 5 poziomów trudności 1:1, domyślny Hero (R-013). Hierarchia źródeł R-012 potwierdzona.
- Cytat właściciela: "Przyjmuję R-012, domyślnie Hero".
- Gdzie: `docs/10-REGULY.md`, `data/difficulty.json` (`default_level`).

### 2026-11-26: Mechanika 1:1 z grą referencyjną i dodatkiem
- Decyzja: domyślna specyfikacja rozgrywki to instrukcje gry podstawowej i dodatku, odwzorowane 1:1. Rozbudowa później, przez nowe reguły.
- Cytat właściciela: "Domyślnie mechanika ma być 1:1 w całej grze. Później będziemy rozbudowywać".
- Gdzie: `docs/gameplay/`, `data/`, reguła R-001 (obowiązuje).
- Konsekwencja: wcześniejszy katalog mechanik z `docs/04-MECHANIKI.md` (pisany przed lekturą instrukcji) zastąpiony odsyłaczem do specyfikacji. Moje wcześniejsze założenie, że dodatek wprowadza "przygotowanie terenu pod zabudowę", było błędne i zostało usunięte.

### 2026-11-26: Własna tożsamość przy mechanice 1:1
- Decyzja: mechanika 1:1, natomiast nazwy wymyślone przez twórców referencji, grafika, audio, teksty, mapy i kampanie są nasze (R-002).
- Powód: bezpieczeństwo prawne i własna marka. Nazwy w `data/` (`ref_name`) to identyfikatory robocze.
- Odrzucony wariant: rekonstrukcja z oryginalnymi nazwami i wyglądem.

### 2026-11-26: Instrukcje nie trafiają do repozytorium
- Decyzja: PDF-y instrukcji (S-001, S-002) zostają poza repo; dokumenty cytują strony.
- Powód: materiał chroniony prawem autorskim.

### 2026-11-26: Układ interfejsu budowy na telefonie (po pierwszym teście właściciela)

- Decyzja właściciela: przyciski przypięte do prawej krawędzi ekranu; budynki kategorii w wysuwanej kolumnie obok panelu; burzenie odznacza inne wybory; przyciski Zatwierdź, Obróć, Anuluj w stałym miejscu; bez pasków przewijania.
- Budowanie przeciągnięciem: domy i burzenie prostokątem, droga i mur linią w kształcie L; po puszczeniu palca domyślnie podgląd i Zatwierdź, w ustawieniach przełącznik "Buduj od razu".
- W trybach przeciągania jeden palec rysuje, mapę przesuwa się dwoma palcami (szczypnięcie zmienia przybliżenie).
- Wdrożenie: `game/scenes/ui/build_ui.gd`, test `game/tests/test_ui.gd`.

### 2026-11-26: Odpowiedzi po raporcie agenta weryfikującego

- Demo ma być zawsze rozbudowane tak, żeby dało się w nim przetestować wszystkie mechaniki i funkcje, jakie mamy, i ma dać się wygrać.
- Walka z potworem w mieście działa na morale jak bitwa (B-004).
- L-108: elita przychodzi ze wspólnej puli imigracji, tempo zależy od popularności, grupy do 8 osób (model eZeus).
- L-109: liczba rabble i koszt śmierci bez zmian (20 % mieszkańców, 6 osób).
- L-114: prośba spełniona po terminie daje −5 opinii.
- L-113, L-118: towar z prośby gracza przychodzi po 90 dniach; wyprawa herosa trwa 150 dni.
- L-117, L-107: wytrzymałości, zasięgi, czas triery i odradzanie wilków bez zmian.
- Pomnik kolonii dodany (B-005).
- Czy cel "sanktuarium" przepada po zniszczeniu sanktuarium: do sprawdzenia w instrukcji (przegląd referencji).

### 2026-11-26: Odpowiedzi po przeglądzie referencji
- Cytat właściciela: "Leć z rekomendacjami" (na pytania 1-5 raportu z przeglądu instrukcji).
- Cel "sanktuarium" przepada po zniszczeniu sanktuarium, nie po utracie łaski (B-011; instrukcja milczy, eZeus liczy cel z postępu budowy).
- Własne angielskie nazwy budynków i domów w polu `name_en`, z wyjątkiem słów zwykłych i historycznych (Granary, Agora, Palace); polska kolumna bez angielskich imion bogów (pytanie 17).
- K-023: zostaje wzór podatku S-004; atrakcyjność działa przez poziom domu, wpis O-007.
- L-053: modlitwa ma szansę wysłuchania pokazywaną w panelu (Z:132), liczba jako [ZAŁOŻENIE].
- Kolejność prac: najpierw kultura i limity budynków (B-008 do B-010), potem przebieg od pustej mapy do domów elity, potem demo, potem mechaniki P1 z przeglądu.

### 2026-11-26: Test APK na Xiaomi 15T Pro
- Cytat właściciela: "Sprawdziłem aplikację na telefonie, działa bardzo dobrze" (APK 0.0.29).
- Wniosek: renderer Mobile, sterowanie dotykiem i demo działają na telefonie odniesienia. Prototyp tłumu E4 (3000 postaci) zostaje w backlogu jako pomiar przed dużymi miastami, bez pilności.

### 2026-11-26: Zapas dóbr w domu (L-100)
- Cytat właściciela: "Tak, wdrażaj zgodnie z rekomendacją".
- Dom trzyma do 2 jednostek żywności na mieszkańca (8 miesięcy) i do 8 jednostek wełny, oliwy i wina (4 miesiące), jak w eZeus; Zeus Heaven podaje tylko zużycie. Powód: z zapasem na 2 miesiące domy w demo co miesiąc spadały o poziom i wypychały mieszkańców na ulicę.

## Proces

### 2026-11-27: Testy Godota na SceneTree, testy rdzenia po jednym w ctest
- Decyzja: testy Godota zostają skryptami na `SceneTree` (wspólne `check()` i `finish()` w `game/tests/check.gd`), bez gdUnit4; testy rdzenia rejestrowane w ctest po jednym przypadku przez `doctest_discover_tests` (`third_party/doctest/cmake/`, skrypt doctest 2.4.11 z poprawką na `;` w nazwie testu), CI uruchamia `ctest -j 4`.
- Powód: polecenie właściciela "Robimy z rekomendacją: 6 i 7" (przegląd niespójności testów). Obecne testy Godota działają w CI bez zależności; gdUnit4 wymagałby przepisania. Jeden wpis ctest na 347 przypadków ukrywał, który pada.
- Wdrożone: `game/tests/check.gd`, `test_simulation.gd`, `test_ui.gd`, `bench_check.gd`; `core/CMakeLists.txt`; `.github/workflows/ci.yml`; `PRAKTYKI.md`, `07-TECHNOLOGIA.md`, `SILNIK.md`.
- Sprawdzone: 347 wpisów ctest, każdy filtr trafia dokładnie jeden przypadek, zestaw `-j 4` 70 s (wcześniej 103 s jednym procesem).

### 2026-11-27: Sonda staje się kontrolą (R-016), plan odporności
- Decyzja: każdy problem znaleziony sondą zamienia się w kontrolę automatyczną; plan wdrożenia warstw (niezmienniki, ciche porażki, progi zdrowia, kontrole statyczne, test losowy, wiele ziaren, narzędzie sond, zapadki, determinizm między platformami) w `docs/PLAN-ODPORNOSCI.md`, etapy E1-E7.
- Powód: przebiegi demo z 2026-11-27 kosztowały kilkadziesiąt sond; inwentarz 30 przypadków w planie, rozdz. 3.
- Cytat właściciela: „wszystko, co kosztuje sondowanie powinno stawać się kandydatami do determinizmu”.
- Niesprawdzone: koszt czasu bramek po dodaniu niezmienników (szacunek 10-20 %).
- Uzupełnienie 2026-11-27: wdrożenie etapów odłożone („Zapisz plan i na razie lecimy po staremu”); R-016 stosowana doraźnie przy każdej nowej sondzie.

### 2026-11-26: Termin "spacerujący" zamiast "chodzik"
- Decyzja: postacie wykonujące zadania budynków (ang. walkers) nazywamy w dokumentacji "spacerujący" (losowy, celowy, dwufazowy). W kodzie zostaje angielskie `walker`.
- Cytat właściciela: "termin chodzicy zamień na spacerujący".
- Wdrożone: wszystkie dokumenty i komentarze; plik `docs/gameplay/02-chodziki.md` przemianowany na `02-spacerujacy.md`.

### 2026-11-26: Zeus Heaven jako źródło liczb (S-004, S-005)
- Decyzja: dane liczbowe i algorytmy uzupełniamy z portalu Zeus Heaven i wskazanych przez niego wątków forum; konflikty rozstrzyga hierarchia R-012.
- Cytat właściciela: "Tutaj masz kilka linków z wiedzą o grze, na ich podstawie uzupełnij wiedzę. Przeskanuj też cały portal w poszukiwaniu brakującej wiedzy."
- Zakres skanu: 200 stron treści portalu (bez forum w całości, galerii i katalogu plików do pobrania) oraz 9 wątków forum wskazanych przez portal jako najważniejsze. Archiwa newsów i opisy przejść kampanii przejrzane pod kątem mechanik.
- Wynik: zamknięte 31 luk, częściowo 13, otwarte 18; otwarte P0: L-001, L-008, L-012b, L-017.
- Nie sprawdzono: strony autora ZeusMapper (formaty plików), bo archiwum internetowe jest zablokowane w tym środowisku; całego forum (tysiące wątków).
- Dane w `data/building_stats.json` i `data/housing.json` są generowane skryptem z pobranych tabel; skrypty i surowe strony zostały w katalogu roboczym sesji, poza repo.

### 2026-11-26: Wytyczne z poprzednich projektów jako praktyki projektu
- Decyzja: `docs/PRAKTYKI.md` przenosi wytyczne właściciela (S-003) na citybuilder. `CLAUDE.md` streszcza zasady obowiązujące każdą sesję.
- Cytat właściciela: "Tutaj masz Guidelines, dostosuj praktyki pod ten projekt".

## Technologia

### 2026-11-26: Build na Androida
- Polecenie właściciela: "Zaczynaj moduły B i przygotuj build na Androida".
- Decyzje wykonawcze (koordynator): tylko arm64-v8a (telefon odniesienia i współczesne urządzenia); minimalne API 24 (domyślne szablonu Godota 4.7); biblioteka z wyrównaniem stron 16 KB (wymóg Google Play dla Androida 15+); eksport z gotowego szablonu APK, bez Gradle; identyfikator pakietu `pl.bogowieolimpu.game` (zatwierdzony przez właściciela: "Zgoda na identyfikator"); klucze wydania wyłącznie w zmiennych środowiskowych, nigdy w repo.
- Sprawdzone: kompilacja NDK 28.2, APK 29,8 MB podpisany i zweryfikowany przez apksigner, zawiera `libbo_gdext.android.so` i dane JSON.
- Nie sprawdzono: uruchomienie na urządzeniu (kontener nie ma emulatora ARM ani telefonu).

### 2026-11-26: Własna warstwa silnika, rdzeń w C++, natywny Vulkan, telefon odniesienia Xiaomi 15T Pro
- Decyzje właściciela: zakres rozbudowy silnika zgodnie z rekomendacją (pyt. 22: własna warstwa na Godocie, bez forka); rdzeń symulacji w C++17 jako samodzielna biblioteka + GDExtension, prezentacja w GDScript (pyt. 23); telefon odniesienia **Xiaomi 15T Pro**; praca od początku na natywnych bibliotekach, bez ograniczeń poziomu WebGL.
- Cytat właściciela: "1. zgoda 2. zgoda 3. co do telefonu to referencyjny to xiaomi 15t pro - chciałbym, żebyśmy od początku pracowali na natywnych bibliotekach, nie na ograniczeniach webgl przeglądarki".
- Wniosek (koordynator): renderer gry = **Mobile (Vulkan)**; renderer Compatibility (OpenGL ES 3, poziom WebGL 2) nie jest celem ani bazą testów (R-015). Forward+ (Vulkan) jako opcja dla PC.
- Sprawdzone w kontenerze: programowy Vulkan 1.4 (Mesa lavapipe) instaluje się z apt; Godot renderuje i robi zrzuty w Mobile i Forward+. Skrypt startowy środowiska zaktualizowany (sterownik Vulkan, Xvfb).
- Nie sprawdzono: parametrów Xiaomi 15T Pro (przyjęte z wiedzy ogólnej, do potwierdzenia na urządzeniu); wydajności na urządzeniu; eksportu na Androida.

### 2026-11-26: Silnik Godot 4, assety w Blenderze w kontenerze, najpierw rozbudowa silnika
- Decyzja: Godot 4 (4.7.2-stable). Assety tworzymy i przerabiamy w Blenderze uruchamianym w kontenerze. Przed treścią gry rozbudowujemy silnik pod założenia.
- Cytat właściciela: "Bierzemy Godot 4. assety będziemy robić/przerabiać w blenderze tutaj na kontenerze. Zanim zaczniemy grę, rozbudujemy silnik pod założenia".
- Interpretacja (do potwierdzenia): warstwa silnika gry na Godocie (rdzeń symulacji w C++ przez GDExtension, moduły mapy, ruchu, systemów miasta, prezentacji, narzędzi i potoku Blendera) bez forka Godota, dopóki pomiar nie wymusi zmian w samym Godocie. Plan: `docs/SILNIK.md`.
- Sprawdzone w kontenerze: Godot bez okna (import, skrypty), zrzuty przez Xvfb i llvmpipe w rendererze Compatibility, Blender 4.5.9 LTS bez okna, eksport glTF i import w Godocie.
- Nie sprawdzono: renderery Vulkan (Forward+, Mobile) nie działają w kontenerze; eksport na Androida (szablony eksportu, SDK) jeszcze nie instalowany; wydajność na telefonie.
- Odrzucone: TypeScript z Babylon.js / Three.js / PixiJS (wcześniejsza rekomendacja), Unity, Unreal, własny silnik.

### 2026-11-26: Rekomendacja stosu TypeScript [NIEAKTUALNE: zastąpiona decyzją o Godocie, 2026-11-26]

- Rekomendacja: TypeScript, domena symulacji jako czysty pakiet testowany w Node; renderer webowy (Babylon.js albo Three.js) wybrany pomiarem prototypu; Android przez Capacitor, iOS i PC w kolejnych etapach.
- Powód: wytyczne właściciela pochodzą z trzech gier w tym stosie (gotowe narzędzia, nawyki, potok testów i zrzutów bez GPU); sesje agentów w chmurze nie uruchamiają edytora Unity, więc testy, zrzuty i buildy w stosie webowym są w pełni dostępne dla agentów.
- Odrzucone: Unity 6 (wcześniejsza rekomendacja robocza z `07-TECHNOLOGIA.md`; brak edytora w kontenerze), Unreal (ciężki na telefonie), własny silnik natywny (koszt).
- Nie sprawdzono: wydajności 3 000 animowanych postaci w WebView na telefonie odniesienia. To jest kryterium prototypu pomiarowego.
- Status: czeka na decyzję właściciela (`11-OTWARTE-PYTANIA.md`, pytanie 11).
