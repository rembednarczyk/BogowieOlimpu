# Budowanie i testy

Wymagania: CMake ≥ 3.22, Ninja, GCC 13 lub Clang, Python 3, Godot 4.7.2, Blender 4.5 LTS (potok assetów),
Mesa lavapipe + Xvfb (zrzuty w kontenerze). W chmurze instaluje je skrypt startowy środowiska (`docs/SILNIK.md`).

## Rdzeń symulacji (C++, bez Godota)

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

```bash
cmake --build build && ctest --test-dir build --output-on-failure
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
python3 tools/sync_game_data.py . && godot --headless --path game --import
```

```bash
godot --headless --path game --script res://tests/test_simulation.gd
```

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

## Złoty hash

`core/tests/test_golden.cpp`. Dwa scenariusze: sam kalendarz i finanse oraz miasto 32×32 (drogi, blokada, las, łąka, 4 budynki usługowe, spacerujący, 2 lata). Zmieniany wyłącznie ręcznie; commit, który go zmienia, nazywa regułę, która go zmieniła (`docs/PRAKTYKI.md`, rozdz. 3).
