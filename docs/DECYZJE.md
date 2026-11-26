# Dziennik decyzji

Pamięć projektu między sesjami. Nowe wpisy na górze sekcji. Nieaktualne oznaczamy dopiskiem **[NIEAKTUALNE: powód, data]**, nigdy nie usuwamy. Wpis: data, decyzja, powód, gdzie wdrożone, cytat właściciela (jeśli jest), odrzucone warianty, czego nie sprawdzono.

## Produkt i zakres

### 2026-11-26: Mechanika 1:1 z grą referencyjną i dodatkiem
- Decyzja: domyślna specyfikacja rozgrywki to instrukcje gry podstawowej i dodatku, odwzorowane 1:1. Rozbudowa później, przez nowe reguły.
- Cytat właściciela: "Domyślnie mechanika ma być 1:1 w całej grze. Później będziemy rozbudowywać".
- Gdzie: `docs/gameplay/`, `data/`, reguła R-001 (obowiązuje).
- Konsekwencja: wcześniejszy katalog mechanik z `docs/04-MECHANIKI.md` (pisany przed lekturą instrukcji) zastąpiony odsyłaczem do specyfikacji. Moje wcześniejsze założenie, że dodatek wprowadza "przygotowanie terenu pod zabudowę", było błędne i zostało usunięte.

### 2026-11-26: Własna tożsamość przy mechanice 1:1
- Decyzja: mechanika 1:1, natomiast nazwy wymyślone przez twórców referencji, grafika, audio, teksty, mapy i kampanie są nasze (R-002).
- Powód: bezpieczeństwo prawne i własna marka. Nazwy w `data/` (`ref_name`) to identyfikatory robocze.
- Odrzucony wariant: rekonstrukcja z oryginalnymi nazwami i wyglądem.

### 2026-11-26: Instrukcje nie trafiają do repozytorium
- Decyzja: PDF-y instrukcji (S-001, S-002) zostają poza repo; dokumenty cytują strony.
- Powód: materiał chroniony prawem autorskim.

## Proces

### 2026-11-26: Wytyczne z poprzednich projektów jako praktyki projektu
- Decyzja: `docs/PRAKTYKI.md` przenosi wytyczne właściciela (S-003) na citybuilder. `CLAUDE.md` streszcza zasady obowiązujące każdą sesję.
- Cytat właściciela: "Tutaj masz Guidelines, dostosuj praktyki pod ten projekt".

## Technologia

### 2026-11-26: Rekomendacja stosu (do potwierdzenia przez właściciela)
- Rekomendacja: TypeScript, domena symulacji jako czysty pakiet testowany w Node; renderer webowy (Babylon.js albo Three.js) wybrany pomiarem prototypu; Android przez Capacitor, iOS i PC w kolejnych etapach.
- Powód: wytyczne właściciela pochodzą z trzech gier w tym stosie (gotowe narzędzia, nawyki, potok testów i zrzutów bez GPU); sesje agentów w chmurze nie uruchamiają edytora Unity, więc testy, zrzuty i buildy w stosie webowym są w pełni dostępne dla agentów.
- Odrzucone: Unity 6 (wcześniejsza rekomendacja robocza z `07-TECHNOLOGIA.md`; brak edytora w kontenerze), Unreal (ciężki na telefonie), własny silnik natywny (koszt).
- Nie sprawdzono: wydajności 3 000 animowanych postaci w WebView na telefonie odniesienia. To jest kryterium prototypu pomiarowego.
- Status: czeka na decyzję właściciela (`11-OTWARTE-PYTANIA.md`, pytanie 11).
