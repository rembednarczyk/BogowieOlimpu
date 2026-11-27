# Mobile UX

Zasada nadrzędna: projektujemy dla telefonu 6" trzymanego poziomo. Tablet i PC dostają ten sam interfejs w skali oraz dodatkowe skróty.

## Kamera

- Przesuwanie jednym palcem, zoom szczypnięciem, obrót dwoma palcami (jeśli kamera pozwala na obrót).
- Szybki powrót do kluczowych miejsc: pałac, sanktuaria, aktualne zagrożenie.
- Pochylenie kamery przy dużym zbliżeniu dla efektu "spaceru po mieście".

## Budowa

- Wybór budynku z panelu kategorii przy prawej krawędzi, jak panel sterowania oryginału (decyzja 2026-11-26, potwierdzona 2026-11-27), maks. 2 dotknięcia do budynku; kolumna budynków obok panelu.
- Podgląd ducha budynku z zasięgami i wpływem na atrakcyjność przed postawieniem.
- Drogi i strefy: tryb przeciągania z widocznym kosztem; zatwierdzenie osobnym przyciskiem, żeby uniknąć przypadkowych budów.
- Wyburzanie: tryb z potwierdzeniem i cofaniem.
- Precyzja: lupa pod palcem lub przesunięty kursor przy stawianiu pojedynczych kafli.

## Informacja

- Dotknięcie budynku: karta z najważniejszymi danymi i jasną przyczyną problemu ("brak oliwy w zasięgu rynku").
- Nakładki: woda, pożar, zawalenie, atrakcyjność, zatrudnienie, dystrybucja towarów, zasięg kultury.
- Powiadomienia z priorytetami: krytyczne (inwazja, gniew boga) przerywają, reszta w dzienniku.

## Sesje mobilne

- Pauza i autozapis przy wyjściu z aplikacji.
- Tryb "szybkie spojrzenie": po powrocie do gry podsumowanie, co się wydarzyło.
- Prędkości gry 0.5x do 4x, pauza z możliwością budowania.

## Skala i układ (decyzja 2026-11-27)

- Interfejs liczony w dp (1/160 cala): współczynnik dpi/160 na telefonie i tablecie, skala systemowa na PC (większy monitor pokazuje więcej miasta, panel ma stały rozmiar jak w oryginale); suwak "Skala interfejsu" 75-150 % w ustawieniach raportu. Kod: `game/scenes/ui/ui_scale.gd`.
- Tylko orientacja pozioma, obie strony (wycięcie aparatu z lewej albo z prawej); marginesy z bezpiecznego obszaru ekranu.
- Najmniejszy ekran: 640 x 360 dp. Co się nie mieści, przewija się (kategorie, kolumna budynków, karta, raport, wojsko, świat); wiersze z przyciskami zawijają się.
- Układ: prawy panel (kategorie, prędkości, Cofnij, Raport, Wojsko, Świat, Widok); nad mapą z lewej kolumna: stan miasta, legenda widoku, karta budynku; u dołu podpowiedź i pasek Zatwierdź, Obróć, Anuluj tylko w trybie budowy. Panel raportu, wojska albo świata zamyka kartę i tryb budowy.
- Czcionki: 16 dp tekst i przyciski, 14 dp kategorie i nazwy budynków (zawijane w dwie linie), 18 i 20 dp tytuły.
- Bramka: `game/tests/test_layout.gd` w sześciu rozmiarach sprawdza 48 dp, położenie na ekranie, brak nakładania i brak uciętych napisów.

## Dostępność

- Minimalny rozmiar celu dotyku: 48 x 48 dp (7,6 mm; decyzja właściciela 2026-11-27).
- Skalowanie tekstu, tryby kolorów, ikony z kształtem oprócz koloru.
- Kontroler i mysz jako opcja na tabletach i PC.

## Do zbadania

- Jak gry z serii odświeżonych citybuilderów przeniosły sterowanie na dotyk.
- Testy z prototypem papierowym/Figma przed fazą 2.
