# 03. Mieszkańcy i domy (Z:29-37)

Miasto ma dwie klasy: **lud pracujący** (domy zwykłe) i **elitę** (domy elity). Lud daje pracowników i lekką piechotę, elita daje ciężką piechotę i jazdę oraz płaci wyższe podatki.

## Domy zwykłe (Common Housing)

### Wyznaczanie

- Gracz wyznacza działki pojedynczo lub przeciąganiem. Każda działka musi mieć **bezpośredni dostęp do drogi** (Z:29).
- Imigranci zajmują działki i stawiają chaty. Na początku przybywają chętnie.

### Ścieżka ewolucji (Z:30-31)

| Poziom | Nazwa ref. | Wymaganie do awansu na ten poziom | Służba wojskowa |
|---|---|---|---|
| 1 | Hut | (start) | brak |
| 2 | Shack | żywność z Agory | brak |
| 3 | Hovel | + woda z Fountain, + dostęp do kultury (1 rodzaj) | rabble |
| 4 | Homestead | + fleece (wełna) | rabble |
| 5 | Tenement | + więcej kultury | rabble |
| 6 | Apartment | + olive oil | rabble |
| 7 | Townhouse | + jeszcze więcej kultury | rabble |

- Townhouse to szczyt zabudowy ludowej.
- Każde wymaganie poziomu niższego obowiązuje również na wyższych (kumulacja).
- **Atrakcyjność**: przy zaspokojonych potrzebach dom nie awansuje, jeśli atrakcyjność otoczenia jest zbyt niska (Z:31). [LUKA: progi atrakcyjności na poziom]
- "Więcej kultury" to więcej **punktów kultury** od wykonawców: Hovel 15, Tenement 35, Townhouse 45; elita 40-80 zależnie od trudności. Tabela punktów i progów: [18](18-domy-i-ekonomia-liczby.md).
- Pojemności, progi atrakcyjności (awans AppE, regres AppD), zużycie dóbr: [18](18-domy-i-ekonomia-liczby.md) i `data/housing.json`.

### Regres (Z:31-32)

- Dom cofa się, gdy skończy mu się zapas, którego wymaga.
- Regres może być wielostopniowy: dom spada do najwyższego poziomu, którego wymagania nadal spełnia. Przykład z instrukcji: Apartment bez oliwy wraca do Tenement; **Townhouse bez żywności wraca aż do Hut**.

### Służba w rabble (Z:136)

- Mieszkańcy domów od poziomu **Hovel** wzwyż walczą jako rabble.
- Walczy tylko część domowników. [LUKA: odsetek]

## Domy elity (Elite Housing)

### Warunki postawienia (Z:32)

Przed postawieniem domu elity muszą być spełnione **wszystkie**:

1. W mieście są **zgromadzone zapasy żywności, wełny i oliwy** (elita nie wprowadzi się do domu bez tych podstaw).
2. Miejsce ma dostęp do **kultury**.
3. Miejsce ma **wysoką atrakcyjność**.
4. Działka przylega do drogi.

Brak któregoś warunku wywołuje komunikat w pasku ostrzeżeń.

Domy elity mają **własne źródło wody** i nie potrzebują Fountain (Z:32, Z:91).

### Ścieżka ewolucji (Z:32-34)

| Poziom | Nazwa ref. | Wymaganie do awansu | Służba wojskowa |
|---|---|---|---|
| 1 | Residence | (start) żywność, wełna, oliwa, kultura, wysoka atrakcyjność | brak |
| 2 | Mansion | + armor (zbroja z Agory), + więcej kultury | hoplites |
| 3 | Manor | + wine, + więcej kultury | hoplites |
| 4 | Estate | + horses (konie z Agory), + więcej kultury, + dostęp do zawodów na Stadium | horsemen (i hoplici bez koni) [NIEJASNE] |

### Warunki utrzymania (Z:34)

Mieszkańcy elity zostają, dopóki mają: żywność, wełnę, oliwę z Agory, wysoką atrakcyjność i dostęp do kultury. Utrata **któregokolwiek** z tych warunków powoduje **wyprowadzkę** i pozostawienie opuszczonego, zrujnowanego domu. Utrata innych dóbr (wino, zbroja, konie) powoduje regres poziomu jak w domach zwykłych.

### Uzbrojenie elity (Z:136-137)

- Mansion i Manor: każda zbroja w domu wyposaża jednego hoplitę. Nie każdy mieszkaniec stać na zbroję.
- Estate: każda para **zbroja + koń** wyposaża jednego jeźdźca. Koń zjada tyle co człowiek (żywność z Agory).
- Gracz może świadomie wstrzymać dostawy koni, aby zachować więcej hoplitów (Z:35).

### Szlachcice (noblemen)

Szlachcic z domu elity jest chodzikiem losowym, który broni domu i siebie przed przestępcami (Z:88).

## Rada projektowa z instrukcji (Z:35)

Nie trzeba spełniać wszystkich żądań. Wyższy poziom oznacza więcej mieszkańców, więc przed awansem trzeba mieć dla nich pracę i żywność.

## Imigracja i emigracja

- Imigracja zależy od popularności władcy (patrz [13-przeglad-popularnosc.md](13-przeglad-popularnosc.md)). Wysoka popularność przyciąga, niska wypędza (Z:159-160).
- Panel populacji pokazuje: wolne miejsca w domach, trend (napływ / odpływ / stabilnie), **największy problem ograniczający imigrację** i liczbę imigrantów w poprzednim miesiącu (Z:35).
- Wykresy: historia populacji, struktura społeczna (rabble, hoplites, horsemen).
- Aphrodite's Haven: nikt nie emigruje (Z:122).
- Hera w dodatku zwiększa przyrost naturalny (P:21). Istnieje więc mechanizm urodzeń.
- Wyburzenie domów zmusza mieszkańców do opuszczenia miasta (Z:60).

## Widok zapasów (See Supplies, Z:35)

Nad domem ikony wszystkich potrzebnych dóbr; migające czerwone tło oznacza brak lub bliski brak.
