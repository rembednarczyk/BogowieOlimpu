# Specyfikacja rozgrywki (referencja 1:1)

Ten katalog opisuje mechaniki gry referencyjnej i dodatku tak, jak przedstawiają je oficjalne instrukcje. Zgodnie z regułą R-001 jest to **domyślna specyfikacja Bogów Olimpu**: implementujemy 1:1, a zmiany wprowadzamy później, świadomie, przez nowe reguły.

## Źródła i cytowanie

| Kod | Źródło |
|---|---|
| `Z:nn` | S-001, instrukcja gry podstawowej (Zeus: Master of Olympus), strona drukowana `nn` |
| `P:nn` | S-002, instrukcja dodatku (Poseidon), strona drukowana `nn` |
| `S-004` | portal Zeus Heaven (zeus.heavengames.com): tabele modelu gry, pomiary |
| `S-005` | wątki forum City Builders z pomiarami społeczności |

Strony podajemy według numeracji drukowanej w instrukcji (tej ze spisu treści).

## Oznaczenia

- **[LUKA]** instrukcja nie podaje wartości lub reguły. Pełna lista: [99-luki.md](99-luki.md).
- **[NIEJASNE]** instrukcja jest niejednoznaczna albo wewnętrznie sprzeczna; opisujemy przyjętą interpretację.
- **[ZAŁOŻENIE]** robocza decyzja projektowa do czasu uzupełnienia luki.
- Nazwy referencyjne (np. `Growers' Lodge`) podajemy po angielsku jako identyfikatory. Nazwy polskie są robocze; finalne nazewnictwo przechodzi przez `docs/02-ORYGINALNOSC.md`.

## Spis

| Plik | Zakres |
|---|---|
| [01-podstawy.md](01-podstawy.md) | Teren, budowa, drogi, czas, przygody, epizody, kolonie, warunki porażki |
| [02-chodziki.md](02-chodziki.md) | Chodziki losowe i celowe, blokady dróg |
| [03-mieszkancy.md](03-mieszkancy.md) | Domy zwykłe i elity, ewolucja, regres, migracja |
| [04-zywnosc.md](04-zywnosc.md) | Farmy, sady, hodowla, łowiectwo, rybołówstwo |
| [05-przemysl-praca.md](05-przemysl-praca.md) | Surowce, warsztaty, płace, zatrudnienie, priorytety |
| [06-dystrybucja-handel.md](06-dystrybucja-handel.md) | Spichlerz, magazyn, agora, handel, gromadzenie zapasów |
| [07-higiena-bezpieczenstwo.md](07-higiena-bezpieczenstwo.md) | Higiena, zaraza, niepokoje, przestępcy, pożary, zawalenia, woda |
| [08-administracja-finanse.md](08-administracja-finanse.md) | Pałac, podatki, finanse, przeprawy wodne |
| [09-kultura.md](09-kultura.md) | Filozofia, atletyka, dramat, zawody, Igrzyska Panhelleńskie |
| [10-mitologia.md](10-mitologia.md) | Bogowie, sanktuaria, ofiary, modlitwy, wyrocznia, zadania, gniew, potwory, herosi |
| [11-wojsko.md](11-wojsko.md) | Armia obywatelska, rozkazy, morale, flota, fortyfikacje, warsztaty wojskowe |
| [12-estetyka.md](12-estetyka.md) | Atrakcyjność, ozdoby, tereny rekreacyjne, pomniki pamiątkowe |
| [13-przeglad-popularnosc.md](13-przeglad-popularnosc.md) | Popularność, raport miasta, prośby i żądania |
| [14-swiat.md](14-swiat.md) | Mapa świata, stany dyplomatyczne, akcje, opinie przywódców |
| [15-atlantyda.md](15-atlantyda.md) | Dodatek: kultura atlantydzka, nauka, hipodrom, piramidy, nowi bogowie |
| [16-interfejs-referencyjny.md](16-interfejs-referencyjny.md) | Zakładki, widoki specjalne, ikony problemów, komunikaty, skróty |
| [17-czas-i-ruch.md](17-czas-i-ruch.md) | Model czasu, prędkości, zasięgi i zachowanie chodzików |
| [18-domy-i-ekonomia-liczby.md](18-domy-i-ekonomia-liczby.md) | Domy (model per trudność), punkty kultury, zużycie, praca, płace, podatki, hipodrom |
| [19-produkcja-i-logistyka-liczby.md](19-produkcja-i-logistyka-liczby.md) | Tempo produkcji, pojemności, doręczyciele, sady, hodowla, utrzymanie |
| [20-atrakcyjnosc-i-budynki.md](20-atrakcyjnosc-i-budynki.md) | Algorytm atrakcyjności, ryzyka, rozmiary, koszty |
| [21-mitologia-liczby.md](21-mitologia-liczby.md) | Sanktuaria, fazy budowy, sanktyfikacja, herosi, zadania |
| [22-wojsko-liczby.md](22-wojsko-liczby.md) | Statystyki jednostek, wrogowie, okup, rajdy |
| [23-przygody-i-edytor.md](23-przygody-i-edytor.md) | Struktura przygód, edytor, zdarzenia, cele |
| [98-konflikty-zrodel.md](98-konflikty-zrodel.md) | Rozbieżności między źródłami i przyjęte wartości |
| [99-luki.md](99-luki.md) | Dane, których źródła nie podają, ze statusem |

## Dane maszynowe

Katalog [`/data`](../../data/README.md) zawiera te same informacje w formie JSON: budynki, towary, domy, chodziki, bogowie, potwory, herosi, wojsko, dyplomacja, igrzyska, teren. Dokumenty md są nadrzędne wobec JSON w razie rozbieżności, a rozbieżność to błąd do poprawienia.
