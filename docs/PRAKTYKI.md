# Praktyki projektu Bogowie Olimpu

Zasady pracy dostosowane do tego projektu na podstawie wytycznych z trzech wcześniejszych gier właściciela (źródło S-003). Oryginał opisuje gry zręcznościowe i otwarty świat; tu przekładamy go na deterministyczny citybuilder z tysiącami chodzików, specyfikacją 1:1 z instrukcji i grą na telefon.

- **Zasada**: obowiązuje zawsze.
- **Wytyczna**: domyślny wybór; odstępstwo wymaga zapisanego powodu (wpis w `docs/DECYZJE.md`).
- Liczby (limity, budżety) są punktem startu. Zmieniamy je decyzją z pomiarem, mechanizm zostaje.

**Narzędzia (Godot 4, decyzja 2026-11-26).** Tam, gdzie poniżej pada przykład narzędzia webowego, obowiązuje odpowiednik: testy logiki w Node → testy natywne rdzenia C++ (ctest, doctest); testy przeglądarkowe → Godot `--headless` z gdUnit4; Playwright + SwiftShader → Xvfb + llvmpipe, renderer Compatibility; dependency-cruiser → skrypt sprawdzający, że `core/` nie dołącza nagłówków Godota; linter TS → clang-tidy i gdlint; Capacitor → eksport Android z Godota.

## 1. Fundament

**Zasada. Każde rozwiązanie stoi na nazwanym wzorcu albo źródle.** Brief, komentarz modułu i ADR podają wzorzec, źródło i różnice z powodem. Jeśli wzorca brak, piszą to wprost.

Wzorce, które w tym projekcie są domyślne:

| Problem | Wzorzec / źródło |
|---|---|
| Stan świata | ECS albo struktury danych zorientowane na dane (Nystrom, "Game Programming Patterns": Data Locality, Component) |
| Budynki, towary, bogowie | Type Object (Nystrom); dane w `data/*.json` |
| Wejście do symulacji | Command (GoF); log komend + ziarno = powtórka |
| Wyjście z symulacji | Observer / Event Queue (Nystrom) |
| Pętla | Fix Your Timestep (Fiedler); stały tick, render interpoluje |
| Determinizm | deterministyczny lockstep RTS (Terrano, Bettner, "1500 Archers on a 28.8", GDC 2001) |
| Ścieżki doręczycieli | A* z rozstrzyganiem remisów numerem kafla; mapy odległości (Dijkstra maps) do magazynów |
| Zasięg usług | indeks przestrzenny (spatial hash) po kaflach |
| Chodzicy losowi | model z gatunku; open-source'owe reimplementacje gier z tej samej rodziny (Julius i Augustus dla Caesar III, Akhenaten dla Pharaoh) jako źródło wiedzy. **Kod na licencji GPL: czytamy, nie kopiujemy**, dopóki nie zapadnie decyzja licencyjna |

**Zasada. Specyfikacja z instrukcji jest źródłem reguł.** `docs/gameplay/` i `data/` są nadrzędne wobec intuicji. Każde odstępstwo trafia do rejestru `docs/ODSTEPSTWA.md` z identyfikatorem, powodem i statusem.

**Wytyczna. Okrojenie wzorca zapisuj jako wybór** (np. A* bez wygładzania ścieżek, mapy odległości tylko do Granary i Storehouse). Następna sesja wie wtedy, co jest długiem.

**Wytyczna. Jeden algorytm, jedna implementacja.** Jeden RNG, jeden indeks przestrzenny, jeden pathfinder, jedna funkcja "czy budynek ma dostęp do drogi".

## 2. Architektura

**Zasada. Symulacja działa bez renderera.** Domena (reguły, ekonomia, chodzicy, bogowie, świat, zapis) to czysty kod uruchamiany w testach bez przeglądarki i GPU. Render, audio, wejście i zapis do pamięci urządzenia są za portami.

**Zasada. Kierunek zależności pilnuje narzędzie w CI** (np. dependency-cruiser): domena nie importuje niczego z zewnątrz; symulacja tylko domenę; UI nie zna silnika ani wnętrza symulacji; brak cykli.

**Zasada. Do symulacji wchodzi się tylko komendami.** Gracz (dotyk, mysz), autopilot testowy, skrypt scenariusza i przyszłe AI przywódców wydają te same komendy: `PlaceBuilding`, `Demolish`, `SetStorageOrder`, `SetTaxRate`, `MusterCompany`, `Pray`, `SendGift`... Rozdzielnik komend to tabela handlerów z typem wymuszającym komplet.

**Zasada. Symulacja emituje zdarzenia, prezentacja ich słucha.** Ewolucja domu, pożar, przybycie boga, spełnienie celu: render, audio, HUD i haptyka reagują na zdarzenia. Narzędzie zrzutów może wstrzyknąć zdarzenie i pokazać sam efekt.

**Zasada. Dane w tabelach, zachowanie w systemach.** Żadnych warunków `if (god === 'demeter')` w logice. Bóg ma w danych listę sanktyfikowanych budynków i efekt modlitwy; system religii jest jeden.

**Zasada. Jedno źródło prawdy.** Siatka kafli, graf dróg, zasięgi usług i atrakcyjność liczy symulacja; render, nakładki ("See Water", "See Appeal") i mapa przeglądowa tylko je czytają. Nakładka liczona osobno w UI to błąd.

**Wytyczna. Widok może być bogatszy od symulacji, jeśli tylko czyta stan.** Animacje pracy, ruch tłumu między kaflami, pogoda, cykl dnia to prezentacja. Złoty hash ich nie widzi.

**Wytyczna. Opcje "gry" mają flagi wyłączające je w testach**: katastrofy, wizyty bogów, inwazje, pogoda, losowe zdarzenia scenariusza.

**Zasada. Limity rozmiaru z bramką w linterze**: plik do 500 linii, funkcja do 120, klasa do 20 metod (test z bazą, która tylko spada).

**Wytyczna. Nowy system dostaje własny moduł, prezenter i wycinek stanu.** Nie dopisujemy pól do wspólnej "sesji" ani metod do fasady renderera.

**Wytyczna. Bez ukrytych singletonów.** Ustawienia, dane scenariusza i kontekst są wstrzykiwane.

## 3. Determinizm i symulacja

**Zasada. Stały tick, render interpoluje.** Prędkość gry zmienia liczbę ticków na sekundę, nigdy długość ticku.

**Zasada. Losowość i czas w domenie tylko przez porty, zakaz pilnuje linter** (bez `Math.random`, `Date.now`, `performance.now`, bez `any`). Osobne strumienie RNG dla niezależnych warstw: migracja, zagrożenia budowlane, zaraza, przestępczość, bogowie, świat, potwory, generacja mapy. Nowy system dostaje nowy strumień, żeby nie przetasować reszty.

**Zasada. Iteracja w kolejności identyfikatorów** tam, gdzie wynik zależy od kolejności: kolejność systemów w jednej jawnej tablicy, budynki i chodzicy po id, remisy A* po numerze kafla, kolejka przydziału pracowników po priorytecie sektora i id.

**Wytyczna. Nowe byty dopisujemy na koniec, z osobną pulą identyfikatorów** (dotyczy też `data/*.json`).

**Zasada. Złoty hash na małym, stałym scenariuszu, aktualizowany tylko ręcznie.** Scenariusz testowy (np. mapa 32×32 kafle) zawiera po jednym egzemplarzu każdego łańcucha: farmę, spichlerz, agorę, dom, fontannę, warsztat, sanktuarium w budowie, jednego chodzika każdego typu. Commit zmieniający hash nazywa regułę, która go zmieniła. Hash nie jest liczony na mapach kampanii.

**Wytyczna. Stały hash nie dowodzi braku zmiany zachowania.** Zmiany chodzików porównujemy też audytem pokrycia usług na mapach referencyjnych.

**Wytyczna. Stan liczalny z ziarna nie trafia do zapisu** (np. oferta handlowa miasta w danym roku, jeśli wynika z ziarna i roku).

**Zasada. Zmiana schematu zapisu to wersja, migracja i test.** Zapis zawiera wersję danych z `data/`; zmiana identyfikatora budynku wymaga migracji.

**Wytyczna. Narzędzia diagnostyczne zapisują stan tą samą drogą co gra.**

**Wytyczna. LOD symulacji tylko tam, gdzie nie zmienia wyniku.** Chodzik poza ekranem nie potrzebuje animacji, ale jego ruch po kaflach liczy się identycznie. Przyspieszenia symulacji (np. zgrubny krok dla chodzików poza ekranem) wchodzą tylko z dowodem identycznego hasha.

## 4. Testy

**Zasada. Logikę sprawdzają testy bez przeglądarki**, krokujące symulację (`step(n)`) i czekające na stan gry, nie na zegar. Przeglądarka tylko do rozruchu, rysowania, HUD na telefonie i dotyku.

**Zasada. Test całej rozgrywki jako bramka zmian ekonomii, chodzików i scenariuszy.** Autopilot wykonuje skrypt budowy (build order) na scenariuszu i musi osiągnąć cele (np. 1500 mieszkańców i domy na poziomie Tenement przed rokiem X, sanktuarium ukończone). Scenariusz, którego autopilot nie przejdzie, nie przechodzi testu.

**Wytyczna. Test niezmienników z "chaotycznym burmistrzem".** Losowo stawia, burzy, zmienia rozkazy magazynów, podatki i płace. Sprawdzamy: brak NaN, brak ujemnych zapasów, populacja ≤ pojemność, każdy doręczyciel ma cel albo status "utknął", determinizm.

**Zasada. Dowód poprawki: test pada bez niej i przechodzi z nią.**

**Zasada. Kontrola liczy niezależnie od poprawki** (nie karmi się tymi samymi liczbami).

**Zasada. Test reguły projektowej obejmuje całą treść.** Każda mapa scenariusza: droga do Grecji osiągalna z punktu wejścia, złoża wymagane przez cele istnieją, łańcuchy wymagane przez cele są wykonalne z dostępnych budynków i handlu.

**Zasada. Test zgodności ze specyfikacją.** Każda reguła z `docs/gameplay/` oznaczona jako zaimplementowana ma test scenariuszowy, który ją odtwarza (np. "Townhouse bez żywności spada do Hut", "Storehouse domyślnie nie przyjmuje pszenicy", "kupiec patrzy tylko na zapas w swoim budynku"). Test cytuje stronę instrukcji.

**Zasada. Każdy zgłoszony błąd dostaje test regresji.**

**Zasada. Refaktor bez zmiany zachowania dowodzi się odciskiem** (pełny przebieg scenariusza: hash stanu co N ticków).

**Zasada. Podniesienie limitu czasu albo budżetu nie jest poprawką.** Najpierw przyczyna; wyjątek z uzasadnieniem i wpisem do długu z warunkiem zdjęcia.

**Wytyczna. Wolny test dostaje mniejszy scenariusz**, a nie wyższy limit.

**Wytyczna. Pułapki**: testy z konfiguracją główną; kontrole przed commitem łańcuchem `&&`; czas `beforeAll` mierzony osobno.

## 5. Audyty i pomiary rozgrywki

**Zasada. Audyt z raportem liczbowym jest osobnym narzędziem obok testów.** Audyty citybuildera:

| Audyt | Co mierzy |
|---|---|
| Pokrycie usług | % domów odwiedzonych przez każdy typ chodzika w ostatnim miesiącu, per dzielnica |
| Logistyka | doręczyciele utknięci, średni czas dostawy, puste przebiegi vendorów, statusy Agory ("Too Distant", "No Supply") |
| Ekonomia | bilans roczny, żywność na mieszkańca, rozkład poziomów domów, bezrobocie |
| Tempo scenariusza | czas osiągnięcia każdego celu przez autopilota, na kilku ziarnach |
| Religia i mit | czas budowy sanktuariów, częstość ataków bogów i potworów |
| Soak | 100 lat gry na prędkości maksymalnej: brak NaN, brak zablokowanej gry, najgorszy tick w budżecie |

**Zasada. Zawsze przed i po, w tym samym wywołaniu i składzie.** Przy wątpliwości drugi scenariusz.

**Wytyczna. Audyt balansu jako bramka.** Przykład: autopilot na każdym scenariuszu kampanii kończy w limicie lat na 3 z 3 ziaren; populacja nie spada o więcej niż 20% bez zdarzenia skryptowego.

**Zasada. Próg audytu zmienia się według jego intencji, nie według wyniku** (zmiana z wpisem w ADR).

**Wytyczna. Beta-testerzy jako agenci z profilami**: nowicjusz na telefonie po angielsku, weteran gatunku na tablecie po polsku, gracz "szybki" na prędkości maksymalnej. Zgłoszenia przechodzą przez proces błędów.

## 6. Zapadki i skany

**Zasada. Każdy policzalny rodzaj błędu albo długu ma kontrolę z bazą, która tylko spada.** Baza zapisuje **miejsca** (rodzaj, scenariusz, kafel zaokrąglony do bloku 8×8), nie samą liczbę. Narzędzie `diff` pokazuje przybyłe i ubyłe.

Kandydaci w tym projekcie: budynki bez drogi w scenariuszach, doręczyciele bez celu w scenariuszach referencyjnych, kafle nieosiągalne z drogi do Grecji, literały tekstu poza katalogami języków, klasy powyżej limitu metod, otwarte luki `L-NNN` oznaczone P0 w zaimplementowanych systemach, odstępstwa od referencji bez statusu.

**Wytyczna. Zapadka powstaje w PR, który naprawia pierwszy egzemplarz.** Uwaga recenzenta powtórzona drugi raz staje się kontrolą.

**Antywzorzec: próg jako sam raport.**

## 7. Wydajność

**Zasada. Budżety wynikają z urządzenia odniesienia.** Wybór telefonu odniesienia to decyzja w `docs/DECYZJE.md`; budżety w `docs/07-TECHNOLOGIA.md`.

**Zasada. Bramką są liczby deterministyczne**: wywołania rysowania, aktywne siatki, instancje, czas ticku symulacji mierzony w Node (najlepszy z N prób, w izolacji). Czas klatki pod rendererem programowym to raport.

**Zasada. Najpierw profil, potem optymalizacja.** W citybuilderze spodziewane koszty: liczba animowanych postaci (instancing, animacja w teksturach), wypełnianie pikseli na telefonie (suwak rozdzielczości renderu), przeliczanie zasięgów i atrakcyjności (rozłożone na ticki, przyrostowe).

**Wytyczna. Typowe zyski, każdy osobno z pomiarem**: instancing budynków i postaci, LOD i culling, gaszenie obiektów < 4 px, amortyzacja przeliczeń symulacji (atrakcyjność co N ticków w grupach), ogranicznik klatek na telefonie (domyślnie), automatyczna rozdzielczość z histerezą, bez alokacji w pętli klatki i ticku.

**Zasada. Każda opcja jakości ma zrzut A/B.**

**Wytyczna. Zmiana chodzików mierzona w najgorszym przypadku**: największe miasto referencyjne, wszyscy chodzicy aktywni, pożar i inwazja naraz.

## 8. Weryfikacja wizualna

**Zasada. Strona testowa stawia scenę w dowolnym stanie** przez parametry adresu: scenariusz, zapis, kafel i zoom kamery, obrót, nakładka, pora dnia, zamrożenie, "przewiń N miesięcy". Zrzuty robi Playwright z rendererem programowym.

**Zasada. Zrzuty są powtarzalne**: zamrożona symulacja, przypięta kamera, ziarno, bez losowości w widoku.

**Zasada. Przed i po z tych samych parametrów, autor opisuje, co widzi.** Zrzuty poza repo.

**Wytyczna. Efekt zdarzenia oglądaj przez wstrzyknięcie zdarzenia** (przybycie boga, sanktyfikacja, pożar, ewolucja domu).

**Wytyczna. Czytelność z dużej odległości mierzona**: każdy typ budynku rozróżnialny przy maksymalnym oddaleniu na ekranie 6". Plansza wszystkich budynków generowana skryptem.

**Wytyczna. Diagnoza przez eksperyment wykluczający** i pomiar wartości pikseli zamiast oceny "na oko".

## 9. Scenariusze, mapy i odstępstwa

**Zasada. Karta scenariusza powstaje przed budową mapy**: mit lub historia, cele, dostępne budynki i bogowie, złoża, partnerzy na mapie świata, oczekiwany przebieg (build order autopilota), zdarzenia skryptowe. Test pada, gdy karty brak albo nie zgadza się z mapą.

**Zasada. Rejestr odstępstw od referencji** (`docs/ODSTEPSTWA.md`). Każda różnica wobec `docs/gameplay/` ma ID, miejsce, stan w grze, stan w referencji, status: celowe z powodem albo do poprawy. "Tak wyszło" nie jest powodem.

**Wytyczna. Kultura (grecka, atlantydzka) jako matryca reguł** w danych: dostępne budynki, ścieżka domów, jednostki, styl architektury, z opisanymi wyjątkami.

**Zasada. Stylizacja siedzi w rysowaniu, kształt jest realistyczny.** Budynki o proporcjach architektury greckiej; uproszczenie tylko jako LOD.

## 10. Projekt rozgrywki

**Zasada. Każdy stan porażki ma wyjście albo jasny komunikat.** Referencja przewiduje porażkę epizodu (dług przez lata, dwukrotna klęska z rywalem, upadek miasta macierzystego); gracz dostaje ostrzeżenia z wyprzedzeniem i możliwość powtórzenia epizodu. Zablokowana gra (np. brak drogi do Grecji, brak ruchu) wykrywana przez soak.

**Wytyczna. Informuj o regule, zanim ukarze.** Referencja ma pasek ostrzeżeń, ikony problemów, raport miasta: zachowujemy wszystkie i rozszerzamy o przyczynę w karcie budynku.

**Wytyczna. Pętle nagrody sprawdzaj pod kątem nadużyć** (np. dary dla miast z malejącą wartością, import bez sprawdzenia zapasów w mieście, wielokrotne pomniki).

**Wytyczna. Research gatunku: bierz regułę, zapisz, czego nie bierzesz** (do `docs/ODSTEPSTWA.md` albo `docs/DECYZJE.md`).

**Wytyczna. AI z klasycznych technik**: AI przywódców na mapie świata jako maszyna stanów z progami opinii z `data/diplomacy.json`; walka jednostek ze steeringiem (Reynolds) i formacjami; trudność jako parametry.

**Wytyczna. Ruch dla oka zgodny z animacją**: prędkość chodzika z tempa klipu.

## 11. Assety i licencje

**Zasada. Żadnego assetu z gry referencyjnej** (grafika, dźwięk, teksty, mapy). Zrzuty i instrukcje służą wyłącznie analizie i nie trafiają do paczki gry ani do publicznego repo.

**Zasada. Lista dopuszczonych licencji to decyzja w repo**, wyjątki wąsko opisane.

**Zasada. Rejestr pochodzenia każdego assetu** (autor, źródło, licencja, użycie, zmiany). Ekran autorów generowany z rejestru; test pada, gdy plik w paczce nie ma wpisu.

**Zasada. Pobieranie i obróbka assetów to skrypt z manifestem** (adres, sha256, autor, licencja). Ciężkie źródła poza repo. Pobrane pliki to niezaufane dane.

**Wytyczna. Potok assetów doprowadza model do stylu gry deterministycznie** (paleta, skala w kaflach, front w jednej osi, budżet trójkątów, LOD).

**Wytyczna. Reguła ogólna zamiast serii poprawek pojedynczego modelu.**

## 12. UI, wejście, języki, telefon

**Zasada. Jedna skala interfejsu liczona w jednym miejscu** (wzorzec: Unity CanvasScaler, Unreal DPI Scale Rule), ramki projektu dla telefonu poziomego i tabletu.

**Zasada. Audyt układu UI skryptem na macierzy rozdzielczości i języków**: elementy poza ekranem, nakładanie, ucięty tekst, pismo < 8 px, każde okno, zakładka i panel budynku.

**Zasada. Dotyk sprawdzany osobno**: obszar ≥ 44 px niezależnie od skali rysunku, zapas między sąsiadami, poprawne współrzędne przy DPR > 1. Citybuilder ma tu szczególne ryzyko: stawianie pojedynczych kafli i przeciąganie dróg (patrz `docs/06-MOBILE-UX.md`).

**Wytyczna. Wiele schematów wejścia z automatycznym przełączaniem**: dotyk domyślnie, mysz i klawiatura na tablecie i PC, skróty jak w referencji (zakładki 1-0, pauza, obrót).

**Zasada. Pauza i autozapis przy schowaniu aplikacji.** Utrata postępu to najgorszy defekt. Referencyjny autozapis styczeń/lipiec zostaje dodatkowo.

**Wytyczna. Ustawienia jednym opisem** (`Setting<T>`: klucz, domyślna wartość, `onChange`). Język jest ustawieniem gracza; zapis i determinizm od niego nie zależą.

**Zasada. Teksty dla gracza tylko w katalogach języków (PL, EN od pierwszego tekstu)**; domena trzyma klucze z parametrami; typ wymusza komplet kluczy; liczba mnoga przez `Intl.PluralRules`; skan z bazą pilnuje literałów. Czcionki z pełnym zestawem znaków polskich i greckich.

**Wytyczna. Teksty jak dla człowieka**: druga osoba, pełne zdania. Komunikaty doradców piszemy sami (`docs/02-ORYGINALNOSC.md`).

**Wytyczna. Paczka natywna bez złudzeń**: WebView ma ten sam koszt rysowania; uprawnienia sprawdzane na urządzeniu; numer wersji z liczby commitów; stały klucz podpisu.

## 13. Audio

**Wytyczna. Przypisanie dźwięków do zdarzeń jako dane**, z podglądem wszystkich klipów.

**Wytyczna. Miks w grupach z osobnymi suwakami**, muzyka zależna od stanu miasta (spokój, rozwój, zagrożenie, wizyta boga), głośność wyrównana.

**Wytyczna. Ambient z miejsca**: dźwięk budynku z jego pozycji, natężenie z zoomu kamery.

**Wytyczna. Decyzje audio w czystej funkcji**, odtwarzanie osobno. Reżyser głosów doradców i bogów: odstęp na sytuację, ważne chwile zawsze, bez powtórzeń z rzędu.

## 14. Praca z agentami AI

**Wytyczna. Start od jednego wątku**; zespół agentów, gdy pracę da się izolować (np. dane `data/`, symulacja, render, UI).

**Zasada. Role z rozdzielonymi uprawnieniami**: koordynator (plan, rejestry, integracja, commit, PR, rozmowa z właścicielem), specjaliści (swój obszar plików), łowca błędów i weryfikator (tylko czytają i uruchamiają).

**Zasada. Granica pakietu jest granicą własności**; zmiana w cudzym obszarze przez kontrakt (port, zdarzenie, komenda).

**Zasada. Weryfikator szuka powodu, żeby nie przyjąć**, nie zna rozumowania autora, robi własne sondy (inne ziarno, scenariusz, język).

**Zasada. Zlecenie i raport w stałym formacie.**
- Zlecenie: cel słowami właściciela, zakres i czego nie ruszać, co wiadomo, kryterium odbioru (liczba, zrzut, test), wzorzec, commit bazowy, strony specyfikacji.
- Raport (ok. 30 linii): zrobione, sprawdzone (polecenia i wyniki), niesprawdzone, zmienione pliki, do decyzji właściciela.
- Stałe zasady w `CLAUDE.md`; brief zawiera tylko zadanie i odstępstwa.

**Zasada. Limit rund**: po dwóch odrzuceniach nowy agent ze zwięzłym briefem.

**Zasada. Liczba równoległych agentów wynika z zasobów maszyny.**

**Zasada. Agenci na wspólnej maszynie nie wykonują operacji globalnych**: bez `git stash`, `pkill` po nazwie, force-push, amend; `rm` z pełną ścieżką; pliki tymczasowe z prefiksem zadania; własny port serwera.

**Zasada. Co powtórzyło się drugi raz, staje się skryptem** (albo umiejętnością agenta).

**Zasada. Prywatność w zapytaniach sieciowych**: ogólny User-Agent, nigdy dane właściciela.

## 15. Ciągłość pracy

**Zasada. Trwałe jest tylko to, co wypchnięte.** Commit i push co ok. 30 min albo po każdym kroku, także jako `wip`.

**Zasada. Start sesji to lektura stałych plików**: `CLAUDE.md`, `docs/BACKLOG.md` (sekcja "Następne"), `docs/DECYZJE.md`, `docs/BLEDY.md`, `docs/11-OTWARTE-PYTANIA.md`, otwarte PR-y.

**Wytyczna. Mapa kodu w `CLAUDE.md`**, aktualizowana po większych zmianach.

**Wytyczna. Tryb nieobecności właściciela jako jawny przełącznik**: bez pytań, opcja rekomendowana z dopiskiem "(rekomendowana, pod nieobecność)", zbiorcze zatwierdzenie.

**Wytyczna. Obejście środowiska jest plikiem w repo.**

## 16. Zmiany, CI i git

**Zasada. Jedna zmiana, jedna gałąź, jeden PR; ryzykowne zmiany (symulacja, zapis, renderer) osobno.**

**Zasada. Czerwony main jest pierwszą pracą sesji.**

**Zasada. Kilka zielonych PR-ów scalaj po kolei**, z aktualizacją z main między nimi.

**Wytyczna. PR lekki, main pełny.** Na PR: typy, lint, architektura, testy powiązane; pełny zestaw (audyty, soak, scenariusze kampanii) na main i w nocy.

**Wytyczna. Zmiana tylko w dokumentacji nie uruchamia testów** (`paths-ignore`).

**Wytyczna. Bez plików konfliktujących przy każdym scaleniu**: dziennik zmian z fragmentów (`changelog.d/`), dane po jednym elemencie w linii, numery w rejestrach nadawane po scaleniu.

**Wytyczna. Nieudaną próbę wycofaj i opisz** w `docs/DECYZJE.md`.

## 17. Proces błędów

**Zasada. Definicja błędu**: rozjazd między tym, co gra pokazuje albo symuluje, a specyfikacją (`docs/gameplay/`) lub tym, co gracz uzna za realne. Celowe odstępstwo jest w rejestrze odstępstw; brak funkcji jest pozycją backlogu.

**Zasada. Zgłoszenie bez dowodu nie istnieje**: miejsce, liczba wystąpień, dowód (licznik, pomiar, zrzut), test "co dziś pada".

**Wytyczna. Rejestr błędów `docs/BLEDY.md`, triage S1–S4** (S1: awaria, utrata zapisu, czerwony main), priorytet = ważność × zasięg / koszt.

**Zasada. Poprawka u przyczyny i jako reguła dla całej treści.**

**Wytyczna. Lista kontrolna recenzenta z błędów, które się zdarzyły**: kolejność systemów w ticku, niedeterminizm, dzielenie przez zero w formułach, ujemne zapasy, reset stanu po wczytaniu, alokacje w pętli, literały poza i18n.

## 18. Dokumentacja i decyzje

**Zasada. Dziennik decyzji `docs/DECYZJE.md`**: data, decyzja, powód, gdzie, cytat właściciela; nowe na górze; nieaktualne oznaczone, nigdy usuwane; liczby przed i po, odrzucone warianty, czego nie sprawdzono.

**Zasada. ADR (`docs/adr/`) dla decyzji zmieniających granice albo zasady**, z sekcją "Wzorzec" i wynikami.

**Wytyczna. Backlog bez sekcji "Zrobione"**: zamknięte pozycje do archiwum z datą.

**Wytyczna. Kamienie milowe z kryterium akceptacji**, od pionowego wycinka (od dotyku gracza do zapisu, wszystkie bramki zielone). Rejestr ryzyk w `docs/00-PLAN.md`.

**Wytyczna. Historia pomiarów należy do ADR i commitów**, nie do komentarzy w plikach danych.

## 19. Współpraca z właścicielem

**Zasada. Raport kończy się trzema listami: zrobione, sprawdzone, niesprawdzone** ("nie sprawdzono na telefonie", "nieodsłuchane").

**Zasada. Grywalna wersja po każdej zmianie widocznej dla gracza** (strona z grą na telefon, APK), pakowana skryptem.

**Zasada. Zmiana widoczna ma zrzut przed i po z tego samego ujęcia.**

**Wytyczna. Warianty do wyboru zamiast jednej propozycji** (makiety HUD, style budynków), wybór zapisany w decyzjach.

**Wytyczna. Wartości do strojenia na urządzeniu to nazwane stałe**, decyzja zapisuje "poprzednia → nowa".

**Wytyczna. Pytanie zawsze z rekomendacją, odpowiedź zapisana.** Niejednoznaczną prośbę interpretujemy i zapisujemy interpretację.

**Wytyczna. Kody w raportach z opisem w nawiasie**, np. "L-001 (kierunek chodzika na skrzyżowaniu)".

**Wytyczna. PR i merge tylko na prośbę właściciela.**

**Wytyczna. Styl**: po polsku, wprost i konkretnie, bez patosu, bez myślników em, bez antytezy kontrastowej.

## 20. Co wdrażać na jakim etapie

| Etap projektu (`docs/00-PLAN.md`) | Wdrażamy |
|---|---|
| Faza 0 (teraz) | dziennik decyzji, backlog, ADR z sekcją "Wzorzec", rejestr odstępstw, rejestr źródeł i reguł, specyfikacja 1:1, `CLAUDE.md` z zasadami |
| Faza 1, dzień 1 kodu | warstwy z domeną bez silnika i strażnikiem zależności; stały tick, RNG ze strumieniami, zakaz losowości i czasu w linterze; komendy i zdarzenia; dane z `data/`; limity rozmiaru; i18n od pierwszego tekstu; test zgodności ze specyfikacją; złoty hash na scenariuszu 32×32 |
| Faza 1, gdy działa pętla ekonomii | autopilot i test całej rozgrywki; chaotyczny burmistrz; audyty pokrycia, logistyki, ekonomii; soak |
| Faza 2 (vertical slice) | strona testowa i zrzuty; budżety z urządzenia odniesienia; jedna skala UI i audyt układu; audyt dotyku; rejestr assetów; grywalna wersja na telefon |
| Faza 3 (treści) | zapadki z bazą po miejscach; karty scenariuszy; rejestr błędów z weryfikatorem; zespół agentów z rozłącznymi obszarami; CI z pełnym przebiegiem na main |

## 21. Antywzorce

- Podnoszenie limitu czasu albo budżetu, żeby test przeszedł.
- Próg jakości tylko jako raport.
- Baza zapadki jako sama liczba.
- Złoty hash na mapie kampanii albo poprawiany automatem.
- Nakładka informacyjna licząca zasięgi osobno od symulacji.
- Warunki na konkretnego boga, budynek albo mapę w kodzie systemu.
- Odstępstwo od specyfikacji bez wpisu w rejestrze.
- Asset, tekst albo nazwa skopiowana z gry referencyjnej.
- Historia pomiarów w komentarzach plików danych.
- Niewypchnięta praca w kontenerze.
