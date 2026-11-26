# 06. Dystrybucja i handel (Z:65-81)

## Granary (spichlerz, Z:65-68)

- Przechowuje **wyłącznie żywność dla ludzi**. Droga wymagana.
- Duży, brzydki, przyciąga szkodniki: **obniża atrakcyjność**, ludzie nie chcą mieszkać obok. Podatny na zawalenie, potrzebuje częstych wizyt superintendenta.
- Przyjmuje dostawy dopiero, gdy jest **w pełni obsadzony**.
- Wypełnienie widać po otworach zasypowych.
- Panel: pracownicy, łączny zapas, zapas każdego rodzaju żywności.

### Rozkazy dla każdego towaru (Granary i Storehouse)

Cykl kliknięć: **Accept → Get → Empty → Don't Accept → Accept**.

| Rozkaz | Działanie |
|---|---|
| Accept | przyjmuje dostawy do limitu (stock limit). **Domyślnie w Granary: Accept do 32 ładunków** dla każdej żywności (Z:66) |
| Get | własny doręczyciel szuka towaru w Pierach, Trading Postach i innych spichlerzach/magazynach aż do limitu. Nie bierze ze spichlerza, który też ma Get na ten towar, ani towaru odłożonego na eksport. **Może zejść z drogi** |
| Empty | doręczyciel wywozi cały zapas niezależnie od limitu: do innego spichlerza, eksportu; pszenicę także do Horse Ranch i Storehouse przyjmującego pszenicę |
| Don't Accept | odmawia dostaw; vendor nadal może pobierać. Limit nie ma znaczenia. Przycisk X ustawia Don't Accept dla wszystkiego |

Rozkazy Get i Empty działają tylko przy pełnej obsadzie (Z:68).

## Storehouse (magazyn, Z:68-69)

- Przechowuje surowce i wyroby. **Nie przechowuje żywności dla ludzi.**
- Może przechowywać pszenicę **tylko dla koni**; domyślnie **Not Accept** dla pszenicy.
- Przyjmuje od warsztatów, wydobycia, Pierów i Trading Postów połączonych drogą.
- Rozkazy jak w Granary.
- Dla dużych dostaw na budowę wozy czekają aż **4 ładunki** materiału zbiorą się **w jednym** magazynie (Z:118).

[LUKA: pojemność Storehouse i Granary w ładunkach]

## Agora (Z:69-73)

### Typy

| Agora | Stoiska | Ustawienie |
|---|---|---|
| Common Agora | do **3** | dłuższy bok nad istniejącą drogą, wolne miejsce po stronie drogi na stoiska |
| Grand Agora | do **6** | droga przez środek wzdłuż, kładziona nad istniejącą drogą; najlepiej długi odcinek bez zabudowy po obu stronach |

### Stoiska (Vendor's Stalls)

| Stoisko | Vendor pobiera z | Uwagi |
|---|---|---|
| Food Vendor's Stall | Granary, import żywności w Pier/Trading Post | nie bierze pszenicy ze Storehouse ani towaru na eksport |
| Fleece Vendor's Stall | Storehouse, import | |
| Oil Vendor's Stall | Storehouse, import | |
| Wine Vendor's Stall | Storehouse, import | |
| Arms Vendor's Stall | Storehouse, import | |
| Horse Trainer's Stall | **bezpośrednio z Horse Ranch** | koni nie da się przechowywać |
| Chariot Vendor's Stall (dodatek) | bezpośrednio z Chariot Factory | |

- Po powrocie vendora z Agory wychodzi **peddler** (chodzik losowy), który sprzedaje domom na trasie potrzebne dobra, jeśli Agora je ma.
- **Agora jest prywatna: miasto nie zarabia na sprzedaży** (Z:73).

### Statusy towaru w panelu Agory

| Status | Znaczenie |
|---|---|
| No Vendor | brak stoiska |
| Buying | brak towaru, vendor poszedł po niego |
| No Goods | vendor jeszcze nie wyszedł |
| Not Buying | gracz zakazał zakupu (kliknięcie przełącza) |
| No Supply | brak źródła |
| Too Distant | źródło za daleko od Agory [LUKA: próg odległości] |
| Stockpiled | towar gromadzony; vendor nie kupuje, peddler dalej sprzedaje to, co ma |
| Distributing | normalna praca |
| No Labor | stoisko bez pracowników |

## Handel (Z:74-80)

### Z kim

- Tylko **sojusznicy, kolonie i wasale** (oraz dalekie miasta, Z:170). Rywale ani nie kupują, ani nie sprzedają.
- Partner musi mieć dobre stosunki; przy złych odmawia handlu. Okoliczności zewnętrzne mogą przerywać handel.
- Na mapie świata szlak **niebieski = morski**, **czerwonobrązowy = lądowy**.
- Panel miasta: co kupuje i sprzedaje, **roczny limit** każdego towaru, ile już wymieniono w tym roku.

### Budynki

- **Trading Post** (lądowy) lub **Pier** (morski, prosty odcinek wybrzeża) **dedykowany konkretnemu miastu**. Droga i wizyty superintendenta wymagane.
- Po postawieniu karawany/statki zaczynają przybywać, ale nic nie wymieniają, dopóki gracz nie ustawi rozkazów.

### Import (Z:77-78)

- Przełącznik Not Buying / Buying + stock limit.
- Kupiec sprzedaje do limitu, ograniczony pojemnością transportu i rocznym limitem miasta.
- Koszt odejmowany ze skarbca przy rozładunku.
- Po odjeździe doręczyciele z Pier/Trading Post rozwożą: surowiec do warsztatu, który go potrzebuje; pszenicę do Horse Ranch; reszta do Storehouse lub żywność do Granary. Vendorzy mogą brać prosto z budynku handlowego.
- **Kupiec patrzy tylko na zapas w swoim budynku handlowym**, a zapasy w reszcie miasta go nie interesują, więc łatwo przeimportować.

### Eksport (Z:78-79)

- Przełącznik Not Selling / Selling + ilość utrzymywana na stanie.
- Doręczyciele budynku handlowego dbają o utrzymanie limitu.
- Kupiec kupuje tyle, ile uniesie; pieniądze trafiają do skarbca. Potem doręczyciele uzupełniają.
- **Towar w budynku handlowym przeznaczony na eksport jest wyłączony z innych zastosowań** (przemysł, domy, budowy).
- Budynki produkcyjne kierują towar w pierwszej kolejności do eksportu, jeśli jest ustawiony.

### Ceny (Z:81)

Panel szczegółów zakładki dystrybucji: cena każdego towaru w Grecji, możliwość importu/eksportu, z kim handlujemy. [LUKA: wartości cen]

Hermes przyspiesza karawany i statki oraz zwiększa ich liczbę w roku (Z:122). Klątwa Zeusa i Posejdona odstrasza kupców; Hermes w gniewie czyni drogi niebezpiecznymi dla karawan (Z:125-126).

## Gromadzenie zapasów (Stockpiling, Z:80)

- Kliknięcie towaru na liście inwentarza miasta przełącza gromadzenie (kolor żółty).
- Towar gromadzony **nie może być użyty przez przemysł, sprzedany kupcom, użyty do budowy sanktuarium ani pobrany przez vendorów**. Peddlerzy mogą rozprowadzać to, co Agora już ma.
- Zastosowanie: przygotowanie dostawy dla innego miasta, postawienie sanktuarium, przyzwanie herosa.

## Widok See Distribution

Agory, magazyny, budynki handlowe, vendorzy, peddlerzy, doręczyciele i domy z ikonami zapasów (zielone = dużo, migające czerwone = brak).
