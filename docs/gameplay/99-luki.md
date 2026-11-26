# 99. Luki w specyfikacji

Dane potrzebne do implementacji 1:1, których źródła nie podają w całości. Priorytet: **P0** blokuje prototyp symulacji, **P1** vertical slice, **P2** wydanie. Status: **zamknięta** (źródło i miejsce), **częściowa**, **otwarta**.

Aktualizacja 2026-11-26 po S-004, S-005 (Zeus Heaven, forum) oraz S-006..S-008 (Julius, Akhenaten, eZeus). Oznaczenia [RODZINA] i [eZeus]: R-014.

## Rdzeń symulacji

| ID | Luka | P | Status |
|---|---|---|---|
| L-001 | Wybór kierunku spacerującego losowego na skrzyżowaniu | P0 | zamknięta [RODZINA]: doc 24 |
| L-002 | Zasięgi spacerujących losowych | P0 | zamknięta: S-004, doc 17 |
| L-003 | Prędkości | P0 | zamknięta: S-004, S-005, doc 17 |
| L-004 | Model czasu | P0 | zamknięta: 51 ticków/dzień, 16 dni/miesiąc, kafel = 15 kroków (S-005, S-007), doc 17, 24 |
| L-005 | Rozmiary budynków | P0 | zamknięta: `building_stats.json` |
| L-006 | Koszty budowy | P0 | zamknięta: `building_stats.json` |
| L-007 | Pracownicy | P0 | zamknięta: `building_stats.json` |
| L-008 | Sektory zatrudnienia i skala priorytetów | P0 | zamknięta: 8 sektorów (S-008 klasa A), przydział [eZeus], doc 24 |
| L-009 | Pojemności domów | P0 | zamknięta: `housing.json` |
| L-010 | Progi atrakcyjności domów | P0 | zamknięta: AppD/AppE per trudność, `housing.json` |
| L-011 | Wymagana kultura | P0 | zamknięta: punkty Ent, doc 18 |
| L-012 | Zużycie dóbr | P0 | zamknięta: doc 18 |
| L-012b | Czas "życia" punktu kultury i wizyty usługi w domu | P0 | zamknięta [RODZINA]: 96 dni, promień obsługi 2, doc 24 |
| L-013 | Atrakcyjność budynków | P0 | zamknięta: algorytm i wartości, doc 20 |
| L-014 | Tempo produkcji | P0 | zamknięta: doc 19 |
| L-015 | Pojemności magazynów | P0 | zamknięta: 32 ładunki, doc 19 |
| L-016 | Próg "Too Distant" dla vendorów | P1 | zamknięta [RODZINA]: 40 kafli (pomiar Zeusa 37), doc 24 |
| L-017 | Formuła migracji | P0 | zamknięta [RODZINA]: doc 24 |
| L-018 | Formuła popularności | P1 | częściowa: struktura [RODZINA] i składniki, doc 24; wagi płac, długu i mobilizacji to założenia |
| L-019 | Kolejność dostaw tam, gdzie instrukcja milczy | P1 | częściowa: K-005 |
| L-020 | Ryzyko zawalenia | P1 | zamknięta: 60 / damage_risk miesięcy |
| L-020b | Próg zapłonu, wzór obniżania ryzyka przez superintendenta | P1 | częściowa: zerowanie w promieniu 2 [RODZINA]; próg pożaru = założenie, doc 24 |
| L-021 | Higiena, zaraza, kwarantanna, śmiertelność | P1 | częściowa: DRI per dom (`housing.json`) |
| L-022 | Niepokoje i eskalacja przestępców | P1 | częściowa: CRI/CRIp per dom |

## Ekonomia

| ID | Luka | P | Status |
|---|---|---|---|
| L-030 | Podatki | P1 | zamknięta: wzór i tabele, doc 18 |
| L-031 | Płace | P1 | zamknięta: doc 18 |
| L-032 | Ceny towarów | P1 | częściowa: przykładowe ceny (doc 19); ceny ustala scenariusz |
| L-033 | Pojemność karawan i statków, częstotliwość | P1 | zamknięta [RODZINA]: 8/12 ładunków, 4/30 dni, doc 24 |
| L-034 | Lata długu do przegranej | P2 | częściowa: założenie 3 lata, doc 24 |
| L-035 | Okno Undo | P2 | częściowa: okno liczy się w czasie gry, "kilka chwil" (S-010; poradnik: cofnięcie wymaga odpauzowania); rozbiórkę da się cofnąć (S-005, średnia pewność). Dokładny czas nieznany; [ZAŁOŻENIE] 1 dzień (51 ticków), cofa komendy budowy jednego ticku |
| L-036 | Wydajność Mint | P1 | zamknięta: doc 19 (próg K-009 otwarty) |
| L-037 | Zwierzęta na budynek | P1 | zamknięta: 8 |
| L-038 | Odrost wypasionej łąki | P2 | otwarta |
| L-039 | Wzrost drzew, żniwa | P1 | zamknięta: doc 19 |
| L-040 | Wpływ odległości od złoża | P1 | zamknięta: doc 19 |

## Religia i mit

| ID | Luka | P | Status |
|---|---|---|---|
| L-050 | Materiały i koszty sanktuariów | P1 | zamknięta: doc 21 |
| L-051 | Rozmiary sanktuariów | P1 | zamknięta (K-021 otwarty) |
| L-052 | Częstotliwość ofiar, czas do utraty łaski | P1 | otwarta; [ZAŁOŻENIE] ofiara co 32 dni od ukończenia, utrata łaski po 192 dniach bez ofiary; kapłan bierze najbliższą owcę lub kozę, bez zwierząt 1 ładunek żywności ze spichlerza |
| L-053 | Szansa modlitwy, cooldowny | P1 | częściowa: kolejność bogów (doc 21); [ZAŁOŻENIE] modlitwa zawsze wysłuchana po odnowieniu: Hermes 48 dni, Ares 384, pozostali 96 (`data/gods_rules.json`) |
| L-054 | Sanktyfikacja | P1 | zamknięta: 4× produkcji przy kolejnym kursie, Apollo 2× wykonawców |
| L-054b | Czas trwania klątw per trudność | P1 | otwarta; [ZAŁOŻENIE] 45 dni na każdym poziomie (eZeus ok. 2,8 mies., doc 24); przeklęty budynek nie zatrudnia i nie pracuje |
| L-055 | Co wywołuje wrogość boga | P1 | częściowa: wrogość i ataki ustawia scenariusz (doc 23) |
| L-056 | Wymagania herosów | P1 | zamknięta: doc 21 |
| L-057 | Potwory w scenariuszu | P1 | częściowa: ustawienia scenariusza (doc 21, 23) |
| L-057b | Statystyki walki potworów, herosów, bogów | P1 | otwarta; [ZAŁOŻENIE] potwór 2000 PŻ × siła, atak 40, pancerz 10; heros 3000 PŻ, atak 60, pancerz 20 (`data/enemies.json`, nacje `monster` i `hero`); bogowie bez postaci |
| L-058 | Treść przepowiedni | P2 | otwarta (treść piszemy sami) |

## Wojsko i świat

| ID | Luka | P | Status |
|---|---|---|---|
| L-060 | Statystyki jednostek | P1 | zamknięta: doc 22, `enemies.json` |
| L-061 | Odsetek służących | P1 | zamknięta: %S i #S w `housing.json` |
| L-062 | Morale | P1 | częściowa [RODZINA]: doc 24 |
| L-063 | Bitwy za granicą | P1 | częściowa [eZeus]: doc 24; [ZAŁOŻENIE] tarcza siły militarnej miasta = 10 żołnierzy |
| L-064 | Skala opinii przywódców | P1 | częściowa [eZeus]: 0-100, doc 24 |
| L-065 | Trybut, okup | P1 | częściowa: okup zamknięty (doc 22); trybut ustawia scenariusz (doc 23); [ZAŁOŻENIE] bez danych scenariusza 200 dr za monetę siły ekonomicznej |
| L-066 | Jednostki atlantydzkie | P2 | częściowa: statystyki wrogów atlantydzkich, poziomy domów jak greckie (S-004) |
| L-067 | Igrzyska: start i szansa | P2 | częściowa [eZeus]: doc 24 |

## Atlantyda

| ID | Luka | P | Status |
|---|---|---|---|
| L-080 | Domy atlantydzkie | P2 | częściowa: wymagania jak greckie, nauka zamiast kultury 1:1 (S-004, S-008); Hut i Shack z greckimi modelami; nazw poziomów brak (nadamy własne, R-002) |
| L-081 | Hipodrom | P2 | zamknięta: doc 18 |
| L-082 | Piramidy | P2 | zamknięta: `monuments.json`, doc 20 |
| L-083 | Nowe struktury estetyczne | P2 | zamknięta: 12 struktur, `buildings.json` |

## Inne

| ID | Luka | P | Status |
|---|---|---|---|
| L-090 | Katastrofy | P2 | zamknięta: zdarzenia skryptowe scenariusza z punktami na mapie (doc 23) |
| L-091 | Poziom trudności | P2 | zamknięta: 5 poziomów i tabele (doc 18, 20) |
| L-092 | Progi pomników Population, Happiness | P2 | częściowa [eZeus]: Population, doc 24 |
| L-093 | Struktura przygód i celów | P1 | częściowa: doc 23; pełna lista typów celów L-093b otwarta; nasz format przygody w `scenarios/README.md` (cele: populacja, skarbiec, sanktuarium, domy, wasal; zdarzenia: inwazja, prośba, dar, relacja) |
| L-094 | Budowa przy pustym skarbcu (czy gra blokuje budowę, gdy brakuje drachm, i czy dopuszcza dług) | P1 | otwarta; [ZAŁOŻENIE] w rdzeniu: budowa zawsze dozwolona, skarbiec może zejść poniżej zera |
| L-095 | Przechodniość terenu poza drogą (plaża, las) dla ruchu na przełaj | P2 | otwarta; [ZAŁOŻENIE] plaża przechodnia, las nie (Z:17-18 mówi tylko o bagnie i skałach) |
| L-096 | Ustawienie budynków nabrzeżnych (czy obrys leży na lądzie, czy częściowo na wodzie; co znaczy "prosty brzeg") | P1 | częściowa: prosty, gładki odcinek brzegu, nie w wąskich kanałach i zatoczkach, łodzie potrzebują miejsca (S-001, S-009); rozmiary 2×2 i 3×3 potwierdzone (S-010). Położenie obrysu względem linii brzegu nieznane; [ZAŁOŻENIE] cały obrys na lądzie, jeden pełny bok przy wodzie, pas wody o kafel za oba narożniki |
| L-097 | Przeprawa: szerokość, największa rozpiętość, próg mała/duża | P1 | częściowa: szerokość 1 kafla, koszt za kafel (S-010, średnia pewność); typ dobierany sam: wąska woda daje małą przeprawę, pod którą statki nie przepłyną (S-001); dwie przeprawy nie stoją obok siebie. Rozpiętość maksymalna i próg nieznane; [ZAŁOŻENIE] do 8 kafli wody (jak most w grze z tej samej rodziny), próg mała/duża przy statkach (D) |
| L-098 | Kafel powrotu fontanny i lecznicy | P2 | zamknięta (średnia pewność): diagramy S-004 (`/units/`) pokazują kafel nad górnym narożnikiem budynku, stykający się z nim tylko po przekątnej; w naszej siatce to przekątna od narożnika północno-zachodniego (ten sam narożnik, od którego zaczyna się szukanie wyjścia) |
| L-100 | Dystrybucja w Agorze: ile ładunków bierze vendor, kiedy stoisko wysyła vendora, ile peddler zostawia w domu | P1 | otwarta; [ZAŁOŻENIE] vendor bierze do 4 ładunków (jak doręczyciel magazynu, doc 19), wychodzi poniżej 200 jednostek w stoisku; dom dostaje zapas na 2 miesiące żywności bieżącego albo następnego poziomu (większa wartość), 4 jednostki wełny, oliwy, wina, 1 zbroję, 1 konia, gdy poziom bieżący lub następny ich wymaga |
| L-101 | Hodowla i sady: rozmnażanie stad, przydział zwierząt do budynku, praca ogrodnika, próg dojrzałości do zbioru, zasięg dostaw z magazynu do przetwórni | P1 | otwarta; [ZAŁOŻENIE] zwierzę dołącza do najbliższej zgrzeblarni lub mleczarni z wolnym miejscem w stadzie 8; stado od 2 do 7 sztuk dostaje młode co 32 dni, młode daje pierwszy ładunek po 12 miesiącach; jeden ogrodnik na miesiąc odwiedza najdłużej nieodwiedzane drzewo i wszystkie drzewa w promieniu 2 kafli; zbieracze biorą rośliny dojrzałe w co najmniej 33 %; magazyn wysyła do 4 ładunków do przetwórni w jej zasięgu dostaw; jeden pasterz naraz |
| L-102 | Wydobycie i warsztaty: czas pracy przy złożu, liczba robotników przy niepełnej obsadzie, zapas przy budynku, kamieniarnia, mennica, pojemność zbrojowni | P1 | otwarta; [ZAŁOŻENIE] 3 robotników × obsada, kurs 25 jednostek, 7 dni pracy przy złożu (dobrane testem: 13/8/6 ładunków przy złożu w 2/10/20 kaflach, pomiar S-005: 13/10/7-8), jeden robotnik na kafel złoża, praca staje przy 4 ładunkach czekających; kamieniarnia 12/rok, gdy kamieniołom jest w 20 kaflach (cykl bloków po 8 płyt pominięty); mennica 200 srebra na 100 dr w 24 dni prosto do skarbca (pomiar 600-700/rok, u nas ok. 500); zbrojownia mieści 3 brązy, pracownia rzeźb 4; las się nie wyczerpuje |
| L-103 | Łowiectwo i rybołówstwo: liczba myśliwych, czas polowania, czy dziki zabijają myśliwych, czas budowy łodzi, cykl połowu, ile jeleni naraz, łowisko jeżowców | P1 | otwarta; [ZAŁOŻENIE] jeden myśliwy naraz, 3 dni polowania, 1 ładunek ze zwierzęcia (wynik testu: 13/rok przy stadzie 7 z przyrostem co 26 dni, S-005: ok. 12); dziki nie atakują; łódź gotowa po 16 dniach, kurs 13 dni na łowisku i 1 ładunek (wynik: 11/rok blisko, 7 przy 20 kaflach, S-005: 13 i 8-10); jelenie do 7 w punkcie; nabrzeże jeżowców bez warunku tego samego brzegu; punkty odradzania i łowiska jako symbole mapy |
| L-104 | Ryzyka: liczby pożaru, okno gaszenia, zaraza, przestępczość | P1 | częściowa (porównanie 2026-11-26 z S-001, S-010, S-008, S-006, S-007): zawalenie 60/damage_risk miesięcy (S-004); pożar według struktury oryginału z S-007 (1 dzień na 8, ryzyko × 1-5; chata ok. 10 miesięcy, S-007 podaje pomiar oryginału ok. 10); higiena miasta jak w rodzinie (S-006), zaraza tylko poniżej 40; umiera 1 na 3 (S-010); zaraza obejmuje domy w okolicy (S-010; promień 5 i 50 % z S-008); eskalacja przestępców niezadowolony → rozbójnik → złodziej → wandal (S-010); przestępcy tylko z domów o zadowoleniu < 50 (S-006); niezadowoleni ignorują blokady (S-010); zarządca biegnie do pożaru (Z:90, zasięg 25 z S-007); higiena i bezpieczeństwo pierwsze przy równym priorytecie (S-010). Zostają założenia: próg zarazy (licznik DRI do 960), kwarantanna 3 miesiące, okno gaszenia 12 dni, rozbójnik zabija przechodnia i 5 mieszkańców najbliższego domu (S-008), najwyżej 3 ataki; wpływ fontanny i zarządcy na higienę (S-010 wspomina, liczb brak); chorzy spacerujący roznoszący zarazę (S-010) jeszcze nie |
| L-105 | Handel: pojemność budynku handlowego, prędkość kupców, położenie doku przy molo, czy jeden kupiec naraz na budynek, ceny (scenariusz) | P1 | otwarta; [ZAŁOŻENIE] budynek handlowy mieści 32 ładunki; kupcy chodzą jak spacerujący; dok 2×2 przed środkiem boku od wody; jeden kupiec naraz na budynek (interwał 4 / 30 dni liczony od odjazdu); bez punktu wejścia kupiec pojawia się przy budynku; partnerzy i ceny ze scenariusza (mapa świata: moduł D) |
| L-106 | Stadnina i fabryka rydwanów: położenie wybiegu 3×3 względem budynku 4×4, pojemność wejścia (pszenica w stadninie, drewno i konie w fabryce), ile koni niesie stajenny, co z koniem, gdy brak pszenicy na utrzymanie | P2 | otwarta; [ZAŁOŻENIE] wybieg przy górnej (lewej) krawędzi po stronie obrotu (0 wschód, 1 południe, 2 zachód, 3 północ); stadnina mieści 400 jednostek pszenicy; fabryka po 3 sztuki drewna i koni; stajenny niesie 1 konia; koń bez pszenicy nie ginie; 1 ładunek zbroi, konia lub rydwanu to 1 sztuka |
| L-107 | Wilki: wielkość watahy, tempo odnawiania, jak często polują, zasięg, kiedy pasterz chroni stado, czy atakują ludzi | P2 | otwarta; [ZAŁOŻENIE] według domyślnych wartości S-008 (eZeus): wataha 8, polowanie średnio raz na 20 dni, zasięg 60 kafli; nowy wilk co 26 dni (jak dziki); naraz poluje jeden wilk z watahy; pasterz lub koziarz tej samej zagrody w promieniu 5 kafli odstrasza wilka; bydła nie ruszają (byki, P:7); ludzi nie atakują (brak w instrukcji); wilk znika po powrocie do legowiska. Legowisko: symbol `w` na mapie |
| L-108 | Elita: tempo przybywania rodzin do domów elity (czy zależy od popularności i puli imigracji), wielkość grupy, wygląd idącego, zasięg i trasa szlachcica | P2 | otwarta; [ZAŁOŻENIE] Z:34 mówi tylko, że elita się wprowadza ("elite immigrants moved in"); każdy dom elity z wolnymi miejscami i bez rodziny w drodze dostaje grupę wypełniającą go, niezależnie od popularności; idą od punktu wejścia jak imigranci, bez wejścia wprowadzają się od razu; nadmiar elity nie staje się bezdomnym ludu; szlachcic spaceruje 384 ticki (najmniejsza rodzina zasięgu), zeruje ryzyko przestępczości jak strażnik |
| L-109 | Wojsko, etap 1: kolejność formowania kompanii, kompanie niepełne, położenie sztandarów wokół pałacu, szyk żołnierzy przy sztandarze, ilu pracowników ubywa z rabble w polu (doc 22: "żołnierz rabble = 6 osób" nie zgadza się z %S), spadek popularności po mobilizacji bez walki (Z:159) | P1 | otwarta; [ZAŁOŻENIE] kolejno jeźdźcy, hoplici, rabble, domy według id; niepełna kompania powstaje z reszty; sztandary na wolnym terenie w pierścieniach od 2 kafli wokół pałacu, co najmniej 3 kafle od siebie; szyk 4×2 przy sztandarze; każdy żołnierz rabble w polu to 6 pracowników mniej i 6 mieszkańców mniej po śmierci (S-004 `player_rules`, doc 22; liczba żołnierzy z %S zostaje, rozbieżność do sprawdzenia w grze); kompanie przeformowują się tylko, gdy wszystkie są w domach; mobilizacja bez wroga w mieście: −2 do zadowolenia na aktualizację jak dług (doc 24) |
| L-110 | Wojsko, etap 2: zasięg, z którego żołnierze i najeźdźcy ruszają na wroga, wybór celu, pościg, kiedy najeźdźcy zdobywają pałac, cel bez pałacu, punkt wejścia inwazji, wybór kolejności w ataku | P1 | otwarta; [ZAŁOŻENIE] walczą z najbliższym wrogiem w promieniu 6 kafli; cios wręcz z sąsiedniego kafla, strzał z zasięgu jednostki; trafienie `max(0, atak − pancerz)` (doc 24); pościg korygowany co 15 ticków; najeźdźca przy pałacu bez obrońców w promieniu 6 kafli zdobywa go (koniec bitwy, wynik 3); bez pałacu celem jest budynek najbliższy wejścia; wchodzą w punkcie `i`, bez niego przy wejściu lądowym; przy końcu wychodzą tam, skąd przyszli; najeźdźcy nie uciekają; morale −10 za śmierć, ucieczka przy 20, odbudowa +5 dziennie (L-062); trybut po kapitulacji i niszczenie budynków przez najeźdźców czekają na moduł świata |
| L-111 | Budowa sanktuarium: czy marmur startowy wlicza się do kolumny "Marmur", tempo pracy rzemieślnika, kolejność faz w modelu uproszczonym, dostawy z budynków handlowych, wstrzymanie budowy | P1 | otwarta; [ZAŁOŻENIE] marmur startowy schodzi z magazynów przy postawieniu, potem cała kolumna marmuru, drewna i rzeźb z doc 21; każdy rzemieślnik na budowie wbudowuje 1 dostarczony ładunek dziennie, kolejno marmur, drewno, rzeźby (daje Artemis ok. 3 mies. i Zeus ok. 7 mies. przy 1 rzemieślniku i stałych dostawach; S-005: 5 i 12 mies.); gildia wysyła 1 rzemieślnika do najbliższej budowy; wóz z magazynu raz dziennie na materiał, 4 ładunki marmuru lub drewna, rzeźba po 1; wóz bierze też z budynku handlowego importującego materiał (Z:117), nigdy towar na eksport; wstrzymanie budowy później; fazy Vriesea modelowane: płytki (marmur ponad 2 × drewno + 4), konstrukcja z posągami i ołtarzem z jednym rzemieślnikiem, rzeźby; po 8 płytkach rzemieślnik wraca do gildii; przy 3 gildiach Artemis 87 dni, Zeus 157 dni (S-005: 5 i 12 mies.); sanktuarium w budowie nie zatrudnia pracowników |
| L-112 | Działanie bogów: jak często i jak daleko bóg sanktyfikuje, ile dają dary modlitewne poza Artemidą, ile pieniędzy daje Hades, który towar daje Demeter | P1 | otwarta; [ZAŁOŻENIE] 1 budynek co 16 dni w promieniu 40 kafli od sanktuarium (Zeus: dowolny producent), premia 4× przy najbliższej produkcji (doc 21); dary po 24 ładunki (Artemis z S-004), Atena po 12 oliwek i oliwy, Demeter pszenica, Posejdon ryby; Hades 1000 dr; Apollo zeruje ryzyko zarazy i kwarantanny. Korzyści stałe zrobione: Hefajstos bez pożarów, Dionizos bez niepokojów, Afrodyta bez emigracji. Brak łaski (brak ofiar) wyłącza wszystko |
| L-113 | Świat: progi opinii dla słownych warunków próśb, ilość towaru z prośby, jednostka daru pieniężnego, skutki ataku i rajdu dla opinii, kiedy wasal przestaje płacić, łup rajdu dla trudności pośrednich, czy kapitulacja liczy się jako porażka | P1 | otwarta; [ZAŁOŻENIE] progi: dowolna 0, "dość zadowolony" i "powyżej przeciętnej" 50, "bardzo dobra" i "bardzo wysoko" 80, "wyjątkowo dobra" i "uwielbia" 90; prośba daje 8 ładunków od razu do magazynów albo 250 dr za monetę, odstęp 96 dni; dar pieniężny 500 dr = 1 jednostka; atak lub rajd: cel 0, atak na sojusznika −30 u sojuszników, atak na rywala +10 u rywali; wasal poniżej 30 nie płaci; odmowa trybutu (brak pieniędzy) −20; łup: B 200 %, M 150 %, H 100 %, T 75 %, O 50 % rocznego limitu; kapitulacja liczy się jak zdobycie miasta. Wartości w `data/diplomacy.json` |
| L-114 | Prośby miast: jak długo prośba po terminie czeka przed wycofaniem, ile daje spełnienie po terminie, czy prośba wraca co rok | P1 | otwarta; [ZAŁOŻENIE] wycofanie 96 dni po terminie z −15 opinii (doc 24), spełnienie w terminie +10 (doc 24), po terminie +5; zdarzenia scenariusza z `repeat` wracają co rok w tym samym miesiącu (doc 23), potwory nigdy |
| L-115 | Gniew bogów i katastrofy: ile budynków niszczy lub podpala bóg, które wybiera, ile osób odchodzi z Afrodytą, ile kradnie Hermes, siła armii Artemidy i Aresa, zasięg i skutki katastrof | P1 | otwarta; [ZAŁOŻENIE] Zeus: pałac i 3 kolejne budynki; Hefajstos: 3 pożary; Apollo: zaraza w 3 domach; Afrodyta: 10 % mieszkańców i spacerujący; Hermes: 25 % skarbca; Artemida: 16 Amazonek, pełne watahy, giną myśliwi; Ares: 16 wojowników Sparti; Hera: całe bydło, −10 opinii u przywódców; budynki wybierane po kolei według id (bez losowania); katastrofa niszczy budynki w promieniu, trzęsienie ziemi i lawa zamieniają teren w skałę. Liczby w `data/gods_rules.json` (`wrath_numbers`) |
| L-116 | Potwory i herosi: zachowanie potwora (jak długo niszczy, jak długo odpoczywa, co gdy brak celu jego rodzaju), dobra dla herosa (w Hall czy w magazynach), progi "doskonały" (popularność, zdrowie), wymagania jeszcze niesprawdzalne | P1 | otwarta; [ZAŁOŻENIE] potwór idzie do najbliższego celu swojego rodzaju (domyślnie magazyn z żywnością), niszczy go po 2 dniach, wraca do punktu startu i odpoczywa 16 dni; bez celu swojego rodzaju bierze najbliższy budynek; dobra herosa liczone z magazynów miasta, nie pobierane (poza 3000 dr Perseusza); "doskonała" popularność ≥ 80, zdrowie: higiena ≥ 80 i brak kwarantanny; triery, mury, igrzyska, stadion, hipodrom, pokrycie podatkowe i dostęp do kultury liczone jako niespełnione do czasu ich modułów; heros zjawia się od razu w Hall i sam idzie na potwora |
| L-117 | Fortyfikacje i triery: wytrzymałość murów, bramy i wieży, co robią wrogowie bez przejścia, ile strzela wieża, położenie drogi w bramie, koszt i czas budowy triery | P1 | otwarta; [ZAŁOŻENIE] mur 200, brama 800, wieża 600 punktów; najeźdźca bez drogi do celu idzie do najbliższej fortyfikacji i bije ją swoim atakiem; potwory przechodzą przez bramy; wieża na kwadracie 2×2 z murów ("podwójna grubość"), strzela jak 3 rabble (pocisk 6, zasięg 11); brama 5×3 z drogą przez środek krótszego boku, mury przy obu końcach dłuższego; triera z 4 drewna i 4 zbroi w 96 dni, jedna na stocznię. Wartości w `data/military.json` i `data/production.json` |
| L-118 | Zadania bogów: czas wyprawy herosa, czy heros może zginąć, rozmiar posągu, rodzaj pomnika przy kilku zdobytych naraz | P2 | otwarta; [ZAŁOŻENIE] wyprawa trwa 200 dni (tam i z powrotem, jak marsz armii, doc 24) i zawsze się udaje; po powrocie miasto ma do postawienia pomnik boga; pomniki pamiątkowe zdobywa się też za potwora pokonanego przez herosa i za podbój miasta (Z:131, Z:177); pomnik stawia się jako budynek `commemorative_monument` bez kosztu, każdy zdobyty raz |
| L-099 | Wysokość terenu | P1 | częściowa: poziomy całkowite do 15 nad terenem płaskim; rampy łączą teren różniący się o 1 poziom i stawia je edytor mapy, nie gracz; półspadek rozkłada różnicę na 2 kafle i też może mieć rampę (S-009); droga przeciągnięta przez rampę staje się schodami; po klifie nie da się budować dróg (S-005); skały do przejścia tylko przez przełęcze i schody (S-001). Nieznane: budynki na granicy poziomów, wpływ wysokości na atrakcyjność, wodę, pożar . W rdzeniu: poziomy 0-15, rampy o 1 poziom, klify blokują ruch, drogi i dostęp do drogi; [ZAŁOŻENIE] budynek stoi na jednym poziomie i nie na rampie; półspadek (rampa na 2 kafle) jeszcze niezaimplementowany |

## Kandydaci na kolejne źródła

- Pliki modelu gry (`figure_model`, pliki tekstowe w katalogu `Model`) albo ich opisy: rozstrzygną L-001, L-008, L-017, L-052, L-057b, L-062.
- Instrukcja edytora przygód (PDF z płyty).
- Strona Pecunii o narzędziach i formatach plików (link z ZeusMapper; obecnie niedostępna z tego środowiska).
- Reimplementacje gier z tej rodziny (Julius/Augustus, Akhenaten) jako źródło wiedzy o algorytmie spacerujących (GPL, tylko do lektury).
