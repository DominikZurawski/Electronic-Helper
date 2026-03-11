# Release / Packaging

Cel: jeden zestaw reguł instalacji i paczkowania, niezależnie od systemu.

## Wspólne zasady
- Instalacja i paczkowanie zawsze używają tych samych reguł CMake `install()`.
- Pliki uruchomieniowe i zasoby trafiają do `CMAKE_INSTALL_PREFIX`.
- CPack tworzy archiwa `.zip/.tgz` na każdym systemie.
- Linux: instalowany jest plik `.desktop` i ikona w `share/icons`.

## Windows (EXE/MSI)
- Wymaga Qt6 + `windeployqt` w PATH.
- Skrypt: `scripts/package_windows.ps1`
- Ikona: `assets/icon.ico` (generowana przez `scripts/generate_icons.py`)
- TODO: podpisywanie kodu (skrypt: `scripts/sign_windows.ps1`)

## macOS (DMG)
- Wymaga Qt6 + `macdeployqt` w PATH.
- Skrypt: `scripts/package_macos.sh`
- Ikona: `assets/icon.icns` (generowana przez `scripts/generate_icons_macos.sh`)
- TODO: podpisywanie + notarization (skrypt: `scripts/sign_macos.sh`)

## Linux (AppImage/TGZ)
- Wymaga `linuxdeployqt` do AppImage.
- Skrypt: `scripts/package_linux.sh`
- Ikona: `assets/icon.svg` + `assets/icon.png`

## Wayland (opcjonalnie)
- GUI działa na Wayland bez zmian.
- Dla pewności można uruchamiać przez `scripts/run_gui.sh`, który ustawia `QT_QPA_PLATFORM=wayland;xcb` gdy wykryje Wayland.

## Minimalny flow
1. Zbuduj Release
2. Zainstaluj do staging
3. Zbundluj Qt
4. Uruchom CPack

Skrypty robią to automatycznie.
