# Technologia

Stan: rekomendacja czeka na decyzję właściciela (`11-OTWARTE-PYTANIA.md`, pyt. 11). Zasada z `PRAKTYKI.md`: **wybór silnika wynika z pomiaru, nie z rankingu**.

## Wymagania

- Android jako platforma pierwszorzędna; iOS i PC w kolejnych etapach.
- Tysiące animowanych postaci i budynków, duża mapa.
- Deterministyczna symulacja testowana bez GPU.
- Praca agentów w sesjach chmurowych: testy, zrzuty i buildy muszą działać w kontenerze bez GPU i bez edytorów z interfejsem graficznym.

## Rekomendacja

| Warstwa | Wybór | Powód |
|---|---|---|
| Język | TypeScript (strict, bez `any` w domenie) | stos z trzech poprzednich gier właściciela; jedna baza kodu dla symulacji, UI i narzędzi |
| Symulacja | czysty pakiet `domain` + `sim`, testy w Node (Vitest) | `PRAKTYKI.md` rozdział 2 i 3 |
| Renderer | Babylon.js albo Three.js, **wybór pomiarem prototypu** | obie biblioteki znane właścicielowi; silnik za jednym modułem re-eksportów |
| UI | warstwa DOM/HTML nad płótnem albo UI w silniku, decyzja po makietach | audyt układu skryptem na macierzy ekranów |
| Paczka mobilna | Capacitor (Android), potem iOS | doświadczenie z poprzednich projektów; świadomość, że WebView ma koszt przeglądarki |
| Testy wizualne | Playwright z rendererem programowym | zrzuty bez GPU w kontenerze |
| Strażnik zależności | dependency-cruiser | reguły warstw w CI |

### Prototyp pomiarowy (warunek decyzji)

Scena: siatka 160×160 kafli, 2 000 budynków (instancing), 3 000 animowanych postaci (animacja w teksturach lub szkieletowa z instancingiem), kamera izometryczna z zoomem. Pomiar na telefonie odniesienia: FPS, czas klatki, wywołania rysowania, temperatura po 10 min. Wariant Babylon.js i Three.js, ta sama scena.

Kryterium przyjęcia: stałe 30 FPS na telefonie odniesienia przy rozdzielczości renderu ≥ 70%.

### Odrzucone warianty

- **Unity 6**: wcześniejsza rekomendacja robocza. Odrzucony, bo agenci w chmurze nie uruchomią edytora, a potok testów i zrzutów właściciela jest webowy.
- **Unreal Engine 5**: ciężki na telefonie, ta sama przeszkoda z edytorem.
- **Własny silnik natywny**: koszt i ryzyko; wytyczne właściciela wskazują, że telefon ogranicza wypełnianie pikseli, czego natywność nie zmienia.

## Budżety wydajności (robocze, do potwierdzenia po wyborze urządzenia odniesienia)

| Element | Telefon odniesienia | Flagowiec |
|---|---|---|
| FPS | 30 stałe (ogranicznik klatek domyślnie) | 60 stałe |
| Wywołania rysowania | < 300 | < 600 |
| Postacie na ekranie | 800 | 3 000 |
| Tick symulacji (15 000 mieszkańców) | < 4 ms w Node na rdzeniu porównywalnym | |
| Pamięć | < 1,5 GB | < 3 GB |

Bramką są liczby deterministyczne (wywołania rysowania, instancje, czas ticku w Node). Czas klatki pod rendererem programowym to raport.

## Narzędzia

- Dane balansu w `data/*.json` (później ewentualnie eksport z arkusza skryptem).
- CI: typy, lint (z zakazem losowości i czasu w domenie, limitami rozmiaru), architektura, testy; pełny zestaw z audytami na main.
- Telemetria w testach zamkniętych tylko za zgodą, bez danych osobowych.
