# 22. Wojsko: liczby

Źródła: S-004 (`enemy-information`, `units`, `housinglevels`, `rabble`), S-005. Dane: `data/enemies.json`, `data/military.json`. Zamyka L-060, L-061, L-065.

## Statystyki jednostek gracza (Grecja)

| Jednostka | HP | Atak | Pancerz | Pancerz/pociski | Atak pociskiem | Zasięg | Szybkostrzelność | Prędkość | Częstość ciosów |
|---|---|---|---|---|---|---|---|---|---|
| Rabble | 100 | 5 | 2 | 3 | 6 | 11 | 90 | 72,53 | 20 |
| Hoplite | 150 | 15 | 6 | 2 | - | - | - | 54,4 | 60 |
| Horseman | 250 | 17 | 4 | 4 | - | - | - | 108,8 | 20 |
| Trireme | 300 | 90 | 4 | 5 | 20 | 9 | 80 | 68 | 100 |
| Wojownicy Aresa | 220 | 16 | 8 | 8 | - | - | - | 72,53 | 100 |
| Amazonki Artemidy | 80 | 5 | 2 | 3 | 8 | 11 | 75 | 68 | 75 |

Szybkostrzelność i częstość ciosów: mniejsza liczba = częściej. Rabble ma zasięg i jest najlepsza przeciw potworom i w obronie z wysokości (S-004 `rabble`).

Pełne tabele dla Atlantydów i wszystkich wrogów: `data/enemies.json`.

## Liczebność

- Kompania: 8 żołnierzy na sztandarze; dom ludu daje rabble według %S i #S z `data/housing.json` (Hovel 5 ... Townhouse 15).
- Każdy żołnierz rabble w polu = 6 osób (pracowników i mieszkańców).
- Mansion 2 hoplitów, Manor 4, Estate 4 jeźdźców.
- Wieża: 15 wartowników; Trireme Wharf: 100 pracowników.
- Maks. 20 kompanii (Z:136).

## Wrogowie

Skład inwazji (kolejno: ciężka piechota, strzelcy, jazda), liczby zaokrąglane w dół:

| Nacja | Skład % |
|---|---|
| Grecy | 60 / 20 / 20 |
| Trojanie | 50 / 20 / 30 |
| Persowie | 30 / 40 / 30 |
| Centaury | 50 / 50 (bez jazdy) |
| Amazonki | 75 / 25 |
| Egipcjanie (P) | 50 / 30 / 20 |
| Majowie (P) | 25 / 75 |
| Fenicjanie (P) | 70 / 30 |
| Oceanidy (P) | 50 / 50, pod wodą |
| Atlantydzi (P) | 40 / 30 / 30 |

Maks. inwazja: 256 żołnierzy.

## Okup

- Okup na poziomie Hero: 100 dr za pieszego, 200 za konnego, 800 za okręt wojenny.
- Mnożnik trudności: B 25%, M 75%, H 100%, T 125%, O 175%.
- Kwota okupu pozwala oszacować liczebność inwazji (średni koszt na żołnierza, np. Grecy na Hero 120 dr).
- Wielkość armii wroga nie zależy od trudności; okup zależy.
- Wojsko opłacone okupem i tak idzie do wyjścia z mapy i może niszczyć budynki na drodze (S-005).

## Rajdy

Łup zależy od trudności: Beginner 2× rocznej ilości towaru miasta, Olympian 0,5× (S-004 trivia). Rajd osłabia garnizon celu przed atakiem.

[LUKA L-062 (morale), L-063 (rozstrzyganie bitew za granicą): nadal otwarte.]
