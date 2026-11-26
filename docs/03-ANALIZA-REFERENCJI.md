# Analiza referencji

Notatki analityczne: dlaczego mechaniki referencji działają i gdzie gracze napotykają tarcie. Same reguły opisuje `docs/gameplay/` (źródła S-001, S-002).

## Rodowód gatunku

Seria citybuilderów "z chodzikami" z przełomu wieków: Caesar III, Pharaoh (z Cleopatrą), Zeus (z Poseidonem), Emperor. Wspólny trzon: siatka izometryczna, domy ewoluujące od potrzeb, chodzicy z zasięgiem, łańcuchy produkcji, scenariusze z celami.

Notatki projektantów referencji (Z:183-187) mówią wprost o kierunku względem poprzedników: gra miała być **mniej poważna, krótsza w rozgrywce i bardziej wyrozumiała dla gracza**, z mitologią w centrum, a gracz jest niezależnym władcą, który nie odpowiada przed nikim.

Współczesne tytuły warte przejrzenia pod kątem UX: Pharaoh: A New Era, Nebuchadnezzar, Builders of Egypt, Anno, Banished, Farthest Frontier, Manor Lords. Open-source'owe reimplementacje gier z tej rodziny (Julius, Augustus, Akhenaten) mogą pomóc zamknąć luki w zachowaniu chodzików; ich kod jest na GPL (patrz `PRAKTYKI.md`, rozdział 1).

## Co wyróżnia referencję (zweryfikowane z instrukcją)

1. **Religia na poziomie miasta.** Domy nie potrzebują świątyń. Bogowie działają przez sanktuaria, ofiary, modlitwy, sanktyfikację, wizyty i gniew (Z:115-133).
2. **Choroby i przestępczość istnieją, ale są łagodniejsze**: zaraza z kwarantanną, przestępcy celują głównie w domy elity (Z:84-89).
3. **Dwie klasy mieszkańców**: lud (7 poziomów, rabble) i elita (4 poziomy, hoplici i jeźdźcy z ekwipunku dostarczanego przez Agorę) (Z:29-35).
4. **Agora ze stoiskami**: vendorzy zaopatrują Agorę, peddler roznosi; Agora jest prywatna, miasto nie zarabia na sprzedaży (Z:69-73).
5. **Żywność zróżnicowana mapą**: trzy uprawy z kalendarzem żniw, sery, ryby, jeżowce, dziczyzna (Z:39-48).
6. **Bogowie z pełnym repertuarem**: hierarchia, sanktuaria wymagające marmuru, drewna i rzeźb, ofiary ze zwierząt, sanktyfikacja budynków, modlitwy, wyrocznie, zadania, ataki z celem zależnym od domeny, potwory (Z:115-133).
7. **Herosi** przyzywani spełnieniem wymagań Hero's Hall, każdy pokonuje dwa konkretne potwory (Z:129-131).
8. **Kultura z czterech rodzajów** (filozofia, atletyka, dramat, zawody) i czteroletni cykl Igrzysk Panhelleńskich (Z:105-113).
9. **Mapa świata z pamięcią**: opinia przywódców z historią interakcji, prośby z terminami, dary z malejącą wartością, ataki i rajdy z reakcją grup (Z:167-178).
10. **Kolonie** z zamrożonym miastem macierzystym (Z:17).
11. **Dodatek**: druga kultura z nauką w miejsce kultury, hipodromem, piramidami, fregatami z "ogniem", nowymi bogami, potworami i herosami (P:3-29).

Korekta wcześniejszej wersji tego pliku: dodatek **nie** wprowadza "przygotowania terenu pod zabudowę"; to było błędne założenie.

## Esencja do zachowania

- Obserwowanie ewolucji domu po zaspokojeniu potrzeb; dotkliwy regres (Townhouse bez żywności spada do Hut).
- Planowanie bloków mieszkalnych otoczonych usługami i ozdobami.
- Sanktuarium jako długi, widowiskowy projekt.
- Przybycie boga lub herosa jako wydarzenie.
- Mapa świata, która pamięta.
- Widoki specjalne, które pozwalają zrozumieć miasto.

## Tarcie do zbadania (kandydaci na rozbudowę po wydaniu 1:1)

Ponieważ R-001 każe odwzorować mechanikę 1:1, poniższe zapisujemy jako pomysły na później (każdy wymagałby reguły i wpisu w `ODSTEPSTWA.md`):

- Chodzicy losowi, którzy omijają domy; brak kontroli trasy poza Roadblock.
- Pułapka importu: kupiec patrzy tylko na zapas w swoim budynku (Z:78).
- Tracenie niezebranych owoców po żniwach i wypas blokujący łąkę.
- Mikrozarządzanie rozkazami magazynów.
- Niejasne wymagania herosów do czasu postawienia Hero's Hall.
