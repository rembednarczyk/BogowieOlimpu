# 23. Przygody, epizody i edytor (zamyka L-093 częściowo)

Źródła: S-004 (`scendesign/*`, `trivia`, `oddities`), S-005 (Nightwolf "How To Do Colonies", "Odds & Ends").

## Struktura przygody

- Przygoda zaczyna się epizodem miasta macierzystego i kończy epizodem zwycięstwa w mieście macierzystym.
- Każdy epizod macierzysty wskazuje, co następuje: kolejny epizod macierzysty, kolonia albo zwycięstwo. Po epizodzie kolonii gra zawsze wraca do miasta macierzystego.
- Przykład: P, P, K, P, K, P, P, K, Zwycięstwo = 6 przycisków epizodów macierzystych (3 oznaczone "kolonia") + 3 epizody kolonii.
- Kolonie na mapie świata oznaczone jako "nieaktywne" gracz może założyć; **gracz sam wybiera kolonię** spośród nieużytych, projektant nie wymusza kolejności.
- Gra nie sprawdza, czy kolonia faktycznie działa; liczy się spełnienie celów epizodu (S-004 `oddities`; zachowanie referencyjne).

## Mapa świata w edytorze

- Kolejność dodawania miast: macierzyste (indeks 0), sojusznicy i wasale, kolonie, rywale; zdarzenia odwołują się do indeksów.
- Każde miasto: typ relacji, trybut płacony i otrzymywany (typ i ilość), szlaki handlowe (lądowe/morskie), towary.
- Wasale płacą trybut na początku każdego roku; podbity rywal również.
- **Maks. 4 rodzaje żywności w całej przygodzie** (miasta, kolonie, handel łącznie); trybut może dać 5. rodzaj.
- Dalekie miasta i miejsca zaczarowane nie wysyłają darów.

## Mapa miasta w edytorze

- Rozmiary map do "enormous"; domyślnie "medium".
- Punkty: wejście i wyjście lądowe (imigranci, karawany), wejście i wyjście morskie, odradzanie zwierząt (dziki, jelenie, wilki), łowiska i ławice jeżowców, punkty potworów, punkty inwazji, punkty katastrof.
- Kraken patroluje wody między morskim wejściem a wyjściem.
- Bez punktu potwora potwór startuje z wejścia lądowego.

## Zdarzenia

- Typy: dary, prośby, inwazje, potwory, katastrofy (fala pływowa, trzęsienie ziemi, lawa, osuwisko), zmiany handlu i relacji.
- Zdarzenie może się powtarzać co rok w tym samym miesiącu; **zdarzenia potworów nie mogą się powtarzać** (każde pojawienie to osobne zdarzenie). Brak limitu zdarzeń na epizod (obserwowano 93).
- Edytor nie ma losowości; projektanci symulują ją wieloma strefami katastrof (zamyka L-090: katastrofy są skryptowane przez scenariusz, z punktami na mapie).
- Potwór w zdarzeniu ma ustawioną siłę, agresywność i cel.

## Cele epizodu (przykłady z poradników)

Populacja, rządzenie miastem (podbój), sanktuarium boga, wykonanie zadania, liczba trier, produkcja X towaru w roku, odłożenie towaru dla kolonii, pokonanie potwora, skarbiec, poziom domów, zysk.

[LUKA L-093b: pełna lista typów celów i parametrów zdarzeń; instrukcja edytora dodatku (PDF) nie została dostarczona.]

## Wskazówki projektowe z poradników (do naszych kart scenariuszy)

- Gracz zawsze musi mieć wyjście z sytuacji; potwory nie mogą startować w miejscu przeznaczonym na zabudowę.
- Rosnąca skala inwazji; duże katastrofy wcześnie w epizodzie.
- Cel populacyjny osiągalny "na styk".
- Herosa udostępniamy tylko, jeśli gracz realnie może spełnić jego wymagania.
- Testy scenariusza na kilku poziomach umiejętności i na każdej ścieżce kolonii.

Te zasady przenosimy do kart scenariuszy (`docs/PRAKTYKI.md`, rozdział 9).
