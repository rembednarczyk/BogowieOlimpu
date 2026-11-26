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

### R-001 [OGÓLNE] Wierność mechanikom gatunku
- Status: propozycja
- Źródło: decyzja autora 2026-11-26 (pierwsza wiadomość)
- Treść: Mechaniki rozgrywki odtwarzają działanie gry referencyjnej i jej dodatku. Odstępstwa są dopuszczalne tylko jako usunięcie ograniczeń sprzętowych lub poprawa wygody i czytelności.
- Uzasadnienie: autor oczekuje, że gra będzie działać jak referencja.
- Wpływ na: `04-MECHANIKI.md`, `03-ANALIZA-REFERENCJI.md`.
- Historia: 2026-11-26 utworzona

### R-002 [OGÓLNE] Własna tożsamość
- Status: propozycja
- Źródło: `02-ORYGINALNOSC.md`
- Treść: Nazwy własne wymyślone przez twórców gry referencyjnej, jej grafika, audio, teksty, mapy i kampanie nie trafiają do gry.
- Uzasadnienie: bezpieczeństwo prawne i własna marka.
- Wpływ na: cała treść gry.
- Historia: 2026-11-26 utworzona

### R-003 [UX] Telefon jako platforma pierwszorzędna
- Status: propozycja
- Źródło: decyzja autora 2026-11-26
- Treść: Każda funkcja musi być w pełni dostępna dotykiem na telefonie 6" w orientacji poziomej.
- Uzasadnienie: autor wskazał grę na telefonie jako wymaganie.
- Wpływ na: `06-MOBILE-UX.md`, `07-TECHNOLOGIA.md`.
- Historia: 2026-11-26 utworzona

### R-004 [TECH] Brak sztucznych limitów
- Status: propozycja
- Źródło: decyzja autora 2026-11-26
- Treść: Limity liczby postaci, budynków i wielkości mapy wynikają wyłącznie z budżetów wydajności, nigdy z ograniczeń przeniesionych z gry referencyjnej.
- Uzasadnienie: autor chce gry bez ograniczeń sprzętowych oryginału.
- Wpływ na: `05-SYMULACJA.md`, `07-TECHNOLOGIA.md`.
- Historia: 2026-11-26 utworzona
