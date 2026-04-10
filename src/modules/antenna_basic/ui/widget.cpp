#include "pep/modules/antenna_basic/ui/widget.hpp"

#include "../model/model.hpp"
#include "../../../ui/calculation/calculation_builders.hpp"
#include "../../../ui/calculation/calculation_document.hpp"
#include "../../../ui/calculation/calculation_view.hpp"

#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <utility>

namespace pep::modules::antenna_basic {

namespace calc = pep::ui::calculation;

namespace {

calc::CalculationDocument build_document(const Input &input, const Output &output) {
  calc::CalculationDocument document;
  document.title = "Kalkulator anten";
  document.intro =
      "Podstawowe zaleznosci dla anten rezonansowych liczone z czestotliwosci pracy.";

  auto section = calc::make_section("Wymiary promiennika",
                                    "Wyniki zakladaja propagacje z predkoscia zblizona do "
                                    "predkosci swiatla w prozni.");

  section.entries.push_back(calc::make_entry(
      "Dlugosc fali", "\\lambda", calc::format_fixed(output.wavelength_m, 3), "m",
      "\\lambda = \\frac{c}{f}", "\\frac{300}{"
                                     + calc::format_fixed(input.frequency_mhz, 3) + "} = "
                                     + calc::format_fixed(output.wavelength_m, 3),
      "Pelna dlugosc fali wyznacza punkt odniesienia dla dalszego doboru anteny."));

  section.entries.push_back(calc::make_entry(
      "Pol fali", "\\frac{\\lambda}{2}", calc::format_fixed(output.half_wave_m, 3), "m",
      "\\frac{\\lambda}{2}", calc::format_fixed(output.wavelength_m, 3) + " / 2 = "
                                 + calc::format_fixed(output.half_wave_m, 3),
      "Dipol polfalowy jest jednym z najczestszych punktow startowych projektu."));

  section.entries.push_back(calc::make_entry(
      "Cwierc fali", "\\frac{\\lambda}{4}", calc::format_fixed(output.quarter_wave_m, 3), "m",
      "\\frac{\\lambda}{4}", calc::format_fixed(output.wavelength_m, 3) + " / 4 = "
                                 + calc::format_fixed(output.quarter_wave_m, 3),
      "Promiennik cwiercfalowy bywa wygodny np. dla anten monopoloowych z przeciwwagami."));

  section.notes.push_back(calc::make_info_note(
      "Uproszczenie modelu",
      "To jest przyblizenie edukacyjne. W praktyce dlugosc fizyczna anteny zalezy tez od "
      "srednicy przewodnika, izolacji i otoczenia."));
  section.steps.push_back(calc::make_step(
      "Krok projektowy",
      "Najpierw wybierz czestotliwosc pracy, a potem potraktuj wynik jako punkt startowy do "
      "strojenia mechanicznego."));
  section.checklist_items.push_back(calc::make_checklist_item(
      "Czy podana czestotliwosc jest wieksza od zera?", input.frequency_mhz > 0.0));
  section.checklist_items.push_back(calc::make_checklist_item(
      "Czy przewidziano zapas na strojenie w realnej konstrukcji?", false));

  document.sections.push_back(std::move(section));
  return document;
}

} // namespace

Widget::Widget(QWidget *parent) : QWidget(parent) {
  auto *root = new QVBoxLayout(this);

  auto *form = new QFormLayout();
  auto *freq_input = new QLineEdit();
  freq_input->setPlaceholderText("MHz");

  form->addRow("Częstotliwość (MHz)", freq_input);

  auto *result = new calc::CalculationView(this);
  auto *button = new QPushButton("Oblicz");

  QObject::connect(button, &QPushButton::clicked, this, [=]() {
    Input input;
    input.frequency_mhz = freq_input->text().toDouble();

    Output out = compute(input);
    result->set_document(build_document(input, out));
  });

  root->addLayout(form);
  root->addWidget(button);
  root->addWidget(result);
  root->addStretch(1);
}

} // namespace pep::modules::antenna_basic
