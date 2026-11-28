# Reguły projektowe

Reguły destylujemy ze źródeł (`09-ZRODLA.md`) i z decyzji autora. Reguła "obowiązuje" jest wiążąca dla dokumentów i kodu.

## Statusy

- **propozycja:** wydestylowana, czeka na akceptację autora.
- **obowiązuje:** zaakceptowana.
- **zawieszona:** w konflikcie lub do ponownej oceny.
- **wycofana:** zastąpiona inną regułą (podajemy którą).

## Kategorie

`OGÓLNE`, `MIASTO`, `DOMY`, `SPACERUJĄCY`, `EKONOMIA`, `RELIGIA`, `MIT` (herosi, potwory), `WOJSKO`, `ŚWIAT` (mapa świata), `SCENARIUSZE`, `UX`, `TECH`, `OPRAWA`, `PROCES`.

## Format reguły

```
### R-NNN [KATEGORIA] <nazwa reguły>
- Status: propozycja
- Źródło: S-NNN / decyzja autora RRRR-MM-DD
- Treść: <jedno lub dwa zdania, jednoznacznie sprawdzalne>
- Uzasadnienie: <po co ta reguła>
- Wpływ na: <dokumenty / systemy, które trzeba dostosować>
- Historia: RRRR-MM-DD utworzona
```

## Reguły

### R-001 [OGÓLNE] Mechanika 1:1 z referencją
- Status: obowiązuje
- Źródło: decyzja właściciela 2026-11-26 ("Domyślnie mechanika ma być 1:1 w całej grze. Później będziemy rozbudowywać"); S-001, S-002
- Treść: Mechaniki rozgrywki odtwarzają 1:1 grę referencyjną i dodatek według `docs/gameplay/`. Każde odstępstwo ma wpis w `docs/ODSTEPSTWA.md`; rozbudowa wymaga nowej reguły.
- Uzasadnienie: wymaganie właściciela.
- Wpływ na: `docs/gameplay/`, `data/`, `04-MECHANIKI.md`, testy zgodności.
- Historia: 2026-11-26 utworzona jako propozycja; 2026-11-26 obowiązuje po potwierdzeniu właściciela, treść zaostrzona do 1:1

### R-002 [OGÓLNE] Własna tożsamość
- Status: propozycja
- Źródło: `02-ORYGINALNOSC.md`
- Treść: Nazwy własne wymyślone przez twórców gry referencyjnej, jej grafika, audio, teksty, mapy i kampanie nie trafiają do gry.
- Uzasadnienie: bezpieczeństwo prawne i własna marka.
- Wpływ na: cała treść gry.
- Historia: 2026-11-26 utworzona; 2026-11-26 potwierdzona przez właściciela

### R-003 [UX] Telefon jako platforma pierwszorzędna
- Status: propozycja
- Źródło: decyzja autora 2026-11-26
- Treść: Każda funkcja musi być w pełni dostępna dotykiem na telefonie 6" w orientacji poziomej.
- Uzasadnienie: autor wskazał grę na telefonie jako wymaganie.
- Wpływ na: `06-MOBILE-UX.md`, `07-TECHNOLOGIA.md`.
- Historia: 2026-11-26 utworzona; 2026-11-26 potwierdzona przez właściciela

### R-004 [TECH] Brak sztucznych limitów
- Status: propozycja
- Źródło: decyzja autora 2026-11-26
- Treść: Limity liczby postaci, budynków i wielkości mapy wynikają wyłącznie z budżetów wydajności, nigdy z ograniczeń przeniesionych z gry referencyjnej.
- Uzasadnienie: autor chce gry bez ograniczeń sprzętowych oryginału.
- Wpływ na: `05-SYMULACJA.md`, `07-TECHNOLOGIA.md`.
- Historia: 2026-11-26 utworzona; 2026-11-26 potwierdzona przez właściciela

### R-005 [MIASTO] Droga jako warunek działania
- Status: obowiązuje
- Źródło: S-001 (Z:24-25)
- Treść: Każdy budynek wymaga przylegania do drogi, z wyjątkami: struktury estetyczne, tereny rekreacyjne, pomniki, mury, drzewa i krzewy sadów, zwierzęta hodowlane.
- Uzasadnienie: fundament logistyki gatunku.
- Wpływ na: walidacja budowy, `data/buildings.json` (`requires_road`).
- Historia: 2026-11-26 utworzona; 2026-11-26 potwierdzona przez właściciela

### R-006 [SPACERUJĄCY] Dwa typy spacerujących i blokady
- Status: obowiązuje
- Źródło: S-001 (Z:19-20, Z:25-26)
- Treść: Spacerujący losowy zawraca na Roadblock, spacerujący celowy przez niego przechodzi. Wykonawcy kultury i nauki są celowi w drodze ze szkoły do miejsca występu i losowi po wyjściu z miejsca występu.
- Uzasadnienie: mechanika sterowania pokryciem usług.
- Wpływ na: `data/walkers.json`, system spacerujących.
- Historia: 2026-11-26 utworzona; 2026-11-26 potwierdzona przez właściciela

### R-007 [DOMY] Regres do najwyższego spełnionego poziomu
- Status: obowiązuje
- Źródło: S-001 (Z:31-32, Z:34)
- Treść: Dom bez wymaganego dobra spada do najwyższego poziomu, którego wymagania spełnia (Townhouse bez żywności staje się Hut). Dom elity bez żywności, wełny, oliwy, kultury albo wysokiej atrakcyjności zostaje opuszczony.
- Uzasadnienie: kluczowe napięcie ekonomii.
- Wpływ na: system ewolucji domów, `data/housing.json`.
- Historia: 2026-11-26 utworzona; 2026-11-26 potwierdzona przez właściciela

### R-008 [EKONOMIA] Żywność tylko ze spichlerza, pszenica w magazynie tylko dla koni
- Status: obowiązuje
- Źródło: S-001 (Z:40, Z:68-71, Z:148)
- Treść: Food vendor pobiera żywność wyłącznie z Granary i z importu; pszenica w Storehouse służy tylko Horse Ranch; Horse Ranch nigdy nie bierze z Granary.
- Uzasadnienie: rozdzielenie łańcuchów żywności i koni.
- Wpływ na: logistyka, `data/goods.json`.
- Historia: 2026-11-26 utworzona; 2026-11-26 potwierdzona przez właściciela

### R-009 [EKONOMIA] Towar na eksport i towar gromadzony są wyłączone z użytku
- Status: obowiązuje
- Źródło: S-001 (Z:78-80)
- Treść: Towar w budynku handlowym ustawionym na sprzedaż oraz towar oznaczony do gromadzenia nie mogą być użyte przez przemysł, budowy ani vendorów (peddler może sprzedać zapas już będący w Agorze).
- Uzasadnienie: decyzje alokacyjne gracza.
- Wpływ na: logistyka, handel.
- Historia: 2026-11-26 utworzona; 2026-11-26 potwierdzona przez właściciela

### R-010 [RELIGIA] Ofiary warunkiem działania sanktuarium
- Status: obowiązuje
- Źródło: S-001 (Z:124), S-002 (P:22)
- Treść: Kapłani pobierają kozy, owce, bydło, a w ich braku żywność z Granary. Długi brak ofiar wyłącza korzyści sanktuarium.
- Uzasadnienie: koszt utrzymania łaski boga.
- Wpływ na: system religii, `data/gods.json`.
- Historia: 2026-11-26 utworzona; 2026-11-26 potwierdzona przez właściciela

### R-011 [PROCES] Praktyki projektu
- Status: obowiązuje
- Źródło: S-003, decyzja właściciela 2026-11-26
- Treść: Obowiązują zasady z `docs/PRAKTYKI.md`; skrót w `CLAUDE.md`.
- Uzasadnienie: doświadczenie z trzech projektów właściciela.
- Wpływ na: cały proces i kod.
- Historia: 2026-11-26 utworzona; 2026-11-26 potwierdzona przez właściciela

### R-012 [PROCES] Hierarchia źródeł
- Status: obowiązuje
- Źródło: decyzja koordynatora 2026-11-26 po dodaniu S-004 i S-005; potwierdzona przez właściciela 2026-11-26 ("Przyjmuję R-012")
- Treść: Reguły jakościowe bierzemy z instrukcji (S-001, S-002). Liczby bierzemy z tabel modelu gry w S-004, potem z opisowych stron S-004, potem z pomiarów S-005. Każda rozbieżność ma wpis K-NNN w `docs/gameplay/98-konflikty-zrodel.md`.
- Uzasadnienie: instrukcja opisuje zasady, ale nie podaje liczb; tabele S-004 pochodzą z plików gry; pomiary forum bywają niespójne.
- Wpływ na: `docs/gameplay/`, `data/`.
- Historia: 2026-11-26 utworzona; 2026-11-26 potwierdzona przez właściciela

### R-013 [OGÓLNE] Domyślny poziom trudności Hero
- Status: obowiązuje
- Źródło: decyzja właściciela 2026-11-26 ("domyślnie Hero")
- Treść: Gra ma 5 poziomów trudności 1:1 z referencją (Beginner, Mortal, Hero, Titan, Olympian). Domyślny jest Hero. Testy, audyty i złoty hash używają poziomu Hero, chyba że test dotyczy innego poziomu.
- Uzasadnienie: decyzja właściciela; Hero to poziom bazowy tabel okupu (100%).
- Wpływ na: `data/difficulty.json`, testy, audyty.
- Historia: 2026-11-26 utworzona

### R-014 [PROCES] Algorytmy spoza źródeł Zeusa
- Status: obowiązuje
- Źródło: lektura S-006 (Julius), S-007 (Akhenaten), S-008 (eZeus), 2026-11-26
- Treść: Gdy S-001..S-005 nie opisują algorytmu, przyjmujemy rozwiązanie rodziny silnika potwierdzone w S-006 i S-007, o ile nie przeczy pomiarom Zeusa. Gdy rodzina milczy, przyjmujemy rozwiązanie S-008 jako założenie robocze. Oznaczenia [RODZINA] i [eZeus] w `docs/gameplay/24-algorytmy-silnika.md`. Kodu tych projektów nie kopiujemy (GPL/AGPL).
- Uzasadnienie: S-006 i S-007 wiernie odtwarzają poprzedników Zeusa na tym samym silniku; ich wartości pokrywają się z pomiarami Zeusa (czas, prędkości, zasięgi). S-008 czyta z oryginału tylko scenariusze, reguły symulacji ustala autor.
- Wpływ na: `docs/gameplay/24-algorytmy-silnika.md`, `data/`.
- Historia: 2026-11-26 utworzona; 2026-11-26 obowiązuje (zgoda właściciela na rekomendacje pyt. 22-23 wraz ze stanem dokumentacji)

### R-015 [TECH] Natywny rendering i biblioteki od początku
- Status: obowiązuje
- Źródło: decyzja właściciela 2026-11-26
- Treść: Gra celuje w natywne API: renderer Godota Mobile (Vulkan) na telefonach, rdzeń w C++ przez GDExtension. Renderer Compatibility (OpenGL ES 3 / poziom WebGL 2) nie jest celem i nie jest bazą testów wizualnych. Testy wizualne w kontenerze idą na Vulkanie programowym (lavapipe) w rendererze Mobile.
- Uzasadnienie: właściciel chce pracować bez ograniczeń poziomu przeglądarki; telefon odniesienia (Xiaomi 15T Pro) obsługuje Vulkan.
- Wpływ na: `docs/07-TECHNOLOGIA.md`, `docs/SILNIK.md`, shadery i materiały (muszą działać w rendererze Mobile).
- Historia: 2026-11-26 utworzona


### R-016 [PROCES] Co wymagało sondy, staje się kontrolą
- Status: obowiązuje
- Źródło: decyzja właściciela 2026-11-27 („wszystko, co kosztuje sondowanie, powinno stawać się kandydatami do determinizmu”)
- Treść: Każde pytanie o stan symulacji, na które odpowiedź wymagała sondy (zmienna `BO_DEMO_*`, program na zapisie, ręczny przegląd raportu lub logu), w tym samym zadaniu zamienia się w kontrolę automatyczną, jeśli problem może wrócić bez ostrzeżenia (błąd stanu, cicha porażka, błąd mapy lub autopilota): niezmiennik, próg twardy, kontrolę statyczną, test regresji albo zapadkę. Strojenie jednego demo zostaje raportem. Wyjątek tylko z wpisem i uzasadnieniem w `docs/PLAN-ODPORNOSCI.md` rozdz. 3.
- Uzasadnienie: demo są deterministyczne, ale chaotyczne; bez kontroli ten sam problem wraca po każdej zmianie i znów kosztuje sondowanie.
- Wpływ na: `docs/PLAN-ODPORNOSCI.md`, `docs/PRAKTYKI.md` rozdz. 4-6, bramki demo, `core/src/invariants.cpp`, `core/src/health.cpp`.
- Historia: 2026-11-27 utworzona; 2026-11-27 doprecyzowana: kontrola dla problemów, które mogą wrócić, strojenie demo raportem (plan, wersja 2)

### R-017 [ASSETY] Licencje assetów: własne, CC0, animacje Mixamo
- Status: obowiązuje
- Źródło: decyzja właściciela 2026-11-27 (odpowiedzi 1a, 2 „tak CC0”, 3; animacje z Mixamo od właściciela)
- Treść: Do repozytorium wchodzą tylko: assety własne (modele budowane skryptami `tools/blender/`), tekstury i inne zasoby na licencji CC0 (np. ambientCG, Poly Haven) oraz animacje postaci z Mixamo dostarczone przez właściciela, wypiekane do naszych modeli. Każdy plik ma wpis w rejestrze pochodzenia `game/models/provenance.json` (plik, źródło, autor, licencja, zmiany). Żadnych assetów, kształtów ani przeróbek grafiki z gry referencyjnej (R-002). Grafikę oryginału wolno oglądać i analizować (zrzuty od właściciela, dane sprite'ów z eZeus: rozmiary, liczby klatek i wariantów) pod kątem proporcji, kolorystyki, kompozycji i rytmu miasta; wnioski zapisujemy słowami i liczbami w `docs/STYL-BUDOWLI.md`, bez odrysowywania kształtów i bez plików graficznych oryginału w repo.
- Uzasadnienie: CC0 nie wymaga podawania autora i pozwala na grę komercyjną; animacje Mixamo są dopuszczone w grach, ale nie jako osobne pliki do rozpowszechniania, dlatego trafiają do repo tylko w postaci wypieczonej.
- Wpływ na: `docs/ASSETY.md`, `docs/STYL-BUDOWLI.md`, `tools/blender/`, `game/models/`, `tools/checks/check_models.py`.
- Historia: 2026-11-27 utworzona; 2026-11-28 poluzowana decyzją właściciela („Poluzować regułę, może oglądać”): analiza grafiki oryginału dozwolona, kopiowanie nadal nie

### R-018 [OPRAWA] Styl budowli miasta
- Status: obowiązuje
- Źródło: decyzje właściciela 2026-11-27 i 2026-11-28 (rundy uwag do domów: szczegół, styl Ghibli, łukowe dachy, historie domów; „Zapisujemy ten wypracowany styl jako regułę pod ogólny klimat budowli w mieście”)
- Treść: Każda budowla miasta jest modelowana w jednym stylu, sprawdzalnym w kodzie i na zrzutach:
  1. Realizm stylizowany: elementy budowane jak w rzeczywistości (cegła po cegle, dachówka po dachówce, drewno ułożone fizycznie), materiały greckie (cegła suszona, kamień, tynk wapienny, terakota, drewno).
  1a. Kolor dachu według funkcji budynku (`docs/STYL-BUDOWLI.md`): domy terakota, budynki publiczne łupkowy błękit, świątynie złoto, warsztaty i magazyny brąz.
  2. Ciepłe światło i paleta: paleta `building.gdshader` (31 kolorów, nasycona, popołudniowa), miękkie przejście światła w cień, chłodny niebieskawy cień, plamy koloru jak od pędzla.
  3. Dachy łukowe i strome: dachy dwuspadowe o 1,4 raza wyższe od rzutu (`ROOF_STEEP`), połać ugięta o 7,5% swojej długości, kalenica ugięta w środku; przybudówki z dachem jednospadowym od wyższej ściany.
  4. Ręczna nieregularność: każdy model wygięty jednym gładkim polem (`Builder._wonk`: przechył, pęcznienie ścian, falujący dach), razem z rekwizytami.
  5. Przerysowana greckość: akroteria, antefiksy, meandry, kominy z terakoty, wieńce nad drzwiami, malowane naczynia, kolumny z entazą; o rozmiar większe niż w archeologii.
  6. Życie przy domu: zieleń na wolnym gruncie, bluszcz, kwiaty w skrzynkach i donicach, pranie, dobra, których dom potrzebuje, widoczne według zapasu.
  7. Bez dziur i przenikania: rekwizyty nie wchodzą w bryły, dachy nie wchodzą w ściany innych części, szczyty domknięte pod łukiem, końce dachówek zamknięte zaprawą, nic nie wisi w powietrzu bez podparcia (bryła stoi na ziemi lub na innej bryle, rekwizyt dotyka czegoś, co stoi); eksport przerywa się przy kolizji i przy braku podparcia (`check_props`, dowód w `tools/blender/test_checks.py`).
  8. Różnorodność: dom ma historię rodziny (2-3 na typ, `docs/ASSETY.md`), trwałą przez awanse; w mieście obrót i lekka różnica skali kolejnych domów nadają dynamikę.
- Uzasadnienie: spójny klimat miasta przy wielu budowlach robionych przez różne sesje i agentów; punkty 3, 4 i 7 mają kontrolę w kodzie.
- Przewodnik: `docs/STYL-BUDOWLI.md` (liczby, kroki budowy, znane błędy, historie każdej kategorii budynków).
- Wpływ na: `tools/blender/bo_asset.py`, `bo_detail.py`, `bo_story.py`, `models.py`, `game/shaders/building.gdshader`, `game/scenes/boot.gd`, `docs/ASSETY.md`, `docs/08-OPRAWA.md`.
- Historia: 2026-11-28 utworzona; 2026-11-28 punkt 7 z kontrolą podparcia (`check_props`), przewodnik
