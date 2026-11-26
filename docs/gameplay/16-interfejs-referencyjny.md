# 16. Interfejs referencyjny (Z:19-27, Z:189-190)

Opis interfejsu gry referencyjnej na PC. Służy jako lista **funkcji informacyjnych**, które musimy zapewnić; układ i sterowanie projektujemy pod dotyk (`docs/06-MOBILE-UX.md`).

## Zakładki panelu sterowania

| Klawisz | Zakładka | Zawartość |
|---|---|---|
| 1 | Population | domy zwykłe i elity; info o populacji |
| 2 | Husbandry | Farms, Orchards, Herding, Land and Sea |
| 3 | Industry | Raw Materials, Workshops; płace, zatrudnienie |
| 4 | Distribution | Granary, Storehouse, Agora i stoiska, Trade Buildings; inwentarz, gromadzenie |
| 5 | Hygiene/Safety | Fountain, Infirmary, Watchpost, Maintenance Office |
| 6 | Administration | Palace, Tax Office, Water Crossing; (P) Hippodrome, Crosswalk, Bridge |
| 7 | Culture | Philosophy, Gymnasium, Drama, Stadium; szanse w igrzyskach |
| 8 | Mythology | Sanctuaries, Artisans' Guild, Hero's Hall |
| 9 | Military | Fortifications, Military Workshops; kompanie, triery, wieże |
| 0 | Aesthetics | Beautification, Recreational Areas, Commemorative Monuments; (P) Pyramids |
| (P) | Science | Bibliotheke, Astronomy, Technology, Museum |
| — | Overview | raport miasta, prośby |

Każda zakładka ma panel informacji i "lupę" (szczegóły).

## Stałe elementy

Clear Land, Roads, Roadblock, Undo, przyciski "idź do" (4 sloty), wiadomości i skrócona skrzynka wiadomości, Goals, Rotate City (lewo, prawo, północ), View Pan-Hellenic World, przełącznik mapa przeglądowa / informacje. Górny pas: skarbiec, populacja, data. Pasek ostrzeżeń (warning banner) i komunikaty paskowe o popularności, zdrowiu, imigracji i żywności.

## Przyciski "idź do" (Z:189)

| Slot | Zdarzenia |
|---|---|
| 1. Zagrożenia i różne | pożar, trzęsienie ziemi, fala pływowa, osuwisko, lawa, zawalenie, zablokowana droga do Grecji |
| 2. Higiena | zaraza |
| 3. Inwazje | armia, potwór, bóg |
| 4. Przybycia | bóg, heros, powrót armii |

## Wiadomości (Z:20, Z:22)

Kategorie z ustawieniem: pełna wiadomość (zatrzymuje grę, okno) albo tylko skrócona. Archiwum wszystkich wiadomości.

## Widoki specjalne ("See ...")

Spłaszczają budynki niezwiązane z tematem; pokazują kolumny lub ikony nad domami i budynkami. Klawisz Spacja przełącza widok specjalny i normalny.

| Widok | Pokazuje |
|---|---|
| See Supplies | dobra domów (zielone / migające czerwone) |
| See Husbandry | budynki rolne |
| See Industry | przemysł, ikony problemów, utknięci doręczyciele |
| See Distribution | agory, magazyny, handel, vendorzy, peddlerzy, zapasy domów |
| See Hygiene | healerzy; kolumna wizyty healera, symbol posiłku |
| See Unrest | watchmani, szlachcice, przestępcy; kolumna ryzyka przestępstwa |
| See Hazards | superintendenci; kolumna ryzyka pożaru/zawalenia |
| See Water | nosiwody; niebieska kolumna |
| See Taxes | clerks; kolumna wizyty, stos monet |
| See Philosophers / Athletes / Actors / Competitors | kolumna wizyty wykonawcy |
| See Culture | kolumna różnorodności kultury |
| See Immortals | bogowie, potwory, herosi; kolumna ofiar przed sanktuariami |
| See Security | wojsko, wrogowie, budynki wojskowe, wkład domów elity |
| See Appeal | jasność kafli |
| See Problems | wszystko, co ma problem |
| See Roads | sieć dróg |
| (P) Show Scholars / Astronomers / Inventors / Curators / All Science | kolumny nauki |

## Ikony problemów (Z:63, Z:164)

| Ikona | Znaczenie |
|---|---|
| fire_risk | budynek zaraz się zapali |
| collapse_risk | budynek zaraz się zawali |
| no_workers | brak pracowników |
| partial_staff | częściowa obsada |
| industry_off | branża wyłączona przez gracza |
| no_raw_materials | warsztat bez surowca |
| house_devolving | dom zaraz się cofnie |
| outlaw_brewing | z domu zaraz wyjdzie przestępca |
| plague | dom zarażony |
| needs_sacrifice | sanktuarium potrzebuje ofiary |
| no_destination | doręczyciel bez prawidłowego celu |

## Panele budynków

Kliknięcie podstawy budynku: panel z informacjami i odnośnikiem do pomocy (Z:26). Rozmowa z mieszkańcami: kliknięcie u stóp (Z:25).

## Pozostałe skróty (Z:190)

Zakładki lokalizacji (4), pauza, obrót kamery, powrót na północ, przewijanie, mapa przeglądowa, zrzut ekranu.
