#include "pep/modules/psu_symmetric/ui/widget.hpp"

#include "../model/model.hpp"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace pep::modules::psu_symmetric {

Widget::Widget(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);

  auto* form = new QFormLayout();
  auto* vin_input = new QLineEdit();
  auto* diode_input = new QLineEdit();
  auto* current_input = new QLineEdit();
  auto* cap_input = new QLineEdit();
  auto* freq_input = new QLineEdit();

  vin_input->setPlaceholderText("VAC RMS");
  diode_input->setPlaceholderText("V na diodę");
  current_input->setPlaceholderText("A");
  cap_input->setPlaceholderText("uF");
  freq_input->setPlaceholderText("50 / 60");

  form->addRow("Uwe (VAC)", vin_input);
  form->addRow("Spadek diody (V)", diode_input);
  form->addRow("I obciążenia (A)", current_input);
  form->addRow("C (uF)", cap_input);
  form->addRow("Częstotliwość sieci (Hz)", freq_input);

  auto* result = new QLabel("Wyniki pojawią się tutaj");
  auto* button = new QPushButton("Oblicz");

  QObject::connect(button, &QPushButton::clicked, this, [=]() {
    Input input;
    input.vin_ac_rms = vin_input->text().toDouble();
    input.diode_drop = diode_input->text().isEmpty() ? 0.9 : diode_input->text().toDouble();
    input.load_current = current_input->text().toDouble();
    input.capacitor_uF = cap_input->text().toDouble();
    input.mains_hz = freq_input->text().isEmpty() ? 50.0 : freq_input->text().toDouble();

    Output out = compute(input);
    result->setText(QString("V+ : %1 V\nV- : %2 V\nRipple (Vpp): %3 V")
                        .arg(out.vdc_plus, 0, 'f', 2)
                        .arg(out.vdc_minus, 0, 'f', 2)
                        .arg(out.ripple_vpp, 0, 'f', 3));
  });

  root->addLayout(form);
  root->addWidget(button);
  root->addWidget(result);
  root->addStretch(1);
}

} // namespace pep::modules::psu_symmetric
