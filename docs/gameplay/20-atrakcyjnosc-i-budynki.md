# 20. Atrakcyjność i parametry budynków

Źródło: S-004 (`appeal`, `buildings/building-model-data`). Dane: `data/building_stats.json` (137 budynków + 2 kopie: koszt na 5 poziomach trudności, rozmiar, pracownicy, atrakcyjność, ryzyko pożaru i zawalenia). Zamyka L-005, L-006, L-007, L-013.

## Algorytm atrakcyjności

Każdy budynek ma 4 parametry: `initial` (INI), `step_tiles` (SZE), `step_change` (STP), `range` (RNG).

Dla kafla w odległości `d` od budynku (1 ≤ d ≤ range):

```
appeal(d) = initial + floor((d - 1) / step_tiles) * step_change
```

Odległość liczona w "pierścieniach" wokół obrysu budynku (metryka Czebyszewa od krawędzi obrysu). Wkłady wszystkich budynków sumują się na kaflu.

Weryfikacja na tabelach S-004:
- Flower Garden (8, 1, -1, 3): 8, 7, 6.
- Fish Pond (18, 1, -3, 6): 18, 15, 12, 9, 6, 3.
- Mint (-10, 2, 2, 6): -10, -10, -8, -8, -6, -6.
- Sanktuarium (20, 1, -2, 6): 20, 18, 16, 14, 12, 10.

Przypadki szczególne (S-004):
- **College i University**: wpływ **pogarsza się** wraz z odległością (College -5, -8; University -5, -8, -11). Wzór obsługuje to ujemnym `step_change` przy ujemnej wartości startowej.
- **Palace**: kafle sztandarów wojskowych nie są częścią budynku przy liczeniu atrakcyjności.
- **Horse Ranch**: 3 kafle obok nieregularnego obrysu nie dostają atrakcyjności; **Pier**: 4 kafle.
- **Boulevard i Avenue**: każdy kafel ozdób i drogi emituje osobno, więc ciąg bulwaru daje bardzo wysoką atrakcyjność (obok bulwaru do 47, obok alei do 27).

Wygląd drogi odzwierciedla atrakcyjność (Z:156).

## Ryzyka budowlane

- `fire_risk` i `damage_risk`: przyrost ryzyka na jednostkę czasu, per trudność; `null` = budynek nie płonie / nie zawala się.
- Zawalenie: `60 / damage_risk` miesięcy bez wizyty superintendenta.
- Wizyta superintendenta zeruje (obniża) ryzyko obu rodzajów.
- Hephaestus z sanktuarium wyłącza pożary w mieście.

[LUKA L-020b: próg zapłonu i dokładny wzór obniżania przez superintendenta.]

## Rozmiary (wybrane)

| Budynek | Rozmiar |
|---|---|
| dom ludu | 2×2 |
| dom elity | 4×4 |
| farma | 3×3 (min. 1 kafel łąki) |
| Granary | 4×4 |
| Storehouse | 3×3 |
| Common Agora | 3×6; Grand Agora 5×6; stoisko 2×2 na agorze |
| Palace | 9×6 |
| Stadium | 10×5 |
| Theater | 5×5 |
| Trading Post | 4×4; Pier 4×4 + 2×2 na wodzie |
| Horse Ranch | 4×4 + 3×3 |
| Trireme Wharf | 3×3 na brzegu |
| Gatehouse | 5×3 |
| Museum | 6×6 |
| Hero's Hall | 4×4 |
| pomnik pamiątkowy | 3×3 |

Pełna lista w `data/building_stats.json`.

## Koszty

Koszt zależy od trudności (B/M/H/T/O), np. Granary 50/80/100/120/150, Palace 125/175/250/300/400. Sanktuaria mają koszt stały (np. Zeus 2920 na każdym poziomie). Pomniki i piramidy nie kosztują drachm.

Rozbieżności kosztów i liczby pracowników między podstronami S-004 rozstrzygamy na korzyść tabeli "Building Model Data" (K-018 do K-021).
