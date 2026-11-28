# 24. Algorytmy symulacji (rodzina silnika i reimplementacje)

Źródła tego rozdziału:

| Kod | Źródło | Wierność wobec Zeusa |
|---|---|---|
| S-006 | Julius, reimplementacja Caesar III (C, AGPL) | wierna swojej grze; ta sama rodzina silnika Impressions |
| S-007 | Akhenaten, reimplementacja Pharaoh (C++, GPL); oryginalne zachowania oznaczone w kodzie `// OG` | wierna swojej grze; bezpośredni poprzednik Zeusa na tym samym silniku |
| S-008 | eZeus, reimplementacja Zeusa (C++, GPL-3) | z oryginału czyta tylko scenariusze, mapy świata i teksty; reguły symulacji i stałe to w większości decyzje autora, z kilkoma błędami |

Kod wszystkich trzech jest na licencji GPL/AGPL: czytamy go jako źródło wiedzy, **nie kopiujemy** (`docs/PRAKTYKI.md`, rozdział 1). Pełne notatki z cytatami plik:linia leżą w katalogu roboczym sesji (poza repo).

Oznaczenia:
- **[RODZINA]**: algorytm potwierdzony w S-006 i S-007 i zgodny z pomiarami Zeusa (S-004, S-005). Przyjmujemy jako zachowanie Zeusa.
- **[eZeus]**: tylko w S-008, klasa B (decyzja autora). Przyjmujemy jako założenie robocze do czasu lepszego źródła.
- Zasada wyboru: R-014.

## Model czasu [RODZINA]

- Dzień = **51 ticków** (klatek), miesiąc = 16 dni, rok = 12 miesięcy = 192 dni = 9 792 ticki.
  - Akhenaten: komentarz `9792` w kodzie czasu = 51 × 16 × 12; Julius używa 50 ticków na dzień.
  - Zeus Heaven: miesiąc = 16 dni po 51 klatek.
- Kafel = 15 kroków; spacerujący o prędkości standardowej robi 1 krok na tick, czyli 51 × 16 / 15 = **54,4 kafla na miesiąc**. Zgodne z S-004.
- Systemy miejskie działają raz na dzień, każdy w ustalonym ticku dnia. Kolejność systemów to jedna tablica, zgodnie z `docs/05-SYMULACJA.md`.

Decyzja dla implementacji: tick symulacji = 1 klatka referencyjna; zegar gry liczy dni po 51 ticków. Zamyka L-004 ostatecznie.

## Spacerujący losowy (zamyka L-001) [RODZINA]

### Start

1. Budynek trzyma licznik kierunku (N, E, S, W). Każdy nowy spacerujący dostaje bieżący kierunek, a licznik przesuwa się o 90° zgodnie z zegarem. Wartość początkowa licznika jest losowa dla budynku (z ziarna).
2. Spacerujący wychodzi na kafel drogi przy budynku. Punkt wyjścia: przegląd kafli wokół obrysu **od północy zgodnie z zegarem**; pierwszy kafel drogi (S-004, S-007).
3. Cel wstępny: punkt 8 kafli od budynku w wybranym kierunku. Najbliższa droga w promieniu 6 kafli od tego punktu staje się celem; spacerujący idzie do niej wyznaczoną trasą. Po dotarciu licznik dystansu zeruje się. Jeśli drogi nie ma, od razu przechodzi do swobodnego krążenia.

### Decyzja na każdym kaflu (swobodne krążenie)

Na środku kafla spacerujący obsługuje okolicę (niżej), potem liczy sąsiednie kafle drogi w 4 kierunkach. **Roadblock i Gatehouse nie liczą się jako droga** dla spacerującego losowego.

| Liczba dróg | Zachowanie |
|---|---|
| 0 | koniec krążenia, powrót do budynku |
| 1 | ślepy zaułek: bierze jedyną drogę (zawraca) |
| 2 | idzie prosto, jeśli może i nie jest to kierunek, z którego przyszedł; inaczej skręca o 90° w swoją "stronę" (prawo albo lewo), aż znajdzie drogę |
| 3-4 | wybór pseudolosowy: `(licznik_kroków + stały_losowy_bajt_kafla) mod 4` daje kierunek; jeśli to droga i nie kierunek powrotu, bierze go; inaczej zmniejsza licznik nieudanych wyborów i skręca w swoją stronę |

- Stały losowy bajt kafla jest generowany przy tworzeniu mapy (z ziarna). Ruch jest więc deterministyczny, choć wygląda losowo.
- Po wyczerpaniu licznika nieudanych wyborów (100 po dotarciu do celu wstępnego, potem 5) spacerujący **ponownie celuje** w punkt wstępny: szuka najbliższej drogi zgodnie z zegarem i przeciwnie od kierunku do celu; bliższa strona wygrywa, remis na korzyść prawej. To ustawia jego "stronę" skrętu.
- Szerokie drogi i place: przy 3 drogach i ciągu ≥ 5 kafli drogi wśród 8 sąsiadów spacerujący traktuje miejsce jak drogę dwukierunkową wzdłuż kierunku ruchu; przy pełnym otoczeniu drogą idzie prosto. Dotyczy bulwarów i alei.

### Zasięg i powrót

- Budżet krążenia liczony w tickach: **384 (≈ 26 kafli), 512 (≈ 34 kafle), 640 (≈ 43 kafle)**. Odpowiada to pomiarom Zeusa 26/34/43 (S-005), które S-004 podaje jako 27/35/44 z kaflem startowym.
- Po wyczerpaniu budżetu spacerujący szuka drogi w promieniu 2 kafli od budynku i wraca najkrótszą drogą, **obsługując domy po drodze**. Bez drogi powrotnej znika.
- Wyjątek Zeusa: Fountain i Infirmary wracają do kafla na północ od budynku (S-004), co pozwala wydłużyć trasę.

### Wysyłanie spacerujących

- Budynek ma naraz tylko jednego spacerującego danego typu.
- Opóźnienie kolejnego spacerującego zależy od obsady: pełna obsada najszybciej, potem coraz wolniej (S-006: 3/7/15/29/44 dni przy ≥100/75/50/25/1%; S-007: 0/1/3/7/15 dni). [ZAŁOŻENIE: wariant S-007 jako bliższy Zeusowi.]

## Obsługa domów i zanik usług (zamyka L-012b) [RODZINA]

- Na środku każdego kafla spacerujący obsługuje **każdy budynek w kwadracie 5×5 (promień 2)** wokół siebie. Duże budynki łapią się z dalszej odległości. Obsługują też spacerujący w drodze do miejsca występu i w drodze powrotnej.
- Wizyta ustawia licznik usługi w domu na **96**; licznik spada o **1 dziennie**, więc usługa trwa **96 dni = 6 miesięcy**. Kolumny w widokach specjalnych to ten licznik.
- Poborca: licznik 50 dni.
- Superintendent zeruje ryzyko pożaru i zawalenia wszystkich budynków w kwadracie 5×5; watchman i healer działają analogicznie na ryzyko przestępczości i choroby domu (patrz niżej).
- Punkty kultury Zeusa (S-004: filozof 15, atleta 20 ...) sumują się z rodzajów, których licznik jest > 0.
- eZeus (S-008) używa promienia 1 i liczników 8: odrzucamy jako niezgodne z rodziną i z pomiarami zasięgu.

## Praca (zamyka L-008) 

- **Sektory Zeusa** (nazwy z oryginalnych tekstów gry, S-008 klasa A): husbandry, industry, storage and distribution, hygiene and safety, administration, culture (w Atlantydzie: science), mythology, military.
  - Przypisanie budynków do sektorów: według zakładek budynków (`data/buildings.json`, pole `tab`); sanktuaria i Artisans' Guild: mythology; Tower, Trireme Wharf, Horse Ranch, Armory, Chariot Factory: military; Tax Office: administration.
- **Pula pracowników**: `populacja × odsetek(trudność, płace)` z tabeli S-004 (identyczna w S-008).
- **Priorytety**: instrukcja (Z:62) opisuje poziom priorytetu ustawiany strzałkami. Przyjmujemy 6 poziomów z S-008: brak, bardzo niski, niski, umiarkowany (domyślny), wysoki, bardzo wysoki. [eZeus]
- **Przydział przy niedoborze** (K-031): zgodnie z instrukcją sektory dostają pracowników kolejno od najwyższego priorytetu. Sektory o równym priorytecie dzielą dostępnych pracowników proporcjonalnie do potrzeb, reszta po jednym według największej części ułamkowej (remis: kolejność sektorów). W sektorze każdy budynek dostaje ten sam ułamek obsady, reszta według części ułamkowych (remis: numer budynku) [eZeus]. Wyłączona branża dostaje 0. eZeus dzieli wszystkie sektory naraz według wag 0/1/3/5/8/12; odrzucamy jako sprzeczne z Z:62.
- **Kto pracuje**: tylko domy ludu (elita nie pracuje, Z:29). Pula = `floor(Σ mieszkańców × odsetek(trudność, płace) / 100)`. Dostęp: budynek z dostępem do drogi [ZAŁOŻENIE do czasu modułu wejścia na mapę; S-005 wymaga połączenia pieszo z punktem wejścia].
- **Dostęp do pracy**: budynek musi mieć połączenie pieszo z punktem wejścia (S-005). Rodzina silnika wymaga dodatkowo, by budynek był w zasięgu "poszukiwacza pracy"; Zeus go nie ma (brak takiego spacerującego w S-004).
- **Wydajność rośnie liniowo z obsadą** [RODZINA]: postęp produkcji rośnie dziennie o liczbę pracowników; tempo z `data/production.json` dotyczy pełnej obsady.

## Nastroje i migracja (zamyka L-017, częściowo L-018) [RODZINA + S-004]

Struktura rodziny silnika, z wejściami Zeusa:

1. **Zadowolenie domu** aktualizowane dwa razy w miesiącu (dzień 0 i 8), kumulatywnie, w zakresie 0-100. Do bieżącej wartości dodaje się:
   - **podatek**: tabela nastrojów Zeusa per trudność (S-004, `data/difficulty.json`: od +7 do -7);
   - **płace**: wyższe od normalnych dodają, niższe odejmują [ZAŁOŻENIE: skala jak w S-006: +1 do +4, ujemne połową różnicy];
   - **bezrobocie**: >25% −3, >17% −2, >10% −1, >4% 0, ≤4% +1 (S-006, S-007);
   - **żywność**: ≥2 rodzaje +2, 1 rodzaj +1, brak: kara rosnąca do −3 (S-006);
   - **dług miasta** i **mobilizacja bez walki**: kara (Z:159-160) [ZAŁOŻENIE: −2 na aktualizację, do strojenia audytem].
2. Miasto poniżej 300 mieszkańców nie narzeka: zadowolenie = wartość bazowa trudności (rodzina: 80/70/60/50/40 dla 5 poziomów; Hero = 60).
3. **Popularność** = średnie zadowolenie zamieszkanych domów.
4. **Migracja** codziennie, partia do 12 osób × procent:

| Popularność | Migracja |
|---|---|
| > 70 | +100% |
| > 60 | +75% |
| ≥ 50 | +50% |
| > 40 | 0 |
| > 30 | −10% |
| > 20 | −25% |
| ≤ 20 | −50% |

   - Partie mniejsze niż 4 osoby czekają, aż uzbiera się 4.
   - Imigracja zablokowana przy więcej niż 3 wrogach na mapie i przy braku miejsca w domach.
   - Imigranci wypełniają najpierw domy z ≥ 8 wolnymi miejscami; dom musi być osiągalny z wejścia.
   - Emigracja tylko powyżej 100 mieszkańców, z najniższych poziomów domów.
   - Aphrodite's Haven blokuje emigrację (Z:122).
5. **Bezdomni**: nadwyżka mieszkańców po regresie domu i domy bez drogi przez > 4 dni.

eZeus (S-008) używa innego modelu (grupy do 8 osadników, wygładzone zadowolenie); odrzucamy jako niezgodny z rodziną.

## Ryzyka budowlane (częściowo zamyka L-020b) [RODZINA + S-004]

- **Zawalenie**: ryzyko rośnie miesięcznie o `damage_risk` z `data/building_stats.json`; budynek zawala się po `60 / damage_risk` miesiącach bez wizyty (S-004). Implementacja: licznik += damage_risk/16 dziennie, próg 60.
- **Pożar** (K-033): struktura oryginału z S-007: ryzyko rośnie tylko w 1 dniu na 8 (maska budynku), o `fire_risk × losowy czynnik 1-5`, próg jak zawalenie; S-007 notuje pomiar oryginału: dom zapala się po ok. 10 miesiącach. U nas chata na Hero średnio po ok. 10,7 miesiąca. eZeus (S-008) ma inny model (spadek utrzymania do 0 w 1,6 roku).
- Wizyta superintendenta zeruje oba liczniki w kwadracie 5×5.
- Płonąca ruina płonie ok. 32 dni; co 8 dni może podpalić sąsiada (szansa 1/4) w kierunku zależnym od losowanego co rok wiatru (S-006). Superintendent gasi pożar, jeśli zdąży (Z:90).

## Choroba i przestępczość (częściowo L-021, L-022) [S-004 + RODZINA]

- Zeus ma per dom przyrosty ryzyka: **DRI** (choroba) i **CRI/CRIp** (przestępczość) w `data/housing.json`. Wizyta healera zeruje ryzyko choroby, wizyta watchmana (i szlachcica) ryzyko przestępczości (Z:85, Z:88).
- Wysokie ryzyko choroby przy braku jedzenia lub healera daje zarazę; zarażony dom ma kwarantannę (Z:85).
- Wysokie ryzyko przestępczości daje kolejno: niezadowolonego, rozbójnika, złodzieja, wandala (Z:87-88). Rodzina silnika: im niższe zadowolenie domu i miasta, tym groźniejszy przestępca (progi < 50, < 30, ≤ 10).
- [LUKA L-021b, L-022b: progi ryzyka i śmiertelność zarazy w Zeusie.]

## Ewolucja domów [RODZINA]

- Sprawdzanie codziennie. Awans od razu po spełnieniu warunków; **regres dopiero po 3 kolejnych nieudanych sprawdzeniach**. [ZAŁOŻENIE z rodziny; eZeus cofa natychmiast]
- Dobra konsumowane w dniach 0 i 8 miesiąca; żywność według tabeli S-004 (0,25 jedn./os./mies.).

## Walka (częściowo L-062) [RODZINA + S-004]

- Obrażenia ciosu: `max(0, atak − pancerz)` (dla pocisków: atak pociskiem − pancerz na pociski). Częstość ciosów i strzałów z kolumn FRQ i MRF w `data/enemies.json`.
- **Morale**: każda śmierć w oddziale obniża morale (5-20 pkt); oddział ucieka przy niskim morale (rodzina: ≤ 20). Morale odbudowuje się z czasem (Z:140). Jednostki od bogów i pożyczone mają stałe morale (Z:140). [ZAŁOŻENIE: wartości rodziny]
- Szarża jazdy i atak z flanki dają premię (S-007: szarża ×4, tył/flanka +2/4/7). [ZAŁOŻENIE]
- eZeus nie ma morale ani pancerza: odrzucamy.

## Bitwy za granicą (L-063) [eZeus]

- Siła armii = liczba żołnierzy (jeźdźcy × 1,5) + 8 za każdego herosa + 8 za Aresa + 0,33 × siła sojuszników.
- Wygrywa strona o większej sile; każda traci `0,5 × siła_przeciwnika / siła_własna` swoich żołnierzy.
- Marsz w jedną stronę: 200 dni [ZAŁOŻENIE do strojenia].
- Siła miast wroga: skala 1-6 z danych scenariusza (S-008 klasa A).

## Opinie przywódców (L-064) [eZeus + S-008 klasa A]

- Skala 0-100; wartość początkowa z danych scenariusza (pole "favour"); domyślnie 60.
- Dar: +3 za jednostkę daru potrzebnego towaru, +1,5 za niepotrzebny; maks. 3 jednostki danego towaru rocznie (malejąca wartość, Z:177).
- Prośba gracza o towar: −10 u wszystkich miast; spełnienie prośby miasta: +10; odmowa lub przeterminowanie: −15.
- Atak i rajd: reakcje grup według `data/diplomacy.json`.
- Prośby miast pochodzą wyłącznie ze zdarzeń scenariusza (S-008).

## Handel (L-033) [RODZINA]

- Karawana przewozi 8 ładunków w każdą stronę, statek 12.
- Odstęp między wizytami: 4 dni karawana, 30 dni statek, maks. 1 nowy kupiec dziennie na całą mapę.
- Roczne limity: z danych mapy świata (S-008 klasa A).

## Dług (L-034) [eZeus + Z:16]

- Instrukcja: ostracyzm po kilku latach ciągłego długu. eZeus: komunikat po roku i blokada imigracji. [ZAŁOŻENIE: ostrzeżenie po 1 roku, blokada imigracji, przegrana po 3 latach ciągłego długu.]

## Inne założenia robocze z eZeus

| Temat | Wartość [eZeus] | Luka |
|---|---|---|
| Wizyta przyjaznego boga bez sanktuarium | co 450 dni, po kolei | L-055 |
| Czas trwania sanktyfikacji i klątwy | 30 000 jedn. eZeus ≈ 2,8 mies. | L-054b |
| Odstęp pomocy boga | ≈ 2 lata (S-004: Hermes krócej, Ares dłużej) | L-053 |
| Igrzyska | VI-VIII każdego roku; szansa = mnożnik(populacja) × (zasięg/100)² | L-067 |
| Pomniki Population | progi 1 000, 2 000, 5 000, 10 000, 15 000, 20 000, 25 000 mieszkańców | L-092 |
| Odrost lasu | ≈ 3,9 roku | L-038 (łąka: brak odrostu w eZeus; instrukcja Z:45 mówi o odroście) |
| Potwór | ginie tylko z ręki przypisanego herosa albo Apolla; heros zawsze wygrywa | L-057b; S-004 trivia: potwora da się zabić wojskiem po bardzo długim czasie, więc przyjmujemy wysokie HP zamiast nieśmiertelności |

## Agora i doręczyciele [RODZINA]

- Vendor Agory sięga do magazynu do 40 kafli (S-006, S-007; pomiar Zeusa S-005: 37).
- Kolejność celów doręczyciela w rodzinie: budynek przetwórczy potrzebujący surowca → magazyn → spichlerz. W Zeusie dochodzi eksport na początku (Z:40-58). Zgodne z naszymi `delivery_priority`.
