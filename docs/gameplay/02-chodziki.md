# 02. Chodziki (Z:25-26)

Każda usługa i każdy transport w mieście jest realizowany przez widoczną postać.

## Dwa typy

| Typ | Zachowanie | Roadblock |
|---|---|---|
| **Losowy** (roaming) | wychodzi z budynku i krąży po drogach, obsługując budynki, które mija | zawraca |
| **Celowy** (destination) | idzie do konkretnego budynku | przechodzi |

## Lista chodzików losowych (Z:25-26)

Athletes, Clerks, Disgruntled citizens, Healers, Noblemen, Outlaws, Peddlers, Superintendents, Watchmen, Water bearers.

Dodatek: Scholars; astronomowie, wynalazcy i kuratorzy wychodzący z miejsc pracy (P:10-12).

## Lista chodzików celowych (Z:26)

Arms vendor, Artisans, Deliverymen, Food vendor, Fleece vendor, Goatherds, Growers, Horse trainer, Oil vendor, Shepherds, Wine vendor.

Dodatek: Chariot vendor, Hostler, Hippodrome handler, Rancher, Metallurgist, Stonecutter (P:7-18).

## Chodziki dwufazowe (Z:26, Z:106-109, P:10-12)

Wykonawcy kultury i nauki są celowi w drodze od szkoły do miejsca występu, a po dotarciu ich koledzy wychodzący z miejsca występu są losowi:

| Szkolenie | Cel (faza celowa) | Faza losowa |
|---|---|---|
| College | Podium | filozofowie z Podium |
| Drama School | Theater | aktorzy z Theater |
| Gymnasium | Stadium | zawodnicy ze Stadium |
| University | Observatory | astronomowie z Observatory |
| University | Museum | kuratorzy z Museum |
| Inventors' Workshop | Laboratory | wynalazcy z Laboratory |

Szkoła i miejsce występu muszą być połączone drogą, inaczej wykonawca nigdy nie wyjdzie.

Gymnasium wysyła też własnych atletów losowo, niezależnie od Stadium (Z:107).

## Doręczyciele (deliverymen)

- Wychodzą z budynku produkcyjnego z ładunkiem i szukają celu według priorytetów danego budynku (patrz [05-przemysl-praca.md](05-przemysl-praca.md) i `data/buildings.json`, pole `delivery_priority`).
- **Utknięcie**: doręczyciel stoi w miejscu, gdy (Z:63):
  - docelowy magazyn ma za mało pracowników,
  - brak drogi do żadnego budynku przyjmującego towar,
  - wszystkie magazyny, Piery i Trading Posty są pełne lub nie przyjmują towaru.
- Doręczyciel z rozkazem "Get" w spichlerzu/magazynie może zejść z drogi, by dojść do towaru (Z:67).

## Wózki z wołami

Ciężkie towary (marmur, drewno, rzeźby, materiały budowlane sanktuariów) przewożą wozy z wołami (Z:52, Z:54, Z:58, Z:117-118).

## Rozmowa z mieszkańcami

Kliknięcie postaci (u stóp) pokazuje, co myśli (Z:25). To kanał informacji dla gracza.

## [LUKA]

- Algorytm wyboru kierunku przez chodzika losowego na skrzyżowaniu.
- Maksymalny dystans trasy chodzika losowego i promień obsługi.
- Prędkość ruchu.
Patrz [99-luki.md](99-luki.md).
