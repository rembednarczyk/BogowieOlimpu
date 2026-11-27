# Backlog

Bez sekcji "Zrobione": zamknięte pozycje przenosimy do `docs/archiwum/BACKLOG-ARCHIWUM.md` z datą. Częściowo zrobione mają dopisek "Zostaje:".

## Następne

1. **Dalej pod demo (P2 z przeglądu referencji)**: igrzyska panhelleńskie (pomniki uczonego i sportowca, Z:110-112), walka morska etap 2 (załogi z puli pracowników, Patrol Area, Crew All, triery za granicą, L-124), wyrocznia, typy celów eZeus spoza naszej listy (L-093b), interfejs wojska i mapy świata. Każda mechanika trafia do demo i do listy w bramce `core/tests/test_demo.cpp`. Zrobione z P1: kultura (B-008, B-010), limity budynków (B-009), cel sanktuarium (B-011), autopilot i bramka demo (B-003), elita (B-012, B-013), dług (L-034), relacje (L-121), modlitwy z szansą (L-053, L-112), klątwy handlowe, pomniki (L-092), kompanie bogów, prośby o wojsko (L-122), gromadzenie i "Nie kupuj", "Man All", nazwy EN, chata ogrodników (B-014), zdobycie pałacu przez porównanie sił (B-015), zapas dóbr w domu (L-100), taktyki kompanii (L-110, B-016, B-017), flota etap 1 (triery, rozkazy, naprawa, desant z morza, L-124), pomoc wojskowa od miast (pomoc obronna i uderzenie, L-125).
2. **Moduł E**: dalej interfejs gracza (wojsko, mapa świata, nakładki), potem modele z Blendera w miejsce brył. Zrobione: panel budowy, przeciąganie, karta budynku, prędkości, katalogi PL/EN, raport miasta z prośbami i celami, zapis i autozapis. Z modułów D zostaje walka morska etap 2 (L-124). Wojsko etapy 1-2 zrobione (L-109, L-110), bogowie etapy 1-2 (budowa sanktuariów, ofiary, sanktyfikacja, modlitwy, L-111, L-112); świat etap 1 (opinie, dary, prośby, trybut, wyprawy, L-113); bogowie wrodzy, potwory i herosi po D5. Scenariusz etap 1 (format przygody, zdarzenia, prośby, cele, L-114); demo 80×60 budowane przez autopilota (`tools/make_demo.py`); gniew bogów i katastrofy (L-115); potwory i herosi (L-116); mury, wieże, brama, triery (L-117); zadania bogów i pomniki (L-118); przygody wieloepizodowe z koloniami (L-119). Zrobione: C1-C9, imigranci z punktu wejścia (O-006), stadnina i fabryka rydwanów (L-106), wilki (L-107), elita i szlachcice (L-108).
3. **Skrypt instalacji narzędzi** w środowisku chmurowym (Godot, Blender, Vulkan lavapipe, Xvfb, Android SDK i NDK, szablony eksportu). Zostaje: potwierdzić w świeżej sesji.
4. **Uzupełnienia szkieletu**: deterministyczny zrzut (zamrożona symulacja zamiast czasu klatki), gdUnit4 dla testów GDScript, clang-tidy w CI, zadanie CI budujące APK.
5. **Prototyp tłumu E4** w Godocie: 3 000 postaci (MultiMesh + VAT) i 2 000 budynków; pomiar na Xiaomi 15T Pro w rendererze Mobile. Eksport APK działa.
6. **Konwencje potoku Blendera** z `docs/ASSETY.md` jako skrypty `tools/blender/` i rejestr pochodzenia assetów. Inwentarz gotowy.
7. **Pomiary w oryginale**: liczby przyjęte jako [ZAŁOŻENIE] bez źródła liczbowego, do zmierzenia w grze referencyjnej (jeśli dostępna) albo w kolejnym źródle. Taktyki kompanii (L-110 (zasięg i wybór celu w walce), `data/military.json` `tactics_rules`, Z:138-139 opisuje je tylko słowami): obronna walczy do 2 kafli (miotający na zasięg broni) z pancerzem +2, ofensywna i specjalna ruszają na wroga do 6 kafli, motłoch w harcach odskakuje o 3 kafle od wroga obok, klin hoplitów daje 125 % ataku, jeździec po ciosie wraca pod sztandar. Organizacja igrzysk (L-123 (igrzyska panhelleńskie), `data/games.json`, Z:111 mówi o wzroście opinii i popularności bez liczb): +10 opinii każdego miasta i +10 zadowolenia każdego zamieszkanego domu. Do pomiaru: odległość, z której kompania rusza na wroga w każdej taktyce, obrażenia od tego samego wroga w szyku obronnym i ofensywnym, zmiana opinii miasta i nastroju po paradzie.
8. **Jeźdźcy w demo**: majątek (poziom elity 4) wymaga kultury 80, czyli zawodników ze stadionu (20 pkt), oraz atrakcyjności 100 (`data/housing.json`, poziom M). Stadion demo (2,12) leży ok. 60 kafli od domów elity, zawodnicy chodzą do 35 kafli (`data/walkers.json`), więc jeźdźcy w obecnym układzie mapy nie powstaną. Rekomendacja: przy następnej przebudowie mapy demo dzielnica elity obok stadionu, z labiryntami żywopłotu i ogrodami. Zachowanie jeźdźców sprawdza test "special tactics: horsemen harass" w `core/tests/test_combat.cpp`.
9. **Konflikty źródeł** otwarte (K-003, K-005, K-009, K-020, K-022, K-024 do K-029) do rozstrzygnięcia testem w oryginalnej grze, jeśli jest dostępna.

## Później

- Ścieżka domów atlantydzkich (L-080) i reszta luk dodatku.
- Przeniesienie skryptów generujących `building_stats.json` i `housing.json` do repo (`tools/`), gdy powstanie struktura kodu; manifest z adresami źródeł.
- Edytor scenariuszy.
- Projekt UX dotykowego (makiety A/B/C do wyboru właściciela).
- Lista dopuszczonych licencji assetów.

## Ryzyka

W `docs/00-PLAN.md`.
