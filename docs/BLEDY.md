# Rejestr błędów

Błąd: rozjazd między tym, co gra pokazuje albo symuluje, a specyfikacją lub tym, co gracz uzna za realne. Zgłoszenie bez dowodu ma status "niepotwierdzone".

## Ważność

| Poziom | Znaczenie |
|---|---|
| S1 | awaria, utrata zapisu, czerwony main, niedeterminizm |
| S2 | zła reguła rozgrywki, zablokowana gra, błąd widoczny dla każdego gracza |
| S3 | błąd w rzadkiej sytuacji, kosmetyka wpływająca na czytelność |
| S4 | kosmetyka |

Priorytet = ważność × zasięg / koszt. S1 zawsze pierwsze.

## Format

```
### B-NNN [S?] <tytuł>
- Status: niepotwierdzone | potwierdzone | w naprawie | naprawione (commit)
- Miejsce i liczba wystąpień:
- Dowód: <licznik / pomiar / zrzut / ziarno + log komend>
- Test "co dziś pada":
- Przyczyna:
```

## Otwarte

Brak. Kod jeszcze nie powstał.
