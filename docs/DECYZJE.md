# Dziennik decyzji

Pamięć projektu między sesjami. Nowe wpisy na górze sekcji. Nieaktualne oznaczamy dopiskiem **[NIEAKTUALNE: powód, data]**, nigdy nie usuwamy. Wpis: data, decyzja, powód, gdzie wdrożone, cytat właściciela (jeśli jest), odrzucone warianty, czego nie sprawdzono.

## Produkt i zakres

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
