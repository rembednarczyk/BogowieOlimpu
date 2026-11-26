# 17. Czas, ruch i chodzicy: liczby

Źródła: S-004 (Zeus Heaven, strony `units`, `trivia`), S-005 (wątek "Ok Folks - Let's Crunch Numbers", pomiary Nero Would i Caesar Alan; dane z plików `figure_model` gry). Dane: `data/walkers.json`.

## Model czasu

| Jednostka | Wartość | Źródło |
|---|---|---|
| Miesiąc | 16 "dni roboczych" (industrial days) | S-005 |
| Dzień roboczy | 51 klatek animacji | S-005 |
| Dzień rozrywki (e-day, licznik spektakli) | 1/2 dnia roboczego; 32 na miesiąc | S-005 (Moquel) |
| Rok | 12 miesięcy = 384 e-dni | S-005 |
| Kafel | 15 "kroków" | S-005 |

Chodzik o prędkości standardowej robi 1 krok na klatkę: 51/15 = 3,4 kafla na dzień roboczy = **54,4 kafla na miesiąc**.

Wniosek dla implementacji (L-004): naturalny tick symulacji to **1 klatka referencyjna**, czyli 816 ticków na miesiąc. Prędkość gry skaluje liczbę ticków na sekundę. Decyzja w `docs/05-SYMULACJA.md` po prototypie.

## Prędkości

Plik gry koduje prędkość indeksem 0-18 (S-005):

| Indeks | Kafle/miesiąc | Indeks | Kafle/miesiąc |
|---|---|---|---|
| 0 | 0 | 10 | 90,7 |
| 1 | 13,6 | 11 | 95,2 |
| 2 | 18,1 | 12 | 108,8 |
| 3 | 27,2 | 13 | 122,4 |
| 4 | 36,3 | 14 | 126,9 |
| 5 | 40,8 | 15 | 136,0 |
| 6 | 54,4 | 16 | 145,1 |
| 7 | 68,0 | 17 | 149,6 |
| 8 | 72,5 | 18 | 163,2 |
| 9 | 81,6 | | |

Implementujemy tablicę indeksów, bez wzoru.

| Postać | Kafle/miesiąc |
|---|---|
| Większość mieszkańców, hoplita, karawana, owca | 54,4 |
| Zarażony mieszkaniec | 13,6 |
| Koza, bydło, wartownik na murze | 40,8 |
| Doręczyciel przyspieszony przez Hermesa | 68 |
| Bóg, heros, potwór, wilk, dzik, jeleń, koń | 68 |
| Myśliwy, rabble gracza, łucznik gracza | 72,53 |
| Statek handlowy | 95,2 |
| Jeździec i rydwan gracza | 108,8 |
| Triera gracza | 68 (S-005) |

Prędkości są takie same na każdym poziomie trudności.

## Zasięgi chodzików losowych (S-004)

| Budynek | Chodzik | Kafle |
|---|---|---|
| Fountain | water bearer | 27 |
| Infirmary | healer | 27 |
| Tax Office | clerk | 35 |
| Gymnasium, Podium, Theater, Stadium | wykonawcy kultury | 35 |
| Bibliotheke, Laboratory, Observatory, Museum | naukowcy | 35 |
| Agora | peddler | 44 |
| Maintenance Office | superintendent | 44 |
| Watchpost | watchman | 44 |

Pomiar forum podaje wartości o 1 mniejsze (26/34/43), bo nie liczy kafla startowego. Przyjmujemy S-004 i liczymy kafel startowy (K-008).

## Zachowanie chodzika losowego

1. Wychodzi z budynku na drogę i idzie, dopóki nie przejdzie limitu kafli albo nie natrafi na Roadblock lub koniec drogi.
2. Po przekroczeniu limitu wraca **najkrótszą drogą** do budynku.
3. Na Roadblock zawraca.

[LUKA L-001, częściowo otwarta: reguła wyboru kierunku na skrzyżowaniu. Źródła nie opisują; kandydat: reimplementacje gier z tej rodziny.]

## Punkt wyjścia z budynku (S-004)

- Chodzik wychodzi na kafel drogi przylegający do budynku. Gdy kilka kafli przylega, gra szuka **od kafla na północ od budynku zgodnie z ruchem wskazówek zegara**; pierwszy znaleziony kafel to punkt wyjścia i powrotu.
- **Wyjątek: Fountain i Infirmary.** Ich chodzicy wracają zawsze do kafla bezpośrednio na północ od budynku, który nie przylega do budynku. Odcinek drogi między wyjściem a tym kaflem nosiwoda przejdzie w całości, więc da się wydłużyć efektywną trasę ponad 27 kafli. To mechanika 1:1 (wykorzystywana przez graczy).
- Bóg po przybyciu do sanktuarium pojawia się na najbliższej drodze, szukając od NE zgodnie z zegarem, w promieniu ok. 10 kafli (S-004 trivia); według forum w promieniu 7 kafli na drodze o najwyższej atrakcyjności (K-022, niepotwierdzone).

## Chodzicy celowi i doręczyciele

- Doręczyciel budynku produkcyjnego niesie **1 ładunek**; doręczyciel Storehouse, Granary i budynku handlowego **4 ładunki** (rzeźba zawsze 1).
- Chodzik celowy, który wyruszył, zawsze dochodzi do celu. Gdy cel zniknie, idzie do jego miejsca i szuka nowego celu albo wraca. Gdy budynek źródłowy zniknie, doręczyciel kończy dostawę i znika; jeśli nie ma celu, znika od razu z ładunkiem (S-005, JonN).
- Doręczyciel z rozkazem Get może iść na przełaj; inne idą drogami.
- Food vendor sięga do Granary w odległości do 37 kafli w linii prostej (S-005).
- Hostler z Chariot Factory szuka koni do 44 kafli; handler hipodromu idzie przez całą mapę (S-005).

## Dostęp do pracowników

Budynek dostaje pracowników z puli miasta, jeśli ma **połączenie pieszo** (nie musi być drogą) z punktem wejścia imigrantów. Budynki na odciętym płaskowyżu albo wyspie bez mostu nie działają (S-005, JonN).
