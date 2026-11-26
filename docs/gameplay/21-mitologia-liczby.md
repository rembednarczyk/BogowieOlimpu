# 21. Mitologia: liczby i reguły uzupełniające

Źródła: S-004 (`sanctuaries`, `gods`, `heroes`, `quests`, `monsters`, `trivia`), S-005 (Vriesea "On the Building of Sanctuaries", Nightwolf "Odds & Ends"). Dane: `data/gods.json`, `data/gods_rules.json`, `data/heroes.json`, `data/quests.json`.

## Limity

- **Maks. 4 sanktuaria w mieście** (S-004 trivia). Sanktuarium można zburzyć i postawić inne. Kolonia ma własny limit i tę samą listę bogów co miasto macierzyste.

## Sanktuaria (zamyka L-050, L-051)

| Bóg | Rozmiar | Marmur na start | Drachmy | Marmur | Drewno | Rzeźby | Pracownicy |
|---|---|---|---|---|---|---|---|
| Zeus | 24×16 | 48 | 2920 | 145 (P: 137) | 28 (P: 24) | 22 (Atl: 26) | 75 |
| Poseidon (Promontory) | 21×14 | 37 | 2480 | 103 | 20 | 12 | 75 |
| Poseidon (Citadel, Atl) | 20×20 | 32 | 2080 | 231 | 52 | 2 + 16 orichalku | 80 |
| Hades | 21×14 | 37 | 1320 | 140 (132) | 28 (24) | 4 | 75 |
| Hera | 20×12 | 30 | 1760 | 88 | 20 | 6 | 65 |
| Demeter | 18×12 | 27 | 840 | 110 | 20 | 10 | 60 |
| Athena | 16×12 | 24 | 2160 | 67 (55) | 22 (16) | 8 | 60 |
| Artemis | 16×10 | 20 | 1520 | 63 | 20 | 6 | 60 |
| Apollo | 16×10 | 20 | 920 | 88 (76) | 22 (16) | 6 | 60 |
| Atlas | 14×14 | 25 | 1760 | 76 | 20 | 4 | 55 |
| Ares | 13×8 | 13 | 960 | 44 (40) | 14 (12) | 2 | 50 |
| Hephaestus | 13×8 | 13 | 760 | 48 (44) | 14 (12) | 6 | 60 |
| Aphrodite | 14×6 | 11 | 720 | 41 (37) | 14 (12) | 6 | 50 |
| Hermes | 12×6 | 9 | 640 | 39 (35) | 14 (12) | 2 | 45 |
| Dionysus | 10×6 | 8 | 400 | 35 (31) | 14 (12) | 2 | 45 |

Wartości w nawiasach: dodatek ma mniejsze wymagania części sanktuariów. Promontory tylko w przygodach greckich, Citadel tylko w atlantydzkich.

## Fazy budowy sanktuarium (S-005, Vriesea)

1. **Płytki.** Rzemieślnik prosi o materiał na 1 płytkę = ½ płyty marmuru. Wóz wiezie 1 płytę na kurs, więc liczba wysłanych płyt = ceil(prośby / 2). Po ułożeniu rzemieślnik musi przejść do następnej pozycji, zanim poprosi o kolejną płytę. **Po 8 płytkach rzemieślnik znika i wraca z gildii** (przerwa). Liczba płytek zależy od sanktuarium (Artemis 38, Zeus 170).
2. **Konstrukcja.** Pracuje **tylko 1 rzemieślnik**. 3 prośby, każda np. 8 marmuru + 4 drewna (Artemis, Zeus) lub 4 + 2 (Aphrodite). Pierwsze użycie drewna (rusztowania).
3. **Rzeźby, posąg, ołtarze.** Rzeźby po 1 na wóz, bez limitu wozów naraz. Posąg boga: 1 rzemieślnik, 2 prośby po 8 marmuru + 4 drewna; 1 lub 2 posągi. Ołtarz: 4 marmuru.

Materiały jadą tylko drogą z magazynu lub budynku handlowego połączonego z budową; wozy przechodzą przez Roadblock. Wybór magazynu przy kilku źródłach jest nieprzewidywalny (obserwacja). Dobrze zorganizowana budowa: Artemis w 5 mies., Zeus w 12 mies.

## Działanie bogów

- **Sanktyfikacja**: budynek daje **4× normalnej produkcji** przy następnym kursie (Demeter, Athena, Artemis, Poseidon, Hades, Hephaestus, Dionysus). Apollo: budynki kultury wysyłają **2× więcej** wykonawców.
- Zeus może sanktyfikować dowolny budynek; Aphrodite domy (więcej mieszkańców).
- Sanktuaria z gajami: Athena 4 drzewa oliwne, Dionysus 4 winorośle, Hephaestus 4 złoża miedzi, Hades złoża srebra, Hera 2 zagajniki pomarańczy, Demeter tworzy łąkę wokół sanktuarium.
- Atlas: gildie wysyłają **2 rzemieślników** zamiast 1.
- Modlitwa Artemis: 24 ładunki wieprzowiny do spichlerzy.
- Okres odnowienia modlitwy zależy od boga: Hermes najkrótszy (miesiące), Ares najdłuższy (lata). [LUKA L-053 częściowo]
- Przed armiami bronią: Athena, Hera, Zeus. Herosi zawsze walczą z najeźdźcami.
- Dary bogów nie liczą się do celów produkcyjnych scenariusza.
- Czas trwania klątwy zależy od trudności (S-004 walkthroughs). [LUKA L-054b: wartości]

### Gniew: uzupełnienia S-004

- Poseidon: niszczy też Fishery, Urchin Quay, Piery i statki; zatrzymuje handel morski.
- Hades: przeklina też Palace; zabija wszystkich chodziców.
- Hera: zamienia ludzi w krowy; przeklina Corral.
- Apollo: przeklina też Infirmary.
- Hermes: zatrzymuje handel lądowy.
- Atlas: uniemożliwia wydobycie marmuru (białego i czarnego).

## Herosi: wymagania (zamyka L-056)

| Heros | Wymagania Hero's Hall |
|---|---|
| Achilles | 32 zbroi; ≥ 3 kompanie hoplitów (Atl: włóczników); sanktuarium Athena lub Hephaestus; brak niepokojów; 16 wina |
| Hercules | doskonały dostęp Hall do kultury (Atl: nauki); wygrane dowolne igrzyska (Atl: działający hipodrom); doskonały dostęp miasta do gimnazjonów (Atl: obserwatoriów); 1500 mieszkańców; 32 wina |
| Jason | 3 triery (Atl: fregaty); 2 kompanie jazdy (Atl: rydwanów); 8 koni; 64 żywności; 16 wina |
| Odysseus | doskonała popularność; doskonałe zdrowie miasta; 8 domów elity; 32 oliwy; 16 wina |
| Perseus | 2 sanktuaria z: Athena, Hermes, Zeus, Hades; 3000 dr (**pobierane** przy przyzwaniu); 16 wełny; 6 rzeźb |
| Theseus | Hall blisko Palace; dobra atrakcyjność wokół Hall; Hall chroniony murami; 32 marmuru; 16 wina |
| Atalanta | sanktuarium Artemis; działający Stadium (Atl: Museum); 32 mięsa; 32 drewna; 8 kompanii |
| Bellerophon | 15 koni w mieście; doskonałe pokrycie podatkowe; 10 000 dr w skarbcu (**nie pobierane**); 24 brązu (Atl: orichalku); 24 wina |

- Hero's Hall staje się dostępny, gdy pojawi się powód (np. komunikat o ataku potwora); potwór przybywa ok. 4 mies. po komunikacie (S-004 walkthroughs, obserwacja).
- Zburzony Hall można odbudować; może być potrzebne ponowne przyzwanie.

## Zadania bogów

Każdy bóg ma 2 zadania (indeks 0 i 1) z tradycyjnym herosem; projektant scenariusza może wskazać innego. Lista w `data/quests.json`. Tytuły zadań redagujemy sami (R-002), mechanika: heros wyrusza do miejsca zaczarowanego na mapie świata.

## Potwory

- Bez herosa potwora da się zabić tylko wielkim kosztem (rabble na wzgórzach, wieże, dziesiątki lat); Apollo zabija każdego potwora.
- Ustawienia potwora w scenariuszu: punkt wejścia, siła, agresywność (pasywny / aktywny / agresywny), cel ataku (domyślnie żywność; obywatele, wojsko, zasoby). Potwór wraca do punktu startu (S-004 scendesign). Zamyka L-057 częściowo; statystyki walki potworów: L-057b.
