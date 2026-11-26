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

## Złoty hash

`core/tests/test_golden.cpp`. Zmieniany wyłącznie ręcznie; commit, który go zmienia, nazywa regułę, która go zmieniła (`docs/PRAKTYKI.md`, rozdz. 3).
