#include "pep/modules/antenna_basic/ui/widget.hpp"

#include "../model/model.hpp"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace pep::modules::antenna_basic {

Widget::Widget(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);

  auto* form = new QFormLayout();
  auto* freq_input = new QLineEdit();
  freq_input->setPlaceholderText("MHz");

  form->addRow("Częstotliwość (MHz)", freq_input);

  auto* result = new QLabel("Wyniki pojawią się tutaj");
  auto* button = new QPushButton("Oblicz");

  QObject::connect(button, &QPushButton::clicked, this, [=]() {
    Input input;
    input.frequency_mhz = freq_input->text().toDouble();

    Output out = compute(input);
    result->setText(QString("Długość fali: %1 m\nPół fali: %2 m\nĆwierć fali: %3 m")
                        .arg(out.wavelength_m, 0, 'f', 3)
                        .arg(out.half_wave_m, 0, 'f', 3)
                        .arg(out.quarter_wave_m, 0, 'f', 3));
  });

  root->addLayout(form);
  root->addWidget(button);
  root->addWidget(result);
  root->addStretch(1);
}

} // namespace pep::modules::antenna_basic
