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
#include <cmath>
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

struct ScaledVoltageDisplay {
  std::string value;
  std::string unit;
};

struct DurationDisplay {
  std::string value;
  std::string unit;
};

ScaledVoltageDisplay format_scaled_voltage(double volts, int precision = 3) {
  if (std::abs(volts) < 1.0) {
    return {format_fixed(volts * 1000.0, precision), "mV"};
  }
  return {format_fixed(volts, precision), "V"};
}

DurationDisplay format_duration(double seconds, int precision = 3) {
  if (std::abs(seconds) < 1e-3) {
    return {format_fixed(seconds * 1e6, precision), "us"};
  }
  return {format_fixed(seconds * 1e3, precision), "ms"};
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

pep::modules::psu_basic::WaveformParams final_power_output_waveform(const Block &active) {
  const auto stages = build_power_stages(active);
  if (stages.empty()) {
    return {};
  }
  return stages.back().output_waveform;
}

pep::modules::psu_basic::WaveformParams final_power_output_waveform_for_rail(
    const Block &active, SupplyRail rail) {
  auto waveform = final_power_output_waveform(active);
  if (rail == SupplyRail::Vee) {
    if (active.variant != BlockVariant::PsuSymmetric) {
      return {};
    }
    waveform.vpeak = -std::abs(waveform.vpeak);
    waveform.vrect = -std::abs(waveform.vrect);
  }
  return waveform;
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

void update_regulator_view(const Block &active, const std::vector<Block> &blocks,
                          const std::vector<Connection> &connections,
                          const ActiveBlockWidgets &widgets) {
  if (!widgets.compute_view) {
    return;
  }

  CalculationDocument document;
  document.title = "Kalkulator stabilizatora";

  const double vout_abs = std::abs(active.regulator_output_v);
  const double rail_sign = active.regulator_supply_rail == SupplyRail::Vee ? -1.0 : 1.0;
  const double vout = rail_sign * vout_abs;
  const double vin_min = active.regulator_input_min_v;
  const double iout = active.regulator_output_current_a;
  const double iz = active.regulator_zener_current_a;
  const double total_current = iout + iz;
  const double series_res =
      (vin_min > vout_abs && total_current > 0.0) ? ((vin_min - vout_abs) / total_current) : 0.0;

  if (active.variant == BlockVariant::RegZener) {
    document.intro =
        "Wariant Zenera liczy minimalny rezystor szeregowy dla zadanego napięcia wejściowego i prądu obciążenia.";
    CalculationSection section =
        make_section("Stabilizator z diodą Zenera",
                     "Model zakłada dodatnią gałąź z rezystorem szeregowym i diodą Zenera do masy.");
    section.entries.push_back(make_entry(
        "Wymagane napięcie wejściowe regulatora", "V_{req,min}", format_fixed(vin_min, 3), "V",
        "V_{req,min} = V_{out} + zapas", format_fixed(vin_min, 3),
        "To minimalne napięcie potrzebne, aby regulator utrzymał zadane wyjście."));
    section.entries.push_back(make_entry(
        "Napięcie wyjściowe", "V_{out}", format_fixed(vout, 3), "V", "V_{out} \\approx V_Z",
        format_fixed(vout, 3),
        "W eksporcie całego projektu trzeba ręcznie dobrać model diody Zenera do docelowego napięcia."));
    section.entries.push_back(make_entry(
        "Suma prądów przez rezystor", "I_R", format_fixed(total_current * 1000.0, 3), "mA",
        "I_R = I_{out,max} + I_Z",
        QString("%1 + %2 = %3 mA")
            .arg(iout * 1000.0, 0, 'f', 3)
            .arg(iz * 1000.0, 0, 'f', 3)
            .arg(total_current * 1000.0, 0, 'f', 3)
            .toStdString(),
        "Przyjęto minimalny zapas prądu Zenera dla utrzymania stabilizacji."));
    section.entries.push_back(make_entry(
        "Rezystor szeregowy", "R", format_fixed(series_res, 3), "Ohm",
        "R = \\frac{V_{in,min} - V_{out}}{I_{out,max} + I_Z}",
        QString("(%1 - %2) / (%3 + %4) = %5")
            .arg(vin_min, 0, 'f', 3)
            .arg(vout_abs, 0, 'f', 3)
            .arg(iout, 0, 'f', 4)
            .arg(iz, 0, 'f', 4)
            .arg(series_res, 0, 'f', 3)
            .toStdString(),
        "To wartość graniczna dla najniższego napięcia wejściowego."));
    section.notes.push_back(make_warning_note(
        "Eksport całego układu",
        "Po eksporcie całego układu wybierz własny model diody Zenera w LTspice. Napięcie i prąd testowy zależą od tego, jaki stabilizator budujesz."));
    section.checklist_items.push_back(make_checklist_item(
        "Czy napięcie wejściowe jest większe od wyjściowego?", vin_min > vout_abs));
    section.checklist_items.push_back(make_checklist_item(
        "Czy suma prądów obciążenia i Zenera jest dodatnia?", total_current > 0.0));
    document.sections.push_back(section);
  } else if (active.variant == BlockVariant::RegZenerBjt) {
    document.intro =
        "Wariant z tranzystorem bipolarnym wykorzystuje Zenera jako referencję, a tranzystor przenosi większy prąd obciążenia.";
    CalculationSection section = make_section(
        "Stabilizator Zenera z tranzystorem",
        "Model zakłada wtórnik emiterowy sterowany przez diodę Zenera, więc tranzystor odciąża samą diodę przy większym prądzie wyjściowym.");
    section.entries.push_back(make_entry(
        "Minimalne wymagane wejście", "V_{req,min}", format_fixed(vin_min, 3), "V",
        "V_{req,min} = |V_{out}| + V_{drop}", format_fixed(vin_min, 3),
        "To przybliżony próg wejścia potrzebny do utrzymania regulacji."));
    section.entries.push_back(make_entry(
        "Napięcie wyjściowe", "V_{out}", format_fixed(vout, 3), "V", "V_{out}",
        format_fixed(vout, 3),
        "Dla rzeczywistego układu trzeba jeszcze sprawdzić spadek baza-emiter wybranego tranzystora."));
    section.entries.push_back(make_entry(
        "Maksymalny prąd wyjściowy", "I_{out,max}", format_fixed(iout * 1000.0, 3), "mA",
        "I_{out,max}", format_fixed(iout * 1000.0, 3),
        "Tranzystor przejmuje główną część prądu obciążenia."));
    section.notes.push_back(make_warning_note(
        "Eksport LTspice",
        "Po eksporcie wybierz własny model diody Zenera i tranzystora NPN. Upewnij się też, że tranzystor wytrzyma przewidywaną moc."));
    document.sections.push_back(section);
  } else {
    document.intro = "Wariant scalony opisuje wymagane napięcia i prądy, ale docelowy model LTspice trzeba dobrać ręcznie.";
    CalculationSection section = make_section(
        "Stabilizator scalony",
        "W tej wersji najważniejsze są napięcie wejściowe, napięcie wyjściowe, prąd i zapas dropout.");
    const double required_vin = vout_abs + std::max(0.0, active.regulator_dropout_v);
    section.entries.push_back(make_entry(
        "Minimalne wymagane wejście", "V_{in,min}", format_fixed(required_vin, 3), "V",
        "V_{in,min} = V_{out} + V_{drop}", QString("%1 + %2 = %3")
                                                  .arg(vout_abs, 0, 'f', 3)
                                                  .arg(active.regulator_dropout_v, 0, 'f', 3)
                                                  .arg(required_vin, 0, 'f', 3)
                                                  .toStdString(),
        "To punkt startowy do doboru konkretnego stabilizatora z katalogu."));
    section.notes.push_back(make_warning_note(
        "Eksport LTspice",
        "Po eksporcie całego układu podstaw własny model stabilizatora scalonego i sprawdź dropout oraz zakres prądu."));
    document.sections.push_back(section);
  }

  pep::modules::psu_basic::WaveformParams input_waveform;
  int power_id = active.regulator_input_source_id;
  if (power_id == 0) {
    power_id = connected_direct_supply_block_id(blocks, connections, active.id);
  }
  power_id = resolve_power_block_id(blocks, connections, power_id);
  if (const Block *power = find_block(blocks, power_id);
      power && power->kind == BlockKind::Power) {
    input_waveform = final_power_output_waveform_for_rail(*power, active.regulator_supply_rail);
  }

  const double input_rect_abs = std::abs(input_waveform.vrect);
  const double input_peak_abs = std::abs(input_waveform.vpeak);
  const double available_source_min =
      std::max(0.0, input_rect_abs - std::max(0.0, input_waveform.ripple_vpp));
  const double required_vin = vout_abs + std::max(0.0, active.regulator_dropout_v);
  const double headroom_margin = available_source_min - required_vin;
  const bool can_regulate = available_source_min >= required_vin && input_rect_abs > 0.0;

  auto output_waveform = input_waveform;
  const double dropout = std::max(0.0, active.regulator_dropout_v);
  const double available_peak = std::max(0.0, input_peak_abs - dropout);
  const double available_rect = std::max(0.0, input_rect_abs - dropout);
  output_waveform.vpeak = can_regulate ? vout : (rail_sign * available_peak);
  output_waveform.vrect = can_regulate ? vout : (rail_sign * available_rect);
  output_waveform.ripple_vpp = can_regulate ? 0.0 : input_waveform.ripple_vpp;
  output_waveform.type = pep::modules::psu_basic::WaveformType::Filtered;
  const double input_avg_abs =
      std::max(0.0, input_rect_abs - std::max(0.0, input_waveform.ripple_vpp) / 2.0);
  const double regulator_loss_w = std::max(0.0, input_avg_abs - vout_abs) * std::max(0.0, iout);
  const double regulator_loss_peak_w =
      std::max(0.0, input_peak_abs - vout_abs) * std::max(0.0, iout);
  const bool thermal_warning = regulator_loss_w >= 1.0 || regulator_loss_peak_w >= 2.0;
  const double zener_resistor_loss_w =
      (active.variant == BlockVariant::RegZener)
          ? std::max(0.0, available_source_min - vout_abs) * std::max(0.0, total_current)
          : 0.0;
  const double zener_diode_loss_w =
      (active.variant == BlockVariant::RegZener) ? vout_abs * std::max(0.0, iz) : 0.0;

  if (active.variant == BlockVariant::RegZener) {
    auto &section = document.sections.front();
    section.entries.insert(section.entries.begin(), make_entry(
        "Dostępne minimum z zasilacza", "V_{src,min}", format_fixed(available_source_min, 3), "V",
        "V_{src,min} = V_{dc} - V_{ripple}", "",
        "To najniższe napięcie, jakie regulator dostaje z wybranego bloku zasilania."));
    section.entries.insert(section.entries.begin() + 1, make_entry(
        "Wymagane minimum regulatora", "V_{req,min}", format_fixed(required_vin, 3), "V",
        "V_{req,min} = V_{out} + V_{drop}", QString("%1 + %2 = %3")
                                                .arg(vout_abs, 0, 'f', 3)
                                                .arg(active.regulator_dropout_v, 0, 'f', 3)
                                                .arg(required_vin, 0, 'f', 3)
                                                .toStdString(),
        "Jeśli źródło spadnie poniżej tej wartości, regulator przestaje utrzymywać stałe wyjście."));
    section.entries.insert(section.entries.begin() + 2, make_entry(
        "Margines regulacji", "\\Delta V", format_fixed(headroom_margin, 3), "V",
        "\\Delta V = V_{src,min} - V_{req,min}",
        QString("%1 - %2 = %3")
            .arg(available_source_min, 0, 'f', 3)
            .arg(required_vin, 0, 'f', 3)
            .arg(headroom_margin, 0, 'f', 3)
            .toStdString(),
        "To nie jest sztucznie dodany zapas. To tylko różnica między tym, co daje zasilacz, a minimum potrzebnym regulatorowi."));
    section.entries.push_back(make_entry(
        "Strata mocy regulatora", "P_{reg}", format_fixed(regulator_loss_w, 3), "W",
        "P_{reg} \\approx (V_{in,avg} - V_{out}) \\cdot I_{out}",
        QString("(%1 - %2) \\cdot %3 = %4")
            .arg(input_avg_abs, 0, 'f', 3)
            .arg(vout_abs, 0, 'f', 3)
            .arg(iout, 0, 'f', 4)
            .arg(regulator_loss_w, 0, 'f', 3)
            .toStdString(),
        "To przybliżona średnia strata mocy dla regulatora liniowego."));
    section.entries.push_back(make_entry(
        "Strata mocy rezystora", "P_R", format_fixed(zener_resistor_loss_w, 3), "W",
        "P_R \\approx (V_{src,min} - V_{out}) \\cdot (I_{out} + I_Z)",
        "",
        "Dobierz rezystor z zapasem mocy, nie tylko z samej rezystancji."));
    section.entries.push_back(make_entry(
        "Strata mocy diody Zenera", "P_Z", format_fixed(zener_diode_loss_w, 3), "W",
        "P_Z \\approx V_Z \\cdot I_Z", "",
        "To uproszczone oszacowanie minimalnej mocy traconej w diodzie."));
    section.checklist_items.push_back(make_checklist_item(
        "Czy wybrane zasilanie ma wystarczający zapas do stabilizacji?", can_regulate));
    section.checklist_items.push_back(
        make_checklist_item("Czy przewidywana strata mocy nie wymaga mocniejszego elementu lub radiatora?",
                            !thermal_warning));
    if (!can_regulate) {
      section.notes.push_back(make_warning_note(
          "Brak zapasu regulacji",
          "Wyjście na wykresie nie pokazuje już stabilnych " + format_fixed(vout, 3) +
              " V. Gdy napięcie wejściowe spada za nisko, model pokazuje przybliżone wyjście ograniczone przez dropout i przepuszczone tętnienia."));
    }
    if (thermal_warning) {
      section.notes.push_back(make_warning_note(
          "Straty cieplne",
          "Na regulatorze lub rezystorze szeregowym może wydzielać się zauważalna moc. Sprawdź dobór mocy elementu i potrzebę radiatora."));
    }
  } else if (!document.sections.empty()) {
    document.sections.front().entries.insert(document.sections.front().entries.begin(), make_entry(
        "Dostępne minimum z zasilacza", "V_{src,min}", format_fixed(available_source_min, 3), "V",
        "V_{src,min} = V_{dc} - V_{ripple}", "",
        "To najniższe napięcie, jakie regulator dostaje z wybranego bloku zasilania."));
    document.sections.front().entries.insert(document.sections.front().entries.begin() + 1, make_entry(
        "Wymagane minimum regulatora", "V_{req,min}", format_fixed(required_vin, 3), "V",
        "V_{req,min} = V_{out} + V_{drop}", QString("%1 + %2 = %3")
                                                .arg(vout_abs, 0, 'f', 3)
                                                .arg(active.regulator_dropout_v, 0, 'f', 3)
                                                .arg(required_vin, 0, 'f', 3)
                                                .toStdString(),
        "To minimalne napięcie potrzebne do utrzymania regulacji."));
    document.sections.front().entries.insert(document.sections.front().entries.begin() + 2, make_entry(
        "Margines regulacji", "\\Delta V", format_fixed(headroom_margin, 3), "V",
        "\\Delta V = V_{src,min} - V_{req,min}",
        QString("%1 - %2 = %3")
            .arg(available_source_min, 0, 'f', 3)
            .arg(required_vin, 0, 'f', 3)
            .arg(headroom_margin, 0, 'f', 3)
            .toStdString(),
        "Dodatni wynik oznacza realny zapas napięcia. Ujemny oznacza utratę regulacji."));
    document.sections.front().entries.push_back(make_entry(
        "Strata mocy regulatora", "P_{reg}", format_fixed(regulator_loss_w, 3), "W",
        "P_{reg} \\approx (V_{in,avg} - V_{out}) \\cdot I_{out}",
        QString("(%1 - %2) \\cdot %3 = %4")
            .arg(input_avg_abs, 0, 'f', 3)
            .arg(vout_abs, 0, 'f', 3)
            .arg(iout, 0, 'f', 4)
            .arg(regulator_loss_w, 0, 'f', 3)
            .toStdString(),
        "To przybliżona średnia strata cieplna stabilizatora."));
    document.sections.front().entries.push_back(make_entry(
        "Szczytowa strata mocy", "P_{reg,pk}", format_fixed(regulator_loss_peak_w, 3), "W",
        "P_{reg,pk} \\approx (V_{in,max} - V_{out}) \\cdot I_{out}", "",
        "Orientacyjny górny punkt pracy cieplnej przy najwyższym napięciu wejściowym."));
    if (!can_regulate) {
      document.sections.front().notes.push_back(make_warning_note(
          "Brak zapasu regulacji",
          "Wyjście na wykresie pokazuje stan po utracie regulacji: napięcie zależy wtedy od wejścia i dropout."));
    }
    if (thermal_warning) {
      document.sections.front().notes.push_back(make_warning_note(
          "Straty cieplne",
          "Przy dużej różnicy między wejściem a wyjściem regulator liniowy może wymagać lepszego elementu lub radiatora."));
    }
  }

  widgets.compute_view->set_document(document);
  widgets.waveform_in->set_params(input_waveform);
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
  const double psrr_db = active.psrr_db;
  const double disturbance_rejection_db = active.supply_disturbance_rejection_db;
  const double disturbance_freq_hz = active.supply_disturbance_freq_hz;
  const double capacitor_esr_ohm = std::max(0.0, active.capacitor_esr_ohm);
  const double transformer_secondary_res_ohm =
      std::max(0.0, active.transformer_secondary_res_ohm);
  const auto shape = waveform_shape_for(active);
  const double vout_peak = amp * gain;
  const double vout_rms = pep::modules::psu_basic::peak_to_rms(vout_peak, shape);
  const double vout_avg_abs = pep::modules::psu_basic::avg_abs_from_peak(vout_peak, shape);
  const double i_rms = (load_r > 0.0) ? vout_rms / load_r : 0.0;
  const double i_avg_abs = (load_r > 0.0) ? vout_avg_abs / load_r : 0.0;
  const double p_load = (load_r > 0.0) ? (vout_rms * vout_rms / load_r) : 0.0;
  const double supply_v_est = (vout_peak + std::max(0.0, headroom) > 0.0)
                                  ? (vout_peak + std::max(0.0, headroom))
                                  : 0.0;
  const double supply_current_est =
      (p_load > 0.0 && supply_v_est > 0.0) ? (p_load / supply_v_est) : 0.0;

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

    if (vout_peak > 0.0 && psrr_db > 0.0 && disturbance_rejection_db > 0.0) {
      const double disturbance_ratio = std::pow(10.0, disturbance_rejection_db / 20.0);
      const double psrr_ratio = std::pow(10.0, psrr_db / 20.0);
      const double allowed_output_disturbance_v = vout_peak / disturbance_ratio;
      const double allowed_supply_ripple_v = allowed_output_disturbance_v * psrr_ratio;
      const auto output_disturbance_display = format_scaled_voltage(allowed_output_disturbance_v, 3);
      const auto supply_ripple_display = format_scaled_voltage(allowed_supply_ripple_v, 3);

      CalculationSection ripple_section =
          make_section("Dopuszczalne tętnienia i PSRR");
      ripple_section.entries.push_back(make_entry(
          "Napięcie odniesienia na obciążeniu",
          "V_{load,ref}",
          format_fixed(vout_peak, 3),
          "V",
          "V_{load,ref} = V_{in} \\cdot A_v",
          QString("%1 \\cdot %2 = %3")
              .arg(amp, 0, 'f', 3)
              .arg(gain, 0, 'f', 2)
              .arg(vout_peak, 0, 'f', 3)
              .toStdString(),
          "Przyjmujemy amplitudę sygnału wyjściowego jako poziom odniesienia."));
      ripple_section.entries.push_back(make_entry(
          "Współczynnik PSRR",
          "k_{PSRR}",
          format_fixed(psrr_ratio, 0),
          "",
          "k_{PSRR} = 10^{PSRR/20}",
          QString("10^{%1/20} = %2")
              .arg(psrr_db, 0, 'f', 1)
              .arg(psrr_ratio, 0, 'f', 0)
              .toStdString(),
          "PSRR opisuje, jak mocno tętnienia zasilania są tłumione na wyjściu."));
      ripple_section.entries.push_back(make_entry(
          "Współczynnik dopuszczalnego poziomu zakłócenia",
          "k_{dist}",
          format_fixed(disturbance_ratio, 0),
          "",
          "k_{dist} = 10^{D/20}",
          QString("10^{%1/20} = %2")
              .arg(disturbance_rejection_db, 0, 'f', 1)
              .arg(disturbance_ratio, 0, 'f', 0)
              .toStdString(),
          "To relacja między sygnałem użytecznym a dopuszczalnym zakłóceniem pochodzącym z zasilania."));
      ripple_section.entries.push_back(make_entry(
          "Dopuszczalna składowa zakłócenia na wyjściu",
          "\\Delta V_{out,max}",
          output_disturbance_display.value,
          output_disturbance_display.unit,
          "\\Delta V_{out,max} = \\frac{V_{load,ref}}{k_{dist}}",
          QString("%1 / %2 = %3")
              .arg(vout_peak, 0, 'f', 3)
              .arg(disturbance_ratio, 0, 'f', 0)
              .arg(allowed_output_disturbance_v, 0, 'f', 6)
              .toStdString(),
          "Tak mały poziom zakłócenia powinien pozostać na wyjściu wzmacniacza."));
      ripple_section.entries.push_back(make_entry(
          "Dopuszczalne tętnienia zasilania",
          "\\Delta V_{cc,max}",
          supply_ripple_display.value,
          supply_ripple_display.unit,
          "\\Delta V_{cc,max} = \\Delta V_{out,max} \\cdot k_{PSRR}",
          QString("%1 \\cdot %2 = %3")
              .arg(allowed_output_disturbance_v, 0, 'f', 6)
              .arg(psrr_ratio, 0, 'f', 0)
              .arg(allowed_supply_ripple_v, 0, 'f', 6)
              .toStdString(),
          "To maksymalny poziom tętnień na szynie zasilania wynikający z PSRR."));
      ripple_section.notes.push_back(make_info_note(
          "Uwaga praktyczna",
          std::string("W praktyce ten wynik warto traktować jako punkt startowy. Rzeczywisty "
                      "PSRR zwykle zależy od częstotliwości. Wpisz częstotliwość zakłócenia, "
                      "dla której odczytujesz PSRR z katalogu: ")
              + QString::number(disturbance_freq_hz, 'f', 1).toStdString() + " Hz."));
      if (active.max_ripple_vpp > 0.0) {
        ripple_section.checklist_items.push_back(make_checklist_item(
            "Czy wpisany limit tętnień zasilania mieści się w dopuszczalnym PSRR?",
            active.max_ripple_vpp <= allowed_supply_ripple_v));
      }
      document.sections.push_back(ripple_section);
    }

    if (supply_current_est > 0.0 && active.max_ripple_vpp > 0.0 && supply_v_est > 0.0) {
      const double supply_peak_est = supply_v_est + active.max_ripple_vpp;
      const auto pulse = pep::modules::psu_basic::estimate_charge_pulse(
          supply_current_est, active.max_ripple_vpp, supply_peak_est, active.mains_hz,
          pep::modules::psu_basic::RectifierType::FullWaveBridge, capacitor_esr_ohm,
          transformer_secondary_res_ohm);
      const auto recharge_interval = format_duration(pulse.recharge_interval_s, 3);
      const auto conduction_time = format_duration(pulse.conduction_time_s, 3);

      CalculationSection charge_section =
          make_section("Ładowanie kondensatora i prąd prostownika");
      charge_section.entries.push_back(make_entry(
          "Szacowany średni prąd zasilacza",
          "I_{load,dc}",
          format_fixed(supply_current_est, 3),
          "A",
          "I_{load,dc} \\approx P_{load} / V_{dc,min}",
          QString("%1 / %2 = %3")
              .arg(p_load, 0, 'f', 3)
              .arg(supply_v_est, 0, 'f', 3)
              .arg(supply_current_est, 0, 'f', 3)
              .toStdString(),
          "To uproszczone oszacowanie prądu pobieranego z zasilacza przez wzmacniacz."));
      charge_section.entries.push_back(make_entry(
          "Odstęp między impulsami ładowania",
          "T_d",
          recharge_interval.value,
          recharge_interval.unit,
          "T_d = 1 / f_{ripple}",
          QString("1 / %1 = %2 s")
              .arg(active.mains_hz * 2.0, 0, 'f', 1)
              .arg(pulse.recharge_interval_s, 0, 'f', 6)
              .toStdString(),
          "Dla mostka Graetza kondensator jest doładowywany dwa razy na okres sieci."));
      charge_section.entries.push_back(make_entry(
          "Szacowany czas doładowania kondensatora",
          "T_c",
          conduction_time.value,
          conduction_time.unit,
          "T_c \\approx \\frac{1}{2 \\pi f} \\sqrt{\\frac{2 \\Delta V}{V_{peak}}}",
          QString("1/(2*pi*%1) * sqrt(2*%2/%3) = %4 s")
              .arg(active.mains_hz, 0, 'f', 1)
              .arg(active.max_ripple_vpp, 0, 'f', 3)
              .arg(supply_peak_est, 0, 'f', 3)
              .arg(pulse.conduction_time_s, 0, 'f', 6)
              .toStdString(),
          "To przybliżenie małosygnałowe: krótkie przewodzenie w pobliżu szczytu sinusoidy."));
      charge_section.entries.push_back(make_entry(
          "Idealny szczyt prądu prostownika",
          "I_{peak,ideal}",
          format_fixed(pulse.ideal_peak_current_a, 2),
          "A",
          "I_{peak,ideal} \\approx 2 I_{load,dc} \\cdot T_d / T_c",
          QString("2 * %1 * %2 / %3 = %4")
              .arg(supply_current_est, 0, 'f', 3)
              .arg(pulse.recharge_interval_s, 0, 'f', 6)
              .arg(pulse.conduction_time_s, 0, 'f', 6)
              .arg(pulse.ideal_peak_current_a, 0, 'f', 3)
              .toStdString(),
          "Ten wynik wynika z bilansu ładunku bez uwzględnienia rezystancji strat."));
      charge_section.entries.push_back(make_entry(
          "Suma rezystancji strat",
          "R_{sum}",
          format_fixed(pulse.series_resistance_ohm, 3),
          "Ohm",
          "R_{sum} = ESR + R_{sec}",
          QString("%1 + %2 = %3")
              .arg(capacitor_esr_ohm, 0, 'f', 3)
              .arg(transformer_secondary_res_ohm, 0, 'f', 3)
              .arg(pulse.series_resistance_ohm, 0, 'f', 3)
              .toStdString(),
          "To uproszczony model strat ograniczających impuls ładowania."));
      charge_section.entries.push_back(make_entry(
          "Szczyt prądu ograniczony rezystancją",
          "I_{peak,R}",
          format_fixed(pulse.resistance_limited_peak_current_a, 2),
          "A",
          "I_{peak,R} \\approx \\Delta V / R_{sum}",
          QString("%1 / %2 = %3")
              .arg(active.max_ripple_vpp, 0, 'f', 3)
              .arg(std::max(pulse.series_resistance_ohm, 1e-9), 0, 'f', 3)
              .arg(pulse.resistance_limited_peak_current_a, 0, 'f', 3)
              .toStdString(),
          "Mniejsze ESR i mniejsza rezystancja uzwojenia zwykle podnoszą pik prądu."));
      charge_section.entries.push_back(make_entry(
          "Szacowany szczyt prądu prostownika",
          "I_{peak,est}",
          format_fixed(pulse.estimated_peak_current_a, 2),
          "A",
          "I_{peak,est} \\approx min(I_{peak,ideal}, I_{peak,R})",
          QString("min(%1, %2) = %3")
              .arg(pulse.ideal_peak_current_a, 0, 'f', 3)
              .arg(pulse.resistance_limited_peak_current_a, 0, 'f', 3)
              .arg(pulse.estimated_peak_current_a, 0, 'f', 3)
              .toStdString(),
          "Jeśli ograniczenie rezystancyjne jest silne, realny impuls będzie niższy i szerszy."));
      charge_section.entries.push_back(make_entry(
          "Szacowany prąd skuteczny uzwojenia wtórnego",
          "I_{sec,rms}",
          format_fixed(pulse.secondary_rms_current_a, 2),
          "A",
          "I_{sec,rms} \\approx I_{peak,est} \\sqrt{T_c / (3 T_d)}",
          QString("%1 * sqrt(%2 / (3 * %3)) = %4")
              .arg(pulse.estimated_peak_current_a, 0, 'f', 3)
              .arg(pulse.conduction_time_s, 0, 'f', 6)
              .arg(pulse.recharge_interval_s, 0, 'f', 6)
              .arg(pulse.secondary_rms_current_a, 0, 'f', 3)
              .toStdString(),
          "Krótki, wysoki impuls może dać większy RMS niż sugeruje sam prąd średni obciążenia."));
      charge_section.notes.push_back(make_info_note(
          "Co to jest ESR?",
          "ESR to zastępcza rezystancja szeregowa kondensatora. Modeluje straty wewnętrzne i "
          "powoduje dodatkowy spadek napięcia oraz grzanie przy prądach tętnień."));
      charge_section.notes.push_back(make_info_note(
          "Dlaczego prąd skuteczny wtórnego bywa większy od średniego prądu obciążenia?",
          "Bo transformator i diody nie przewodzą stale. Kondensator doładowuje się krótkimi "
          "impulsami o dużej amplitudzie, a RMS mocno rośnie od wysokich pików."));
      charge_section.notes.push_back(make_info_note(
          "Jak symulować straty w LTspice?",
          "Kondensator: ustaw parametr Rser albo dodaj osobny rezystor szeregowy. "
          "Uzwojenie transformatora lub indukcyjność: ustaw Rser uzwojenia albo dodaj rezystor "
          "w szereg z odpowiednim uzwojeniem."));
      charge_section.notes.push_back(make_warning_note(
          "Ograniczenia modelu",
          "To model edukacyjny oparty na krótkim impulsie trójkątnym i przybliżeniu wokół "
          "szczytu sinusoidy. Nie uwzględnia rozproszenia transformatora, odzysku diod ani "
          "nasycenia rdzenia."));
      charge_section.checklist_items.push_back(make_checklist_item(
          "Czy ESR i rezystancja uzwojenia nie są sztucznie zbyt małe względem planowanego transformatora?",
          pulse.series_resistance_ohm > 0.0));
      charge_section.checklist_items.push_back(make_checklist_item(
          "Czy szacowany prąd skuteczny wtórnego nie jest dużo większy od średniego prądu zasilacza?",
          pulse.secondary_rms_current_a <= supply_current_est * 2.0));
      document.sections.push_back(charge_section);
    }

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
    int source_id = active->amp_power_pos_source_id;
    if (source_id == 0) {
      source_id = pep::modules::project_design::connected_direct_supply_block_id(
          blocks, connections, active->id);
    }
    if (source_id == 0) {
      warnings.push_back(
          "Projektowanie zasilacza: brak wybranego lub podłączonego źródła zasilania.");
    } else {
      const Block *source = find_block(blocks, source_id);
      const int psu_id =
          pep::modules::project_design::resolve_power_block_id(blocks, connections, source_id);
      const Block *psu = find_block(blocks, psu_id);
      if (!source || (source->kind != BlockKind::Power && source->kind != BlockKind::Regulator)) {
        warnings.push_back("Projektowanie zasilacza: wskazany blok nie jest poprawnym zasilaniem.");
      } else if (!psu || psu->kind != BlockKind::Power) {
        warnings.push_back(
            "Projektowanie zasilacza: dla wybranego źródła nie znaleziono nadrzędnego zasilacza.");
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
  } else if (active->kind == BlockKind::Regulator) {
    update_regulator_view(*active, blocks, connections, widgets);
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
