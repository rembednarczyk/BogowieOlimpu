# 99. Luki w specyfikacji

Instrukcje opisują reguły jakościowo. Poniższe dane są potrzebne do implementacji 1:1, a instrukcje ich nie podają. Każda luka ma identyfikator `L-NNN`. Po uzupełnieniu (nowe źródło S-NNN) wpisujemy wartość do dokumentu i `data/*.json`, a lukę oznaczamy jako zamkniętą.

Priorytet: **P0** blokuje prototyp symulacji (faza 1), **P1** blokuje vertical slice, **P2** potrzebne do wydania.

## Rdzeń symulacji

| ID | Luka | Priorytet | Gdzie |
|---|---|---|---|
| L-001 | Algorytm wyboru kierunku chodzika losowego na skrzyżowaniach | P0 | 02 |
| L-002 | Maksymalna długość trasy i promień obsługi każdego chodzika losowego | P0 | 02 |
| L-003 | Prędkości chodzików i jednostek | P0 | 02 |
| L-004 | Długość ticku, miesiąca i roku w czasie rzeczywistym przy prędkościach gry | P0 | 01 |
| L-005 | Rozmiary budynków (footprint w kaflach) | P0 | data/buildings |
| L-006 | Koszty budowy wszystkich budynków | P0 | data/buildings |
| L-007 | Liczba pracowników każdego budynku | P0 | data/buildings |
| L-008 | Lista sektorów zatrudnienia i skala priorytetów | P0 | 05 |
| L-009 | Pojemności domów na każdym poziomie (zwykłe i elita) | P0 | 03 |
| L-010 | Progi atrakcyjności dla poziomów domów i dla elity | P0 | 03, 12 |
| L-011 | Liczba rodzajów kultury wymagana na każdym poziomie | P0 | 03 |
| L-012 | Zużycie dóbr przez dom (ile i jak często) oraz pojemność zapasu domu | P0 | 03 |
| L-013 | Wartości i zasięgi wpływu na atrakcyjność każdego budynku | P0 | 12 |
| L-014 | Tempo produkcji każdego budynku (ładunki na miesiąc) | P0 | 04, 05 |
| L-015 | Pojemności Granary, Storehouse, Pier, Trading Post (w ładunkach) | P0 | 06 |
| L-016 | Próg odległości "Too Distant" dla vendorów Agory | P1 | 06 |
| L-017 | Formuła migracji (imigracja, emigracja) i jej powiązanie z popularnością | P0 | 03, 13 |
| L-018 | Formuła popularności (wagi czynników) | P1 | 13 |
| L-019 | Kolejność dostaw dla Foundry, Growers' Lodge, Wheat Farm (Granary vs Storehouse) | P1 | 04, 05 |
| L-020 | Ryzyka pożaru i zawalenia: przyrost w czasie per budynek, redukcja przez superintendenta | P1 | 07 |
| L-021 | Formuła higieny i prawdopodobieństwa zarazy, długość kwarantanny, śmiertelność | P1 | 07 |
| L-022 | Formuła niepokojów i progi eskalacji przestępców | P1 | 07 |

## Ekonomia

| ID | Luka | Priorytet |
|---|---|---|
| L-030 | Wysokość podatku per poziom domu, mnożniki stawek i atrakcyjności | P1 |
| L-031 | Płace: wartość "normal" i mnożniki poziomów | P1 |
| L-032 | Ceny towarów (kupno i sprzedaż) | P1 |
| L-033 | Pojemność karawany i statku, częstotliwość wizyt | P1 |
| L-034 | Liczba lat długu do przegranej | P2 |
| L-035 | Okno czasowe Undo | P2 |
| L-036 | Wydajność Mint (drachmy na ładunek) | P1 |
| L-037 | Liczba kóz na Dairy, owiec na Carding Shed, bydła na Corral | P1 |
| L-038 | Czas odrostu wypasionej łąki | P2 |
| L-039 | Wzrost drzew i winorośli w funkcji przycinania; długość żniw | P1 |
| L-040 | Wpływ odległości od złoża na wydajność budynków wydobywczych | P1 |

## Religia i mit

| ID | Luka | Priorytet |
|---|---|---|
| L-050 | Koszt i ilość marmuru na fundament każdego sanktuarium; ilości materiałów do ukończenia | P1 |
| L-051 | Rozmiary sanktuariów | P1 |
| L-052 | Częstotliwość ofiar i czas do utraty łaski | P1 |
| L-053 | Szansa wysłuchania modlitwy, cooldown, cooldown wyroczni | P1 |
| L-054 | Czas trwania i siła sanktyfikacji | P1 |
| L-055 | Co wywołuje wrogość boga i częstotliwość ataków | P1 |
| L-056 | Wymagania Hero's Hall dla każdego herosa | P1 |
| L-057 | Statystyki potworów, herosów, bogów w walce | P1 |
| L-058 | Treść przepowiedni wyroczni | P2 |

## Wojsko i świat

| ID | Luka | Priorytet |
|---|---|---|
| L-060 | Statystyki jednostek (HP, atak, zasięg, prędkość) | P1 |
| L-061 | Odsetek mieszkańców służących w rabble | P1 |
| L-062 | Formuła morale | P1 |
| L-063 | Rozstrzyganie bitew za granicą | P1 |
| L-064 | Skala opinii przywódców i wartości zmian za akcje | P1 |
| L-065 | Wysokość trybutu, okupu | P1 |
| L-066 | Wymagania i statystyki jednostek atlantydzkich (archers, spearmen, charioteers) | P2 |
| L-067 | Warunek rozpoczęcia Igrzysk i formuła szansy wygranej | P2 |

## Atlantyda

| ID | Luka | Priorytet |
|---|---|---|
| L-080 | Ścieżka ewolucji domów atlantydzkich (nazwy poziomów, wymagania, w tym rodzaje nauki) | P2 |
| L-081 | Hipodrom: progi wielkości, przychody, wpływ na popularność | P2 |
| L-082 | Piramidy: efekty, rozmiary, ilości materiałów | P2 |
| L-083 | Lista nowych struktur estetycznych | P2 |

## Inne

| ID | Luka | Priorytet |
|---|---|---|
| L-090 | Katastrofy naturalne: mechanika i wyzwalanie | P2 |
| L-091 | Co zmienia poziom trudności | P2 |
| L-092 | Progi pomników Population i Happiness | P2 |
| L-093 | Struktura przygód, epizodów i typów celów (dla edytora i kampanii) | P1 |

## Jak zamykamy luki

1. Kolejne źródła od autora (np. tabele społeczności, pliki danych gry, nagrania rozgrywki z pomiarami).
2. Jeśli źródła brak: wartość robocza oznaczona `[ZAŁOŻENIE]` i strojona audytem balansu (patrz `docs/PRAKTYKI.md`, rozdział o audytach).
