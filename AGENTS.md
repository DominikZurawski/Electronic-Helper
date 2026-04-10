# AGENTS.md

Ten plik jest kanonicznym źródłem zasad dla agentów AI pracujących w tym repozytorium.

Cel: rozwijać projekt bez ponownego dużego refaktoru i bez dokładania logiki do niewłaściwych warstw.

## Cel Programu

Ten program służy do wspomagania projektowania prostych układów elektronicznych.
Jego głównym celem jest umożliwienie użytkownikowi składania projektu z bloków funkcjonalnych
(np. zasilaczy, wzmacniaczy, filtrów), obliczania podstawowych parametrów, wizualizacji połączeń
oraz eksportu projektu do LTspice.

Priorytetem nie jest rozbudowana architektura sama w sobie, tylko:
- poprawna logika obliczeń
- czytelny podział na warstwy
- łatwość rozwijania kolejnych bloków i eksportu
- unikanie ponownego przeciążania warstwy UI

Przy podejmowaniu decyzji projektowych preferuj rozwiązania, które ułatwiają dodawanie nowych
bloków, reguł obliczeń i eksportów, zamiast rozbudowywania logiki bezpośrednio w GUI.

## Start Here

Przed rozpoczęciem pracy:

1. Przeczytaj ten plik.
2. Znajdź właściwą warstwę dla zmiany.
3. Wybierz możliwie najmniejszą sensowną zmianę.
4. Jeśli zmieniasz logikę, dodaj test lub rozszerz istniejący.
5. Po zakończeniu uruchom build i odpowiednie testy.

## Właściwa Warstwa

- `model/` = logika domenowa, reguły, dane, przetwarzanie
- `ui/` = zbieranie danych, prezentacja, obsługa interakcji, wywołanie logiki
- `export/` = składanie danych do formatu wyjściowego, bez dialogów i bez logiki UI
- `ltspice/` = techniczne szczegóły integracji z LTspice

Praktyczna reguła:
- jeśli kod odpowiada na pytanie „jak to działa”, nie powinien lądować bezpośrednio w callbacku Qt

Przykłady:
- walidacja parametrów zasilacza -> `model/`
- reakcja na kliknięcie przycisku -> `ui/`
- składanie pliku `.asc` -> `export/`
- mapowanie szczegółów formatu LTspice -> `ltspice/`

## Reguły Obowiązkowe

- nie dopisuj nowej logiki biznesowej do ciężkich plików UI
- nie mieszaj `ui`, `model`, `export` i `ltspice`
- model nie powinien znać Qt, jeśli nie jest to absolutnie konieczne
- eksport nie powinien znać okien dialogowych
- nie zaczynaj dużego refaktoru bez konkretnej potrzeby
- nie przepisuj stabilnego kodu tylko dlatego, że da się go napisać „ładniej”

## DDD-lite

Nie robimy pełnego DDD.

Robimy tylko tyle:
- model ma pozostać możliwie niezależny od UI i frameworka
- UI nie powinno zawierać reguł biznesowych
- eksport ma operować na danych wejściowych i wyjściowych, nie na interakcji z użytkownikiem

## TDD-lite

- dla logiki domenowej, eksportu, parsowania i helperów technicznych: najpierw test albo test w tym samym kroku
- dla bugfixów: jeśli to możliwe, dodaj test regresyjny
- dla czysto wizualnych zmian GUI: test nie jest obowiązkowy, ale build i istniejące testy muszą przejść

## Kiedy Wydzielić Helper Lub Nowy Plik

Wydziel, gdy:
- logika ma własne wejście i wyjście
- da się ją sensownie nazwać
- może być testowana osobno
- zaczyna puchnąć plik koordynujący

Nie wydzielaj pliku tylko po to, żeby „było bardziej enterprise”.

## When Unsure

- wybierz mniejszą zmianę zamiast większej
- nie rób refaktoru „przy okazji”, jeśli nie pomaga bezpośrednio w zadaniu
- jeśli zmiana dotyka architektury, najpierw opisz krótko kierunek i ryzyko
- jeśli nie wiesz, gdzie umieścić logikę, preferuj `model/` zamiast callbacku UI

## Checklista Przed Zakończeniem

1. Czy kod trafił do właściwej warstwy?
2. Czy da się go testować bez GUI, jeśli to logika?
3. Czy nie dociążyłem znowu ciężkiego pliku?
4. Czy build przechodzi?
5. Czy właściwe testy przechodzą?

## Domyślny Workflow

1. Znajdź właściwą warstwę.
2. Jeśli zmieniasz regułę, dopisz lub rozszerz test.
3. Implementuj najmniejszą sensowną zmianę.
4. Uruchom:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --test-dir build/debug --output-on-failure
```

Jeśli zadanie dotyczy tylko części systemu, uruchom przynajmniej właściwy podzbiór testów dla tej zmiany.

## Złota Zasada

Lepiej dodać mały helper i jeden test niż znowu dopisywać dziesiątki linii do pliku, który już kiedyś trzeba było ciężko refaktorować.
