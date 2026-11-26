# Dziennik decyzji

Pamięć projektu między sesjami. Nowe wpisy na górze sekcji. Nieaktualne oznaczamy dopiskiem **[NIEAKTUALNE: powód, data]**, nigdy nie usuwamy. Wpis: data, decyzja, powód, gdzie wdrożone, cytat właściciela (jeśli jest), odrzucone warianty, czego nie sprawdzono.

## Produkt i zakres

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

## Proces

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
