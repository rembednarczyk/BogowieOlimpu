# 10. Mitologia (Z:115-133, P:21-29)

## Panteon i hierarchia

W danej przygodzie część bogów jest **przyjazna** (chce kultu), część **wroga** (rzuca przeszkody), reszta nieaktywna. Hierarchia decyduje o sile: bóg wyżej zwykle pokonuje boga niżej (P:23).

| Ranga | Bóg | Domena (wg instrukcji) | Gra |
|---|---|---|---|
| 1 | Zeus | władca niebios | Z |
| 2 | Poseidon | władca mórz | Z |
| 3 | Hades | władca podziemi | Z |
| 4 | Hera | królowa niebios | P |
| 5 | Demeter | płodność | Z |
| 6 | Athena | wojownicy, rzemiosło, mądrość | Z |
| 7 | Artemis | łowcy | Z |
| 8 | Apollo | proroctwo | Z |
| 9 | Atlas | dźwigający świat | P |
| 10 | Ares | wojna | Z |
| 11 | Hephaestus | kowal niebios | Z |
| 12 | Aphrodite | czułe namiętności | Z |
| 13 | Hermes | posłaniec bogów | Z |
| 14 | Dionysus | wino | Z |

Wyjątki w starciach:
- **Zeus** (z sanktuarium) broni miasta przed każdym innym bogiem (Z:119).
- **Hera** (z sanktuarium) przegania Zeusa i sprawia, że nie wyrządza szkód (P:21-22).
- **Aphrodite** (z sanktuarium) przegania Ares, Hephaestus, Hermes, Dionysus (Z:122). [NIEJASNE: P:23 wymienia tylko Ares i Hephaestus. Przyjmujemy listę z Z:122 jako pełniejszą]

## Bóg przyjazny: wizyty

Przyjazny bóg pojawia się w mieście, kusząc obietnicą korzyści. Kultem jest **budowa sanktuarium**. Im wyższa ranga, tym większe i droższe sanktuarium.

## Budowa sanktuarium (Z:116-118)

1. **Postawienie fundamentu i schodów** wymaga marmuru zgromadzonego w magazynach i drachm. Ilość marmuru i koszt widać w menu sanktuariów. [LUKA: wartości per sanktuarium]
2. Footprint; duże sanktuaria można zamrozić obrysem (klawisz) i obracać. Wymagana droga.
3. Dokończenie wymaga **artisans** z **Artisans' Guild** oraz dostaw **marble, wood, sculpture**.
   - Artisans' Guild: droga wymagana; rzemieślnicy hałasują, **obniżają atrakcyjność**.
   - Rzemieślnik idzie na budowę i czeka na wóz z materiałem.
   - Wóz jedzie z Storehouse albo z budynku handlowego **importującego** potrzebny materiał. Towar przeznaczony na eksport jest niedostępny.
   - **Materiały budowlane (marmur, drewno) tylko w dużych dostawach: wóz czeka, aż w jednym magazynie będą 4 ładunki.** Rzeźby jadą po jednej sztuce.
   - Więcej gildii = więcej rzemieślników na budowie.
4. Panel budowy: % ukończenia, brakujące materiały, przyczyna zatrzymania.
5. Sanktuarium mocno podnosi atrakcyjność okolicy.
6. Budowę można **wstrzymać i wznowić** (P:28-29); rzemieślnicy kończą bieżące zadanie i idą do innej budowy lub do domu.

Dodatek: Citadel of Poseidon (wariant atlantydzki) wymaga białego marmuru, rzeźb, drewna i orichalku (P:26).

## Działanie sanktuarium

- Ukończone sanktuarium daje **stałe korzyści**, umożliwia **modlitwę**, a bóg spacerując **sanktyfikuje** budynki (magiczne iskry nad dachem; czasowy bonus). [LUKA: czas trwania sanktyfikacji]
- **Ofiary** (Z:124): kapłani co jakiś czas wychodzą po kozę lub owcę (dodatek: także bydło). Bez zwierząt biorą żywność z Granary. **Długi brak ofiar = bóg uznaje, że się od niego odwrócono, sanktuarium traci korzyści.** Widok See Immortals: kolumna przed sanktuarium = jak dawno była ofiara.
- **Modlitwa** (Z:124): przycisk w panelu sanktuarium. Bóg może odmówić; ignoruje zbyt częste prośby. Panel pokazuje szansę wysłuchania.
- **Wyrocznia** (Z:124): Zeus' Stronghold i Oracle of Apollo. "Consult Oracle" daje przepowiednię; nie odpowiada, jeśli niedawno już przepowiadała.
- **Pomoc samorzutna**: wiele bóstw udziela błogosławieństwa modlitewnego z własnej inicjatywy, gdy widzi problem.

## Sanktuaria i ich efekty

Szczegóły w `data/gods.json`. Skrót:

| Bóg | Sanktuarium | Stałe korzyści | Sanktyfikuje | Modlitwa (i pomoc samorzutna) |
|---|---|---|---|---|
| Zeus | Zeus' Stronghold | wyrocznia; broni przed każdym bogiem; wzrost opinii innych przywódców | dowolny budynek (wykonuje sanktyfikację każdego boga) | pokonuje następną armię atakującą miasto (samorzutnie w wielkim zagrożeniu) |
| Poseidon | Promontory of Poseidon | Kraken patroluje wody i zabija wrogów; ryby i jeżowce łowią się szybciej | Fishery, Urchin Quay, Horse Ranch | owoce morza przy niskim zapasie żywności |
| Hades | Gates of Hades | Cerberus krąży i atakuje wrogów; złoża srebra przy sanktuarium | Tax Office, Mint, Foundry | pieniądze (lub skłania sojusznika do datku) |
| Demeter | Garden of Demeter | żyzna łąka (meadow) wokół sanktuarium | farmy | żywność do spichlerzy |
| Athena | Arbor of Athena | święty gaj oliwny; silniejsi żołnierze; osobiście broni miasta | Carding Shed, Olive Press | oliwki i oliwa |
| Artemis | Artemis' Menagerie | 2 kompanie Amazonek; poluje na drapieżniki zabijające ludzi lub zwierzęta | Boar Hunter's Lodge | wieprzowina |
| Apollo | Oracle of Apollo | wyrocznia; lepsi filozofowie, atleci, aktorzy, zawodnicy (igrzyska); **zabija potwory atakujące miasto** | Podium, Gymnasium, Theater, Stadium (dodatek: Bibliotheke, Laboratory, Observatory, Museum) | leczy zarazę i poprawia zdrowie |
| Ares | Ares' Fortress | 2 kompanie Sparti; jego Dragon zjada czasem najeźdźcę, wilka lub dzika | brak w instrukcji | towarzyszy armii w bitwie w innym mieście (także samorzutnie) |
| Hephaestus | Forge of Hephaestus | złoża miedzi przy sanktuarium; **zapobiega pożarom** | Armory, Sculpture Studio | Talos broni miasta przez kilka miesięcy |
| Aphrodite | Aphrodite's Haven | **nikt nie emigruje**; wyższa atrakcyjność miasta; przegania Ares, Hephaestus, Hermes, Dionysus | domy (więcej mieszkańców, jeśli jest miejsce) | więcej mieszkańców we wszystkich domach (samorzutnie przy pustostanach) |
| Hermes | Hermes' Refuge | szybsze i liczniejsze karawany i statki; szybsi doręczyciele | brak w instrukcji | spełnia jedną zaległą prośbę innego miasta bez kosztów |
| Dionysus | Grove of Dionysus | winnica przy sanktuarium; **brak niepokojów** | Winery | wino (dostarczają satyrowie) |
| Hera (P) | Orchard of Hera | 2 zagajniki pomarańczy; przegania Zeusa; wyższa dzietność | Corral, Orange Tenders' Lodge | towar w Agorach (tylko stoiska z pracownikami) |
| Atlas (P) | Pillar of Atlas | kamieniarze pracują szybciej; gildie wysyłają więcej rzemieślników | Masonry Shop, Black Marble Workshop | przyspiesza budowę: materiały w drodze pojawiają się od razu, przez krótki czas rzemieślnicy pracują bez nowych dostaw |

Apollo jest **jedynym bogiem, który zabija potwory** (Z:129).

## Zadania (Quests, Z:124, P:24)

- Przyjazny bóg (czasem bóg nieobecny w przygodzie) prosi o wykonanie zadania i podaje warunki.
- **Zadania wykonują wyłącznie herosi** (P:24). Heros musi być w mieście (zbudowany i wyposażony Hero's Hall).
- Wysłanie: zakładka przeglądu (nazwa boga), przycisk Goals, albo panel Hero's Hall.
- Zadania nie mają terminu (Z:164).
- Nagroda: pomnik God Statue w jednym z dwóch rozmiarów, wybranym przez boga (Z:156).

## Bóg wrogi: ataki (Z:125-127, P:22)

Wrogi bóg atakuje osobiście albo, gdy jest zajęty, wysyła potwora. Ponadto "zawsze ma kilka sztuczek w zanadrzu".

| Bóg | Cel ataku |
|---|---|
| Zeus | niszczy Palace, potem inne budynki; potrafi rzucić klątwę każdego boga; **kupcy nie przybywają do przeklętego miasta** |
| Poseidon | Trireme Wharf, Urchin Quay, Fishery przeklęte (nieczynne do zdjęcia klątwy); statki handlowe nie przypływają, dopóki jest w mieście |
| Hades | **wszyscy chodzicy w mieście trafiają do podziemi** (puste ulice); przeklina Foundry i Mint |
| Demeter | niszczy uprawy, zabija owce i kozy |
| Athena | zostawia Walls i Towers bez obsady; niszczy przemysł oliwny |
| Artemis | pełny atak jej i Amazonek; zabija myśliwych; wzmacnia watahy wilków |
| Apollo | zaraza; kultura przestaje działać na domy; (dodatek) przeklina budynki nauki |
| Ares | wkracza z armią Sparti |
| Hephaestus | podpala budynki; metal w Sculpture Studio i Armory staje się bezużyteczny |
| Aphrodite | chodzicy i część mieszkańców odchodzą za nią z miasta |
| Hermes | kradnie pieniądze; drogi niebezpieczne dla karawan |
| Dionysus | zabiera chodziców z ulic; przeklina winorośle |
| Hera (P) | "dziwne rzeczy z ludźmi" [NIEJASNE]; zabija połowę bydła; niszczy uprawy pomarańczy; obniża opinię wszystkich przywódców o graczu |
| Atlas (P) | zamienia w kamień ludzi na swojej drodze; przeklina Masonry Shop i Black Marble Workshop (niszczy marmur na miejscu, wstrzymuje dostawy); przeklina Artisans' Guild (pracujący rzemieślnik ginie, reszta zostaje w gildii) |

[LUKA: co sprawia, że bóg staje się wrogi; skrypt scenariusza czy reakcja na działania gracza. Instrukcja mówi o "urazie" (perceived slight)]

## Potwory (Z:127-129, P:23-24)

Potwór zabija ludzi i niszczy budynki. Jest sługą boga: w służbie boga przyjaznego pomaga, wrogiego atakuje; czasem działa samodzielnie.

| Potwór | Pan | Heros-pogromca | Gra |
|---|---|---|---|
| Cyclops | Zeus | Odysseus | Z |
| Kraken | Poseidon | Perseus | Z |
| Cerberus | Hades | Hercules | Z |
| Medusa | Demeter | Perseus | Z |
| Hydra | Athena | Hercules | Z |
| Calydonian Boar | Artemis | Theseus | Z |
| Scylla | Apollo | Odysseus | Z |
| Dragon | Ares | Jason | Z |
| Talos | Hephaestus | Jason | Z |
| Hector | Aphrodite | Achilles | Z |
| Minotaur | Hermes | Theseus | Z |
| Maenads | Dionysus | Achilles | Z |
| Sphinx | Hera | Atalanta | P |
| Chimera | Atlas | Bellerophon | P |
| Echidna | brak (zawsze samodzielna) | Bellerophon | P |
| Harpies | brak (zawsze samodzielne) | Atalanta | P |

Sposoby pokonania potwora: duża liczba żołnierzy elity, Apollo, **heros**.

## Herosi (Z:129-131, P:24-25)

- Herosi przychodzą, gdy miasto ma dla nich **ważne zadanie** (potwór, quest) i **jest ich godne**.
- Hero's Hall: buduje się, gdy jest powód; droga wymagana. Panel pokazuje **listę wymagań** do spełnienia; spełnione są odhaczane. [LUKA: wymagania poszczególnych herosów]
- Przybyły heros zamieszkuje w Hall i sam zajmuje się potworem.
- Pokonanie potwora w mieście daje pomnik Hero.
- Dodatek, wymagania w Atlantydzie: Hercules żąda edukacji przez różnych naukowców i dostępu do Hippodrome; Jason żąda floty fregat (liczba w panelu).

## Widoki

Zakładka mitologii: sanktuaria i statusy, atakujący bogowie i potwory (kliknięcie centruje). Szczegóły: postęp budów, szansa modlitwy, przyjaźni bogowie bez sanktuarium, herosi i ich działania, wrodzy bogowie i ich potwory. See Immortals.
