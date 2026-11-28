# Budowanie i testy

Wymagania: CMake ≥ 3.22, Ninja, GCC 13 lub Clang, Python 3, Godot 4.7.2, Blender 4.5 LTS (potok assetów),
Mesa lavapipe + Xvfb (zrzuty w kontenerze). W chmurze instaluje je skrypt startowy środowiska (`docs/SILNIK.md`).

## Rdzeń symulacji (C++, bez Godota)

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

```bash
cmake --build build && ctest --test-dir build -j 4 --output-on-failure
```

```bash
python3 tools/checks/check_core.py .
```

## Rozszerzenie Godota i projekt gry

```bash
git submodule update --init third_party/godot-cpp
```

```bash
cmake -S . -B build-gd -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBO_BUILD_GDEXT=ON -DBO_BUILD_TESTS=OFF && cmake --build build-gd
```

```bash
python3 tools/sync_game_data.py . && godot --headless --path game --import  # dane, przygody i katalogi tekstów (game/i18n)
```

```bash
godot --headless --path game --script res://tests/test_simulation.gd
godot --headless --path game --script res://tests/test_ui.gd
godot --headless --path game --script res://tests/test_layout.gd
```

Demo i bramka całej rozgrywki: `python3 tools/make_demo.py` generuje `scenarios/demo.json` i `scenarios/demo_build.json`; test "demo gate" w `core/tests/test_demo.cpp` musi wygrać demo przed 7. rokiem i pokazać każdą mechanikę z listy `kMechanics` (bieg do 10 lat; mapa 120×60, dzielnica wschodnia od dnia 960). Demo bogów (przygoda z kolonią, DECYZJE 2026-11-27): `python3 tools/make_demo_gods.py`, test "demo gate: gods" (`core/tests/test_demo_gods.cpp`, oba epizody i zwycięstwo, `BO_DEMO_LOG=1` raport co pół roku, `BO_DEMO_HOUSE`, `BO_DEMO_INV`, `BO_DEMO_SAVE_AT`). Demo Atlantydy: `python3 tools/make_demo_atlantis.py`, test "demo gate: Atlantis" (`core/tests/test_demo_atlantis.cpp`, wygrana przed 8. rokiem, potem gospodarka dodatku, do 10 lat, `BO_DEMO_LOG=1` raport co pół roku). Demo wojska Atlantydy: `python3 tools/make_demo_atlantis_army.py`, test "demo gate: Atlantean army" (`core/tests/test_demo_atlantis_army.cpp`, do 4 lat, `BO_DEMO_LOG=1`). Demo bogów Atlantydy: `python3 tools/make_demo_atlantis_gods.py`, test "demo gate: Atlantean gods" (`core/tests/test_demo_atlantis_gods.cpp`, do 8 lat). Demo elity Atlantydy: `python3 tools/make_demo_atlantis_elite.py`, test "demo gate: Atlantean elite" (`core/tests/test_demo_atlantis_elite.cpp`, do 10 lat). Demo herosów: `python3 tools/make_demo_heroes.py`, test "demo gate: heroes" (`core/tests/test_demo_heroes.cpp`, do 10 lat; `BO_DEMO_LOG=1` drukuje braki wymagań obu herosów). Zrzut warstw pomników: `screenshot.gd ... build_view=6 demo=atlantis_army`. Modele budynków (`game/models/*.glb` w Git LFS: po klonowaniu `git lfs install && git lfs pull`): `blender -b --factory-startup --python tools/blender/models.py` (wynik powtarzalny, `game/models/*.glb` i `provenance.json`), kontrola `python3 tools/checks/check_models.py .` (w CI) i dowód kontroli eksportu `blender -b --factory-startup --python tools/blender/test_checks.py`; przewodnik stylu: `docs/STYL-BUDOWLI.md`. Test układu (`test_layout.gd`): przyciski co najmniej 48 dp, na ekranie, bez nakładania i bez uciętych napisów w sześciu rozmiarach od 640x360 dp do 1920x1080. Zrzut w dp telefonu: `screenshot.gd ... --resolution 2772x1280 ... dpi=447` (Xiaomi 15T Pro). Zrzut widoku specjalnego: `screenshot.gd ... frames=400 speed=3000 view=water` (nazwy widoków w `boot.gd` `OVERLAY_VIEWS`). Raport roczny przy strojeniu:

```bash
BO_DEMO_LOG=1 ./build/core/bo_core_tests -tc="demo gate*"
```

Podglądy przy strojeniu (zmienne środowiska): `BO_DEMO_LOG=2` (zrzut budynków i domów elity co rok), `BO_DEMO_MAP=1` (mapa na końcu), `BO_DEMO_APPEAL=x0,y0,x1,y1` (atrakcyjność pól), `BO_DEMO_ELITE=1` (domy elity co miesiąc), `BO_DEMO_VENUES=1` (stadiony, gimnazja, zawodnicy), `BO_DEMO_MONTHLY=rok`, `BO_DEMO_TRACE=typ`, `BO_DEMO_YEARS=n`, `BO_DEMO_INV=1` (najazdy i kompanie co 64 dni), `BO_DEMO_REJECT=1` (odrzucone komendy z dniem), `BO_DEMO_HOUSE=x,y` (budynek z kafla co miesiąc: ryzyka, nastrój, obchody). Lista `kMechanics` obejmuje też łańcuchy produkcji `kChains` (każdy dobra w magazynie) i najwyższe poziomy domów zwykłych i elity (`core/tests/demo_probes.h`).

## Zrzut ekranu (renderer Mobile, Vulkan programowy)

```bash
xvfb-run -a -s "-screen 0 1280x720x24" godot --path game --rendering-driver vulkan --rendering-method mobile --resolution 1280x720 --script res://tests/screenshot.gd -- out=/tmp/shot.png frames=10
```

## Android (arm64-v8a, renderer Mobile na Vulkanie)

Wymagania (instaluje skrypt startowy środowiska): JDK 17, Android SDK w `/opt/tools/android-sdk` (platform-tools, build-tools 35.0.1, platforms android-35, NDK 28.2.13676358), szablony eksportu Godota 4.7.2 w `~/.local/share/godot/export_templates/4.7.2.stable`. Ścieżki SDK, JDK i klucza debug są w ustawieniach edytora Godota (`~/.config/godot/editor_settings-4.7.tres`, klucze `export/android/*`).

Biblioteka rdzenia i wiązania dla arm64 (wynik: `game/bin/libbo_gdext.android.so`, strony 16 KB):

```bash
cmake -S . -B build-android -G Ninja -DCMAKE_BUILD_TYPE=Release -DBO_BUILD_GDEXT=ON -DBO_BUILD_TESTS=OFF -DCMAKE_TOOLCHAIN_FILE=/opt/tools/android-sdk/ndk/28.2.13676358/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DGODOTCPP_TARGET=template_debug && cmake --build build-android
```

APK debug (preset `Android` w `game/export_presets.cfg`, podpis kluczem debug edytora):

```bash
python3 tools/sync_game_data.py . && godot --headless --path game --export-debug "Android" ../build-android/bogowie-olimpu.apk
```

Instalacja na telefonie z włączonym debugowaniem USB:

```bash
adb install -r build-android/bogowie-olimpu.apk
```

Klucze wydania nie trafiają do repo: eksport wydania czyta je ze zmiennych `GODOT_ANDROID_KEYSTORE_RELEASE_PATH`, `GODOT_ANDROID_KEYSTORE_RELEASE_USER`, `GODOT_ANDROID_KEYSTORE_RELEASE_PASSWORD`.

## Prototyp tłumu E4

Bramka liczb deterministycznych (wywołania rysowania i instancje; FPS pod llvmpipe to raport); `out=` zapisuje zrzut zbliżenia:

```bash
xvfb-run -a -s "-screen 0 1280x720x24" godot --path game --rendering-driver vulkan --rendering-method mobile --resolution 1280x720 --script res://tests/bench_check.gd
```

Na telefonie: Raport, przycisk „Test wydajności”; przyciski 800 / 3000 / 6000 postaci, Animacja, Zbliżenie.

## Złoty hash

`core/tests/test_golden.cpp`. Dwa scenariusze: sam kalendarz i finanse oraz miasto 32×32 (drogi, blokada, las, łąka, 4 budynki usługowe, agora ze stoiskiem, bulwar, cofnięta droga, spacerujący, 2 lata). Zmieniany wyłącznie ręcznie; commit, który go zmienia, nazywa regułę, która go zmieniła (`docs/PRAKTYKI.md`, rozdz. 3).
