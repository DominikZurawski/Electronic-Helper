# Pomocnik Projektanta Elektronika (C++20)

Celem projektu jest zestaw modułów i kalkulatorów pomagających w projektowaniu elektroniki. Wersja startowa to CLI z rejestrem modułów i wstępną obsługą importu z LTspice.

## Szybki start

```bash
cmake -S . -B build
cmake --build build
./build/ppe list
```

## Polecenia CLI

```bash
ppe list
ppe run <module-id> [args...]
ppe ltspice-import <path>
```

## Plan rozwoju

- moduły: zasilacze niestabilizowane, zasilacze symetryczne, anteny, filtry, stabilizatory
- checklisty projektowe i ostrzeganie o brakach
- import LTspice: parsing `.asc`/`.cir` i mapowanie na obiekty projektu
- integracja AI: lokalne podpowiedzi, generowanie checklist

## Docker (dla każdego)

Jeśli ktoś nie ma lokalnie C++/Qt, może użyć Dockera:

```bash
docker build -t ppe-dev .
docker run --rm -it ppe-dev
```

GUI na Linuksie (X11):

```bash
docker run --rm -it -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix ppe-dev
```

Więcej w: docs/DEV_DOCKER.md

## Instalatory i paczki (wszystkie systemy)

Wszystkie systemy używają tych samych reguł `install()` i CPack. Dzięki temu dodajesz plik raz i działa wszędzie.

Skróty:
- Windows: `scripts/package_windows.ps1`
- macOS: `scripts/package_macos.sh`
- Linux: `scripts/package_linux.sh`

Szczegóły w: docs/RELEASE.md

## CI i paczki na wszystkie systemy

- CI buduje i testuje na Windows/Linux/macOS.
- Release workflow tworzy paczki na wszystkie systemy.

Pliki workflow:
- `.github/workflows/ci.yml`
- `.github/workflows/release.yml`

## Ikony aplikacji

- `assets/icon.png` oraz `assets/icon.ico` generuje skrypt:
  - `scripts/generate_icons.py`
- macOS: wygeneruj `assets/icon.icns`:
  - `scripts/generate_icons_macos.sh`

## Release jednym tagiem

Wystarczy utworzyć tag, np. `v0.1.0` — GitHub Actions zbuduje paczki na wszystkich systemach i opublikuje Release.

## Wayland (Linux)

Jeśli ktoś ma problemy z GUI na Wayland, można uruchomić:
`./scripts/run_gui.sh`

## TODO podpisywanie instalatorów

- Windows: `scripts/sign_windows.ps1`
- macOS: `scripts/sign_macos.sh`
