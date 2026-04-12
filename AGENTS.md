# AGENTS.md

Kanoniczne zasady dla agentów AI w tym repo.

## Cel

Projekt wspiera projektowanie prostych układów elektronicznych: składanie bloków funkcjonalnych,
obliczenia, wizualizację połączeń i eksport do LTspice oraz aspekt edukacyjny. Krotkie, praktyczne
wskazowki dla uzytkownika sa mile widziane, jesli pomagaja zrozumiec uklad albo uniknac bledu.
Priorytet: poprawna logika, prosty podział na warstwy i łatwe dodawanie nowych bloków bez
dokładania logiki do UI.

## Zasada Glowna

Wprowadzaj najmniejsza sensowna zmiane we wlasciwej warstwie. Nie rob duzego refaktoru bez
konkretnej potrzeby.

## Tryb Codex

- zbieraj tylko kontekst potrzebny do biezacego zadania
- nie czytaj wielu plikow na zapas
- najpierw znajdz miejsce zmiany, potem czytaj lokalnie i wasko
- odpowiadaj krotko, konkretnie i zadaniowo
- nie powtarzaj tego, co widac juz w kodzie lub diffie
- dla prostych zmian nie tworz dlugiego planu
- preferuj male patche, maly zakres plikow i celowane testy
- uruchamiaj tylko build i testy adekwatne do zmiany, chyba ze potrzebna jest pelna weryfikacja

## Warstwy

- `model/` - logika domenowa, reguly, dane, przetwarzanie
- `ui/` - wejscie uzytkownika, prezentacja, interakcje, wywolanie logiki
- `export/` - skladanie danych do formatu wyjsciowego, bez logiki UI
- `ltspice/` - techniczne szczegoly integracji z LTspice

Jesli kod odpowiada na pytanie "jak to dziala", zwykle nie powinien trafic do callbacku Qt.

## Reguly

- nie dopisuj nowej logiki biznesowej do ciezkich plikow UI
- nie mieszaj `ui`, `model`, `export` i `ltspice`
- model nie powinien znac Qt, jesli nie jest to konieczne
- eksport nie powinien znac dialogow ani interakcji uzytkownika
- nie przepisuj stabilnego kodu tylko dlatego, ze da sie go napisac ladniej
- jesli nie wiesz, gdzie umiescic logike, preferuj `model/` zamiast UI

## Testy

- mysl testami: dla logiki, eksportu, parsowania i helperow technicznych dodaj test lub rozszerz
  istniejacy w tym samym kroku
- dla bugfixow, jesli to mozliwe, dodaj test regresyjny
- dla zmian czysto wizualnych GUI test nie jest wymagany, ale build i wlasciwe testy musza przejsc

## Workflow

1. Znajdz wlasciwa warstwe.
2. Dodaj lub rozszerz test, jesli zmienia sie logika.
3. Wprowadz najmniejsza sensowna zmiane.
4. Uruchom wlasciwy build i odpowiednie testy.

Domyslnie:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --test-dir build/debug --output-on-failure
```
