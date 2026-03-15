#include "pep/modules/psu_basic/ui/widget.hpp"

#include "../model/model.hpp"
#include "../export/export.hpp"
#include "../export/template_export.hpp"
#include "schematic_widget.hpp"
#include "waveform_widget.hpp"

#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace pep::modules::psu_basic {

Widget::Widget(QWidget* parent) : QWidget(parent) {
  auto* root = new QHBoxLayout(this);
  root->setSpacing(12);

  auto* left = new QWidget();
  auto* left_layout = new QVBoxLayout(left);

  auto* form = new QFormLayout();
  auto* vin_input = new QLineEdit();
  auto* vin_min = new QLineEdit();
  auto* vin_max = new QLineEdit();
  auto* diode_input = new QLineEdit();
  auto* diode_tol = new QLineEdit();
  auto* current_input = new QLineEdit();
  auto* current_tol = new QLineEdit();
  auto* cap_input = new QLineEdit();
  auto* cap_tol = new QLineEdit();
  auto* freq_input = new QLineEdit();
  auto* freq_tol = new QLineEdit();
  auto* rectifier_input = new QComboBox();
  auto* waveform_input = new QComboBox();

  rectifier_input->addItem("Dwupołówkowy (mostek)", static_cast<int>(RectifierType::FullWaveBridge));
  rectifier_input->addItem("Jednopołówkowy", static_cast<int>(RectifierType::HalfWave));

  waveform_input->addItem("Sinus", static_cast<int>(WaveformType::Sinus));
  waveform_input->addItem("Jednopołówkowy", static_cast<int>(WaveformType::HalfWave));
  waveform_input->addItem("Dwupołówkowy", static_cast<int>(WaveformType::FullWave));
  waveform_input->addItem("Po filtracji", static_cast<int>(WaveformType::Filtered));

  vin_input->setPlaceholderText("VAC RMS");
  vin_min->setPlaceholderText("min (np. 230)");
  vin_max->setPlaceholderText("max (np. 240)");
  diode_input->setPlaceholderText("V na diodę");
  diode_tol->setPlaceholderText("% (np. 10)");
  current_input->setPlaceholderText("A");
  current_tol->setPlaceholderText("% (np. 10)");
  cap_input->setPlaceholderText("uF");
  cap_tol->setPlaceholderText("% (np. 20)");
  freq_input->setPlaceholderText("50 / 60");
  freq_tol->setPlaceholderText("% (np. 2)");

  vin_input->setToolTip("Wartość skuteczna napięcia transformatora (RMS).\nPrzykład: 12 VAC.");
  vin_min->setToolTip("Minimalne napięcie sieci/transformatora (RMS).");
  vin_max->setToolTip("Maksymalne napięcie sieci/transformatora (RMS).");
  diode_input->setToolTip("Spadek napięcia na jednej diodzie prostownika.\nTypowo 0.7–1.0 V.");
  diode_tol->setToolTip("Rozrzut spadku diody w procentach.");
  current_input->setToolTip("Prąd pobierany przez obciążenie.");
  current_tol->setToolTip("Rozrzut prądu obciążenia w procentach.");
  cap_input->setToolTip("Pojemność kondensatora filtrującego.");
  cap_tol->setToolTip("Rozrzut pojemności w procentach.");
  freq_input->setToolTip("Częstotliwość sieci (zwykle 50 Hz lub 60 Hz).\nWpływa na tętnienia.");
  freq_tol->setToolTip("Rozrzut częstotliwości sieci w procentach.");
  rectifier_input->setToolTip("Rodzaj prostownika wpływa na częstotliwość tętnień.");
  waveform_input->setToolTip("Wybierz przebieg do wizualizacji.");

  auto* vin_row = new QWidget();
  auto* vin_row_layout = new QHBoxLayout(vin_row);
  vin_row_layout->setContentsMargins(0, 0, 0, 0);
  vin_row_layout->addWidget(vin_input, 2);
  vin_row_layout->addWidget(vin_min, 1);
  vin_row_layout->addWidget(vin_max, 1);
  form->addRow("Uwe (VAC) / zakres", vin_row);

  auto* diode_row = new QWidget();
  auto* diode_row_layout = new QHBoxLayout(diode_row);
  diode_row_layout->setContentsMargins(0, 0, 0, 0);
  diode_row_layout->addWidget(diode_input, 2);
  diode_row_layout->addWidget(diode_tol, 1);
  form->addRow("Spadek diody (V) / tol (%)", diode_row);

  auto* current_row = new QWidget();
  auto* current_row_layout = new QHBoxLayout(current_row);
  current_row_layout->setContentsMargins(0, 0, 0, 0);
  current_row_layout->addWidget(current_input, 2);
  current_row_layout->addWidget(current_tol, 1);
  form->addRow("I obciążenia (A) / tol (%)", current_row);

  auto* cap_row = new QWidget();
  auto* cap_row_layout = new QHBoxLayout(cap_row);
  cap_row_layout->setContentsMargins(0, 0, 0, 0);
  cap_row_layout->addWidget(cap_input, 2);
  cap_row_layout->addWidget(cap_tol, 1);
  form->addRow("C (uF) / tol (%)", cap_row);

  auto* freq_row = new QWidget();
  auto* freq_row_layout = new QHBoxLayout(freq_row);
  freq_row_layout->setContentsMargins(0, 0, 0, 0);
  freq_row_layout->addWidget(freq_input, 2);
  freq_row_layout->addWidget(freq_tol, 1);
  form->addRow("Częstotliwość (Hz) / tol (%)", freq_row);
  form->addRow("Prostownik", rectifier_input);
  form->addRow("Przebieg", waveform_input);

  auto* result = new QLabel("Wyniki pojawią się tutaj");
  auto* button = new QPushButton("Oblicz");
  auto* export_btn = new QPushButton("Eksport do LTspice (.asc, z szablonu)");

  auto* waveform = new WaveformWidget();
  auto* schematic = new SchematicWidget();

  auto read_or = [](QLineEdit* edit, double fallback) {
    return edit->text().isEmpty() ? fallback : edit->text().toDouble();
  };

  auto recompute = [=]() {
    Input input;
    input.vin_ac_rms = vin_input->text().toDouble();
    input.vin_min = read_or(vin_min, 0.0);
    input.vin_max = read_or(vin_max, 0.0);
    input.diode_drop = read_or(diode_input, 0.9);
    input.diode_tol_pct = read_or(diode_tol, 10.0);
    input.load_current = read_or(current_input, 0.0);
    input.current_tol_pct = read_or(current_tol, 10.0);
    input.capacitor_uF = read_or(cap_input, 0.0);
    input.cap_tol_pct = read_or(cap_tol, 20.0);
    input.mains_hz = read_or(freq_input, 50.0);
    input.mains_tol_pct = read_or(freq_tol, 2.0);
    input.rectifier = static_cast<RectifierType>(rectifier_input->currentData().toInt());

    Output out = compute(input);
    result->setText(QString(
        "Vpeak: %1 V\nVrect: %2 V\nRipple f: %3 Hz\nVdc (bez obc.): %4 V\n"
        "Vdc (z obc.): %5 V\nRipple (Vpp): %6 V\nVmin: %7 V\n\n"
        "Zakresy (tolerancje):\nVdc (z obc.) %8 .. %9 V\n"
        "Ripple (Vpp) %10 .. %11 V\nVmin %12 .. %13 V")
        .arg(out.vpeak, 0, 'f', 2)
        .arg(out.vrect, 0, 'f', 2)
        .arg(out.ripple_hz, 0, 'f', 1)
        .arg(out.vdc_no_load, 0, 'f', 2)
        .arg(out.vdc_loaded, 0, 'f', 2)
        .arg(out.ripple_vpp, 0, 'f', 3)
        .arg(out.vmin, 0, 'f', 2)
        .arg(out.vdc_loaded_min, 0, 'f', 2)
        .arg(out.vdc_loaded_max, 0, 'f', 2)
        .arg(out.ripple_vpp_min, 0, 'f', 3)
        .arg(out.ripple_vpp_max, 0, 'f', 3)
        .arg(out.vmin_min, 0, 'f', 2)
        .arg(out.vmin_max, 0, 'f', 2));

    WaveformParams params;
    params.vpeak = out.vpeak;
    params.vrect = out.vrect;
    params.ripple_vpp = out.ripple_vpp;
    params.mains_hz = input.mains_hz;
    params.type = static_cast<WaveformType>(waveform_input->currentData().toInt());
    waveform->set_params(params);

    SchematicStage stage = SchematicStage::Transformer;
    switch (params.type) {
      case WaveformType::Sinus:
      case WaveformType::Square:
      case WaveformType::Triangle:
        stage = SchematicStage::Transformer;
        break;
      case WaveformType::HalfWave:
      case WaveformType::FullWave:
        stage = SchematicStage::Rectifier;
        break;
      case WaveformType::Filtered:
        stage = SchematicStage::Capacitor;
        break;
    }
    schematic->set_stage(stage);
  };

  QObject::connect(button, &QPushButton::clicked, this, recompute);
  QObject::connect(waveform_input, &QComboBox::currentIndexChanged, this, recompute);
  QObject::connect(rectifier_input, &QComboBox::currentIndexChanged, this, recompute);
  QObject::connect(vin_input, &QLineEdit::textChanged, this, recompute);
  QObject::connect(vin_min, &QLineEdit::textChanged, this, recompute);
  QObject::connect(vin_max, &QLineEdit::textChanged, this, recompute);
  QObject::connect(diode_input, &QLineEdit::textChanged, this, recompute);
  QObject::connect(diode_tol, &QLineEdit::textChanged, this, recompute);
  QObject::connect(current_input, &QLineEdit::textChanged, this, recompute);
  QObject::connect(current_tol, &QLineEdit::textChanged, this, recompute);
  QObject::connect(cap_input, &QLineEdit::textChanged, this, recompute);
  QObject::connect(cap_tol, &QLineEdit::textChanged, this, recompute);
  QObject::connect(freq_input, &QLineEdit::textChanged, this, recompute);
  QObject::connect(freq_tol, &QLineEdit::textChanged, this, recompute);

  QObject::connect(export_btn, &QPushButton::clicked, this, [=, this]() {
    ExportInput exp;
    exp.vin_ac_rms = vin_input->text().toDouble();
    exp.mains_hz = read_or(freq_input, 50.0);
    exp.load_current = read_or(current_input, 0.0);
    exp.capacitor_uF = read_or(cap_input, 0.0);

    QSettings settings("PomocnikProjektantaElektronika", "PPE");
    QString template_path = settings.value("ltspice/templates/psu_basic_asc", "").toString();
    if (template_path.isEmpty()) {
      template_path = settings.value("ltspice/template_asc", "").toString(); // kompatybilność wstecz
    }

    std::string template_asc;
    QString template_label = "wbudowany (resource)";

    if (!template_path.isEmpty() && QFile::exists(template_path)) {
      QFile template_file(template_path);
      if (template_file.open(QIODevice::ReadOnly)) {
        template_asc = template_file.readAll().toStdString();
        template_label = template_path;
      }
    }

    if (template_asc.empty()) {
      QMessageBox::warning(this,
                           "Eksport LTspice",
                           "Nie mogę wczytać szablonu PSU niestabilizowany (.asc).\n"
                           "W tej wersji aplikacji nie ma już wbudowanego szablonu dla tego modułu.\n"
                           "Ustaw opcjonalnie własny szablon w: Plik → Ustawienia LTspice.");
      return;
    }

    const auto patched = export_schematic_from_asc_template(exp.vin_ac_rms, exp.mains_hz, exp.load_current, exp.capacitor_uF, template_asc);

    QString out_path = QFileDialog::getSaveFileName(this, "Zapisz schemat LTspice", QString(), "Schematic (*.asc)");
    if (out_path.isEmpty()) return;
    if (!out_path.endsWith(".asc", Qt::CaseInsensitive)) out_path += ".asc";

    QFile asc_file(out_path);
    if (!asc_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      QMessageBox::warning(this, "Eksport LTspice", "Nie udało się zapisać pliku:\n" + out_path);
      return;
    }
    asc_file.write(patched.asc.c_str(), static_cast<int>(patched.asc.size()));

    QString msg = "Zapisano:\n" + out_path + "\n\nUżyty szablon:\n" + template_label;
    if (!patched.warnings.empty()) {
      msg += "\n\nUwagi:\n";
      for (const auto& w : patched.warnings) {
        msg += "- " + QString::fromStdString(w) + "\n";
      }
    }
    QMessageBox::information(this, "Eksport LTspice", msg);
  });

  left_layout->addLayout(form);
  left_layout->addWidget(button);
  left_layout->addWidget(export_btn);
  left_layout->addWidget(result);
  left_layout->addStretch(1);

  auto* right = new QWidget();
  auto* right_layout = new QVBoxLayout(right);
  right_layout->addWidget(new QLabel("Przebieg napięcia"));
  right_layout->addWidget(waveform);
  right_layout->addWidget(new QLabel("Schemat blokowy"));
  right_layout->addWidget(schematic);
  right_layout->addStretch(1);

  root->addWidget(left, 1);
  root->addWidget(right, 1);

  recompute();
}

} // namespace pep::modules::psu_basic
