# 01. Podstawy

## Przygody, epizody, cele (Z:16-17)

- Gra składa się z **przygód** (adventures). Przygoda dzieli się na **epizody**. Każdy epizod ma zestaw **celów**.
- Cele są widoczne na początku epizodu i odhaczane po spełnieniu. Część celów można kliknąć, by zobaczyć szczegóły. Przycisk Cele (Goals) pokazuje postęp w dowolnym momencie.
- Spełnienie wszystkich celów przenosi gracza do następnego epizodu.
- Przygody można grać w dowolnej kolejności; lista jest posortowana od najłatwiejszej.
- Dostępne są też samouczki (tutorials).
- Dodatek zawiera edytor przygód (P:5); gra podstawowa również miała edytor (P:4 wspomina "Zeus Adventure Editor").

## Kolonie i miasto macierzyste (Z:17, Z:169)

- Na końcu niektórych epizodów gracz opuszcza miasto i zakłada **kolonię**.
- Podczas nieobecności miastem macierzystym (parent city) zarządza zastępca. Po powrocie miasto jest **dokładnie w stanie, w jakim zostało** (symulacja miasta macierzystego jest zamrożona).
- Relacje z miastem macierzystym podczas kolonizacji opisuje [14-swiat.md](14-swiat.md).

## Warunki porażki (Z:16, Z:169, Z:172)

Epizod przegrywa się, gdy:

1. Miasto jest **nieprzerwanie zadłużone przez kilka lat**: lud skazuje władcę na ostracyzm. [LUKA: liczba lat]
2. **Rywal, któremu już płacisz trybut, pokonuje cię ponownie** (dwie porażki z tym samym rywalem).
3. Gracz sprowokował rywala do ataku na własne miasto, by zrzucić trybut, i **przegrał tę obronę**.
4. **Miasto macierzyste upada lub zostaje zniszczone przez potwora**, gdy gracz zakłada kolonię.

## Teren (Z:17-18)

| Teren | Budowa | Ruch pieszych | Uwagi |
|---|---|---|---|
| Łąka (grassland) | prawie wszystko | swobodny | nie nadaje się pod farmy ani hodowlę |
| Łąka kwietna (meadow, fioletowe kwiaty) | wszystko | swobodny | jedyny teren pod farmy, sady i zwierzęta; szkoda zabudowywać go czymś innym |
| Las | po wykarczowaniu | b.d. | źródło drewna (Timber Mill); wykarczowany las **nigdy nie odrasta** |
| Bagno (marsh) | nic | przejezdne | |
| Plaża | nic | b.d. | piasek się przesuwa |
| Skała zwykła | nic | utrudniony, tylko przez przełęcze i schody | |
| Skała rudonośna (miedź) | nic | jak skała | wydobycie przez Foundry |
| Skała rudonośna (srebro) | nic | jak skała | wydobycie przez Mint |
| Kamieniołom (quarry, płaska biało-szara skała) | nic | b.d. | marmur przez Masonry Shop; kamieniołom pogłębia się w trakcie wydobycia |
| Woda | budynki nabrzeżne | nie | ryby, jeżowce; Pier, Water Crossing, Urchin Quay, Fishery, Trireme Wharf muszą stać przy brzegu |

Dodatek dodaje: skała z rudą orichalku (czerwone samorodki), kamieniołom czarnego marmuru (ciemnoszara płaska skała pod trawą), stada jeleni (P:8-9).

## Droga do Grecji (Z:17)

- Mapa zawiera jedną drogę łączącą teren miasta z resztą Grecji.
- Gracz może przebudować jej przebieg, ale **nie może blokować jej początku ani końca**. Zablokowanie skutkuje problemami (imigranci, karawany). Istnieje komunikat "droga do Grecji zablokowana" (Z:189).

## Budowa (Z:19, Z:24-25)

- Wybór budynku z zakładki panelu sterowania. Pojawia się czerwony obrys; na poprawnym miejscu zmienia się w zielony podgląd. Kliknięcie stawia budynek.
- Większość budynków wymaga czystego terenu. **Clear Land** usuwa przeszkody (pojedynczo lub przeciąganiem obszaru).
- **Niemal każdy budynek musi przylegać do drogi**, by działać. Wyjątki: struktury estetyczne, tereny rekreacyjne, pomniki pamiątkowe, mury, drzewa oliwne, winorośle, kozy, owce (Z:24-25), drzewa pomarańczowe i bydło (P:7-8, analogia).
- Budynek z pracownikami pokazuje ich przy pracy przed wejściem.
- Część budynków można obracać przed postawieniem (Palace, Stadium, Sanctuary, Gatehouse, Horse Ranch w dodatku, Pyramid).
- Duże budowle można "zamrozić" obrysem i przewinąć widok, by sprawdzić przeszkody (Z:117).
- **Undo**: cofa ostatnią akcję i zwraca wydane pieniądze, ale tylko przez krótki czas, gdy przycisk jest aktywny (Z:20). [LUKA: okno czasowe]
- **Pauza** zatrzymuje czas, a budowanie dalej działa (Z:25).

## Drogi i blokady (Z:19-20)

- Drogi stawia się pojedynczo lub przeciąganiem.
- **Roadblock** (blokada drogi): chodziki losowe zawracają na blokadzie; chodziki celowe przechodzą przez nią.

## Kamera (Z:21)

- Widok izometryczny z obrotem co 90° w obie strony oraz powrotem na północ.
- Mapa przeglądowa (Overview Map) z prostokątem aktualnego widoku.
- Zakładki (bookmarki) lokalizacji: 4 sloty (Z:190).

## Czas i kalendarz

- Czas płynie w miesiącach i latach; data widoczna na górnym pasku obok skarbca i populacji (Z:24).
- Prędkość gry i przewijania ustawiana w opcjach (Z:22).
- **Autozapis co pół roku: w styczniu i lipcu** (Z:22).
- Zdarzenia kalendarzowe: żniwa (patrz [04-zywnosc.md](04-zywnosc.md)), coroczne limity handlu, trybut roczny, igrzyska według roku.

## Poziom trudności (Z:22)

Istnieje ustawienie trudności. [LUKA: co dokładnie zmienia]

## Katastrofy naturalne (Z:189)

Komunikaty "idź do" wymieniają: trzęsienie ziemi (pęknięcia gruntu), falę pływową zalewającą ląd, osuwisko, wypływ lawy. [LUKA: mechanika, częstotliwość, skutki; prawdopodobnie zdarzenia skryptowe scenariusza]

## Drapieżniki

- Wilki porywają owce i kozy; pasterze zwykle pilnują stad (Z:45).
- Dziki atakują i zabijają myśliwych (Z:46).
- Bydło w dodatku jest chronione przez byki przed wilkami (P:7).
