# Styl budowli miasta: przewodnik

Jedno miejsce dla każdego, kto buduje nowy model budynku (sesja albo agent). Reguła: R-018 (`10-REGULY.md`). Historia uwag właściciela i stan wdrożeń: `ASSETY.md`. Wzorzec w kodzie: domy w `tools/blender/models.py`, detal w `bo_detail.py`, historie w `bo_story.py`, kontrole w `bo_asset.py` (`check_props`).

Materiały gry referencyjnej (zrzuty, instrukcja) służą tylko do analizy proporcji i nastroju, poza repo (R-002, R-017). Kształty, detale i nazwy są nasze.

## 1. Klimat w liczbach

| Cecha | Wartość | Gdzie w kodzie |
|---|---|---|
| Skala | 1 kafelek = 2 m (`M = 0.5`); dom 2 x 2 kafelki = działka 4 x 4 m | `bo_asset.py` |
| Paleta | 28 kolorów, ciepłe i nasycone (popołudnie lata): tynk, kamień, terakota, drewno, trawa, 3 kolory kwiatów, pranie, turkus okiennic | `PALETTE` w `bo_asset.py` = `building.gdshader` (pilnuje `check_models.py`) |
| Światło | miękkie przejście w cień (smoothstep od -0,05 do 0,35), cień chłodny niebieskawy, strona słoneczna ciepła, cienka ciepła poświata krawędzi, plamy koloru jak od pędzla | `light()` w `building.gdshader` |
| Dachy dwuspadowe | 1,4 raza wyższe od rzutu, połać ugięta o 7,5% swojej długości, kalenica ugięta o 8 cm w środku | `ROOF_STEEP`, `tiled_roof(curve, sag)` |
| Przybudówki | dach jednospadowy od wyższej ściany, bez wchodzenia w ściany innych części | `shed_roof` |
| Ręczna nieregularność | przechył do 1,2 cm na metr, pęcznienie ścian 2-3,5%, falujący dach do 3,6 cm; jedno gładkie pole dla całego modelu z rekwizytami; widok z daleka tylko przechył | `Builder._wonk` |
| Odstęp od ściany | rekwizyt co najmniej 3,5 cm od lica muru (tynk wystaje 3 cm) | bryła `wall` w `brick_wall` |
| Budżet trójkątów | z bliska do 120 000, z daleka do 3 000 | `check_models.py` |
| Miasto | historia z id budynku, obrót o ćwierć obrotu drzwiami do najbliższej drogi (do 4 kafelków), skala 0,92-1,00 | `boot.gd`: `house_story`, `door_turns`, `house_scale` |

## 2. Elementy stylu (R-018)

1. Realizm budowany: cegła po cegle, dachówka po dachówce (wklęsła pod spodem, wypukła na styku), drewno ułożone fizycznie (każda kłoda leży na dwóch pod nią), naczynia jako gładkie bryły obrotowe.
2. Ghibli: ciepło, miękki cień, zieleń wszędzie, gdzie jest wolny grunt, kwiaty w skrzynkach i donicach, pranie, lekko krzywe bryły, łukowe strome dachy.
3. Przerysowana greckość: akroteria i antefiksy o rozmiar większe, meandry, fryzy, terakotowe kominy z daszkiem, wieńce laurowe nad drzwiami, czarnofigurowe pasy na naczyniach, kolumny z entazą.
4. Życie przy budynku: dobra, których budynek potrzebuje, widoczne według zapasu (maska `goods`); narzędzia i wytwory tego, kto w nim pracuje.
5. Historia: 2-3 warianty każdego budynku, każdy z jednym dużym elementem od frontu, czytelnym z kamery gry (markiza, pergola, farbowana wełna); wariant trwa przez awanse i zapisy.

## 3. Budowa nowego budynku krok po kroku

1. Rozmiar z `data/building_stats.json`; model mieści się w obrysie i nie schodzi pod ziemię (pilnuje `check_models.py`).
2. Cokół pod każdą ścianą (`footing`), także pod przybudówką; każda część bryły w swojej grupie (`group`).
3. Ściany przez `_room` lub `brick_wall` z otworami; wysokość otworu nad cokołem.
4. Dach przez `tiled_roof` (zwraca wysokość kalenicy dla komina i ozdób), przybudówki `shed_roof`, strzecha `thatch_roof`. Nic nie stawiamy na dachu na sztywnej wysokości.
5. Każda pozioma płyta, na której coś stoi (podłoga portyku, balkon, architraw), rejestrowana jako bryła (`b.solid`), żeby kontrola podparcia ją widziała.
6. Dobra i rekwizyty przez funkcje `bo_detail.py`; każdy rejestruje swoje pudełko (`b.prop`).
7. Wolny grunt jako sloty (`S.slot`), zestaw historii przez `S.dress`; duży element od frontu ręcznie, w miejscu czytelnym z kamery.
8. Zieleń na końcu (`grass` szuka wolnego gruntu promieniami).
9. Widok z daleka (`b.detail` False) z tym samym podparciem i tymi samymi rekwizytami w prostszej formie.
10. Eksport: `blender -b --factory-startup --python tools/blender/models.py -- game/models <model>`; przerywa się przy każdym naruszeniu z nazwą elementu.
11. Kontrola: `python3 tools/checks/check_models.py .` i `blender -b --factory-startup --python tools/blender/test_checks.py`.
12. Przegląd oczami: zrzuty z czterech stron, z góry i od dołu (okap, szczyt), w przeglądarce domów i w grze po `godot --headless --path game --import` (bez importu gra pokazuje stare modele).

## 4. Błędy, które już się zdarzyły (i ich kontrola)

| Objaw | Przyczyna | Zabezpieczenie |
|---|---|---|
| Rekwizyt w ścianie, w innym rekwizycie, poza działką | ręczne współrzędne | `check_props` (kolizje) |
| Dach przybudówki przebija ścianę | dach z wysokości ściany niższej części | `check_props` (punkty dachu względem ścian innych grup), `shed_roof` |
| Skrzydło wisi 30 cm nad ziemią | brak cokołu | `check_props` (podparcie brył), `test_checks.py` |
| Skrzynka z kwiatami odstaje od ściany, drąg nie sięga słupka | odstęp większy niż tynk | `check_props` (rekwizyt musi dotykać czegoś, co stoi) |
| Dziura w szczycie | wklęsły wielokąt pod łukowym dachem nie wyświetla się | szczyt z wachlarza trójkątów (`tiled_roof`) |
| Cegły przebijają tynk po wygięciu | płaski tynk bez wierzchołków nie gnie się z cegłami | zagęszczenie długich krawędzi przed `_wonk` |
| Prześwit pod dachówkami | otwarte końce koryt na okapie i pod kalenicą | pas zaprawy w pierwszym i ostatnim rzędzie |
| Deska wystaje ponad dachówki | prosta obróbka na wygiętej połaci | obróbka w odcinkach po łuku |
| "Drewniany" dach | podkład wyżej niż dno dachówek | podkład 14 cm pod połacią, w kolorze terakoty |
| Białe plamy na dachach z daleka | białe podstawy kominów i obróbki | kolor zaprawy |
| Element w powietrzu pod balkonem | haki zamiast zastrzałów | zastrzał z kamiennego wspornika w ścianie |
| Gra pokazuje stare modele | brak importu | `godot --headless --path game --import` po eksporcie |

## 5. Historie budynków (propozycja do akceptacji poza domami zwykłymi)

Zasada: wariant opowiada, kto w budynku mieszka albo pracuje. Zmienia akcenty kolorów, rekwizyty, czasem układ (odbicie lustrzane) i ma jeden duży element od frontu. Mechanika bez zmian; wybór w oprawie z id budynku.

| Kategoria | Warianty (2-3) | Duży element od frontu |
|---|---|---|
| Domy zwykłe (przyjęte) | pasterze, garncarze, ogrodnicy i pszczelarze | wełna, markiza z garnkami, winorośl |
| Domy elity | rodzina atlety, kupiec morski, kapłańska rodzina | mała palestra z dyskami; żagiel jako markiza i amfory transportowe; ołtarzyk i herma przy bramie |
| Gospodarstwa i sady | stara zagroda, młode gospodarstwo, zamożny gospodarz | stóg i żuraw studni; nowy płot i sadzonki; wóz i bogata brama |
| Warsztaty i wydobycie | warsztat rodzinny, cech z uczniami, stary mistrz | wytwór rzemiosła na pokaz (stos desek, bloki kamienia, rząd amfor wina), szyld cechu, zarośnięta pracownia z kotem |
| Spichlerze i magazyny | drewniany, kamienny, z żurawiem | rampa z workami; mur z blankami; żuraw z ładunkiem |
| Handel | stragan rodzinny, kupiec z daleka, stragan świąteczny | markiza w paski w kolorze towaru; egzotyczne kosze i dywany; girlandy i wstęgi |
| Administracja, kultura, nauka | fundator, miasto, mecenas | posąg i tablica fundatora; godło miasta na fryzie; malowany fronton |
| Sanktuaria | archaiczne, klasyczne, ludowe | drewniane kolumny w jaskrawej polichromii; marmur; wota, girlandy i ofiary |
| Mury i wieże | nowe, połatane, porośnięte | czyste blanki; łaty z innego kamienia; bluszcz |
| Ozdoby | według pory roku | kwiaty, liście, owoce |

Kultura atlantydzka dostaje własny język oprawy (do decyzji właściciela przed jej pierwszym modelem); powyższe kategorie obowiązują ją tak samo.
