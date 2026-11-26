# Backlog

Bez sekcji "Zrobione": zamknięte pozycje przenosimy do `docs/archiwum/BACKLOG-ARCHIWUM.md` z datą. Częściowo zrobione mają dopisek "Zostaje:".

## Następne

1. **Test APK na Xiaomi 15T Pro** przez właściciela: start, renderer Mobile, przewijanie palcem, spacerujący w ruchu. Wynik decyduje o prototypie E4.
2. **Moduły D**: kolonie i kolejne epizody przygody, taktyki kompanii i walka morska; potem moduł E (prezentacja: interfejs gracza na telefon). Wojsko etapy 1-2 zrobione (L-109, L-110), bogowie etapy 1-2 (budowa sanktuariów, ofiary, sanktyfikacja, modlitwy, L-111, L-112); świat etap 1 (opinie, dary, prośby, trybut, wyprawy, L-113); bogowie wrodzy, potwory i herosi po D5. Scenariusz etap 1 (format przygody, zdarzenia, prośby, cele, L-114); demo 64×48 gra przygodę `scenarios/demo.json`; gniew bogów i katastrofy (L-115); potwory i herosi (L-116); mury, wieże, brama, triery (L-117); zadania bogów i pomniki (L-118). Zrobione: C1-C9, imigranci z punktu wejścia (O-006), stadnina i fabryka rydwanów (L-106), wilki (L-107), elita i szlachcice (L-108).
3. **C7 ryzyka, C9 handel, punkt wejścia na mapę (O-006)**. Zrobione C1, C2, C3, C6, C8 i etap 1 C4/C5; wysokość terenu bez półspadków.
4. **Skrypt instalacji narzędzi** w środowisku chmurowym (Godot, Blender, Vulkan lavapipe, Xvfb, Android SDK i NDK, szablony eksportu). Zostaje: potwierdzić w świeżej sesji.
5. **Uzupełnienia szkieletu**: deterministyczny zrzut (zamrożona symulacja zamiast czasu klatki), gdUnit4 dla testów GDScript, clang-tidy w CI, zadanie CI budujące APK.
6. **Prototyp tłumu E4** w Godocie: 3 000 postaci (MultiMesh + VAT) i 2 000 budynków; pomiar na Xiaomi 15T Pro w rendererze Mobile. Eksport APK działa.
7. **Konwencje potoku Blendera** z `docs/ASSETY.md` jako skrypty `tools/blender/` i rejestr pochodzenia assetów. Inwentarz gotowy.
8. **Konflikty źródeł** otwarte (K-003, K-005, K-009, K-020, K-022, K-024 do K-029) do rozstrzygnięcia testem w oryginalnej grze, jeśli jest dostępna.

## Później

- Ścieżka domów atlantydzkich (L-080) i reszta luk dodatku.
- Przeniesienie skryptów generujących `building_stats.json` i `housing.json` do repo (`tools/`), gdy powstanie struktura kodu; manifest z adresami źródeł.
- Edytor scenariuszy.
- Projekt UX dotykowego (makiety A/B/C do wyboru właściciela).
- Lista dopuszczonych licencji assetów.

## Ryzyka

W `docs/00-PLAN.md`.
