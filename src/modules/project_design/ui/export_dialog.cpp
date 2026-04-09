#include "export_dialog.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace pep::modules::project_design {

std::optional<std::string> prompt_tran_directive(QWidget *parent, bool has_amplifier) {
  QDialog dialog(parent);
  dialog.setWindowTitle("Eksport LTspice — symulacja (.tran)");
  dialog.setMinimumSize(760, 520);
  dialog.resize(760, 520);

  auto *layout = new QVBoxLayout(&dialog);

  auto *expl = new QLabel(&dialog);
  expl->setWordWrap(true);
  expl->setTextFormat(Qt::RichText);
  expl->setText(
      "<div style='font-size:14px; font-weight:600; margin:0 0 4px 0;'>Do czego służy "
      "<code>.tran</code>?</div>"
      "<div style='margin:0 0 10px 0;'>"
      "Analiza czasowa (transient) — pokazuje przebiegi w funkcji czasu "
      "(np. rozruch zasilania, tętnienia, odpowiedź wzmacniacza)."
      "</div>"
      "<div style='font-size:14px; font-weight:600; margin:0 0 4px 0;'>Składnia</div>"
      "<div style='margin:0 0 8px 0; padding:8px; background:#f6f6f6; border:1px solid #dedede;'>"
      "<code>.tran &lt;Tstep&gt; &lt;Tstop&gt; [Tstart [dTmax]]</code>"
      "</div>"
      "<div style='margin:0 0 6px 0;'>Znaczenie parametrów (w kolejności jak w składni):</div>"
      "<ul style='margin:0 0 10px 18px;'>"
      "<li><code>Tstep</code> — krok próbkowania dla wykresu (często 0 lub pomijany)</li>"
      "<li><code>Tstop</code> — czas końcowy symulacji (wymagany)</li>"
      "<li><code>Tstart</code> — od kiedy zapisywać dane (pomija początkowy rozruch)</li>"
      "<li><code>dTmax</code> — maksymalny krok całkowania (ogranicza „skakanie” solvera)</li>"
      "</ul>"
      "<div style='margin:0;'>Możesz pominąć <code>.tran</code> w eksporcie i dopisać ją później w "
      "LTspice.</div>");
  layout->addWidget(expl);

  auto *include = new QCheckBox("Dodaj dyrektywę .tran do eksportu", &dialog);
  include->setChecked(true);
  layout->addWidget(include);

  auto *form = new QFormLayout();
  auto *preset = new QComboBox(&dialog);
  preset->addItem("Automatycznie (dobór domyślny)", "auto");
  preset->addItem("Zasilacz (tętnienia / stan ustalony)", "psu");
  preset->addItem("Audio / wzmacniacz (krótka symulacja)", "audio");
  preset->addItem("Własne (ręcznie)", "custom");

  auto *custom = new QLineEdit(&dialog);
  custom->setPlaceholderText(".tran 0 10m 0 1u  (przykład)");

  auto *preview = new QLabel(&dialog);
  preview->setWordWrap(true);
  preview->setStyleSheet("color: #333; padding:6px; background:#fafafa; border:1px solid #e3e3e3;");
  preview->setTextFormat(Qt::RichText);

  auto *warn = new QLabel(&dialog);
  warn->setWordWrap(true);
  warn->setStyleSheet("color: #b00020;");

  form->addRow("Propozycja", preset);
  form->addRow("Dyrektywa", custom);
  form->addRow("Podgląd", preview);
  form->addRow("", warn);
  layout->addLayout(form);

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  buttons->button(QDialogButtonBox::Ok)->setText("Eksportuj");
  buttons->button(QDialogButtonBox::Cancel)->setText("Anuluj");
  layout->addWidget(buttons);

  auto compute = [&]() -> QString {
    const QString preset_id = preset->currentData().toString();
    if (preset_id == "psu") {
      return ".tran 0 2500m 2460m 10u";
    }
    if (preset_id == "audio") {
      return ".tran 0 20m 0 1u";
    }
    if (preset_id == "auto") {
      return has_amplifier ? ".tran 0 20m 0 1u" : ".tran 0 2500m 2460m 10u";
    }
    return custom->text().trimmed();
  };

  auto refresh = [&]() {
    const bool enabled = include->isChecked();
    preset->setEnabled(enabled);
    const bool is_custom = preset->currentData().toString() == "custom";
    custom->setEnabled(enabled && is_custom);
    if (!is_custom && enabled) {
      custom->setText(compute());
    }

    if (!enabled) {
      preview->setText("Brak .tran (eksport bez dyrektywy).");
      warn->setText("");
      buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
      return;
    }

    const QString directive = compute();
    preview->setText("<code>" + directive.toHtmlEscaped() + "</code>");
    if (directive.isEmpty()) {
      warn->setText("Podaj dyrektywę .tran albo odznacz dodawanie .tran.");
      buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
      return;
    }
    if (!directive.startsWith(".tran", Qt::CaseInsensitive)) {
      warn->setText("Dyrektywa musi zaczynać się od .tran");
      buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
      return;
    }

    warn->setText("");
    buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
  };

  QObject::connect(include, &QCheckBox::toggled, &dialog, refresh);
  QObject::connect(preset, &QComboBox::currentIndexChanged, &dialog, refresh);
  QObject::connect(custom, &QLineEdit::textChanged, &dialog, refresh);
  QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  preset->setCurrentIndex(has_amplifier ? preset->findData("audio") : preset->findData("psu"));
  refresh();

  if (dialog.exec() != QDialog::Accepted) {
    return std::nullopt;
  }
  if (!include->isChecked()) {
    return std::nullopt;
  }

  return compute().toStdString();
}

} // namespace pep::modules::project_design
