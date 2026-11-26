# 09. Kultura (Z:105-113)

Kultura ma cztery rodzaje dostępne dla domów: **filozofia, atletyka, dramat, zawody**. Wymagania domów rosną wraz z liczbą rodzajów kultury w zasięgu (patrz [03-mieszkancy.md](03-mieszkancy.md)).

## Filozofia (Z:105-106)

- **College** (droga) szkoli filozofów. Szkoli tylko kilku na miesiąc.
- **Podium** (droga, połączone drogą z College).
- Filozof idzie z College na Podium (celowy). Gdy wykład trwa, inni filozofowie wychodzą z Podium do domów (losowi).
- Więcej Podiów wymaga więcej College.

## Atletyka (Z:107)

- **Gymnasium** (droga) wysyła **atletów** (losowi), którzy trenują mieszkańców.

## Dramat (Z:107-108)

- **Drama School** (droga), szkoli niewielu aktorów miesięcznie.
- **Theater** (droga). Aktor idzie z Drama School do Theater (celowy); podczas spektaklu inni aktorzy z Theater krążą po ulicach (losowi).

## Zawody (Competitions, Z:108-109)

- **Stadium**: **tylko jeden w mieście**, droga wymagana, można obracać.
- Gymnasium szkoli zawodników **tylko jeśli w mieście jest Stadium**. Zawodnik idzie z Gymnasium na Stadium (celowy). Stadium wysyła też własnych zawodników (losowi).
- Dostęp do zawodów jest wymagany przez Estate.
- Działający Stadium jest warunkiem organizacji Igrzysk.

## Widoki

See Philosophers, See Athletes, See Actors, See Competitors: kolumna przed domem = jak dawno był wykonawca. **See Culture**: kolumna = różnorodność dostępnych rozrywek.

## Sanktyfikacja

Apollo sanktyfikuje Podium, Gymnasium, Theater, Stadium: wychodzi z nich więcej wykonawców (Z:110, Z:121).

## Igrzyska Panhelleńskie (Z:110-113)

### Cykl

| Rok (rok mod 4) | Igrzyska | Dyscyplina | Nagroda symboliczna |
|---|---|---|---|
| 0 | Olympian | wszystkie | laur |
| 1 | Isthmian | filozofia | suchy dziki seler |
| 2 | Pythian | dramat (aktorzy) | liście laurowe (bay) |
| 3 | Nemean | atleci i zawodnicy | świeży dziki seler |

- Igrzyska "mogą zacząć się odbywać w pewnym momencie" panowania (zależne od scenariusza). [LUKA: warunek startu]
- Implementacja: co rok od początku epizodu, wynik w sierpniu, szansa z zasięgu wykonawców według eZeus (L-123, `core/src/games.cpp`). Organizacja własnych igrzysk jeszcze nie działa.

### Udział

- Miasto bez wykonawców danej dyscypliny **nie jest zapraszane**.
- Szansa na wygraną rośnie z **liczbą** budynków danej dyscypliny i zasięgiem (np. więcej Podium i College = lepsza szansa na Isthmian). Panel kultury pokazuje szanse i porady.
- Apollo zwiększa szanse (Z:112).

### Nagrody

- Wzrost opinii innych przywódców.
- **Pomnik pamiątkowy**: Scholar (wygrana Pythian lub Isthmian), Athlete (wygrana Nemean lub Olympian).

### Organizacja igrzysk

- Wygrana Olympian = prawo do organizacji igrzysk **za 4 lata**, pod warunkiem działającego Stadium.
- Organizacja: parada zawodników z całej Grecji; wzrost opinii innych miast, wzrost popularności, pomnik pamiątkowy.
- Wygrana własnych igrzysk = ponowne prawo za 4 lata.
- Brak działającego Stadium w terminie: miasto traci prawo, inne miasto organizuje, a **gracz nie jest zaproszony w tym roku**.
