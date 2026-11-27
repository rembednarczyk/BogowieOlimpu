# Technologia

Stan: **silnik wybrany: Godot 4** (decyzja właściciela 2026-11-26). Plan rozbudowy silnika: `docs/SILNIK.md`.

## Stos

| Warstwa | Wybór | Status |
|---|---|---|
| Silnik | Godot 4.7.2-stable | decyzja |
| Rdzeń symulacji | C++17, samodzielna biblioteka, GDExtension | decyzja |
| Prezentacja, UI, narzędzia | GDScript | decyzja |
| Assety 3D | Blender 4.5 LTS w kontenerze, skrypty, eksport glTF | decyzja |
| Testy rdzenia | ctest + doctest (natywnie, bez Godota) | rekomendacja |
| Testy Godota | skrypt na `SceneTree` bez okna (`--headless`), wspólne `check()` i `finish()` w `game/tests/check.gd` | decyzja 2026-11-27 (zamiast gdUnit4) |
| Skala interfejsu | `canvas_items`, obszar interfejsu = piksele okna / (dpi/160 albo skala systemowa) × suwak; `game/scenes/ui/ui_scale.gd` | decyzja 2026-11-27 |
| Testy rdzenia w ctest | `doctest_discover_tests` (`third_party/doctest/cmake/`): jeden wpis ctest na przypadek, `ctest -j 4` | decyzja 2026-11-27 |
| Renderer | Mobile (Vulkan); Forward+ opcjonalnie na PC | decyzja (R-015) |
| Zrzuty | Xvfb + Mesa lavapipe (Vulkan 1.4), renderer Mobile | sprawdzone |
| Lint | clang-format, clang-tidy (rdzeń), gdlint/gdformat (GDScript) | rekomendacja |
| Strażnik zależności | `core/` nie dołącza nagłówków Godota: sprawdzane skryptem w CI | rekomendacja |
| Mobile | eksport Android z Godota (szablony eksportu), potem iOS | decyzja |

## Renderer

Renderer gry: **Mobile (Vulkan)** (R-015). Compatibility (OpenGL ES 3, poziom WebGL 2) nie jest celem. W kontenerze Vulkan działa programowo (Mesa lavapipe): zrzuty i testy wizualne w rendererze Mobile, a czas klatki pod lavapipe to tylko raport.

## Telefon odniesienia

**Xiaomi 15T Pro** (decyzja właściciela). Parametry przyjęte z wiedzy ogólnej, do potwierdzenia na urządzeniu: MediaTek Dimensity 9400+, GPU Mali (Immortalis), ekran ok. 6,8" 2772×1280, 120 Hz (potwierdzone pomiarem właściciela 2026-11-27), 12 GB RAM, Vulkan 1.3+. To telefon z wyższej półki: budżety ustalamy na nim, a dla słabszych urządzeń przewidujemy suwak rozdzielczości renderu i LOD.

## Odrzucone warianty

- TypeScript + Babylon.js / Three.js / PixiJS, Unity, Unreal, własny silnik: decyzja właściciela na rzecz Godota (`DECYZJE.md`).

## Budżety wydajności (robocze, do potwierdzenia pomiarem na Xiaomi 15T Pro)

| Element | Telefon odniesienia (Xiaomi 15T Pro) | Słabszy telefon (do ustalenia) |
|---|---|---|
| FPS | 60 stałe (ogranicznik klatek domyślnie; 120 Hz ekranu (pomiar właściciela 2026-11-27) nie wykorzystujemy w pełni, by nie grzać) | 30 stałe |
| Wywołania rysowania | < 600 | < 300 |
| Postacie na ekranie | 3 000 | 800 |
| Tick symulacji (15 000 mieszkańców) | < 0,5 ms średnio na tick (51 ticków/dzień przy prędkości 1x) | |
| Pamięć | < 3 GB | < 1,5 GB |

Bramka: liczby deterministyczne (wywołania rysowania, instancje, czas ticku w natywnym benchmarku rdzenia). Czas klatki pod llvmpipe to raport.

Pomiar E4 na Xiaomi 15T Pro (właściciel, 2026-11-27, APK 0.0.43, scena `bench_crowd`): 6 000 postaci z animacją VAT i 2 000 budynków, 120 klatek/s, czas klatki średnio 8,3 ms i najgorszy 8,3 ms w oknie 120 klatek, 19 wywołań rysowania, 53 MB pamięci. 120 klatek/s to odświeżanie ekranu 120 Hz (synchronizacja pionowa), więc zapasu nie widać: dwa razy budżet postaci bez jednego spadku klatki. Bez cieni i bez LOD; czas GPU i CPU osobno nie zmierzony.

## Środowisko

Godot i Blender instalowane w `/opt/tools`; skrypt instalacyjny do zapisania w ustawieniach środowiska chmurowego, żeby kolejne sesje miały narzędzia od startu.
