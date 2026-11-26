# Backlog

Bez sekcji "Zrobione": zamknięte pozycje przenosimy do `docs/archiwum/BACKLOG-ARCHIWUM.md` z datą. Częściowo zrobione mają dopisek "Zostaje:".

## Następne

1. **Moduły B (mapa i ruch)** z `docs/SILNIK.md`: siatka i teren, budowa, graf dróg, ścieżki, chodzik losowy, chodzicy celowi, indeks przestrzenny; rozszerzenie złotego hasha o scenariusz 32×32.
2. **Skrypt instalacji narzędzi** zapisany w środowisku chmurowym (Godot, Blender, Vulkan lavapipe, Xvfb). Zostaje: potwierdzić w świeżej sesji; dopisać scons, Android SDK i szablony eksportu, gdy będą potrzebne.
3. **Uzupełnienia szkieletu**: deterministyczny zrzut (zamrożona symulacja zamiast czasu klatki), gdUnit4 dla testów GDScript, clang-tidy w CI, budowa GDExtension na Androida (arm64). Zostaje z modułów A: nic.
4. **Prototyp tłumu E4** w Godocie: 3 000 postaci (MultiMesh + VAT) i 2 000 budynków; pomiar na Xiaomi 15T Pro w rendererze Mobile. Wymaga szablonów eksportu Android i sposobu dostarczania APK właścicielowi.
5. **Konwencje potoku Blendera** z `docs/ASSETY.md` jako skrypty `tools/blender/` i rejestr pochodzenia assetów. Inwentarz gotowy.
6. **Konflikty źródeł** otwarte (K-003, K-005, K-009, K-020, K-022, K-024 do K-029) do rozstrzygnięcia testem w oryginalnej grze, jeśli jest dostępna.

## Później

- Ścieżka domów atlantydzkich (L-080) i reszta luk dodatku.
- Przeniesienie skryptów generujących `building_stats.json` i `housing.json` do repo (`tools/`), gdy powstanie struktura kodu; manifest z adresami źródeł.
- Edytor scenariuszy.
- Projekt UX dotykowego (makiety A/B/C do wyboru właściciela).
- Lista dopuszczonych licencji assetów.

## Ryzyka

W `docs/00-PLAN.md`.
