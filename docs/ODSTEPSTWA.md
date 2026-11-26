# Rejestr odstępstw od referencji

Każda różnica między grą a specyfikacją `docs/gameplay/` (referencja 1:1) ma wpis. "Tak wyszło" nie jest powodem.

## Format

```
### O-NNN: <krótki opis>
- Miejsce: <system / plik / scenariusz>
- Referencja: <co mówi specyfikacja, ze stroną Z:nn / P:nn>
- W grze: <jak jest u nas>
- Status: celowe | do poprawy | do decyzji
- Powód / zgłoszenie: <reguła R-NNN, decyzja z DECYZJE.md, błąd B-NNN>
```

## Odstępstwa

### O-001: Sterowanie i interfejs pod dotyk
- Miejsce: cały interfejs.
- Referencja: panel sterowania z zakładkami, prawy przycisk myszy, klawiatura (Z:19-27, Z:190).
- W grze: interfejs dotykowy z `docs/06-MOBILE-UX.md`; wszystkie funkcje informacyjne z `docs/gameplay/16-interfejs-referencyjny.md` zachowane.
- Status: celowe.
- Powód: R-003 (telefon jako platforma pierwszorzędna).

### O-002: Kamera i rozdzielczość
- Miejsce: render.
- Referencja: rozdzielczość 800×600 lub 1024×768, obrót co 90° (Z:21-22).
- W grze: płynny zoom i rozdzielczość urządzenia; obrót do decyzji (pyt. 7).
- Status: celowe.
- Powód: R-004 (brak ograniczeń sprzętowych).

### O-003: Autozapis przy schowaniu aplikacji
- Miejsce: zapis.
- Referencja: autozapis w styczniu i lipcu (Z:22).
- W grze: oba mechanizmy (referencyjny i przy schowaniu aplikacji).
- Status: celowe.
- Powód: `docs/PRAKTYKI.md` rozdział 12.

### O-004: Nazwy, grafika, audio, teksty
- Miejsce: cała treść.
- Referencja: nazwy i treść gry referencyjnej.
- W grze: własne (proces w `docs/02-ORYGINALNOSC.md`).
- Status: celowe.
- Powód: R-002.
