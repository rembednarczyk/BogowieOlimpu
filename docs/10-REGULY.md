# Reguły projektowe

Reguły destylujemy ze źródeł (`09-ZRODLA.md`) i z decyzji autora. Reguła "obowiązuje" jest wiążąca dla dokumentów i kodu.

## Statusy

- **propozycja:** wydestylowana, czeka na akceptację autora.
- **obowiązuje:** zaakceptowana.
- **zawieszona:** w konflikcie lub do ponownej oceny.
- **wycofana:** zastąpiona inną regułą (podajemy którą).

## Kategorie

`OGÓLNE`, `MIASTO`, `DOMY`, `CHODZIKI`, `EKONOMIA`, `RELIGIA`, `MIT` (herosi, potwory), `WOJSKO`, `ŚWIAT` (mapa świata), `SCENARIUSZE`, `UX`, `TECH`, `OPRAWA`, `PROCES`.

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

### R-006 [CHODZIKI] Dwa typy chodzików i blokady
- Status: obowiązuje
- Źródło: S-001 (Z:19-20, Z:25-26)
- Treść: Chodzik losowy zawraca na Roadblock, chodzik celowy przez niego przechodzi. Wykonawcy kultury i nauki są celowi w drodze ze szkoły do miejsca występu i losowi po wyjściu z miejsca występu.
- Uzasadnienie: mechanika sterowania pokryciem usług.
- Wpływ na: `data/walkers.json`, system chodzików.
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

