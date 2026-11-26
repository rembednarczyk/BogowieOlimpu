# Backlog

Bez sekcji "Zrobione": zamknięte pozycje przenosimy do `docs/archiwum/BACKLOG-ARCHIWUM.md` z datą. Częściowo zrobione mają dopisek "Zostaje:".

## Następne

1. **Mechaniki P1 z przeglądu referencji** (kolejność według decyzji właściciela 2026-11-26): przegrana po latach długu (L-034), bunty wasali i kolonii, atak rywala, prośby o wojsko, modlitwy Zeusa, Aresa, Hefajstosa, Afrodyty i Hermesa, wojska bogów, klątwy na handel, brakujące pomniki, obsada wież przez wartowników, grubszy mur mocniejszy, gromadzenie zapasów, "Not Buying" w agorze. Każda nowa mechanika trafia do demo (`tools/make_demo.py`) i do listy mechanik w bramce `core/tests/test_demo.cpp`. Także: szansa modlitwy w panelu (L-053), własne nazwy EN budynków i domów (`name_en`) i poprawki kolumny PL (imiona bogów, pomniki atlantydzkie). Zrobione: kultura dwufazowa (B-008, B-010), limit pałacu i stadionu (B-009), cel sanktuarium (B-011), autopilot i bramka demo (B-003), elita (B-012, B-013).
2. **Test APK na Xiaomi 15T Pro** przez właściciela: start, renderer Mobile, przewijanie palcem, spacerujący w ruchu. Wynik decyduje o prototypie E4.
3. **Moduł E**: dalej interfejs gracza (wojsko, mapa świata, nakładki), potem modele z Blendera w miejsce brył. Zrobione: panel budowy, przeciąganie, karta budynku, prędkości, katalogi PL/EN, raport miasta z prośbami i celami, zapis i autozapis. Z modułów D zostają taktyki kompanii i walka morska. Wojsko etapy 1-2 zrobione (L-109, L-110), bogowie etapy 1-2 (budowa sanktuariów, ofiary, sanktyfikacja, modlitwy, L-111, L-112); świat etap 1 (opinie, dary, prośby, trybut, wyprawy, L-113); bogowie wrodzy, potwory i herosi po D5. Scenariusz etap 1 (format przygody, zdarzenia, prośby, cele, L-114); demo 80×60 budowane przez autopilota (`tools/make_demo.py`); gniew bogów i katastrofy (L-115); potwory i herosi (L-116); mury, wieże, brama, triery (L-117); zadania bogów i pomniki (L-118); przygody wieloepizodowe z koloniami (L-119). Zrobione: C1-C9, imigranci z punktu wejścia (O-006), stadnina i fabryka rydwanów (L-106), wilki (L-107), elita i szlachcice (L-108).
4. **C7 ryzyka, C9 handel, punkt wejścia na mapę (O-006)**. Zrobione C1, C2, C3, C6, C8 i etap 1 C4/C5; wysokość terenu bez półspadków.
5. **Skrypt instalacji narzędzi** w środowisku chmurowym (Godot, Blender, Vulkan lavapipe, Xvfb, Android SDK i NDK, szablony eksportu). Zostaje: potwierdzić w świeżej sesji.
6. **Uzupełnienia szkieletu**: deterministyczny zrzut (zamrożona symulacja zamiast czasu klatki), gdUnit4 dla testów GDScript, clang-tidy w CI, zadanie CI budujące APK.
7. **Prototyp tłumu E4** w Godocie: 3 000 postaci (MultiMesh + VAT) i 2 000 budynków; pomiar na Xiaomi 15T Pro w rendererze Mobile. Eksport APK działa.
8. **Konwencje potoku Blendera** z `docs/ASSETY.md` jako skrypty `tools/blender/` i rejestr pochodzenia assetów. Inwentarz gotowy.
9. **Konflikty źródeł** otwarte (K-003, K-005, K-009, K-020, K-022, K-024 do K-029) do rozstrzygnięcia testem w oryginalnej grze, jeśli jest dostępna.

## Później

- Ścieżka domów atlantydzkich (L-080) i reszta luk dodatku.
- Przeniesienie skryptów generujących `building_stats.json` i `housing.json` do repo (`tools/`), gdy powstanie struktura kodu; manifest z adresami źródeł.
- Edytor scenariuszy.
- Projekt UX dotykowego (makiety A/B/C do wyboru właściciela).
- Lista dopuszczonych licencji assetów.

## Ryzyka

W `docs/00-PLAN.md`.
