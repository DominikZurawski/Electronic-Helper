#include "active_block_view.hpp"

#include "../model/connection_ops.hpp"
#include "../model/model.hpp"
#include "enum_ui.hpp"

#include "../../psu_basic/model/model.hpp"
#include "../../../ui/calculation/calculation_builders.hpp"
#include "../../../ui/calculation/calculation_view.hpp"
#include "../../../ui/calculation/calculation_document.hpp"
#include "../../../ui/calculation/calculation_validation.hpp"

#include <QLabel>
#include <QTabWidget>

#include <algorithm>
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

pep::modules::psu_basic::WaveformType transformer_waveform_type_for(const Block &active) {
  if (active.transformer_waveform == SignalWaveform::Square) {
    return pep::modules::psu_basic::WaveformType::Square;
  }
  if (active.transformer_waveform == SignalWaveform::Triangle) {
    return pep::modules::psu_basic::WaveformType::Triangle;
  }
  return pep::modules::psu_basic::WaveformType::Sinus;
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

struct PowerStageViewData {
  QString title;
  CalculationSection section;
  pep::modules::psu_basic::WaveformParams input_waveform;
  pep::modules::psu_basic::WaveformParams output_waveform;
};

std::vector<PowerStageViewData> build_power_stages(const Block &active) {
  const bool symmetric = active.variant == BlockVariant::PsuSymmetric;
  const auto transformer = pep::modules::psu_basic::compute_transformer(
      pep::modules::psu_basic::TransformerInput{active.transformer_primary_v,
                                                active.transformer_secondary_v,
                                                active.transformer_turns_ratio,
                                                transformer_waveform_for(active),
                                                transformer_quantity_for(active),
                                                transformer_mode_for(active)});
  const double tol = active.transformer_primary_tol_pct;
  std::string secondary_range;
  std::string secondary_peak_range;
  double vin_min = 0.0;
  double vin_max = 0.0;
  if (tol > 0.0) {
    const double factor_min = 1.0 - tol / 100.0;
    const double factor_max = 1.0 + tol / 100.0;
    vin_min = transformer.secondary_rms * factor_min;
    vin_max = transformer.secondary_rms * factor_max;
    const double sec_min = vin_min;
    const double sec_max = vin_max;
    const double sec_peak_min = transformer.secondary_peak * factor_min;
    const double sec_peak_max = transformer.secondary_peak * factor_max;
    secondary_range =
        "[" + format_fixed(sec_min, 3) + " – " + format_fixed(sec_max, 3) + "]";
    secondary_peak_range =
        "[" + format_fixed(sec_peak_min, 3) + " – " + format_fixed(sec_peak_max, 3) + "]";
  }
  pep::modules::psu_basic::Input input;
  input.vin_ac_rms = transformer.secondary_rms;
  if (tol > 0.0) {
    input.vin_min = vin_min;
    input.vin_max = vin_max;
  }
  input.mains_hz = active.mains_hz;
  input.load_current = active.load_current;
  input.capacitor_uF = active.capacitor_uF;
  input.cap_tol_pct = active.capacitor_tol_pct;
  input.diode_drop = active.diode_drop;
  input.rectifier = pep::modules::psu_basic::RectifierType::FullWaveBridge;

  const auto output = pep::modules::psu_basic::compute(input);
  const double required_cap_uF = pep::modules::psu_basic::required_capacitance_uF(
      active.load_current, active.max_ripple_vpp, output.ripple_hz);

  std::vector<PowerStageViewData> stages;

  PowerStageViewData transformer_stage;
  transformer_stage.title = "Transformator";
  transformer_stage.section = make_section(
      "Transformator",
      "Wyniki transformatora są liczone dla wybranego przebiegu i sposobu podania napięcia.");
  transformer_stage.section.entries.push_back(make_entry(
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
      "Przekładnia n oznacza stosunek Np:Ns.",
      secondary_range));
  transformer_stage.section.entries.push_back(make_entry(
      "Napięcie wtórne szczytowe",
      "V_{s,peak}",
      format_fixed(transformer.secondary_peak, 3),
      "V",
      "V_{s,rms} = f(V_{s,peak})",
      transformer_rms_formula_label(active).toStdString(),
      QString("Przebieg %1 wymaga własnej konwersji RMS/peak.")
          .arg(transformer_waveform_label(active))
          .toStdString(),
      secondary_peak_range));
  transformer_stage.section.steps.push_back(make_step(
      "Ustal dane wejściowe",
      "Wybierz, czy podajesz napiecie skuteczne czy szczytowe oraz jaki przebieg analizujesz."));
  transformer_stage.section.steps.push_back(make_step(
      "Sprawdz oznaczenie transformatora",
      symmetric
          ? "Zapis 2x12 V oznacza dwa osobne uzwojenia po 12 V. Do obliczen wpisuj 12 V, a nie 24 V."
          : "Dla zasilacza niesymetrycznego wpisuj napiecie pojedynczego uzwojenia wtórnego, np. 12 V."));
  transformer_stage.section.steps.push_back(make_step(
      "Dodaj zabezpieczenia przed transformatorem",
      "Przed transformatorem warto dodac bezpiecznik oraz warystor, aby zabezpieczyc uklad przed "
      "zbyt wysokim pradem i przepieciami."));
  transformer_stage.input_waveform.vpeak = transformer.primary_peak;
  transformer_stage.input_waveform.vrect = transformer.primary_peak;
  transformer_stage.input_waveform.ripple_vpp = 0.0;
  transformer_stage.input_waveform.mains_hz = active.mains_hz;
  transformer_stage.input_waveform.type = transformer_waveform_type_for(active);
  transformer_stage.output_waveform = transformer_stage.input_waveform;
  transformer_stage.output_waveform.vpeak = transformer.secondary_peak;
  transformer_stage.output_waveform.vrect = transformer.secondary_peak;
  stages.push_back(std::move(transformer_stage));

  PowerStageViewData rectifier_stage;
  rectifier_stage.title = "Prostownik";
  rectifier_stage.section = make_section(
      "Prostownik",
      "Ten etap pokazuje, jak sygnał z transformatora zamienia się w przebieg jednopolowy po mostku.");
  rectifier_stage.section.entries.push_back(make_entry(
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
  if (tol > 0.0) {
    const double vpeak_min = vin_min * std::sqrt(2.0);
    const double vpeak_max = vin_max * std::sqrt(2.0);
    rectifier_stage.section.entries.back().value_secondary =
        "[" + format_fixed(vpeak_min, 2) + " – " + format_fixed(vpeak_max, 2) + "]";
  }
  rectifier_stage.section.entries.push_back(make_entry(
      "Napięcie po prostowaniu",
      "V_{rect}",
      format_fixed(output.vrect, 2),
      "V",
      "V_{rect} = V_{peak} - 2 \\cdot V_d",
      QString("%1 - 2 \\cdot %2 = %3")
          .arg(output.vpeak, 0, 'f', 2)
          .arg(active.diode_drop, 0, 'f', 2)
          .arg(output.vrect, 0, 'f', 2)
          .toStdString(),
      "Przyjęto mostek Graetza z dwoma spadkami na diodach w torze przewodzenia."));
  if (tol > 0.0) {
    const double vpeak_min = vin_min * std::sqrt(2.0);
    const double vpeak_max = vin_max * std::sqrt(2.0);
    const double vrect_min = vpeak_min - 2.0 * active.diode_drop;
    const double vrect_max = vpeak_max - 2.0 * active.diode_drop;
    rectifier_stage.section.entries.back().value_secondary =
        "[" + format_fixed(vrect_min, 2) + " – " + format_fixed(vrect_max, 2) + "]";
  }
  rectifier_stage.section.entries.push_back(make_entry(
      "Częstotliwość tętnień po prostowaniu",
      "f_{ripple}",
      format_fixed(output.ripple_hz, 1),
      "Hz",
      "f_{ripple} = 2 \\cdot f_{mains}",
      QString("2 \\cdot %1 = %2")
          .arg(active.mains_hz, 0, 'f', 1)
          .arg(output.ripple_hz, 0, 'f', 1)
          .toStdString(),
      "Dla prostownika pełnookresowego tętnienia mają dwukrotnie większą częstotliwość niż sieć."));
  rectifier_stage.input_waveform.vpeak = transformer.secondary_peak;
  rectifier_stage.input_waveform.vrect = transformer.secondary_peak;
  rectifier_stage.input_waveform.ripple_vpp = 0.0;
  rectifier_stage.input_waveform.mains_hz = active.mains_hz;
  rectifier_stage.input_waveform.type = transformer_waveform_type_for(active);
  rectifier_stage.output_waveform.vpeak = output.vrect;
  rectifier_stage.output_waveform.vrect = output.vrect;
  rectifier_stage.output_waveform.ripple_vpp = 0.0;
  rectifier_stage.output_waveform.mains_hz = active.mains_hz;
  rectifier_stage.output_waveform.type = pep::modules::psu_basic::WaveformType::FullWave;
  stages.push_back(std::move(rectifier_stage));

  PowerStageViewData filtration_stage;
  filtration_stage.title = "Filtracja";
  filtration_stage.section = make_section(
      "Filtracja",
      "Na tym etapie sygnał po prostowniku jest wygładzany kondensatorem, więc wejściem jest już przebieg pełnookresowy.");
  filtration_stage.section.entries.push_back(make_entry(
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
  filtration_stage.section.entries.push_back(make_entry(
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
  filtration_stage.section.entries.push_back(make_entry(
      "Minimalne napięcie w dolinie tętnień",
      "V_{min}",
      format_fixed(output.vmin, 2),
      "V",
      "V_{min} = V_{rect} - V_{ripple}",
      QString("%1 - %2 = %3")
          .arg(output.vrect, 0, 'f', 2)
          .arg(output.ripple_vpp, 0, 'f', 3)
          .arg(output.vmin, 0, 'f', 2)
          .toStdString(),
      "To najniższa wartość napięcia między kolejnymi doładowaniami kondensatora."));
  if (tol > 0.0) {
    filtration_stage.section.entries[0].value_secondary =
        "[" + format_fixed(output.ripple_vpp_min, 3) + " – " +
        format_fixed(output.ripple_vpp_max, 3) + "]";
    filtration_stage.section.entries[1].value_secondary =
        "[" + format_fixed(output.vdc_loaded_min, 2) + " – " +
        format_fixed(output.vdc_loaded_max, 2) + "]";
    filtration_stage.section.entries[2].value_secondary =
        "[" + format_fixed(output.vmin_min, 2) + " – " + format_fixed(output.vmin_max, 2) + "]";
  }
  if (required_cap_uF > 0.0) {
    filtration_stage.section.entries.push_back(make_entry(
        "Wymagana pojemność dla zadanych tętnień",
        "C_{min}",
        format_fixed(required_cap_uF, 1),
        "uF",
        "C = \\frac{I_{load}}{f \\cdot V_{ripple}}",
        QString("%1 / (%2 \\cdot %3) = %4 uF")
            .arg(active.load_current, 0, 'f', 3)
            .arg(output.ripple_hz, 0, 'f', 1)
            .arg(active.max_ripple_vpp, 0, 'f', 3)
            .arg(required_cap_uF, 0, 'f', 1)
            .toStdString(),
        "To pojemnosc potrzebna, aby nie przekroczyc zalozonych tetnien w uproszczonym modelu."));
  }
  filtration_stage.section.notes.push_back(make_warning_note(
      "Model uproszczony",
      "Wyniki sa edukacyjnym przyblizeniem. Realny uklad zalezy od spadkow napiecia, obciazenia i ESR kondensatora."));
  filtration_stage.section.notes.push_back(make_info_note(
      "Dobor pojemnosci",
      "Wieksza pojemnosc zwykle zmniejsza tetnienia, ale nie warto przesadzac. Zbyt duzy "
      "kondensator zwieksza koszt i rozmiar zasilacza, a przy wlaczeniu pobiera wiekszy prad "
      "ladowania. Bardziej obciaza tez transformator i mostek prostowniczy."));
  filtration_stage.section.checklist_items.push_back(
      make_checklist_item("Czy napiecie po prostowaniu jest bezpieczne dla dalszych blokow?", output.vrect > 0.0));
  filtration_stage.section.checklist_items.push_back(make_checklist_item(
      "Czy tetnienia sa akceptowalne dla zalozonego obciazenia?", output.ripple_vpp < 1.0));
  if (required_cap_uF > 0.0) {
    filtration_stage.section.checklist_items.push_back(make_checklist_item(
        "Czy dobrana pojemnosc spelnia zalozony limit tetnien?", active.capacitor_uF >= required_cap_uF));
  }
  filtration_stage.input_waveform = stages.back().output_waveform;
  filtration_stage.output_waveform.vpeak = output.vpeak;
  filtration_stage.output_waveform.vrect = output.vrect;
  filtration_stage.output_waveform.ripple_vpp = output.ripple_vpp;
  filtration_stage.output_waveform.mains_hz = input.mains_hz;
  filtration_stage.output_waveform.type = pep::modules::psu_basic::WaveformType::Filtered;
  stages.push_back(std::move(filtration_stage));

  PowerStageViewData load_stage;
  load_stage.title = "Obciążenie";
  load_stage.section = make_section(
      "Obciążenie i moc",
      "Szybkie podsumowanie obciążenia zasilacza po filtracji.");
  load_stage.section.entries.push_back(make_entry(
      "Prąd skuteczny obciążenia",
      "I_{rms}",
      format_fixed(active.load_current, 3),
      "A",
      "I_{rms} = I_{load}",
      QString("%1").arg(active.load_current, 0, 'f', 3).toStdString(),
      "Po filtracji prąd obciążenia traktujemy jako stały."));
  const double p_load = output.vdc_loaded * active.load_current;
  load_stage.section.entries.push_back(make_entry(
      "Moc na obciążeniu",
      "P_{load}",
      format_fixed(p_load, 2),
      "W",
      "P = V_{dc} \\cdot I_{load}",
      QString("%1 \\cdot %2 = %3")
          .arg(output.vdc_loaded, 0, 'f', 2)
          .arg(active.load_current, 0, 'f', 3)
          .arg(p_load, 0, 'f', 2)
          .toStdString(),
      "To uproszczona moc wydzielana na obciążeniu rezystancyjnym."));
  if (tol > 0.0) {
    const double p_min = output.vdc_loaded_min * active.load_current;
    const double p_max = output.vdc_loaded_max * active.load_current;
    load_stage.section.entries.back().value_secondary =
        "[" + format_fixed(p_min, 2) + " – " + format_fixed(p_max, 2) + "]";
  }
  load_stage.input_waveform = stages.back().output_waveform;
  load_stage.output_waveform = load_stage.input_waveform;
  stages.push_back(std::move(load_stage));

  return stages;
}

CalculationDocument build_power_document(const std::vector<PowerStageViewData> &stages) {
  CalculationDocument document;
  document.title = "Kalkulator zasilacza";
  document.intro = "Wyniki i wzory są grupowane sekcjami, aby dało się łatwo dodać kolejne elementy.";
  for (const auto &stage : stages) {
    document.sections.push_back(stage.section);
  }
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
    for (auto *view : widgets.power_compute_views) {
      if (view) {
        view->set_document(document);
      }
    }
  } else {
    const auto stages = build_power_stages(active);
    const auto document = build_power_document(stages);
    widgets.compute_view->set_document(document);

    for (std::size_t i = 0; i < widgets.power_compute_views.size() && i < stages.size(); ++i) {
      if (widgets.power_compute_views[i]) {
        widgets.power_compute_views[i]->set_document(make_module_document(stages[i].section));
      }
    }
    if (!stages.empty()) {
      const std::size_t active_power_tab =
          std::clamp(widgets.active_power_calculator_tab_index, 0, static_cast<int>(stages.size() - 1));
      widgets.waveform_in->set_params(stages[active_power_tab].input_waveform);
      widgets.waveform_out->set_params(stages[active_power_tab].output_waveform);
    }
    return;
  }

  pep::modules::psu_basic::WaveformParams empty_waveform;
  widgets.waveform_in->set_params(empty_waveform);
  widgets.waveform_out->set_params(empty_waveform);
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

pep::modules::psu_basic::WaveformShape waveform_shape_for(const Block &active) {
  if (active.signal_waveform == SignalWaveform::Square) {
    return pep::modules::psu_basic::WaveformShape::Square;
  }
  if (active.signal_waveform == SignalWaveform::Triangle) {
    return pep::modules::psu_basic::WaveformShape::Triangle;
  }
  return pep::modules::psu_basic::WaveformShape::Sine;
}

void update_amplifier_view(const Block &active, const ActiveBlockWidgets &widgets) {
  const double amp = (active.signal_amp_v > 0.0 ? active.signal_amp_v : 1.0);
  const double hz = (active.signal_hz > 0.0 ? active.signal_hz : 1000.0);
  const double gain = (active.gain > 0.0 ? active.gain : 1.0);
  const double load_r = active.load_resistance_ohm;
  const double target_power = active.target_power_w;
  const double headroom = active.supply_headroom_v;
  const auto shape = waveform_shape_for(active);
  const double vout_peak = amp * gain;
  const double vout_rms = pep::modules::psu_basic::peak_to_rms(vout_peak, shape);
  const double vout_avg_abs = pep::modules::psu_basic::avg_abs_from_peak(vout_peak, shape);
  const double i_rms = (load_r > 0.0) ? vout_rms / load_r : 0.0;
  const double i_avg_abs = (load_r > 0.0) ? vout_avg_abs / load_r : 0.0;
  const double p_load = (load_r > 0.0) ? (vout_rms * vout_rms / load_r) : 0.0;

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
        "f = f_{in}",
        QString("%1").arg(hz, 0, 'f', 1).toStdString(),
        "Częstotliwość wejściowa jest przenoszona do prezentacji przebiegu."));
    document.sections.push_back(section);

    CalculationSection power_section = make_section("Moc i nagrzewanie");
    if (load_r > 0.0) {
      power_section.entries.push_back(make_entry(
          "Napiecie skuteczne na obciazeniu",
          "V_{rms}",
          format_fixed(vout_rms, 3),
          "V",
          "V_{rms} = f(V_{peak})",
          "",
          "Zalezy od ksztaltu przebiegu."));
      power_section.entries.push_back(make_entry(
          "Prad skuteczny obciazenia",
          "I_{rms}",
          format_fixed(i_rms, 3),
          "A",
          "I_{rms} = V_{rms} / R",
          QString("%1 / %2 = %3")
              .arg(vout_rms, 0, 'f', 3)
              .arg(load_r, 0, 'f', 2)
              .arg(i_rms, 0, 'f', 3)
              .toStdString(),
          "Wartosc skuteczna odpowiada za nagrzewanie rezystora."));
      power_section.entries.push_back(make_entry(
          "Prad sredni |i(t)|",
          "I_{avg}",
          format_fixed(i_avg_abs, 3),
          "A",
          "I_{avg} = avg(|i(t)|)",
          "",
          "Wazne np. dla strat na diodach: P = U_F * I_{avg}."));
      power_section.entries.push_back(make_entry(
          "Moc na obciazeniu",
          "P_{load}",
          format_fixed(p_load, 2),
          "W",
          "P = I_{rms}^2 \\cdot R",
          QString("%1^2 \\cdot %2 = %3")
              .arg(i_rms, 0, 'f', 3)
              .arg(load_r, 0, 'f', 2)
              .arg(p_load, 0, 'f', 2)
              .toStdString(),
          "Uproszczony model mocy czynnej na rezystorze."));
    }

    if (load_r > 0.0 && target_power > 0.0) {
      const double v_rms_need = std::sqrt(target_power * load_r);
      const double v_peak_need = pep::modules::psu_basic::rms_to_peak(v_rms_need, shape);
      const double i_rms_need = v_rms_need / load_r;
      power_section.entries.push_back(make_entry(
          "Napiecie skuteczne dla zadanej mocy",
          "V_{rms,req}",
          format_fixed(v_rms_need, 2),
          "V",
          "V_{rms} = \\sqrt{P \\cdot R}",
          QString("sqrt(%1 \\cdot %2) = %3")
              .arg(target_power, 0, 'f', 2)
              .arg(load_r, 0, 'f', 2)
              .arg(v_rms_need, 0, 'f', 2)
              .toStdString(),
          "To wartosc skuteczna potrzebna na obciazeniu."));
      power_section.entries.push_back(make_entry(
          "Szczytowe napiecie na obciazeniu",
          "V_{peak,req}",
          format_fixed(v_peak_need, 2),
          "V",
          "V_{peak} = f(V_{rms})",
          "",
          "Zalezy od ksztaltu przebiegu."));
      power_section.entries.push_back(make_entry(
          "Prad skuteczny dla zadanej mocy",
          "I_{rms,req}",
          format_fixed(i_rms_need, 3),
          "A",
          "I_{rms} = V_{rms} / R",
          QString("%1 / %2 = %3")
              .arg(v_rms_need, 0, 'f', 2)
              .arg(load_r, 0, 'f', 2)
              .arg(i_rms_need, 0, 'f', 3)
              .toStdString(),
          "Taki prad powinien zapewnic zasilacz."));
      power_section.entries.push_back(make_entry(
          "Minimalne napiecie zasilania",
          "V_{dc,min}",
          format_fixed(v_peak_need + headroom, 2),
          "V",
          "V_{dc,min} = V_{peak} + zapas",
          QString("%1 + %2 = %3")
              .arg(v_peak_need, 0, 'f', 2)
              .arg(headroom, 0, 'f', 2)
              .arg(v_peak_need + headroom, 0, 'f', 2)
              .toStdString(),
          "Przyklad dla klasy B: zapas kompensuje spadki i tetnienia."));
    }

    document.sections.push_back(power_section);
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

void update_validation(const Block *active, const std::vector<Block> &blocks,
                       const std::vector<Connection> &connections,
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

  if (active && active->kind == BlockKind::Amplifier &&
      active->amp_design_mode == AmpDesignMode::SupplyForAmp) {
    int psu_id = active->amp_power_source_id;
    if (psu_id == 0) {
      psu_id = pep::modules::project_design::connected_power_block_id(blocks, connections,
                                                                      active->id);
    }
    if (psu_id == 0) {
      warnings.push_back("Projektowanie zasilacza: brak wybranego lub podłączonego zasilacza.");
    } else {
      const Block *psu = find_block(blocks, psu_id);
      if (!psu || psu->kind != BlockKind::Power) {
        warnings.push_back("Projektowanie zasilacza: wskazany blok nie jest zasilaczem.");
      } else {
        if (active->load_resistance_ohm <= 0.0) {
          warnings.push_back("Projektowanie zasilacza: brak obciążenia (R) we wzmacniaczu.");
        }
        const bool has_target = active->target_power_w > 0.0;
        const bool has_waveform = active->signal_amp_v > 0.0 && active->gain > 0.0;
        if (!has_target && !has_waveform) {
          warnings.push_back("Projektowanie zasilacza: podaj moc docelową lub amplitudę i wzmocnienie.");
        }
        if (psu->transformer_secondary_v <= 0.0 || psu->load_current <= 0.0) {
          warnings.push_back(
              "Projektowanie zasilacza: parametry PSU nie zostały przeliczone (sprawdź wywołanie "
              "automatycznego uzupełniania).");
        }
        if (active->max_ripple_vpp <= 0.0) {
          warnings.push_back("Projektowanie zasilacza: brak limitu tętnień (Vpp).");
        }
      }
    }
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
    for (auto *view : widgets.power_compute_views) {
      if (!view) {
        continue;
      }
      const auto &issues = view->validation_issues();
      calculation_issues.insert(calculation_issues.end(), issues.begin(), issues.end());
    }
    if (calculation_issues.empty()) {
      calculation_issues = widgets.compute_view->validation_issues();
    }
  }

  update_validation(active, blocks, connections, calculation_issues, widgets.validation);

  if (widgets.bottom_tabs && widgets.waveform_tab_index >= 0) {
    widgets.bottom_tabs->setCurrentIndex(widgets.waveform_tab_index);
  }
}

} // namespace pep::modules::project_design
