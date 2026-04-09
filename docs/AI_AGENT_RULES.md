# Zasady Dla Agentów AI

Cel: rozwijać projekt bez ponownego dużego refaktoru.

## 1. Właściwa warstwa

- `model/` = logika domenowa i dane
- `ui/` = tylko zbieranie danych, wyświetlanie wyniku, wywołanie logiki
- `export/` = składanie danych do formatu wyjściowego, bez dialogów
- `ltspice/` = techniczne szczegóły LTspice

Reguła:
- jeśli kod odpowiada na pytanie „jak to działa”, nie powinien lądować bezpośrednio w callbacku Qt

## 2. DDD-lite

Nie robimy pełnego DDD.

Robimy tylko tyle:
- model nie powinien znać Qt, jeśli nie musi
- UI nie powinno zawierać reguł biznesowych
- eksport nie powinien znać okien dialogowych

## 3. TDD-lite

- dla logiki domenowej, eksportu, parsowania i helperów technicznych: najpierw test albo test w tym samym kroku
- dla bugfixów: jeśli to możliwe, test regresyjny
- dla czysto wizualnych zmian GUI: test nie jest obowiązkowy, ale build i istniejące testy muszą przejść

## 4. Kiedy wydzielić helper lub nowy plik

Wydziel, gdy:
- logika ma własne wejście i wyjście
- da się ją sensownie nazwać
- może być testowana osobno
- zaczyna puchnąć plik koordynujący

Nie wydzielaj pliku tylko po to, żeby „było bardziej enterprise”.

## 5. Czego nie robić

- nie dopisuj nowej logiki biznesowej do ciężkich plików UI
- nie mieszaj `ui`, `model`, `export` i `ltspice`
- nie zaczynaj dużego refaktoru bez konkretnej potrzeby
- nie przepisuj stabilnego kodu tylko dlatego, że da się go napisać „ładniej”

## 6. Checklista przed zakończeniem zmiany

1. Czy kod trafił do właściwej warstwy?
2. Czy da się go testować bez GUI, jeśli to logika?
3. Czy nie dociążyłem znowu ciężkiego pliku?
4. Czy build przechodzi?
5. Czy właściwe testy przechodzą?

## 7. Domyślny workflow

1. Znajdź właściwą warstwę.
2. Jeśli zmieniasz regułę, dopisz lub rozszerz test.
3. Implementuj.
4. Uruchom:
   - `cmake --build build`
   - `ctest --test-dir build --output-on-failure`
   - `cmake --build build --target format-check`

## 8. Złota zasada

Lepiej dodać mały helper i jeden test niż znowu dopisywać dziesiątki linii do pliku, który już kiedyś trzeba było ciężko refaktorować.
