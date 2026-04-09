# Plan Refaktoringu

Cel: uporządkować architekturę projektu etapami, bez przepisywania wszystkiego od zera, tak aby:
- łatwiej było zmieniać pojedyncze fragmenty,
- ograniczyć ryzyko regresji,
- poprawić podział odpowiedzialności między `ui`, `model`, `export` i `core`.

## Status Teraz

Stan refaktoringu:
- wystarczająco zamknięty
- dalsze zmiany powinny już wynikać z nowych funkcji albo konkretnych błędów, nie z ogólnego "sprzątania"

- Etap 1: zakończony
- Etap 2: zakończony
- Etap 3: zakończony
- Etap 4: zakończony
- Etap 5: zakończony
- Etap 6: opcjonalny

Ostatnio zrobione:
- `project_design/export` dostał bezpośrednie testy helperów `asc_ops.*` dla nagłówka, przesuwania współrzędnych, unikalności nazw i podmiany `FLAG`
- `ltspice_export` dostał bezpośrednie testy helperów renderujących tytuł i `Rload`
- `project_design/export` zostało dalej rozdzielone: budowanie wartości źródła wzmacniacza trafiło z `export_render.*` do `export/export_value_builders.*`
- `ltspice_export` zostało rozdzielone: helpery renderujące netlistę i schemat trafiły do `src/ltspice/ltspice_export_render.*`
- `project_design/export` zostało dalej rozdzielone: kolejność komponentów trafiła z `export_topology.*` do `export/export_component_order.*`
- rozszerzone zostały testy `project_design/export` o unikalność nazw sieci i warianty źródła sygnału
- dodane zostały testy regresyjne dla `ltspice_export`
- `project_design/model.cpp` przestał zależeć od `QStringList` i rozbija dodatkowe szyny zasilania bez Qt
- Etap 3 został domknięty jako zakończony
- `project_design/model.hpp` przestał zależeć od `QString`
- etykiety portów zostały wydzielone do helpera UI zamiast siedzieć w modelu
- tekstowe pola modelu `project_design` (`title`, `label`, `extra_pos_rails_csv`) przeszły na `std::string`
- `project_design/model` trzyma już głównie dane domenowe, a konwersje do `QString` dzieją się na granicy UI
- dodane zostały testy regresyjne dla `psu_basic/model`

Najbliższy krok:
- traktować obecny zestaw testów jako bazę regresji i rozszerzać go już tylko przy nowych funkcjach albo naprawach błędów
- ocenić, czy Etap 6 w ogóle jest potrzebny, zamiast otwierać kolejną serię refaktorów bez wyraźnej potrzeby
- nie ruszać stabilnych obszarów bez wyraźnego zysku modularności lub testowalności

Aktualny efekt:
- `src/modules/project_design/ui/widget.cpp` zmniejszył się z około `1589` linii do około `622`
- logika UI została rozbita na mniejsze pliki pomocnicze w `src/modules/project_design/ui/`
- reguły połączeń i automatyczne mapowanie zasilania zostały wydzielone do `src/modules/project_design/model/connection_ops.*`
- renderowanie canvasu, piny i linie sygnałowe zostały wydzielone do `src/modules/project_design/ui/canvas_scene.*`
- wiązania `QObject::connect(...)` i callbacków widoku zostały wydzielone do `src/modules/project_design/ui/widget_bindings.*`
- akcje użytkownika związane z blokami, klikaniem portów i eksportem aktywnego elementu zostały wydzielone do `src/modules/project_design/ui/widget_actions.*`
- `project_design/export/export.cpp` przestał mieszać dialogi, operacje tekstowe `.asc`, ładowanie szablonów i planowanie topologii w jednym pliku
- `project_design/export/export.cpp` został odchudzony także z renderowania fragmentów i pomocniczego składania danych bloków
- `project_design/export/export_topology.*` odpowiada już za sieci eksportu, a kolejność komponentów została wydzielona do `project_design/export/export_component_order.*`
- `project_design/export/export_render.*` skupia się już na renderowaniu fragmentów `.asc`, a budowanie wartości instancji trafiło do `project_design/export/export_value_builders.*`
- `src/ltspice/ltspice_export.cpp` jest już koordynatorem API, a detale renderowania trafiły do `src/ltspice/ltspice_export_render.*`
- model `project_design` nie trzyma już tekstów bloków i portów w `QString`
- `project_design/model.hpp` nie przecieka już typami Qt przez funkcje opisowe
- `project_design/model.cpp` nie używa już Qt do przetwarzania dodatkowych szyn zasilania
- pojawiły się automatyczne testy regresyjne dla `project_design/model/connection_ops.*`
- pojawiły się automatyczne testy regresyjne dla `project_design/export/*`
- pojawiły się automatyczne testy regresyjne dla `psu_basic/model`
- pojawiły się automatyczne testy regresyjne dla `ltspice_export`
- Etap 4 można uznać za domknięty: logika operacyjna została wyniesiona z ciężkich callbacków i głównych plików eksportu do osobnych helperów i serwisów
- testy `project_design/export` obejmują już nie tylko składanie końcowego `.asc`, ale też niskopoziomowe helpery `asc_ops.*`
- Etap 5 można uznać za domknięty: najważniejsze reguły domenowe i helpery po refaktorze są objęte automatycznymi testami

## Dziennik Postępu

- 2026-04-08: wydzielenie dialogu `.tran` z `export.cpp` do `ui/export_dialog.*`
- 2026-04-08: wydzielenie akcji eksportu LTspice z `widget.cpp` do `ui/export_actions.*`
- 2026-04-08: wydzielenie helperów formularza i listy połączeń do `ui/connection_ui.*`
- 2026-04-08: wydzielenie auto-layoutu canvasu do `ui/canvas_layout.*`
- 2026-04-08: wydzielenie synchronizacji formularza do `ui/form_sync.*`
- 2026-04-08: wydzielenie panelu aktywnego elementu, walidacji i wykresów do `ui/active_block_view.*`
- 2026-04-08: wydzielenie reguł połączeń, automatycznego podpinania nowych bloków i mapowania zasilania do `model/connection_ops.*`
- 2026-04-08: wydzielenie renderowania canvasu, pinów i linii sygnałowych do `ui/canvas_scene.*`
- 2026-04-08: wydzielenie wiązań sygnałów Qt i callbacków widoku do `ui/widget_bindings.*`
- 2026-04-08: wydzielenie akcji użytkownika do `ui/widget_actions.*`
- 2026-04-08: dodanie testów regresyjnych dla `model/connection_ops.*`
- 2026-04-08: dodanie testów regresyjnych dla `project_design/export.*`
- 2026-04-08: usunięcie `QPointF` z modelu `project_design` przez wprowadzenie `CanvasPoint`
- 2026-04-08: zastąpienie `QString` przez `std::string` w identyfikatorach portów i endpointów
- 2026-04-08: zastąpienie tekstowych `kind`, `variant` i `signal_waveform` enumami domenowymi
- 2026-04-08: wydzielenie operacji na `.asc` do `project_design/export/asc_ops.*`
- 2026-04-08: wydzielenie ładowania szablonów LTspice do `project_design/export/template_loader.*`
- 2026-04-08: wydzielenie budowania sieci eksportu i kolejności komponentów do `project_design/export/export_topology.*`
- 2026-04-08: wydzielenie renderowania fragmentów i helperów bloków do `project_design/export/export_render.*`
- 2026-04-08: wydzielenie kolejności komponentów eksportu do `project_design/export/export_component_order.*`
- 2026-04-08: wydzielenie helperów renderowania LTspice do `src/ltspice/ltspice_export_render.*`
- 2026-04-08: wydzielenie budowania wartości instancji wzmacniacza do `project_design/export/export_value_builders.*`
- 2026-04-08: dodanie bezpośrednich testów helperów `ltspice_export_render.*`
- 2026-04-08: dodanie bezpośrednich testów helperów `project_design/export/asc_ops.*`
- 2026-04-08: dodanie testów regresyjnych dla `psu_basic/model`
- 2026-04-08: przeniesienie `title`, `PortDef::label` i `extra_pos_rails_csv` z `QString` do `std::string` w modelu `project_design`
- 2026-04-08: wydzielenie `port_type_label(...)` do helpera UI i usunięcie `QString` z API nagłówka modelu
- 2026-04-08: usunięcie `QStringList` z implementacji modelu `project_design` i dodanie testu dla dynamicznych szyn zasilania
- 2026-04-08: dodanie testów regresyjnych dla `ltspice_export`
- 2026-04-08: rozszerzenie testów `project_design/export` o unikalne nazwy sieci i warianty `SINE/PULSE/PWL`

## Zasady

- Refaktoryzujemy etapami, bez dużego "big bang".
- Każdy etap musi kończyć się działającym buildem.
- Każdy etap powinien mieć mały, jasny zakres.
- Najpierw rozdzielamy odpowiedzialności, dopiero potem ewentualnie upraszczamy API i nazwy.
- Unikamy zmian "dla samej zmiany", jeśli nie poprawiają modularności, testowalności albo czytelności.

## Stan Wyjściowy

Największe miejsca wymagające porządków:
- `src/modules/project_design/ui/widget.cpp`
- `src/modules/project_design/export/export.cpp`
- model `project_design`, który był mocno związany z Qt

Postęp wykonany do tej pory:
- dialog `.tran` został wydzielony z `export.cpp` do osobnego pliku UI
- akcje eksportu LTspice zostały wydzielone z `widget.cpp` do osobnego helpera UI
- helpery dla formularza i listy połączeń zostały wydzielone z `widget.cpp`
- automatyczne rozmieszczanie bloków na canvasie zostało wydzielone z `widget.cpp`
- synchronizacja formularza została wydzielona z `widget.cpp`
- panel aktywnego elementu, walidacja i aktualizacja wykresów zostały wydzielone z `widget.cpp`
- reguły połączeń i automatyczne podpinanie bloków zostały wydzielone do warstwy `model`
- renderowanie canvasu zostało wydzielone do osobnego helpera UI
- wiązania sygnałów i zdarzeń zostały wydzielone z konstruktora `Widget`
- akcje użytkownika dla bloków i eksportu zostały wydzielone z `Widget`
- logika planowania eksportu została rozbita na osobne helpery dla szablonów, operacji `.asc` i topologii
- logika renderowania fragmentów i budowania danych bloków dla eksportu została wydzielona z `export.cpp`

## Etap 1: Oddzielanie UI od logiki eksportu

Status:
- zakończony

Cel:
- usunąć elementy dialogowe i zależności UI z warstwy `export`

Zakres:
- wydzielanie dialogów i promptów z `export.cpp`
- pozostawienie w `export` tylko logiki składania `.asc`, walidacji eksportu i mapowania połączeń

Kryterium zakończenia:
- publiczne API `export` nie zależy od `QWidget`
- `export.cpp` nie tworzy dialogów Qt

Wykonane:
- przeniesienie dialogu `.tran` do `ui/export_dialog.*`
- usunięcie zależności dialogowych z warstwy `export`

## Etap 2: Rozbicie `project_design/ui/widget.cpp`

Status:
- zakończony

Cel:
- zmniejszyć klasę `Widget`, która trzymała zbyt dużo odpowiedzialności

Zakres:
- wydzielenie części odpowiedzialnej za eksport akcji użytkownika
- wydzielenie połączeń portów
- wydzielenie obsługi canvasu i linii sygnałowych
- wydzielenie synchronizacji formularza z aktywnym blokiem

Wykonane:
- wydzielenie akcji eksportu do `ui/export_actions.*`
- wydzielenie helperów formularza i listy połączeń do `ui/connection_ui.*`
- wydzielenie auto-layoutu canvasu do `ui/canvas_layout.*`
- wydzielenie synchronizacji formularza do `ui/form_sync.*`
- wydzielenie panelu aktywnego elementu i walidacji do `ui/active_block_view.*`
- uproszczenie dodawania bloków w `widget.cpp` przez wspólny przepływ
- wydzielenie renderowania canvasu do `ui/canvas_scene.*`
- wydzielenie wiązań sygnałów Qt i callbacków widoku do `ui/widget_bindings.*`
- wydzielenie akcji użytkownika do `ui/widget_actions.*`

Kryterium zakończenia:
- `widget.cpp` wyraźnie maleje
- główna klasa `Widget` staje się koordynatorem, a nie miejscem dla całej logiki

Ocena:
- kryterium zostało spełnione; dalsze cięcie `widget.cpp` dawałoby już mały zysk względem kosztu

## Etap 3: Uporządkowanie modelu `project_design`

Status:
- zakończony

Cel:
- ograniczyć zależność modelu domenowego od Qt

Zakres:
- przejrzenie użycia `QString`, `QPointF` i innych typów Qt w modelu
- decyzja, co zostaje w warstwie UI, a co powinno przejść do czystszego modelu
- ewentualne wprowadzenie prostszych struktur domenowych niezależnych od widoku

Kryterium zakończenia:
- model ma mniej zależności od Qt albo zależności są jasno uzasadnione
- łatwiej testować logikę bez uruchamiania GUI

Wykonane:
- zastąpienie `QPointF` w modelu własną strukturą `CanvasPoint`
- przeniesienie konwersji pozycji bloków do `QPointF` na granicę warstwy UI
- zastąpienie `QString` przez `std::string` w `PortDef::id` i `Endpoint::port_id`
- przeniesienie konwersji identyfikatorów portów do granic warstwy UI i grafu
- zastąpienie `QString` przez `std::string` w `Block::title`, `PortDef::label` i `Block::extra_pos_rails_csv`
- przeniesienie konwersji opisowych tekstów do granicy UI
- zastąpienie parserów opartych o `QString` parserami opartymi o `std::string_view`
- wydzielenie `port_type_label(...)` z modelu do helpera UI
- usunięcie ostatniego użycia Qt z implementacji `project_design/model.cpp`
- zastąpienie tekstowych pól `kind`, `variant` i `signal_waveform` enumami
- dodanie jawnych konwersji enum `<->` identyfikatory UI tylko na granicy warstw

Ocena:
- etap można uznać za zakończony; pozostałe użycia Qt są już po stronie UI, a nie modelu domenowego

## Etap 4: Wydzielenie usług domenowych

Status:
- zakończony

Cel:
- oddzielić logikę operacyjną od kodu kontrolek i zdarzeń

Zakres:
- wydzielenie helperów lub serwisów dla walidacji połączeń
- wydzielenie helperów lub serwisów dla automatycznego łączenia zasilania
- wydzielenie helperów lub serwisów dla obliczeń i przeliczania wyników
- wydzielenie helperów lub serwisów dla składania danych do eksportu

Kryterium zakończenia:
- logika reguł biznesowych nie siedzi bezpośrednio w callbackach Qt

Wykonane:
- wydzielenie reguł połączeń do `model/connection_ops.*`
- wydzielenie automatycznego podpinania nowego bloku do istniejących połączeń
- wydzielenie mapowania `Vcc/Vee/GND` przy wyborze źródła zasilania wzmacniacza
- wydzielenie niskopoziomowych operacji na schematach `.asc` do `export/asc_ops.*`
- wydzielenie ładowania szablonów LTspice do `export/template_loader.*`
- wydzielenie planowania topologii eksportu do `export/export_topology.*`
- wydzielenie kolejności komponentów eksportu do `export/export_component_order.*`
- wydzielenie renderowania fragmentów i helperów bloków do `export/export_render.*`
- wydzielenie budowania wartości instancji wzmacniacza do `export/export_value_builders.*`
- wydzielenie helperów renderowania LTspice do `src/ltspice/ltspice_export_render.*`

Ocena:
- kryterium zostało spełnione; dalsze cięcie Etapu 4 dawałoby już mały zysk względem kosztu i ryzyka

## Etap 5: Testy

Status:
- zakończony

Cel:
- zabezpieczyć projekt przed regresjami po refaktoringu

Zakres:
- testy dla `psu_basic/model`
- testy dla `project_design/model`
- testy dla `project_design/export`
- testy podstawowych reguł połączeń portów

Kryterium zakończenia:
- najważniejsze reguły domenowe są testowane automatycznie

Wykonane:
- dodanie testu `tests/test_project_design_connection_ops.cpp`
- dodanie targetu `ppe_project_design_tests` i testu `project_design_connection_ops`
- dodanie testu `tests/test_project_design_export.cpp`
- dodanie targetu `ppe_project_design_export_tests` i testu `project_design_export`
- dodanie testu `tests/test_psu_basic_model.cpp`
- dodanie targetu `ppe_psu_basic_model_tests` i testu `psu_basic_model`
- dodanie testu `tests/test_ltspice_export.cpp`
- dodanie targetu `ppe_ltspice_export_tests` i testu `ltspice_export`
- pokrycie testami:
  - generowania netlisty LTspice dla prostego zasilacza
  - doboru domyślnego i wyliczanego `Rload`
  - wyboru schematu mostkowego vs jednopołówkowego
  - dynamicznych dodatkowych szyn zasilania w `project_design/model`
  - podstawowych obliczeń dla prostownika jednopołówkowego i mostkowego
  - zależności tętnień od częstotliwości i pojemności
  - porządku zakresów tolerancji w `psu_basic/model`
  - wykrywania duplikatów połączeń
  - automatycznego podpinania nowego bloku
  - automatycznego łączenia szyn zasilania
  - usuwania bloku razem z połączeniami
  - przepinania wzmacniacza między źródłami zasilania
  - nagłówka i dyrektywy `.tran` w eksporcie
  - ostrzeżeń dla nieobsługiwanych wariantów i elementów
  - ostrzeżeń dla sprzecznych typów sieci w eksporcie
  - kolejności komponentów i mapowania sieci po wydzieleniu `export_topology.*`
  - helperów `asc_ops.*` odpowiedzialnych za usuwanie nagłówka, przesuwanie współrzędnych, unikalność nazw i podmianę `FLAG`

Ocena:
- kryterium zostało spełnione; dalsze rozszerzanie Etapu 5 ma sens głównie przy nowych funkcjach, nowych bugach albo zmianach w istniejących regułach
- aktualne testy obejmują też:
  - budowanie danych źródła sygnału i renderowanie po wydzieleniu `export_render.*`
  - unikalne nazwy sieci dla rozłącznych grup endpointów
  - warianty generatora wejściowego `SINE`, `PULSE` i `PWL`

## Etap 6: Dalsze porządki tylko jeśli nadal są potrzebne

Status:
- opcjonalny

Cel:
- domknięcie tematów, które zostaną po etapach 1-5

Przykłady:
- poprawa nazw
- dalsze uproszczenie API
- drobne porządki w `main.cpp` i konfiguracji modułów

Warunek:
- ten etap robimy tylko wtedy, gdy wcześniejsze etapy pokażą realną potrzebę

Ocena:
- na ten moment brak wyraźnej potrzeby otwierania Etapu 6
- dalsze porządki powinny już być robione przy okazji konkretnych zmian funkcjonalnych

## Kiedy Uznajemy Refaktoring Za Zakończony

Refaktoring uznajemy za wystarczająco zamknięty, gdy:
- `project_design` ma wyraźniejszy podział odpowiedzialności
- najcięższe pliki są mniejsze i prostsze
- eksport nie miesza się z UI
- najważniejsza logika ma testy
- dalsze zmiany funkcjonalne można robić bez ciągłego "sprzątania przy okazji"

Status końcowy:
- warunki zostały spełnione

## Czego Nie Robimy

- nie przepisujemy całej aplikacji od zera
- nie wymieniamy wszystkiego na nowe wzorce tylko dlatego, że są "ładniejsze"
- nie rozwlekamy refaktoringu bez końca
- nie ruszamy stabilnych fragmentów bez konkretnego zysku
