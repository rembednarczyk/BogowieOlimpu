# Biblioteki zewnętrzne

Rejestr pochodzenia (`docs/PRAKTYKI.md`, rozdział 11). Każda pozycja: wersja, źródło, licencja, sha256.

| Biblioteka | Wersja | Źródło | Licencja | Pliki | sha256 |
|---|---|---|---|---|---|
| doctest | 2.4.11 | https://github.com/doctest/doctest | MIT | `doctest/doctest.h` | 44faa038e9c3f9728efbda143748d01124ea0a27f4bf78f35a15d8fab2e039fb |
| doctest (skrypty CMake) | 2.4.11 | https://github.com/doctest/doctest `scripts/cmake/` | BSD-3-Clause (z CMake) | `doctest/cmake/doctest.cmake` | 38b354a72dd86a22919a9c71aebdd367e368281e4036f1e2e9150b76e16b5b4b |
| | | | | `doctest/cmake/doctestAddTests.cmake` (zmieniony: `;` w nazwie testu, miejsca z komentarzem "Bogowie Olimpu") | oryginał 68ff6641f7fed897e8579be58b96d01cd5a3cae3abe9ecb185e2d0a94ff7cce2 |
| nlohmann/json | 3.11.3 | https://github.com/nlohmann/json | MIT | `nlohmann/json.hpp` | 9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6 |
| godot-cpp | gałąź 4.5 (commit w `.gitmodules` / submodule) | https://github.com/godotengine/godot-cpp | MIT | `godot-cpp/` | submodule |

Licencje leżą obok plików. Nie dodajemy bibliotek na licencjach GPL/AGPL (kod reimplementacji S-006..S-008 tylko czytamy).
