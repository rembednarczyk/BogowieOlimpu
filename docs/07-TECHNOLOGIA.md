# Technologia

Stan: **silnik wybrany: Godot 4** (decyzja właściciela 2026-11-26). Plan rozbudowy silnika: `docs/SILNIK.md`.

## Stos

| Warstwa | Wybór | Status |
|---|---|---|
| Silnik | Godot 4.7.2-stable | decyzja |
| Rdzeń symulacji | C++17, samodzielna biblioteka, GDExtension | rekomendacja (pyt. 23) |
| Prezentacja, UI, narzędzia | GDScript | rekomendacja |
| Assety 3D | Blender 4.5 LTS w kontenerze, skrypty, eksport glTF | decyzja |
| Testy rdzenia | ctest + doctest (natywnie, bez Godota) | rekomendacja |
| Testy Godota | gdUnit4 bez okna (`--headless`) | rekomendacja |
| Zrzuty | Xvfb + Mesa llvmpipe, renderer Compatibility | sprawdzone |
| Lint | clang-format, clang-tidy (rdzeń), gdlint/gdformat (GDScript) | rekomendacja |
| Strażnik zależności | `core/` nie dołącza nagłówków Godota: sprawdzane skryptem w CI | rekomendacja |
| Mobile | eksport Android z Godota (szablony eksportu), potem iOS | decyzja |

## Renderer

Godot ma trzy renderery: Forward+ (Vulkan), Mobile (Vulkan), Compatibility (OpenGL). Na telefonach domyślny jest Mobile. W kontenerze działa tylko Compatibility (brak programowego Vulkana). Wybór renderera gry po pomiarze na telefonie odniesienia (pyt. 24).

## Odrzucone warianty

- TypeScript + Babylon.js / Three.js / PixiJS, Unity, Unreal, własny silnik: decyzja właściciela na rzecz Godota (`DECYZJE.md`).

## Budżety wydajności (robocze, do potwierdzenia po wyborze telefonu odniesienia)

| Element | Telefon odniesienia | Flagowiec |
|---|---|---|
| FPS | 30 stałe (ogranicznik klatek domyślnie) | 60 stałe |
| Wywołania rysowania | < 300 | < 600 |
| Postacie na ekranie | 800 | 3 000 |
| Tick symulacji (15 000 mieszkańców) | < 0,5 ms średnio na tick (51 ticków/dzień przy prędkości 1x) | |
| Pamięć | < 1,5 GB | < 3 GB |

Bramka: liczby deterministyczne (wywołania rysowania, instancje, czas ticku w natywnym benchmarku rdzenia). Czas klatki pod llvmpipe to raport.

## Środowisko

Godot i Blender instalowane w `/opt/tools`; skrypt instalacyjny do zapisania w ustawieniach środowiska chmurowego, żeby kolejne sesje miały narzędzia od startu.
