#include "active_block_view.hpp"

#include "../model/model.hpp"
#include "enum_ui.hpp"

#include "../../psu_basic/model/model.hpp"
#include "../../../ui/calculation/calculation_builders.hpp"
#include "../../../ui/calculation/calculation_view.hpp"
#include "../../../ui/calculation/calculation_document.hpp"
#include "../../../ui/calculation/calculation_validation.hpp"

#include <QLabel>
#include <QTabWidget>

#include <vector>

namespace pep::modules::project_design {
using pep::ui::calculation::CalculationDocument;
using pep::ui::calculation::CalculationEntry;
using pep::ui::calculation::CalculationSection;
using pep::ui::calculation::CalculationValidationIssue;
using pep::ui::calculation::CalculationValidationSeverity;
using pep::ui::calculation::format_validation_compact_summary;
using pep::ui::calculation::format_fixed;
using pep::ui::calculation::make_checklist_item;
using pep::ui::calculation::make_entry;
using pep::ui::calculation::make_info_note;
using pep::ui::calculation::make_section;
using pep::ui::calculation::make_step;
using pep::ui::calculation::make_warning_note;

namespace {

QString endpoint_label(const Block &block, const PortDef &port) {
  return QString("#%1 %2: %3")
      .arg(block.id)
      .arg(QString::fromStdString(block.title))
      .arg(QString::fromStdString(port.label));
}

QString ports_text_for(const Block &active) {
  QString ports_text = "Porty:\n";
  for (const auto &port : ports_for(active)) {
    ports_text +=
        "- " + QString::fromStdString(port.label) + " (" + port_type_label(port.type) + ")\n";
  }
  return ports_text.trimmed();
}

pep::modules::psu_basic::WaveformShape transformer_waveform_for(const Block &active) {
  if (active.transformer_waveform == SignalWaveform::Square) {
    return pep::modules::psu_basic::WaveformShape::Square;
  }
  if (active.transformer_waveform == SignalWaveform::Triangle) {
    return pep::modules::psu_basic::WaveformShape::Triangle;
  }
  return pep::modules::psu_basic::WaveformShape::Sine;
}

pep::modules::psu_basic::VoltageQuantity transformer_quantity_for(const Block &active) {
  return active.transformer_voltage_quantity == VoltageQuantity::Peak
             ? pep::modules::psu_basic::VoltageQuantity::Peak
             : pep::modules::psu_basic::VoltageQuantity::Rms;
}

pep::modules::psu_basic::TransformerSolveMode transformer_mode_for(const Block &active) {
  return active.transformer_solve_mode == TransformerSolveMode::RatioFromSecondary
             ? pep::modules::psu_basic::TransformerSolveMode::RatioFromSecondary
             : pep::modules::psu_basic::TransformerSolveMode::SecondaryFromRatio;
}

QString transformer_waveform_label(const Block &active) {
  switch (active.transformer_waveform) {
  case SignalWaveform::Square:
    return "prostokątny";
  case SignalWaveform::Triangle:
    return "trójkątny";
  default:
    return "sinusoidalny";
  }
}

QString transformer_rms_formula_label(const Block &active) {
  switch (active.transformer_waveform) {
  case SignalWaveform::Square:
    return "Vrms = Vpeak";
  case SignalWaveform::Triangle:
    return "Vrms = Vpeak / sqrt(3)";
  default:
    return "Vrms = Vpeak / sqrt(2)";
  }
}

CalculationDocument make_module_document(CalculationSection section) {
  CalculationDocument document;
  document.sections.push_back(std::move(section));
  return document;
}

CalculationDocument build_power_document(const Block &active) {
  const auto transformer = pep::modules::psu_basic::compute_transformer(
      pep::modules::psu_basic::TransformerInput{active.transformer_primary_v,
                                                active.transformer_secondary_v,
                                                active.transformer_turns_ratio,
                                                transformer_waveform_for(active),
                                                transformer_quantity_for(active),
                                                transformer_mode_for(active)});
  pep::modules::psu_basic::Input input;
  input.vin_ac_rms = transformer.secondary_rms;
  input.mains_hz = active.mains_hz;
  input.load_current = active.load_current;
  input.capacitor_uF = active.capacitor_uF;
  input.diode_drop = 0.9;
  input.rectifier = pep::modules::psu_basic::RectifierType::FullWaveBridge;

  const auto output = pep::modules::psu_basic::compute(input);
  CalculationDocument document;
  document.title = "Kalkulator zasilacza";
  document.intro = "Wyniki i wzory są grupowane sekcjami, aby dało się łatwo dodać kolejne elementy.";

  CalculationSection transformer_section = make_section(
      "Transformator",
      "Wyniki transformatora są liczone dla wybranego przebiegu i sposobu podania napięcia.");
  transformer_section.entries.push_back(make_entry(
      "Napięcie wtórne skuteczne",
      "V_{s,rms}",
      format_fixed(transformer.secondary_rms, 3),
      "V",
      "V_{s,rms} = \\frac{V_{p,rms}}{n}",
      QString("%1 / %2 = %3")
          .arg(active.transformer_primary_v, 0, 'f', 2)
          .arg(transformer.turns_ratio, 0, 'f', 3)
          .arg(transformer.secondary_rms, 0, 'f', 3)
          .toStdString(),
      "Przekładnia n oznacza stosunek Np:Ns."));
  transformer_section.entries.push_back(make_entry(
      "Napięcie wtórne szczytowe",
      "V_{s,peak}",
      format_fixed(transformer.secondary_peak, 3),
      "V",
      "V_{s,rms} = f(V_{s,peak})",
      transformer_rms_formula_label(active).toStdString(),
      QString("Przebieg %1 wymaga własnej konwersji RMS/peak.")
          .arg(transformer_waveform_label(active))
          .toStdString()));
  transformer_section.steps.push_back(make_step(
      "Ustal dane wejściowe",
      "Wybierz, czy podajesz napiecie skuteczne czy szczytowe oraz jaki przebieg analizujesz."));
  transformer_section.steps.push_back(make_step(
      "Sprawdz oznaczenie transformatora",
      "Zapis 2x12 V oznacza dwa osobne uzwojenia po 12 V. Do obliczen wpisuj 12 V, a nie 24 V."));
  document.sections.push_back(transformer_section);

  CalculationSection rectifier_section = make_section(
      "Prostownik i filtracja",
      "Każdy wynik ma przypisany wzór i podstawienie, więc nie ma osobnej sekcji wzorów.");
  rectifier_section.entries.push_back(make_entry(
      "Napięcie szczytowe po transformatorze",
      "V_{peak}",
      format_fixed(output.vpeak, 2),
      "V",
      "V_{peak} = V_{rms} \\cdot \\sqrt{2}",
      QString("%1 \\cdot \\sqrt{2} = %2")
          .arg(transformer.secondary_rms, 0, 'f', 3)
          .arg(output.vpeak, 0, 'f', 2)
          .toStdString(),
      "Dla prostownika pełnookresowego dalej odejmujemy spadki na dwóch diodach."));
  rectifier_section.entries.push_back(make_entry(
      "Napięcie po prostowaniu",
      "V_{rect}",
      format_fixed(output.vrect, 2),
      "V",
      "V_{rect} = V_{peak} - 2 \\cdot V_d",
      QString("%1 - 2 \\cdot 0.9 = %2").arg(output.vpeak, 0, 'f', 2).arg(output.vrect, 0, 'f', 2).toStdString(),
      "Przyjęto mostek Graetza z dwoma spadkami na diodach w torze przewodzenia."));
  rectifier_section.entries.push_back(make_entry(
      "Tętnienia szczyt-szczyt",
      "V_{ripple}",
      format_fixed(output.ripple_vpp, 3),
      "Vpp",
      "V_{ripple} \\approx \\frac{I_{load}}{f \\cdot C}",
      QString("%1 / (%2 \\cdot %3) = %4")
          .arg(active.load_current, 0, 'f', 3)
          .arg(active.mains_hz * 2.0, 0, 'f', 1)
          .arg(active.capacitor_uF * 1e-6, 0, 'g', 6)
          .arg(output.ripple_vpp, 0, 'f', 3)
          .toStdString(),
      "To uproszczony model edukacyjny dla kondensatora filtrującego."));
  rectifier_section.entries.push_back(make_entry(
      "Średnie napięcie pod obciążeniem",
      "V_{dc}",
      format_fixed(output.vdc_loaded, 2),
      "V",
      "V_{dc} = V_{rect} - \\frac{V_{ripple}}{2}",
      QString("%1 - %2 / 2 = %3")
          .arg(output.vrect, 0, 'f', 2)
          .arg(output.ripple_vpp, 0, 'f', 3)
          .arg(output.vdc_loaded, 0, 'f', 2)
          .toStdString(),
      "To orientacyjne napięcie średnie po filtracji kondensatorem."));
  rectifier_section.notes.push_back(make_warning_note(
      "Model uproszczony",
      "Wyniki sa edukacyjnym przyblizeniem. Realny uklad zalezy od spadkow napiecia, obciazenia i ESR kondensatora."));
  rectifier_section.checklist_items.push_back(
      make_checklist_item("Czy napiecie po prostowaniu jest bezpieczne dla dalszych blokow?", output.vrect > 0.0));
  rectifier_section.checklist_items.push_back(make_checklist_item(
      "Czy tetnienia sa akceptowalne dla zalozonego obciazenia?", output.ripple_vpp < 1.0));
  document.sections.push_back(rectifier_section);
  return document;
}

void update_power_view(const Block &active, const ActiveBlockWidgets &widgets) {
  if (!widgets.compute_view) {
    return;
  }
  if (active.variant == BlockVariant::PsuSwitching) {
    CalculationDocument document;
    document.title = "Kalkulator zasilacza impulsowego";
    document.intro =
        "Ten wariant dostanie osobne moduły obliczeń. Nie pokazujemy tutaj pól zasilacza liniowego.";
    CalculationSection section = make_section(
        "Moduły obliczeń w przygotowaniu",
        "Dla zasilacza impulsowego pojawią się osobne kalkulatory etapowe, np. dla doboru topologii, transformatora impulsowego lub elementów filtra.");
    section.notes.push_back(make_info_note(
        "Osobny wariant",
        "To celowe rozdzielenie. Zasilacz impulsowy nie dzieli tych samych kroków obliczeniowych co zasilacz liniowy."));
    document.sections.push_back(section);
    widgets.compute_view->set_document(document);
    if (widgets.transformer_compute_view) {
      widgets.transformer_compute_view->set_document(document);
    }
    if (widgets.rectifier_compute_view) {
      widgets.rectifier_compute_view->set_document(document);
    }
  } else {
    const auto document = build_power_document(active);
    widgets.compute_view->set_document(document);

    if (widgets.transformer_compute_view && !document.sections.empty()) {
      widgets.transformer_compute_view->set_document(
          make_module_document(document.sections.front()));
    }
    if (widgets.rectifier_compute_view && document.sections.size() > 1) {
      widgets.rectifier_compute_view->set_document(
          make_module_document(document.sections[1]));
    }
  }
  pep::modules::psu_basic::WaveformParams input_waveform;
  const auto transformer = pep::modules::psu_basic::compute_transformer(
      pep::modules::psu_basic::TransformerInput{active.transformer_primary_v,
                                                active.transformer_secondary_v,
                                                active.transformer_turns_ratio,
                                                transformer_waveform_for(active),
                                                transformer_quantity_for(active),
                                                transformer_mode_for(active)});
  pep::modules::psu_basic::Input input;
  input.vin_ac_rms = transformer.secondary_rms;
  input.mains_hz = active.mains_hz;
  input.load_current = active.load_current;
  input.capacitor_uF = active.capacitor_uF;
  input.diode_drop = 0.9;
  input.rectifier = pep::modules::psu_basic::RectifierType::FullWaveBridge;
  const auto output = pep::modules::psu_basic::compute(input);
  input_waveform.vpeak = output.vpeak;
  input_waveform.vrect = output.vrect;
  input_waveform.ripple_vpp = output.ripple_vpp;
  input_waveform.mains_hz = input.mains_hz;
  input_waveform.type = pep::modules::psu_basic::WaveformType::Sinus;
  widgets.waveform_in->set_params(input_waveform);

  pep::modules::psu_basic::WaveformParams output_waveform = input_waveform;
  output_waveform.type = pep::modules::psu_basic::WaveformType::Filtered;
  widgets.waveform_out->set_params(output_waveform);
}

pep::modules::psu_basic::WaveformType waveform_type_for(const Block &active) {
  if (active.signal_waveform == SignalWaveform::Square) {
    return pep::modules::psu_basic::WaveformType::Square;
  }
  if (active.signal_waveform == SignalWaveform::Triangle) {
    return pep::modules::psu_basic::WaveformType::Triangle;
  }
  return pep::modules::psu_basic::WaveformType::Sinus;
}

void update_amplifier_view(const Block &active, const ActiveBlockWidgets &widgets) {
  const double amp = (active.signal_amp_v > 0.0 ? active.signal_amp_v : 1.0);
  const double hz = (active.signal_hz > 0.0 ? active.signal_hz : 1000.0);
  const double gain = (active.gain > 0.0 ? active.gain : 1.0);

  if (widgets.compute_view) {
    CalculationDocument document;
    document.title = "Kalkulator wzmacniacza";
    document.intro = "Ten sam renderer i format dokumentu będzie używany dla kolejnych elementów.";
    CalculationSection section = make_section("Parametry sygnału");
    section.entries.push_back(make_entry(
        "Amplituda wyjściowa",
        "V_{out}",
        format_fixed(amp * gain, 3),
        "V",
        "V_{out} = V_{in} \\cdot A_v",
        QString("%1 \\cdot %2 = %3").arg(amp, 0, 'f', 3).arg(gain, 0, 'f', 2).arg(amp * gain, 0, 'f', 3).toStdString(),
        "Prosty model edukacyjny wzmacniacza napięciowego."));
    section.entries.push_back(make_entry(
        "Częstotliwość sygnału",
        "f",
        format_fixed(hz, 1),
        "Hz",
        "",
        "",
        "Częstotliwość wejściowa jest przenoszona do prezentacji przebiegu."));
    document.sections.push_back(section);
    widgets.compute_view->set_document(document);
  }

  pep::modules::psu_basic::WaveformParams input_waveform;
  input_waveform.vpeak = amp;
  input_waveform.vrect = amp;
  input_waveform.ripple_vpp = 0.0;
  input_waveform.mains_hz = hz;
  input_waveform.type = waveform_type_for(active);
  widgets.waveform_in->set_params(input_waveform);

  pep::modules::psu_basic::WaveformParams output_waveform = input_waveform;
  output_waveform.vpeak = amp * gain;
  output_waveform.vrect = amp * gain;
  widgets.waveform_out->set_params(output_waveform);
}

void update_validation(const std::vector<Block> &blocks, const std::vector<Connection> &connections,
                       const std::vector<CalculationValidationIssue> &calculation_issues,
                       QLabel *validation) {
  std::vector<QString> warnings;
  for (const auto &connection : connections) {
    const Block *from = find_block(blocks, connection.a.block_id);
    const Block *to = find_block(blocks, connection.b.block_id);
    if (!from || !to) {
      continue;
    }

    const auto from_port = find_port(*from, connection.a.port_id);
    const auto to_port = find_port(*to, connection.b.port_id);
    if (!from_port.has_value() || !to_port.has_value()) {
      continue;
    }

    if (!ports_compatible(from_port->type, to_port->type)) {
      warnings.push_back("Niekompatybilne typy portów: " + endpoint_label(*from, *from_port) +
                         " ↔ " + endpoint_label(*to, *to_port));
    }
  }

  if (!calculation_issues.empty()) {
    const auto summary =
        pep::ui::calculation::summarize_validation_issues(calculation_issues);
    warnings.push_back("Dokument obliczeń: "
                       + QString::fromStdString(format_validation_compact_summary(summary)));
  }

  for (const auto &issue : calculation_issues) {
    QString line = "Dokument obliczeń: ";
    if (issue.severity == CalculationValidationSeverity::Error) {
      line += "[błąd] ";
    }
    line += QString::fromStdString(issue.message);
    warnings.push_back(line);
  }

  if (warnings.empty()) {
    validation->setText("Walidacja: OK (miękka walidacja – nie blokuje eksportu).");
    return;
  }

  QString text = "Walidacja (ostrzeżenia):\n";
  for (const auto &warning : warnings) {
    text += "- " + warning + "\n";
  }
  validation->setText(text.trimmed());
}

} // namespace

void recompute_and_validate(const Block *active, const std::vector<Block> &blocks,
                            const std::vector<Connection> &connections,
                            const ActiveBlockWidgets &widgets) {
  if (!widgets.active_title || !widgets.compute_view || !widgets.validation ||
      !widgets.ports_label || !widgets.waveform_in || !widgets.waveform_out) {
    return;
  }

  if (!active) {
    widgets.active_title->setText("Aktywny element: —");
    CalculationDocument document;
    document.title = "Kalkulator";
    document.intro = "Wyniki pojawią się tutaj.";
    widgets.compute_view->set_document(document);
    widgets.validation->setText("Walidacja: —");
    widgets.ports_label->setText("");
    return;
  }

  widgets.active_title->setText(QString("Aktywny element: #%1 %2")
                                    .arg(active->id)
                                    .arg(QString::fromStdString(active->title)));
  widgets.ports_label->setText(ports_text_for(*active));

  if (active->kind == BlockKind::Power) {
    update_power_view(*active, widgets);
  } else {
    update_amplifier_view(*active, widgets);
  }

  std::vector<CalculationValidationIssue> calculation_issues =
      widgets.compute_view->validation_issues();
  if (active->kind == BlockKind::Power) {
    calculation_issues.clear();
    if (widgets.transformer_compute_view) {
      const auto &issues = widgets.transformer_compute_view->validation_issues();
      calculation_issues.insert(calculation_issues.end(), issues.begin(), issues.end());
    }
    if (widgets.rectifier_compute_view) {
      const auto &issues = widgets.rectifier_compute_view->validation_issues();
      calculation_issues.insert(calculation_issues.end(), issues.begin(), issues.end());
    }
    if (calculation_issues.empty()) {
      calculation_issues = widgets.compute_view->validation_issues();
    }
  }

  update_validation(blocks, connections, calculation_issues, widgets.validation);

  if (widgets.bottom_tabs && widgets.waveform_tab_index >= 0) {
    widgets.bottom_tabs->setCurrentIndex(widgets.waveform_tab_index);
  }
}

} // namespace pep::modules::project_design
