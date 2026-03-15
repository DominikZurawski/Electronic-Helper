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
- `include/pep/modules/psu_basic_ui.hpp`
- `include/pep/modules/psu_symmetric_ui.hpp`
- `include/pep/modules/antenna_basic_ui.hpp`
