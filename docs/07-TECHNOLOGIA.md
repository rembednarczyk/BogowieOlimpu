# Technologia

Stan: do decyzji. Poniżej opcje i rekomendacja robocza.

## Wymagania

- Android i iOS jako platformy pierwszorzędne, PC (Steam) jako drugorzędna.
- Tysiące animowanych postaci, setki budynków, duża mapa.
- Deterministyczna symulacja w osobnym wątku.
- Szybkie iteracje designu (hot reload danych).

## Opcje silnika

| Silnik | Plusy | Minusy |
|---|---|---|
| Unity 6 (URP) + DOTS/ECS | Dojrzały mobile, duży rynek assetów, ECS do masowej symulacji, instancjonowanie GPU | Licencjonowanie, złożoność DOTS |
| Godot 4 | Otwarte, lekkie, brak opłat, dobre 2D/3D | Słabsza wydajność 3D na mobile, mniej narzędzi AAA |
| Unreal Engine 5 | Najwyższa jakość grafiki, Mass Entity do tłumów | Ciężki na mobile, duże buildy, dłuższe iteracje |
| Własny silnik (np. C++/Rust + bgfx/wgpu) | Pełna kontrola wydajności | Bardzo duży koszt i ryzyko |

**Rekomendacja robocza:** Unity 6 z URP. Rdzeń symulacji jako osobna biblioteka C# (bez zależności od Unity), uruchamiana w testach poza silnikiem. Rendering postaci przez GPU instancing z animacją w teksturach (vertex animation textures).

## Budżety wydajności (wartości robocze)

| Element | Telefon średni | Flagowiec |
|---|---|---|
| FPS | 30 stałe | 60 stałe |
| Draw calls | < 300 | < 600 |
| Postacie na ekranie | 800 | 3 000 |
| Pamięć RAM | < 1.5 GB | < 3 GB |
| Rozmiar instalacji | < 2 GB z pobieranymi pakietami | jw. |

Urządzenie referencyjne "średnie": do ustalenia (np. telefon z Androidem z segmentu średniego sprzed 3 lat).

## Narzędzia

- Repozytorium danych balansu w arkuszach z eksportem do plików danych.
- CI: testy symulacji, test powtarzalności, buildy mobilne.
- Telemetria w testach zamkniętych (anonimowa, za zgodą).
