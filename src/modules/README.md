# Moduły

Każdy moduł ma własny katalog:

- `psu_basic/`
- `psu_symmetric/`
- `antenna_basic/`
- `project_design/`

Wewnątrz modułów:
- `model/` = logika/obliczenia
- `ui/` = GUI modułu
- `export/` = eksport LTspice (jeśli dotyczy)
- `graph/` = elementy grafu/scene (project_design)

Interfejsy GUI (czytelne include’y):
- `include/pep/modules/project_design.hpp`
- `include/pep/modules/antenna_basic_ui.hpp`

Uwaga:
- `psu_basic` i `psu_symmetric` są obecnie „headless” (bez własnego GUI).
- Współdzielone komponenty UI (np. `WaveformWidget`) zostają w `psu_basic/ui/` i są używane przez `project_design`.
