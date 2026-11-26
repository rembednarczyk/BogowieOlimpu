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

Brak.

## Naprawione

### B-001 [S2] Zapis gry przekłamywał kwarantannę i dni płonięcia budynku
- Status: naprawione (commit z testem "B-001" w `core/tests/test_save.cpp`)
- Miejsce i liczba wystąpień: `core/src/serialize_city.cpp`, `read_risks`; każdy wczytany zapis od formatu v12 z domem w kwarantannie albo płonącym budynkiem.
- Dowód: demo w 412. dniu, zapis i odczyt w tym samym ticku dawały inny hash stanu; dom nr 22 miał kwarantannę 1 dzień przed zapisem i 256 po odczycie. Znalezione przy teście zapisu w interfejsie (krok E).
- Test "co dziś pada": "B-001: burning days and quarantine survive a save" pada bez poprawki, przechodzi z nią.
- Przyczyna: `write_risks` zapisuje cztery liczniki, dni płonięcia (1 bajt), potem kwarantannę; `read_risks` czytał pięć liczb 4-bajtowych, a dni płonięcia na końcu. Zapisy są zgodne wstecz: zmieniła się tylko kolejność czytania, układ bajtów został.
