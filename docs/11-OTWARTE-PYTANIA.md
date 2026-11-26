# Otwarte pytania

Pytania blokujące oznaczamy **[B]**. Każde pytanie ma rekomendację. Po decyzji odpowiedź trafia do `DECYZJE.md` albo do reguły w `10-REGULY.md`, a pytanie dostaje dopisek z odnośnikiem.

## Produkt

1. **[B]** Model biznesowy. Rekomendacja: premium (jednorazowy zakup), bez mechanik pay-to-win.
2. **[B]** Platformy i kolejność. Rekomendacja: Android (Capacitor) jako pierwszy, potem przeglądarka/PC, potem iOS.
3. Zakres wydania 1.0. Rekomendacja: odpowiednik gry podstawowej; kultura atlantydzka jako pierwsze rozszerzenie (zgodnie z `04-MECHANIKI.md`).
4. Ostateczny tytuł i języki startowe. Rekomendacja: robocze "Bogowie Olimpu", PL i EN od pierwszego tekstu.
5. Tryb wieloosobowy. Rekomendacja: później; determinizm z log komend zostawia tę drogę otwartą.

## Rozgrywka

6. ~~Jak wiernie odwzorowujemy spacerujących losowych?~~ Rozstrzygnięte: 1:1 (R-001). Algorytm to luka L-001.
7. Kamera: obrót co 90° jak w referencji czy swobodny? Rekomendacja: obrót co 90° plus płynny zoom (zgodność z czytelnością siatki), swobodny obrót do sprawdzenia w prototypie.
8. ~~Przenoszenie dorobku między epizodami?~~ Rozstrzygnięte: 1:1, miasto macierzyste zamrożone podczas kolonizacji (Z:17).
9. Tryb łagodny (bez klęsk i gniewu bogów). Rekomendacja: później, jako rozbudowa; referencja ma ustawienie trudności (L-091).
10. Mity spoza referencji w kampanii. Rekomendacja: tak, kampanie piszemy sami (R-002), mechanika ta sama.
16. **[B]** Limit 20 kompanii (Z:136) a R-004 (brak sztucznych limitów). Limit wpływa na balans walki, nie tylko na sprzęt. Rekomendacja: zostawić 20 jako wartość referencyjną w danych (1:1), zmiana jako przyszła rozbudowa.
17. **[ROZSTRZYGNIĘTE 2026-11-26: własne nazwy EN w polu `name_en` danych, zwykłe i historyczne słowa zostają; DECYZJE.md "Odpowiedzi po przeglądzie referencji"]** Nazewnictwo w grze. Instrukcje używają nazw w większości ogólnych lub mitologicznych (Granary, Agora, Hero's Hall). Rekomendacja: lista kontrolna z `02-ORYGINALNOSC.md` dla każdej nazwy; nazwy z mitologii i historii zostają, wymyślone przez twórców referencji (np. nazwa paktu atlantydzkiego) zastępujemy.
18. Kolejność dostaw tam, gdzie instrukcja milczy (L-019). Rekomendacja: przyjąć kolejność "potrzebujący budynek przetwórczy → eksport → magazyn" jako założenie robocze.

## Technologia

11. ~~Silnik~~ Rozstrzygnięte: Godot 4 (`DECYZJE.md`).
22. ~~Zakres rozbudowy silnika~~ Rozstrzygnięte: własna warstwa na Godocie, bez forka.
23. ~~Język rdzenia~~ Rozstrzygnięte: C++17 + GDExtension, GDScript dla prezentacji.
24. ~~Renderer~~ Rozstrzygnięte: Mobile (Vulkan), R-015.
12. ~~Telefon odniesienia~~ Rozstrzygnięte: Xiaomi 15T Pro.
13. Zapis w chmurze. Rekomendacja: najpierw lokalny zapis z eksportem pliku; usługi platform później.

## Źródła i prawo

14. Konsultacja prawna granic inspiracji przed fazą 3. Rekomendacja: tak, przed publicznym materiałem promocyjnym.
15. Skład zespołu i budżet na grafikę i audio. Wpływa na zakres "AAA" z `01-WIZJA.md`.
19. Źródła liczb do luk P0. Częściowo rozstrzygnięte: S-004 i S-005 zamknęły większość luk. Otwarte: L-001, L-008, L-012b, L-017. Rekomendacja: jeśli właściciel ma oryginalną grę, krótkie pomiary w niej (nagranie + licznik) rozstrzygną te luki i konflikty K-NNN szybciej niż dalsze szukanie źródeł.
20. ~~Domyślny poziom trudności~~ Rozstrzygnięte: Hero, 5 poziomów 1:1 (R-013).
21. ~~Potwierdzenie R-012~~ Rozstrzygnięte: przyjęta przez właściciela.
