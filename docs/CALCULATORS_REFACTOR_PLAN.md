# Plan Refaktoringu Kalkulatorów

Cel: przebudować panel kalkulatorów tak, aby:
- był czytelny przy rosnącej liczbie obliczeń,
- wspierał treści edukacyjne i instrukcje dla użytkownika,
- wyświetlał wzory przy wynikach, a nie w osobnej, odklejonej sekcji,
- korzystał z `QWebEngineView + KaTeX`,
- był generyczny i łatwy do ponownego użycia dla kolejnych elementów projektu.

## Zasady Architektoniczne

- Nie budujemy rozwiązań tylko dla zasilacza.
- Każdy nowy mechanizm musi być projektowany jako wspólny dla wielu kalkulatorów.
- Logika domenowa nie renderuje HTML i nie zna `QWebEngineView`.
- Moduły obliczeniowe zwracają dane pośrednie, a osobna warstwa renderuje je do HTML/KaTeX.
- Panel kalkulatorów ma wspierać:
  - wynik liczbowy,
  - wzór,
  - podstawienie liczb,
  - jednostkę,
  - interpretację wyniku,
  - instrukcję lub uwagę praktyczną,
  - ostrzeżenia i walidację.

## Status

Stan obecny:
- wspólna architektura kalkulatorów działa już na `CalculationDocument`, `CalculationRenderResult` i `CalculationView`,
- `project_design` i `antenna_basic` korzystają już ze wspólnego panelu obliczeń zamiast lokalnych, ręcznie składanych bloków tekstu,
- zasilacz liniowy ma już sekcje z wynikami, wzorami, podstawieniami, notatkami i checklistami,
- `ppe_gui` używa już prawdziwego `QWebEngineView + KaTeX`, a testy widgetów pozostają na stabilnym fallbacku `QTextBrowser`, żeby nie uzależniać regresji UI od headlessowego uruchamiania Chromium,
- lewy panel `project_design` przeszedł drugą iterację UX: dane administracyjne są u góry, moduły kalkulatora są bezpośrednio pod nimi jako zakładki etapowe, a wspólny panel wyników nie dubluje już sekcji dla zasilacza,
- domyślny motyw ciemny jest obsługiwany już nie tylko w HTML kalkulatora, ale też na poziomie globalnej palety i podstawowych kontrolek Qt, żeby startowy render nie wpadał w jasne pola i białe karty,
- otwartym tematem pozostaje już głównie dalsze dopracowanie wyglądu i treści kalkulatorów, nie fundament techniczny.

Postęp planu:
- [x] Etap 0: analiza problemu i wybór kierunku `QWebEngineView + KaTeX`
- [x] Etap 1: przygotowanie infrastruktury `Qt WebEngine`
- [x] Etap 2: wprowadzenie generycznego modelu danych obliczeń
- [x] Etap 3: implementacja wspólnego renderera HTML + KaTeX
- [x] Etap 4: integracja nowego panelu kalkulatora z `project_design`
- [x] Etap 5: migracja zasilacza liniowego na nowy renderer
- [x] Etap 6: dodanie treści edukacyjnych i sekcji instruktażowych
- [x] Etap 7: przygotowanie API pod kolejne elementy
- [x] Etap 8: testy regresyjne i domknięcie pierwszej iteracji
- [ ] Etap 9: druga iteracja UX i modułowe kalkulatory etapowe

## Etap 9: Druga Iteracja UX i Modułowe Kalkulatory Etapowe

Status:
- częściowo zakończony

Cel:
- przebudować kalkulatory tak, aby użytkownik widział kolejne etapy obliczeń jako osobne moduły,
- przenieść pola stricte obliczeniowe z górnej sekcji do modułów kalkulatora,
- dopracować inicjalizację motywu i realny układ panelu po uruchomieniu aplikacji.

Założenia:
- górna sekcja ma zawierać głównie dane administracyjne elementu, np. typ i podtyp,
- dolna sekcja ma domyślnie zajmować więcej miejsca, jeśli górna część jest mniejsza,
- kalkulatory etapowe mają być projektowane jako reużywalne moduły, bo dla innych wariantów, np. zasilacza impulsowego, zestaw obliczeń będzie inny,
- współdzielone pozostają fundamenty: `CalculationDocument`, `CalculationView`, walidacja, renderer HTML/KaTeX.

Zakres:
- poprawić pierwszy render motywu tak, aby startowy widok od razu respektował aktywną paletę systemową,
- dodać obsługę etapów/sekcji jako zakładek w wspólnym panelu wyników,
- przebudować `project_design`, aby dla zasilacza liniowego wejścia transformatora i prostownika były w osobnych modułach w dolnej sekcji,
- zmniejszyć górny formularz zasilacza do danych administracyjnych,
- ustawić domyślne proporcje splittera tak, aby dolna sekcja dostawała więcej miejsca po odchudzeniu górnej.

Pierwsze zadania:
- [x] dopisać poprawny re-render motywu w `CalculationView`,
- [x] dodać generyczne zakładki sekcji w rendererze / widoku kalkulatora,
- [x] przenieść wejścia transformatora i prostownika do dolnych modułów w `project_design`,
- [x] uaktualnić testy widgetów pod nowy układ.

Wykonane:
- `CalculationView` przechowuje już bieżący dokument i wykonuje ponowny render po `showEvent` oraz po zmianach palety i stylu, dzięki czemu startowy motyw nie czeka na pierwszą interakcję użytkownika,
- wspólny renderer HTML obsługuje teraz sekcje jako zakładki, więc każdy moduł może pokazywać kolejne etapy obliczeń w osobnych kartach bez budowania dedykowanego widoku od zera,
- dodano generyczny `CalculationModuleWidget`, który łączy formularz wejściowy i panel wyników i może być ponownie użyty dla kolejnych kalkulatorów i wariantów elementów,
- w `project_design` górna sekcja zasilacza liniowego została odchudzona do danych administracyjnych, a wejścia transformatora oraz prostownika przeniesiono do dolnych modułów tabowych,
- po zmniejszeniu górnej sekcji przebudowano lewy panel na jeden spójny obszar przewijany bez sztucznego pionowego podziału, a moduły kalkulatora znajdują się bezpośrednio pod sekcją typu i podtypu,
- dopięto inicjalizację i ponowną aplikację motywu ciemnego na poziomie palety aplikacji, podstawowych kontrolek Qt i widoku wyników, tak aby domyślny dark mode działał od pierwszego renderu,
- rozpoczęto drugą iterację polerki wizualnej: lżejsze zakładki, spokojniejsze karty wyników, mniejszy kontrast ramek i bardziej nowoczesny wygląd panelu obliczeń,
- testy renderera i widgetów zostały rozszerzone pod nowy układ z zakładkami oraz większym panelem wyników.

Otwarte braki:
- kolejne warianty, zwłaszcza zasilacz impulsowy, nadal wymagają własnych modułów wejściowych i własnych dokumentów obliczeń, ale fundament pod takie różnicowanie jest już gotowy.

## Etap 0: Analiza i Decyzja Techniczna

Status:
- zakończony

Cel:
- wybrać docelowy kierunek przebudowy kalkulatorów.

Decyzje:
- idziemy w `QWebEngineView + KaTeX`,
- budujemy osobny model danych obliczeń zamiast składać długie `QString`,
- renderer ma być wspólny dla wszystkich kalkulatorów,
- wzory, podstawienia i wyjaśnienia mają być wyświetlane razem z wynikiem.

Wykonane:
- analiza obecnego problemu użyteczności,
- analiza potrzeb edukacyjnych programu,
- wybór wariantu z `QWebEngineView + KaTeX`,
- ustalenie, że rozwiązanie ma być generyczne, nie specyficzne tylko dla zasilacza.

## Etap 1: Infrastruktura Qt WebEngine

Status:
- zakończony

Cel:
- przygotować techniczny fundament pod renderowanie HTML i wzorów KaTeX.

Zakres:
- rozszerzyć `CMakeLists.txt` o `Qt6::WebEngineWidgets`,
- upewnić się, że `ppe_gui` może linkować `QWebEngineView`,
- zdecydować, jak zachowywać się w środowiskach bez WebEngine,
- dodać zasoby statyczne KaTeX do projektu.

Pliki przewidywane do zmiany:
- [CMakeLists.txt](/home/dominik/Dokumenty/Projekty/Elekronika/Pomocnik%20projektanta/CMakeLists.txt)
- [assets/ppe.qrc](/home/dominik/Dokumenty/Projekty/Elekronika/Pomocnik%20projektanta/assets/ppe.qrc)
- nowy katalog np. `assets/katex/`

Kryterium zakończenia:
- aplikacja buduje się z `QWebEngineView`,
- dostępne są lokalne zasoby KaTeX,
- istnieje minimalny ekran testowy renderujący prosty wzór.

Wykonane:
- dodano opcjonalne wykrywanie `Qt6::WebEngineWidgets` w buildzie GUI,
- dodano fallback renderer bez WebEngine, aby wspólny panel obliczeń dało się rozwijać już teraz,
- dodano zasoby `assets/calculation/*` i realne zasoby `assets/katex/*`,
- dodano wspólny widget renderujący HTML dla kalkulatorów,
- naprawiono warunek w `CMakeLists.txt`, żeby `ppe_gui` i testy widgetowe budowały się także wtedy, gdy dostępne jest `Qt6::Widgets`, ale brakuje `Qt6WebEngineWidgets`.
- przywrócono build `ppe_gui` oraz testów widgetowych po instalacji `Qt6WebEngineWidgets`,
- dodano metadane pakietowe dla zależności runtime, tak aby instalator pakietu mógł dociągnąć Qt Base i Qt WebEngine automatycznie.
- rozdzielono strategię runtime i testową: aplikacja korzysta z `QWebEngineView`, a testy widgetów sprawdzają wspólne UI kalkulatorów na fallbacku `QTextBrowser`, co stabilizuje uruchamianie w środowiskach headless.

## Etap 2: Generyczny Model Danych Obliczeń

Status:
- zakończony

Cel:
- oddzielić logikę obliczeń od sposobu prezentacji.

Zakres:
- wprowadzić wspólny model np.:
  - `CalculationDocument`
  - `CalculationSection`
  - `CalculationEntry`
  - `CalculationNote`
- przewidzieć pola:
  - tytuł,
  - symbol,
  - wartość,
  - jednostka,
  - wzór w TeX,
  - podstawienie w TeX,
  - opis edukacyjny,
  - ostrzeżenie lub poziom ważności.

Ważne:
- model nie może zależeć od `QWebEngineView`,
- model powinien nadawać się do użycia przez zasilacz, wzmacniacz, antenę i kolejne moduły.

Proponowana lokalizacja:
- `src/ui/calculation/` albo `src/modules/shared/calculation/`

Kryterium zakończenia:
- istnieje neutralny model danych obliczeń,
- co najmniej jeden test jednostkowy tworzy i waliduje przykładowy dokument obliczeń.

Wykonane:
- dodano generyczne struktury `CalculationDocument`, `CalculationSection` i `CalculationEntry`,
- dodano generyczne `CalculationNote` oraz poziomy ważności notatek,
- dodano generyczne buildery `make_entry`, `make_section`, `make_info_note`, `make_warning_note`,
- dodano generyczne kroki instruktażowe i checklisty w modelu dokumentu,
- dodano generyczny walidator dokumentu obliczeń, żeby kolejne kalkulatory mogły sprawdzać kompletność danych przed renderowaniem,
- dodano test pełnego przepływu `document -> render result`, żeby wspólne API było sprawdzane integracyjnie, a nie tylko punktowo,
- model jest niezależny od konkretnego kalkulatora,
- pierwszy kalkulator korzysta już z tego formatu pośredniego.

## Etap 3: Wspólny Renderer HTML + KaTeX

Status:
- zakończony

Cel:
- zbudować jednolity renderer dla wszystkich kalkulatorów.

Zakres:
- dodać komponent np. `CalculationWebView`,
- dodać generator HTML z modelu `CalculationDocument`,
- załadować KaTeX lokalnie,
- opracować CSS dla:
  - sekcji,
  - kart wyników,
  - wzorów,
  - podstawień,
  - notatek edukacyjnych,
  - ostrzeżeń,
  - jednostek i symboli.

Ważne:
- wspierać tryb jasny i ciemny,
- HTML ma być generowany z danych pośrednich, nie ręcznie sklejany w logice modułu.

Kryterium zakończenia:
- renderer potrafi wyświetlić kilka sekcji z wartościami, wzorami i komentarzami,
- wzory KaTeX renderują się poprawnie,
- ciemny motyw działa poprawnie.

Wykonane częściowo:
- dodano wspólny generator HTML dla dokumentu obliczeń,
- dodano wspólny `CalculationView`,
- renderer obsługuje sekcje, wyniki, wzory, podstawienia i komentarze,
- renderer obsługuje także generyczne notatki i ostrzeżenia,
- renderer obsługuje także generyczne kroki i checklisty,
- renderer pokazuje teraz także wspólne uwagi walidacyjne dla niekompletnych dokumentów obliczeń,
- renderer zwraca teraz także jawny wynik renderowania z listą problemów walidacyjnych, nie tylko sam HTML,
- renderowanie niesie teraz także generyczne podsumowanie walidacji z liczbą błędów i ostrzeżeń,
- `CalculationView` pokazuje teraz także osobny, widoczny banner statusu walidacji nad panelem obliczeń,
- dodano także krótki, generyczny formatter summary walidacji do użycia poza rendererem HTML,
- placeholdery KaTeX zostały zastąpione prawdziwymi assetami i fontami pakowanymi do `qrc`,
- renderer wybiera teraz wariant `theme-light` albo `theme-dark` na podstawie palety widoku,
- dodano automatyczny test generatora HTML,
- dodano automatyczny test helperów builderów dla warstwy generycznej,
- `project_design` korzysta już z nowego renderera zamiast pojedynczego `QLabel`.
- testy widgetów celowo weryfikują wspólny układ kalkulatorów przez fallback `QTextBrowser`, a nie przez proces Chromium, dzięki czemu pozostają stabilne i nadal testują nasz kod zamiast infrastruktury `Qt WebEngine`.

## Etap 4: Integracja Panelu Kalkulatora z `project_design`

Status:
- zakończony

Cel:
- zastąpić obecny `QLabel` wspólnym, rozszerzalnym panelem obliczeń.

Zakres:
- zamienić obecny `compute_result` w [active_block_view.hpp](/home/dominik/Dokumenty/Projekty/Elekronika/Pomocnik%20projektanta/src/modules/project_design/ui/active_block_view.hpp)
  i [widget.cpp](/home/dominik/Dokumenty/Projekty/Elekronika/Pomocnik%20projektanta/src/modules/project_design/ui/widget.cpp)
  na nowy widget renderujący dokument obliczeń,
- zachować wykresy i walidację jako osobne elementy panelu,
- nie przenosić logiki domenowej do warstwy WebEngine.

Kryterium zakończenia:
- `project_design` używa nowego panelu bez regresji funkcjonalnej,
- stary długi tekst wyniku przestaje być głównym sposobem prezentacji.

Wykonane:
- obecny panel wyników w `project_design` został przepięty na wspólny `CalculationView`,
- zasilacz i wzmacniacz budują dokument obliczeń w generycznym formacie.
- `project_design` korzysta już także z jawnych problemów walidacyjnych renderera i dokleja je do etykiety walidacji obok ostrzeżeń o połączeniach.
- wspólny panel kalkulatora potrafi już sam pokazać status walidacji dokumentu, więc kolejne moduły nie muszą budować osobnego UI tylko po to, aby komunikować braki obliczeń.
- `project_design` pokazuje teraz również krótkie podsumowanie typu `1 bledow, 2 ostrzezen` zanim rozwinie listę szczegółów dokumentu obliczeń.
- po uruchomieniu realnego widoku poprawiono layout lewego panelu: formularz parametrów dostał własny obszar przewijania, a wyniki wspólnego kalkulatora nie są już spychane przez rosnącą liczbę pól wejściowych.

Otwarte braki:
- dalsze dopracowanie wyglądu po uruchomieniu na realnym WebEngine należy już do Etapu 3, ale podstawowy układ panelu po stronie `project_design` został już skorygowany po weryfikacji runtime.

## Etap 5: Migracja Zasilacza Liniowego

Status:
- zakończony

Cel:
- przepisać obecny kalkulator zasilacza tak, aby wykorzystywał nowy model danych i renderer.

Zakres:
- rozbić wynik na sekcje:
  - `Transformator`
  - `Prostownik`
  - `Filtracja`
  - `Napięcia końcowe`
  - `Uwagi praktyczne`
- dla każdego wyniku pokazać:
  - wzór,
  - podstawienie,
  - wynik,
  - jednostkę,
  - krótkie wyjaśnienie.

Ważne:
- wzory mają być przypisane bezpośrednio do wyniku, którego dotyczą,
- nie tworzyć osobnej, odklejonej sekcji „wzory”.

Kryterium zakończenia:
- zasilacz liniowy działa na nowym rendererze,
- użytkownik widzi wynik wraz z odpowiadającym mu wzorem i komentarzem.

Wykonane:
- kalkulator zasilacza liniowego działa już na wspólnym rendererze,
- wyniki są rozbite na sekcje `Transformator` oraz `Prostownik i filtracja`,
- przy wynikach wyświetlane są wzory, podstawienia, jednostki i komentarze,
- zasilacz korzysta też z notatek, kroków i checklist.

## Etap 6: Warstwa Edukacyjna

Status:
- zakończony

Cel:
- wzmocnić edukacyjny charakter programu.

Zakres:
- dodać obsługę:
  - komentarzy interpretacyjnych,
  - krótkich instrukcji,
  - praktycznych wskazówek projektowych,
  - ostrzeżeń o typowych błędach,
  - notatek „co oznacza ten wynik”.

Przykłady:
- „dla sinusoidy napięcie szczytowe jest większe od skutecznego o `sqrt(2)`”
- „duże tętnienia sugerują zwiększenie pojemności kondensatora”
- „dla transformatora `2x12 V` wpisujemy napięcie pojedynczego uzwojenia”

Kryterium zakończenia:
- panel obliczeń pokazuje nie tylko liczby, ale też sens wyniku dla użytkownika.

Wykonane:
- dodano generyczne notatki informacyjne i ostrzegawcze,
- dodano generyczne kroki instruktażowe,
- dodano generyczne checklisty kontrolne,
- zasilacz korzysta już z tych typów treści,
- antena korzysta już z tych typów treści,
- panel obliczeń pokazuje nie tylko liczby, ale także interpretację i wskazówki dla użytkownika.

Otwarte braki:
- treści edukacyjne można dalej rozwijać, ale kryterium etapu zostało już spełnione.

## Etap 7: API Pod Kolejne Elementy

Status:
- zakończony

Cel:
- upewnić się, że ten sam mechanizm da się łatwo wykorzystać dla kolejnych modułów.

Zakres:
- przygotować prosty, powtarzalny kontrakt dla nowych kalkulatorów,
- określić, jak kolejne moduły mają budować `CalculationDocument`,
- przygotować przykładowy adapter dla przynajmniej jednego innego elementu niż zasilacz.

Ważne:
- rozwiązanie ma być gotowe dla:
  - wzmacniaczy,
  - filtrów,
  - anten,
  - przyszłych elementów dodawanych do projektu.

Kryterium zakończenia:
- da się dodać nowy kalkulator bez kopiowania rendererów i bez nowego, specjalnego UI.

Wykonane:
- wspólne buildery dokumentu obliczeń są już używane przez więcej niż jeden przypadek w `project_design`,
- moduł `antenna_basic` został przepięty na wspólny `CalculationView` i generyczny `CalculationDocument`,
- dodano test widgetu potwierdzający użycie wspólnego panelu obliczeń poza zasilaczem.

Otwarte braki:
- kolejne migracje mogą rozszerzać reuse, ale kryterium etapu zostało już spełnione.

## Etap 8: Testy i Domknięcie Iteracji

Status:
- zakończony

Cel:
- zabezpieczyć nową architekturę przed regresjami.

Zakres:
- testy modelu danych obliczeń,
- testy generatora HTML,
- smoke testy widgetu kalkulatora,
- testy dla dark mode,
- testy regresyjne dla zasilacza po migracji.

Wykonane:
- istnieją testy builderów dokumentu obliczeń,
- istnieją testy renderera HTML,
- dodano test walidatora dokumentu obliczeń jako kolejny element zabezpieczający API przed regresjami.
- dodano także test pipeline’u, który sprawdza wspólny przepływ od builderów do rendererowego wyniku,
- istnieje test widgetu dla `antenna_basic`,
- istnieje test widgetu dla `project_design`.
- testy widgetowe Qt budują się znowu poprawnie po przywróceniu spójnego linkowania warstwy walidacji i renderera.

Kryterium zakończenia:
- nowa architektura kalkulatorów jest objęta podstawowymi testami,
- pierwsza iteracja nadaje się do użycia także przez kolejne moduły.

## Dziennik Postępu

- 2026-04-09: utworzenie osobnego planu refaktoringu kalkulatorów
- 2026-04-09: wybór kierunku `QWebEngineView + KaTeX`
- 2026-04-09: zapisanie wymogu, że rozwiązanie ma być generyczne i gotowe pod kolejne elementy
- 2026-04-09: dodanie generycznego modelu dokumentu obliczeń (`CalculationDocument`)
- 2026-04-09: dodanie wspólnego renderera HTML i widgetu `CalculationView`
- 2026-04-09: przepięcie `project_design` z `QLabel` na wspólny panel obliczeń
- 2026-04-09: dodanie opcjonalnej integracji z `Qt6::WebEngineWidgets` oraz fallbacku bez WebEngine
- 2026-04-09: dodanie generycznych notatek (`CalculationNote`) do modelu dokumentu obliczeń
- 2026-04-09: dodanie testu `calculation_renderer` dla generatora HTML
- 2026-04-09: dodanie generycznych builderów dokumentu obliczeń i testu `calculation_builders`
- 2026-04-09: dodanie generycznych kroków instruktażowych i checklist do dokumentu obliczeń
- 2026-04-09: migracja `antenna_basic` na wspólny `CalculationView` i generyczny dokument obliczeń
- 2026-04-09: dodanie testu `antenna_basic_widget` potwierdzającego reuse wspólnego panelu kalkulatora
- 2026-04-09: dodanie generycznego walidatora `CalculationDocument` i testu `calculation_validation`
- 2026-04-09: podłączenie walidatora do wspólnego renderera HTML, tak aby niekompletne dokumenty były oznaczane w panelu kalkulatora
- 2026-04-09: wprowadzenie jawnego `CalculationRenderResult`, aby warstwa UI mogła korzystać z HTML i problemów walidacyjnych bez parsowania treści
- 2026-04-09: integracja problemów walidacyjnych dokumentu obliczeń z etykietą walidacji w `project_design`
- 2026-04-09: dodanie widocznego bannera statusu walidacji bezpośrednio do wspólnego `CalculationView`
- 2026-04-09: dodanie testu `calculation_pipeline` dla pełnego przepływu `document -> render result`
- 2026-04-09: dodanie generycznego `CalculationValidationSummary` i podpięcie go do `CalculationRenderResult` oraz bannera statusu
- 2026-04-09: dodanie generycznego compact summary walidacji i użycie go w komunikatach `project_design`
- 2026-04-10: domknięcie etapów 2, 4, 5, 6, 7 i 8 na podstawie spełnionych kryteriów w repo
- 2026-04-10: przywrócenie builda `ppe_gui` na fallbacku `Qt6::Widgets` bez `Qt6WebEngineWidgets`
- 2026-04-10: zastąpienie placeholderów KaTeX realnymi assetami i domknięcie Etapu 1
- 2026-04-10: dodanie zależności runtime do pakowania RPM/DEB dla GUI z WebEngine
- 2026-04-10: domknięcie Etapu 3 przez realny WebEngine, prawdziwe assety KaTeX i wsparcie `theme-light` / `theme-dark`

## Sposób Aktualizacji Tego Planu

Przy każdej zakończonej części prac:
- zaznaczyć etap lub podetap jako wykonany,
- dopisać krótki wpis do `Dziennika Postępu`,
- uzupełnić sekcję `Wykonane` we właściwym etapie,
- nie zamykać etapu, jeśli rozwiązanie działa tylko dla jednego przypadku i nie nadaje się do ponownego użycia.
