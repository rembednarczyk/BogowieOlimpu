# Architektura symulacji

## Zasady

1. **Separacja.** Rdzeń symulacji to czysta biblioteka bez zależności od silnika. Rendering, audio i UI czytają stan symulacji i wysyłają do niej komendy.
2. **Determinizm.** Ten sam stan początkowy, to samo ziarno losowe i te same komendy dają identyczny wynik na każdym urządzeniu. Wymaga: arytmetyki stałoprzecinkowej lub ściśle kontrolowanej, własnego generatora liczb losowych, stałej kolejności iteracji.
3. **Stały krok.** Symulacja działa w stałych tickach (wartość robocza: 10 ticków na sekundę przy prędkości 1x). Prędkość gry zmienia liczbę ticków na sekundę. Rendering interpoluje pozycje między tickami.
4. **Dane poza kodem.** Definicje budynków, towarów, jednostek i bogów w plikach danych (JSON/YAML lub format binarny generowany z arkuszy).

## Model danych

- **Mapa:** siatka kafli z warstwami (teren, wysokość, budynek, droga, atrakcyjność, zasięgi wody, ryzyka).
- **Encje:** budynki, chodziki, jednostki, potwory, bogowie jako encje w układzie ECS lub struktur tablicowych (data-oriented).
- **Systemy:** każdy system aktualizuje swoją część stanu w ustalonej kolejności, np.: imigracja, zatrudnienie, produkcja, logistyka, chodziki, usługi, ewolucja domów, ryzyka, religia, mapa świata, zdarzenia.
- **Częstotliwości:** nie każdy system musi działać co tick. Ewolucja domów i atrakcyjność mogą przeliczać się co N ticków, rozłożone na grupy budynków.

## Chodziki i ścieżki

- Graf dróg utrzymywany przyrostowo przy budowie i wyburzaniu.
- Chodziki celowe: A* lub wstępnie obliczone pola odległości do magazynów, z cache tras.
- Chodziki losowe: zasady wyboru kierunku na skrzyżowaniach zgodne z regułami gatunku (do wydestylowania ze źródeł), limit dystansu, powrót do budynku.
- Obsługa w zasięgu: przy każdym kroku chodzik oznacza budynki w promieniu obsługi. Implementacja przez przestrzenne indeksy kafli.

## Zapis gry

- Zapis to zrzut stanu symulacji plus wersja danych. Rozmiar docelowy poniżej 5 MB dla dużego miasta.
- Autozapis przy zejściu aplikacji do tła (krytyczne na mobile).
- Wersjonowanie formatu z migracjami.

## Testy

- Testy jednostkowe systemów.
- Testy scenariuszowe: skrypt komend, uruchomienie N ticków, porównanie stanu z oczekiwanym.
- Test powtarzalności: dwa przebiegi z tym samym ziarnem dają identyczny hash stanu. Uruchamiany w CI.
- Benchmark: miasto referencyjne, pomiar czasu ticku na docelowym telefonie.

## Budżet czasu (wartości robocze)

- Tick symulacji przy 15 000 mieszkańców: poniżej 4 ms na rdzeniu telefonu średniej klasy.
- Symulacja w osobnym wątku, rendering niezależny.
