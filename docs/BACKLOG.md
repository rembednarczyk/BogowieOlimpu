# Backlog

Bez sekcji "Zrobione": zamknięte pozycje przenosimy do `docs/archiwum/BACKLOG-ARCHIWUM.md` z datą. Częściowo zrobione mają dopisek "Zostaje:".

## Następne

1. **Dalej pod demo (P2 z przeglądu referencji)**: igrzyska panhelleńskie (pomniki uczonego i sportowca, Z:110-112), taktyki kompanii i walka morska, wyrocznia, prośby gracza o pomoc wojskową sojuszników, typy celów eZeus spoza naszej listy (L-093b), interfejs wojska i mapy świata. Każda mechanika trafia do demo i do listy w bramce `core/tests/test_demo.cpp`. Zrobione z P1: kultura (B-008, B-010), limity budynków (B-009), cel sanktuarium (B-011), autopilot i bramka demo (B-003), elita (B-012, B-013), dług (L-034), relacje (L-121), modlitwy z szansą (L-053, L-112), klątwy handlowe, pomniki (L-092), kompanie bogów, prośby o wojsko (L-122), gromadzenie i "Nie kupuj", "Man All", nazwy EN, chata ogrodników (B-014), zdobycie pałacu przez porównanie sił (B-015), zapas dóbr w domu (L-100).
2. **Test APK na Xiaomi 15T Pro** przez właściciela: start, renderer Mobile, przewijanie palcem, spacerujący w ruchu. Wynik decyduje o prototypie E4.
3. **Moduł E**: dalej interfejs gracza (wojsko, mapa świata, nakładki), potem modele z Blendera w miejsce brył. Zrobione: panel budowy, przeciąganie, karta budynku, prędkości, katalogi PL/EN, raport miasta z prośbami i celami, zapis i autozapis. Z modułów D zostają taktyki kompanii i walka morska. Wojsko etapy 1-2 zrobione (L-109, L-110), bogowie etapy 1-2 (budowa sanktuariów, ofiary, sanktyfikacja, modlitwy, L-111, L-112); świat etap 1 (opinie, dary, prośby, trybut, wyprawy, L-113); bogowie wrodzy, potwory i herosi po D5. Scenariusz etap 1 (format przygody, zdarzenia, prośby, cele, L-114); demo 80×60 budowane przez autopilota (`tools/make_demo.py`); gniew bogów i katastrofy (L-115); potwory i herosi (L-116); mury, wieże, brama, triery (L-117); zadania bogów i pomniki (L-118); przygody wieloepizodowe z koloniami (L-119). Zrobione: C1-C9, imigranci z punktu wejścia (O-006), stadnina i fabryka rydwanów (L-106), wilki (L-107), elita i szlachcice (L-108).
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
