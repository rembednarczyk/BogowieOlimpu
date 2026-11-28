# Styl budowli miasta: przewodnik

Jedno miejsce dla każdego, kto buduje nowy model budynku (sesja albo agent). Reguła: R-018 (`10-REGULY.md`). Historia uwag właściciela i stan wdrożeń: `ASSETY.md`. Wzorzec w kodzie: domy w `tools/blender/models.py`, detal w `bo_detail.py`, historie w `bo_story.py`, kontrole w `bo_asset.py` (`check_props`).

Grafikę gry referencyjnej wolno oglądać i analizować (zrzuty od właściciela, dane sprite'ów z eZeus), wnioski zapisujemy tu słowami i liczbami; pliki graficzne oryginału nie trafiają do repo, kształtów nie odrysowujemy (R-002, R-017). Kształty, detale i nazwy są nasze.

## 1. Klimat w liczbach

| Cecha | Wartość | Gdzie w kodzie |
|---|---|---|
| Skala | 1 kafelek = 2 m (`M = 0.5`); dom 2 x 2 kafelki = działka 4 x 4 m | `bo_asset.py` |
| Paleta | 34 kolory w 64 miejscach (`PALETTE_SIZE` 64 w `bo_asset.py` i shaderze, zgodność pilnuje `check_models.py`), ciepłe i nasycone (popołudnie lata): tynk, kamień, terakota, drewno, trawa, 3 kolory kwiatów, pranie, turkus okiennic | `PALETTE` w `bo_asset.py` = `building.gdshader` (pilnuje `check_models.py`) |
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

### Kolor dachu według funkcji (przyjęte 2026-11-28)

Z daleka budynek rozpoznaje się po kolorze dachu (wniosek z analizy referencji, rozdział 7). Kolor podaje `tiles` w `tiled_roof` i `shed_roof` (część palety):

| Kategoria | Dach | Część palety |
|---|---|---|
| Domy zwykłe i elity | terakota | `roof_tile` (0,82; 0,40; 0,26) |
| Budynki publiczne, administracja, kultura, nauka | łupkowy błękit | `roof_slate` (0,40; 0,52; 0,68) |
| Sanktuaria i świątynie | złota dachówka | `roof_gold` (0,90; 0,70; 0,30) |
| Warsztaty i produkcja | brąz | `roof_brown` (0,58; 0,36; 0,22) |
| Magazyny, spichlerze, punkt handlowy, przystań | zielona patyna miedzi | `roof_patina` (0,26; 0,50; 0,46) |
| Gospodarstwa (rolnictwo i hodowla) | strzecha | `thatch` |
| Handel (stoiska, agora) | terakota i markiza w kolorze towaru | `roof_tile` + kolor towaru |
| Wojsko (mury, wieże, bramy) | szary kamień | `stone` |
| Ozdoby | bez dachu | |

**Ściany:** domy elity i budynki publiczne z niebiesko-szarego kamienia (`stone_cool` (0,68; 0,71; 0,80)), domy ludu w ciepłym tynku i cegle; chłodne ściany przy ciepłej ziemi dają kontrast (rozdział 8, punkt 2).

**Posadzki:** place, aleje i platformy sanktuariów z jasnego kamienia z barwnymi płytami w powtarzalnym wzorze naszego projektu (meander, rozety, romby) w kolorach palety (fiolet, róż, turkus, zieleń dopiszemy przy pierwszym placu).

**Gęstość ozdób według zamożności:** dzielnice ludu i gospodarstwa proste (trawa, pranie, dobra), centrum i dzielnice elity gęste (kolumnady wzdłuż ulic, posągi, fontanny, sadzawki, ogrody ze strzyżonych żywopłotów, cyprysy). Model budynku niesie ozdoby swojej klasy; ozdoby miasta (budynki estetyczne) dokładają gęstości tam, gdzie gracz je stawia.

Te same grupy i kolory koloruje minimapa miasta, gdy powstanie (decyzja właściciela 2026-11-28): gracz rozpoznaje budynek po kolorze na mapie i w mieście.

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

## 5. Historie budynków (przyjęte 2026-11-28 jako punkt wyjścia)

Tabela jest punktem wyjścia: przy pierwszym pełnym modelu kategorii jej warianty doprecyzowujemy (rekwizyty, akcenty, element od frontu) i wpisujemy tutaj, jak przy domach zwykłych w `ASSETY.md`.

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

Kategorie i zasada wariantów obowiązują też kulturę atlantydzką (rozdział 6).

## 6. Wygląd Atlantydy (przyjęty 2026-11-28)

Te same zasady R-018 (budowa element po elemencie, miękkie światło, łukowe dachy, ręczna nieregularność, życie przy budynku, bez dziur, historie), inny materiał i ornament:
- **Orichalk:** czerwonawozłoty metal na okuciach, zwieńczeniach, krawędziach dachów, drzwiach i posągach; połyskuje (niska szorstkość w shaderze), na wyższych poziomach więcej.
- **Błękit morza:** tynki i dachówki w odcieniach turkusu, błękitu i bieli; woda blisko budynków (kanały, baseny, fontanny).
- **Muszle i koralowce:** zamiast palmet i meandrów ornament z muszli, koralowców, fal i spiral; antefiksy w kształcie muszli, ogrody z muszli.
- **Kształty:** łagodniejsze, bardziej zaokrąglone niż greckie (kopuły, łuki, okrągłe okna), dachy łukowe tak samo strome.
- **Paleta:** nowe kolory (orichalk, turkus, koral, perłowa biel) dopisujemy do `PALETTE` przy pierwszym modelu atlantydzkim, w obu miejscach naraz (pilnuje `check_models.py`); miejsc jest 64.
- Nazwy, kształty i detale nasze; z gry referencyjnej tylko kategorie i liczby (R-002, R-017).

## 7. Analiza referencji: eZeus (2026-11-28)

eZeus (S-008) nie zawiera obrazów: wczytuje grafikę z zainstalowanej gry. Zawiera za to rozmiary każdej klatki sprite'ów (`spriteData/*30.h`, skala natywna: kafelek 58 x 30 px) i to, jak budynek je składa (`textures/ebuildingtextures*.cpp`, `buildings/*.cpp`). Z tego wnioski liczbowe; postać ma ok. 30-35 px wysokości (tragarz 28, kapłan 34, uzdrowiciel 37, mediana klatek), 1 m w pionie to ok. 17,8 px.

**Wysokość ponad rzutem (najwyższa klatka budynku, px; w nawiasie w wysokościach postaci):**

| Grupa | Budynki |
|---|---|
| Niskie tło miasta | mennica 13 (0,4), szpital 17 (0,5), warsztat rękodzielników 23 (0,7), kamieniarz 25 (0,8), mleczarnia 25 (0,8), gimnazjon 27 (0,8), urząd podatkowy 28 (0,8), odlewnia i tłocznia oliwy po 28-29 (0,9), winiarnia 34 (1,0), fontanna 34 (1,0) |
| Średnie | agora 41 (1,2), tartak 42 (1,3), posterunek 48 (1,4), obserwatorium 49 (1,5), college 54 (1,6), teatr 55 (1,6), spichlerz 60 (1,8), zbrojownia 64 (1,9) |
| Domy | dom ludu najwyższy poziom 68 (2,0), dom elity 89 (2,7) |
| Znaki orientacyjne | szkoła dramatyczna 90 (2,7), biblioteka 92 (2,8), uniwersytet 107 (3,2), pałac 110 (3,3), warsztat wynalazców 117 (3,5), muzeum 171 (5,2) |

**Dom ludu poziom po poziomie** (wysokość ponad rzutem 2 x 2, px; dwa warianty na poziom): poziom 1: 0 i 0, poziom 2: 0 i 0, poziom 3: 2 i 3, poziom 4: 8 i 5, poziom 5: 16 i 12, poziom 6: 31 i 36, poziom 7: 68 i 65. Uwaga: niski budynek mieści się w wysokości rombu rzutu (60 px), więc dla poziomów 1-4 liczba zaniża jego wysokość; pewne jest, że do poziomu 5 dom jest niski i przysadzisty, a w górę rośnie dopiero na poziomach 6-7. Domy atlantydzkie mają te same wysokości (`poseidonCommonHouse`). Dom elity: 60-91 px na każdym etapie.

**Drzewa, ozdoby, posągi** (wysokość całej klatki, px): drzewo oliwne 36-62 i drzewa dodatku 30-66 na jednym kafelku (1-2 postaci, czyli mniej więcej dom poziomu 5-6), winorośl 47-48, kolumny ozdobne 19-62, park 30-90, altana 84, ogród kwiatowy 60 (płaski), proporce 21-52, pomnik pamiątkowy 148-155 na 3 x 3, **posąg boga 219-257 na 2 x 2 (7-8 postaci: najwyższy obiekt w mieście poza muzeum)**.

**Kolory według funkcji na minimapie eZeus** (`widgets/eminimap.cpp`, kolory mapy, nie farby budynków; pokazują, jak oryginał grupuje funkcje): domy ludu ceglasta czerwień (164, 65, 49); domy elity i świątynie pomarańczowa czerwień (238, 65, 16); magazyny, spichlerze, punkt handlowy i przystań jasny błękit (115, 186, 247); kultura i sport (podium, college, stadion, gimnazjon) morska zieleń (33, 129, 115); rolnictwo i hodowla oliwkowy brąz (123, 113, 49); pałac złoto (230, 162, 0); drogi jasna szarość (225, 225, 225); pozostałe czarne. Teren: sucha ziemia od piaskowej (214, 170, 99) do zielonkawej z zaroślami (95, 170, 99), plaża (238, 202, 164), żyzna (155, 110, 110), las (90, 129, 41), woda (25, 105, 115), miedź (206, 105, 8), marmur i srebro (125, 235, 255), czarny marmur (45, 45, 60), skały (123, 146, 164).

eZeus nie zawiera kolorów samych budynków ani tekstur: grafika jest tylko w plikach oryginalnej gry (`Zeus_General/*.sg3`, `*.555`), wczytywanych z instalacji gracza. Do analizy farb potrzebne są zrzuty albo pliki z legalnej kopii właściciela (pliki poza repo, wnioski słowami, R-017).

Wnioski:
1. **Sylwetka miasta ma dwa piętra:** niskie, szerokie tło (domy, warsztaty, gospodarstwa, większość usług do ok. 2 postaci) i kilka wysokich znaków orientacyjnych (pałac, nauka, muzeum: 3-5 postaci). Wysokość jest nagrodą dla ważnych budowli.
2. **Dom ludu na najwyższym poziomie to ok. 2 postaci nad rzutem** (szerokość rzutu 116 px, wysokość 68 px: bryła szersza niż wyższa). Nasz dom miejski z kominem ma ok. 4 postaci, czyli względnie mniej więcej dwa razy więcej (do oceny przy proporcjach, pytanie 5 właściciela).
3. **Dwa warianty na każdy poziom domu,** wybierane z ziarna budynku (`esmallhouse.cpp`: `seed() % liczba`). U nas trzy historie z id: zgodne co do zasady, bogatsze w treści.
4. **Budynki żyją:** większość ma nakładki pracy z wieloma klatkami (rzeźbiarnia 36, laboratorium 41, zagroda 48, warsztat wynalazców 38), więc ruch przy pracy jest regułą, nie wyjątkiem (u nas kanał `working`, do zrobienia przy warsztatach).
5. **Towary widać na budynku:** magazyny mają nakładki zapełnienia (`stor_bays`), stoiska i sprzedawcy osobne nakładki towaru (u nas maska dóbr i plan wypełnienia magazynu).

Ze zrzutu właściciela (analiza słowami): kolor dachu według funkcji (przyjęte wyżej); ważne budowle na kamiennym podwyższeniu z wybrukowanym placem i wzorem pośrodku; latarnie lub pochodnie ze złotymi kulami i kolorowe proporce wokół placów; cyprysy i okrągłe drzewka przy budynkach, kwiaty w kamiennych skrzynkach przy ścieżkach; posągi na cokołach przy wejściach; mocny kontrast światła i wyraźny zarys brył (kontur do próby, pytanie 4); piaskowo-ochrowy grunt i jasne kamienne drogi.

## 8. Analiza referencji: zrzuty z Zeus Heaven (S-011, 2026-11-28)

Obejrzane jednorazowo (osiedle domów, domy ludu, dzielnica usług, sanktuarium Zeusa, świątynia, małe miasto) i usunięte; poniżej tylko opis słowami. Kształtów nie przenosimy, uczymy się kolorystyki, gęstości i rytmu.

**Kolory i materiały**
1. Dach mówi o funkcji (potwierdza rozdział 2): domy ludu i elity ceglasto-pomarańczowa dachówka; pałac i główne budynki złoto-żółta; drobne budynki usług w zieleni miedzianej patyny (turkusowo-zielone dachy) i w łupkowym błękicie; szopy gospodarcze i zabudowania rolne w zgaszonej oliwkowej zieleni; świątynia w sanktuarium czerwona dachówka nad białym marmurem.
2. Zimne ściany przy ciepłej ziemi: wiele ścian, zwłaszcza domów elity i budynków publicznych, w niebiesko-szarym, lekko liliowym kamieniu; strony w cieniu wyraźnie błękitne. Domy ludu w piaskowym tynku, z ciemnym brązem drewnianych balkonów.
3. Grunt piaskowo-ochrowy z trawą w kępach; drogi i place z kremowo-białego kamienia.
4. Kolorowe posadzki: place, aleje i platformy sanktuariów mają w jasnym bruku wstawki z płyt w kolorze fioletu, różu, turkusu i zieleni, w powtarzalnym wzorze (romby, rozety, meander).

**Gęstość i rytm**
5. Bogactwo widać po gęstości ozdób: dzielnice elity i centrum są zapchane ozdobami (kolumnady wzdłuż ulic, posągi, fontanny, sadzawki, ogrody), a dzielnice ludu i gospodarstwa są proste.
6. Kolumnady jako ozdoba ulic: rzędy wolno stojących kolumn wzdłuż alei i obwodu placów.
7. Ogrody: żywopłoty strzyżone w spirale i esy, klomby kwiatów wzdłuż dróg, rzędy cyprysów, sadzawki z błękitną wodą o miękkich, falistych brzegach.
8. Domy ludu wyższych poziomów stoją gęsto w szeregach; schodkowe, wysokie bryły z drewnianymi balkonami i daszkami na różnych wysokościach. Domy elity z ogrodzonymi ogrodami (biała balustrada) i rzędem cyprysów.
9. Sanktuarium: stopniowa platforma z białego marmuru z barwnymi płytami, świątynia z kolumnadą na szczycie, rzędy posągów z brązu na cokołach wzdłuż schodów i ogromny posąg boga przy wejściu.
10. Pomniki: masywny cokół z płaskorzeźbą zwieńczony złotą kulą, dobrze widoczny nad dachami.
11. Rynek: otwarte stragany z kolorowymi daszkami i masy towarów na widoku (beczki, amfory, kosze w rzędach).
12. Wokół pałacu las kolorowych proporców i latarni ze złotymi kulami.

**Światło**
13. Światło z lewej u góry, cień w prawo w dół; cienie chłodne, niebieskawe, przejścia miękkie, faktura malarska (zgodne z naszym `light()`).

Decyzje właściciela 2026-11-28 (wszystkie według rekomendacji): zieleń patyny dla magazynów i handlu (1), niebiesko-szare ściany domów elity i budynków publicznych (2), barwne posadzki placów, alei i sanktuariów (4), gęstość ozdób według zamożności jako reguła R-018 (5); wpisane w rozdziale 2.

## 9. Kontur (przyjęty 2026-11-28: cienki)

Kontur w przestrzeni ekranu (`game/shaders/outline.gdshader`, `boot.gd` `set_outline`, w zrzutach `outline=1` lub `2`; w przeglądarce domów przełącznik "Kontur"): ciepła ciemnobrązowa linia (0,22; 0,14; 0,09, krycie 80 %) tam, gdzie głębia między sąsiednimi pikselami skacze o więcej niż 0,18 kafelka (36 cm). Obrysowuje sylwetki, okapy nad ścianami, dachy na tle gruntu i postacie; spoiny cegieł i dachówek zostają bez linii. Grubość: cienki 1 px, wyraźny 2 px do sąsiadów. Koszt: jeden dodatkowy przebieg pełnoekranowy z czterema odczytami głębi (w przeglądarce dodatkowo drugi render sceny). Decyzja właściciela 2026-11-28: cienki (1 px) domyślnie w grze (`OUTLINE_DEFAULT`), wyłączony nad widokami specjalnymi (kolumny i płytki nakładek), test w `test_ui.gd`.

## 10. Proporcje (przyjęte 2026-11-28: B, poziomy 5 i 6 zamienione)

Wersja A: obecne domy. Wersja B (`BO_PROPORTION=B` w eksporcie, modele w `game/models_b/` poza repo): dachy poziomów 1-5 w nachyleniu z rzutu (bez `ROOF_STEEP`), kamienica z niższymi kondygnacjami (parter 2,0 m zamiast 2,3 m, piętro 1,85 m zamiast 2,1 m, drzwi 1,7 m); poziomy 6-7 bez zmian. Porównanie: `screenshot.gd build_view=9` (z kamery gry) i `build_view=9 side=1` (z boku), postać 1,75 m przy każdym domu, przy wersji B bryły zastępcze pałacu (1,62 x wysokość domu miejskiego) i posągu boga (3,57 x), według proporcji sprite'ów eZeus.

| Poziom | A | B |
|---|---|---|
| 1 szałas | 2,73 m (1,6 postaci) | 2,73 m |
| 2 chata | 4,21 m (2,4) | 3,67 m (2,1) |
| 3 lepianka | 4,32 m (2,5) | 3,98 m (2,3) |
| 4 zagroda | 4,47 m (2,6) | 4,17 m (2,4) |
| 5 kamienica | 6,76 m (3,9) | 5,83 m (3,3) |
| 6 mieszkanie | 6,20 m (3,5) | 6,20 m (3,5) |
| 7 dom miejski | 7,29 m (4,2) | 7,29 m (4,2) |

Wysokość mierzona do najwyższego punktu (komin); dane z oryginału: dom ludu najwyższego poziomu ok. 2 postaci nad rzutem, niskie poziomy przysadziste.

Wersja C (`BO_PROPORTION=C`, `game/models_c/` poza repo): B i dodatkowo niższe kondygnacje mieszkania (parter o 0,25 m, piętro o 0,2 m) i domu miejskiego (o 0,3 i 0,25 m), okna piętra mieszkania 15 cm niżej, portyk, architraw, fryz, gzyms i rekwizyty historii przesunięte razem z piętrami. Mieszkanie 5,79 m (3,3 postaci zamiast 3,6), dom miejski 6,74 m (3,9 zamiast 4,2). Porównanie: `build_view=9 compare=b,c`.

Decyzja właściciela 2026-11-28: wersja B; poziom 5 mieszkanie, poziom 6 kamienica (pełne kondygnacje, steep 1,4). Wysokości do komina: 2,73 / 3,67 / 3,98 / 4,17 / 5,95 / 6,78 / 7,29 m (1,6 / 2,1 / 2,3 / 2,4 / 3,4 / 3,9 / 4,2 postaci). Budynki ważne: pałac 1,6 raza, posąg boga 2,5 raza wysokości domu miejskiego.
