# 98. Konflikty między źródłami

Hierarchia źródeł (R-012):
1. **Reguły jakościowe**: instrukcje (S-001, S-002).
2. **Liczby**: tabele S-004 wyciągnięte z plików modelu gry ("Building Model Data", "Housing Models") > opisowe podstrony S-004 > pomiary forum S-005.
3. Konflikt nierozstrzygnięty: wybieramy wartość według hierarchii, oznaczamy status "otwarty" i dodajemy test, który łatwo przestawić.

| ID | Temat | Źródło A | Źródło B | Przyjęte | Status |
|---|---|---|---|---|---|
| K-001 | Co sanktyfikuje Hephaestus | Z:121 i S-004 sanctuaries: Armory, Sculpture Studio | S-004 gods: Armory, Foundry | Armory, Sculpture Studio | rozstrzygnięty |
| K-002 | Kogo przegania Aphrodite | Z:122: Ares, Hephaestus, Hermes, Dionysus | P:23: Ares, Hephaestus | lista z Z:122 (potwierdza S-004) | rozstrzygnięty |
| K-003 | Co sanktyfikuje Atlas | P:22: Masonry Shop, Black Marble Workshop | S-004: Timber Mill, Refinery | instrukcja | otwarty |
| K-004 | Czy Amazonki Artemis mogą walczyć za granicą | Z:141: wojska bogów wszędzie | S-004 gods: nie | tak (instrukcja; S-008 używa oryginalnego tekstu gry w oknie wysyłki) | rozstrzygnięty |
| K-005 | Kolejność dostaw pszenicy | Z:40: Horse Ranch, eksport, Granary | S-005 JonN: farma woli Storehouse od Granary | instrukcja + Storehouse tylko z Accept dla pszenicy | otwarty |
| K-006 | Odrastanie lasu | Z:18: wykarczowany las nie odrasta | S-005: drzewa ścięte przez drwali odrastają | oba: Clear Land usuwa las trwale, wycinka odrasta | rozstrzygnięty |
| K-007 | Zużycie żywności | S-004: 0,25/os./mies. (3/rok) | S-005 Moquel: 3,125/rok | S-004 | rozstrzygnięty |
| K-008 | Zasięg spacerujących | S-004: 27/35/44 | S-005: 26/34/43 | S-004, z kaflem startowym | rozstrzygnięty |
| K-009 | Próg startu mennicy | S-005: 200 srebra | S-005: 250 srebra | 200 | otwarty |
| K-010 | Wydajność Growers' Lodge | S-004: 20/rok | S-005: 12, 18, 24, 25 | 20 (maks. 25) | rozstrzygnięty |
| K-012 | Miesiąc dostawy zboża | Z:40, S-004: lipiec | S-005 JonN: styczeń | lipiec | rozstrzygnięty |
| K-017 | Apollo sanktyfikuje Museum | P:25-26: tak | S-004: Bibliotheke, Laboratory, Observatory | instrukcja | rozstrzygnięty |
| K-018 | Koszt Hunting Lodge (Mortal) | S-004 husbandry: 30 | S-004 model data: 32 | model data | rozstrzygnięty |
| K-019 | Koszt Artisans' Guild (Olympian) | S-004 mythology: 175 | model data: 125 | model data | rozstrzygnięty |
| K-020 | Pracownicy Sculpture Studio | S-004 industry: 12; S-008: 12 | model data: 18; S-005: 18 | 18 | otwarty |
| K-021 | Rozmiar Ares' Fortress, Promontory | sanctuaries: 13×8, 21×14; S-008 układy: to samo | model data: 11×8, 20×14 | 13×8, 21×14 | rozstrzygnięty |
| K-022 | Miejsce pojawienia się boga | S-004 trivia: najbliższa droga od NE, do 10 kafli | S-005: do 7 kafli, droga o najwyższej atrakcyjności | S-004 | otwarty |
| K-023 | Atrakcyjność a podatek | Z:97: wyższa atrakcyjność = wyższy podatek | S-004 i S-008: wzór bez atrakcyjności | wzór S-004; atrakcyjność działa pośrednio przez poziom domu | rozstrzygnięty |
| K-024 | Rozmiar Garden of Demeter | S-004: 18×12 | S-008 układ: 19×12 | 18×12 | otwarty |
| K-025 | Ryzyko pożaru i zawalenia farm marchwi i cebuli | S-004 tabela: kolumny odwrócone względem farmy pszenicy | brak drugiego źródła | jak w S-004 | otwarty |
| K-026 | Podatek Homestead na Olympian | S-004: TRM 2 | S-008: 1 | 2 | otwarty |
| K-027 | Rozmiar Gatehouse | S-004: 5×3 | S-008: 5×2 | 5×3 | otwarty |
| K-028 | Żywność Residence i Mansion na miesiąc | S-004: 1 i 2 (zaokrąglenie w dół) | S-008: 2 i 3 (zaokrąglenie) | S-004 | otwarty |
| K-029 | Koszt mostu (Beginner) | S-004: 8 | S-008: 5 | S-004 | otwarty |
| K-030 | Brąz na zbroję | S-005: 1 | S-008: 2 (decyzja autora) | 1 | rozstrzygnięty |
| K-031 | Przydział pracowników przy niedoborze | Z:62: sektory o wyższym priorytecie dostają pracowników pierwsze | S-008: podział proporcjonalny do potrzeb × wag priorytetu | instrukcja: kolejno od najwyższego priorytetu; równe priorytety dzielą proporcjonalnie do potrzeb, reszta według części ułamkowych | otwarty |

## Ocena S-008 (eZeus) po weryfikacji

- eZeus nie czyta plików modelu gry; statystyki budynków są wpisane w kod. Kolumna Beginner (koszt, pożar, zawalenie) zgadza się z naszą prawie wszędzie; pozostałe poziomy trudności liczy płaskimi mnożnikami (×1/1,8/2,25/2,75/3,25), więc nasze tabele per poziom są dokładniejsze.
- Zgodne z naszymi danymi: koszty sanktuariów, liczby rzeźb w sanktuariach, hipodrom, pojemności magazynów i wozów, wymagania herosów (eZeus pomija 3 triery Jazona), wszystkie pary potwór-heros.
- Nie przejmujemy uproszczeń eZeus: budynki bez wymogu drogi, jednolity zasięg spacerujących 40, jednolita liczba pracowników sanktuariów, ogólne statystyki walki, ciągła produkcja farm.
