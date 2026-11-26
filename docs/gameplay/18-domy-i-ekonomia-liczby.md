# 18. Domy, praca i finanse: liczby

Źródła: S-004 (`housinglevels`, `taxation`, `worker-percentages`), S-005. Dane: `data/housing.json`, `data/difficulty.json`, `data/logistics.json`.

## Poziomy trudności

Pięć poziomów: **Beginner (B), Mortal (M), Hero (H), Titan (T), Olympian (O)**. Trudność zmienia koszty budynków, progi domów, ryzyka, podatki, odsetek pracujących, okup i łup z rajdów (zamyka L-091).

## Domy: pojemność, wymagania, żołnierze

| Poziom | Ludzie | Wymagania (narastająco) | Maks. żołnierzy |
|---|---|---|---|
| Hut | 8 | brak | 0 |
| Shack | 16 | żywność | 0 |
| Hovel | 24 | + woda, kultura (Ent 15) | 5 |
| Homestead | 32 | + wełna, atrakcyjność | 6 |
| Tenement | 40 | + więcej kultury (Ent 35) | 10 |
| Apartment | 48 | + oliwa, atrakcyjność | 12 |
| Townhouse | 60 | + atrakcyjność, kultura (Ent 45) | 15 |
| Residence | 6 | żywność, wełna, oliwa, kultura, atrakcyjność | 0 |
| Mansion | 10 | + zbroja | 2 hoplitów |
| Manor | 16 | + wino | 4 hoplitów |
| Estate | 20 | + konie, pełna kultura | 4 jeźdźców |

- Żołnierz rabble w polu reprezentuje 6 osób; powołanie zabiera 6 pracowników z puli, śmierć zmniejsza populację o 6 (S-004, S-005).
- Postawienie domu elity wymaga w magazynach miasta: **2 ładunki żywności, 1 wełny, 1 oliwy** oraz miejsca o wysokiej atrakcyjności.
- Estate trzyma maks. 4 konie.

## Model domu (pełne tabele per trudność: `data/housing.json`)

| Kolumna | Znaczenie |
|---|---|
| AppD | atrakcyjność, poniżej której dom się cofa |
| AppE | atrakcyjność, od której dom może awansować |
| Ent | punkty kultury/nauki potrzebne do awansu |
| W, Fd, Fl, Oil, Wi, Ar, Hrs | wymagane: woda, żywność, wełna, oliwa, wino, zbroja, konie |
| %S | procent mieszkańców jako żołnierze |
| HS, #S | pojemność koni, liczba żołnierzy |
| CRI, CRIp | przyrost ryzyka przestępczości (gra podstawowa, dodatek) |
| DRI | przyrost ryzyka choroby |
| TRM | mnożnik podatku |

Przykład, Hero: Townhouse AppD 25, AppE 100, Ent 45, TRM 2, DRI 5. Elita na Hero: Residence Ent 50, Mansion 60, Manor 70, Estate 80; TRM 16.

Histereza atrakcyjności (AppD < AppE) jest kluczowa: dom awansuje przy AppE, cofa się dopiero poniżej AppD. Odpowiada to obserwacji z instrukcji (Z:31) i zamyka L-010.

## Punkty kultury i nauki (zamyka L-011)

Dom zbiera punkty od wykonawców, którzy go minęli niedawno. Suma musi osiągnąć Ent poziomu.

| Chodzik | Punkty |
|---|---|
| filozof (Podium) | 15 |
| atleta (Gymnasium) | 20 |
| aktor (Theater) | 25 |
| zawodnik (Stadium) | 20 |
| bonus działającego Stadium, całe miasto | 10 |
| uczony (Bibliotheke) | 15 |
| astronom (Observatory) | 25 |
| wynalazca (Laboratory) | 20 |
| kurator (Museum) | 20 |
| bonus działającego Museum, całe miasto | 10 |

Przykład: Townhouse (45) = filozof 15 + atleta 20 + bonus Stadium 10, bez teatru.

[LUKA L-012b: jak długo punkt od wizyty "żyje" (zanik kolumny w widoku kultury).]

## Zużycie dóbr (zamyka L-012)

- 1 ładunek w magazynie = **100 jednostek** w Agorze i domu.
- Żywność: **0,25 jednostki na osobę na miesiąc** (od Shack wzwyż); koń w Estate je tyle co osoba.
- Wełna, oliwa, wino: **2 jednostki na dom na miesiąc**, niezależnie od liczby mieszkańców.

| Dom (pełny) | Żywność/mies. | Wełna | Oliwa | Wino |
|---|---|---|---|---|
| Shack | 4 | | | |
| Hovel | 6 | | | |
| Homestead | 8 | 2 | | |
| Tenement | 10 | 2 | | |
| Apartment | 12 | 2 | 2 | |
| Townhouse | 15 | 2 | 2 | |
| Residence | 1 | 2 | 2 | |
| Mansion | 2 | 2 | 2 | |
| Manor | 4 | 2 | 2 | 2 |
| Estate (z 4 końmi) | 6 | 2 | 2 | 2 |

Zbroja i konie nie są zużywane; uzupełnia się je po stracie w walce.

## Praca

Odsetek mieszkańców, którzy pracują (pomiar, S-004 `worker-percentages`):

| Trudność | brak | b. niskie | niskie | normalne | wysokie | b. wysokie |
|---|---|---|---|---|---|---|
| Beginner | 42 | 46 | 49 | 52 | 55 | 57 |
| Mortal | 37 | 41 | 44 | 47 | 50 | 52 |
| Hero | 32 | 36 | 39 | 42 | 45 | 47 |
| Titan | 29 | 33 | 36 | 39 | 42 | 44 |
| Olympian | 27 | 31 | 34 | 37 | 40 | 42 |

Płaca roczna na pracownika (zamyka L-031): brak 0, b. niskie 2, niskie 2,5, normalne 3, wysokie 3,4, b. wysokie 4 drachmy.

## Podatki (zamyka L-030)

`podatek_mies = TRM(dom, trudność) × mieszkańcy × stawka`, tylko domy zarejestrowane przez clerka.

| Stawka | Wartość |
|---|---|
| none | 0 |
| very low | 0,03 |
| low | 0,07 |
| normal | 0,09 |
| high | 0,11 |
| very high | 0,15 |
| outrageous | 0,20 |

TRM: domy ludu 1 (Hut-Hovel) i 2 (Homestead-Townhouse; na Titan Homestead 1); elita 22/18/16/14/12 (B/M/H/T/O).

Instrukcja (Z:97) mówi też, że atrakcyjność okolicy podnosi podatek; tabela S-004 tego nie uwzględnia (K-023, do weryfikacji).

### Wpływ stawki na nastroje

| Stawka | B | M | H | T | O |
|---|---|---|---|---|---|
| none | 7 | 4 | 3 | 2 | 1 |
| very low | 5 | 3 | 2 | 1 | 1 |
| low | 3 | 1 | 1 | 0 | 0 |
| normal | 2 | 0 | 0 | 0 | 0 |
| high | 0 | 0 | -1 | -2 | -3 |
| very high | -1 | -2 | -3 | -4 | -5 |
| outrageous | -2 | -4 | -5 | -6 | -7 |

[LUKA L-018, częściowo: pozostałe składniki popularności (płace, bezrobocie, żywność, dług, mobilizacja) i formuła migracji L-017.]

## Hipodrom (dodatek)

| Długość (stadia) | Konie | Dochód dr/mies. |
|---|---|---|
| 4-10 | 4 | 10 |
| 12-22 | 4 | 20 |
| 24-34 | 8 | 40 |
| 36-58 | 8 | 60 |
| 60-98 | 12 | 100 |
| 100-198 | 12 | 200 |
| 200+ | 24 | 500 |

Powyżej 200 stadiów mieszkańcy są niezadowoleni (niepokój). Zamyka L-081.
