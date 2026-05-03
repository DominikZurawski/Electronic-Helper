#include "form_sync.hpp"

#include "../model/connection_ops.hpp"
#include "../model/model.hpp"

#include "../../psu_basic/model/model.hpp"

#include <QComboBox>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QStackedWidget>

#include <cmath>

namespace pep::modules::project_design {

namespace {

enum class PowerFamily { Linear, Switching };

double read_or(QLineEdit *edit, double fallback) {
  if (!edit || edit->text().isEmpty()) {
    return fallback;
  }
  QString normalized = edit->text().trimmed();
  normalized.replace(',', '.');
  bool ok = false;
  const double value = normalized.toDouble(&ok);
  return ok ? value : fallback;
}

struct SyncGuard {
  bool *flag = nullptr;

  explicit SyncGuard(bool *f) : flag(f) {
    if (flag) {
      *flag = true;
    }
  }

  ~SyncGuard() {
    if (flag) {
      *flag = false;
    }
  }
};

bool widgets_ready(const FormWidgets &widgets) {
  return widgets.props_stack && widgets.power_family && widgets.power_linear_variant &&
         widgets.power_design_mode && widgets.transformer_mode && widgets.transformer_waveform &&
         widgets.transformer_voltage_quantity && widgets.variant &&
         widgets.transformer_primary_input && widgets.transformer_primary_tol_input &&
         widgets.transformer_ratio_input &&
         widgets.transformer_secondary_input && widgets.vin_input && widgets.diode_drop_input &&
         widgets.freq_input &&
         widgets.current_input && widgets.cap_input && widgets.cap_tol_input &&
         widgets.max_ripple_input && widgets.regulator_variant &&
         widgets.regulator_power_source && widgets.regulator_supply_rail &&
         widgets.regulator_source_min_input &&
         widgets.regulator_input_min_input && widgets.regulator_margin_input &&
         widgets.regulator_output_input &&
         widgets.regulator_current_input && widgets.regulator_zener_current_input &&
         widgets.regulator_dropout_input &&
         widgets.amp_waveform && widgets.amp_design_mode && widgets.amp_power_source_pos &&
         widgets.amp_power_source_neg &&
         widgets.amp_amp_input &&
         widgets.amp_freq_input && widgets.amp_gain_input &&
         widgets.amp_load_input && widgets.amp_power_input && widgets.amp_headroom_input &&
         widgets.amp_psrr_input && widgets.amp_disturbance_rejection_input &&
         widgets.amp_disturbance_freq_input && widgets.amp_cap_esr_input &&
         widgets.amp_transformer_res_input &&
         widgets.amp_max_ripple_input;
}

PowerFamily power_family_for(BlockVariant variant) {
  if (variant == BlockVariant::PsuSwitching) {
    return PowerFamily::Switching;
  }
  return PowerFamily::Linear;
}

pep::modules::psu_basic::WaveformShape to_psu_waveform(SignalWaveform waveform) {
  switch (waveform) {
  case SignalWaveform::Square:
    return pep::modules::psu_basic::WaveformShape::Square;
  case SignalWaveform::Triangle:
    return pep::modules::psu_basic::WaveformShape::Triangle;
  default:
    return pep::modules::psu_basic::WaveformShape::Sine;
  }
}

pep::modules::psu_basic::VoltageQuantity to_psu_voltage_quantity(VoltageQuantity quantity) {
  return quantity == VoltageQuantity::Peak ? pep::modules::psu_basic::VoltageQuantity::Peak
                                           : pep::modules::psu_basic::VoltageQuantity::Rms;
}

pep::modules::psu_basic::TransformerSolveMode to_psu_transformer_mode(
    TransformerSolveMode mode) {
  return mode == TransformerSolveMode::RatioFromSecondary
             ? pep::modules::psu_basic::TransformerSolveMode::RatioFromSecondary
             : pep::modules::psu_basic::TransformerSolveMode::SecondaryFromRatio;
}

void update_transformer_controls(const FormWidgets &widgets) {
  const bool linear = static_cast<PowerFamily>(widgets.power_family->currentData().toInt()) ==
                      PowerFamily::Linear;
  const bool solve_ratio = static_cast<TransformerSolveMode>(
                               widgets.transformer_mode->currentData().toInt()) ==
                           TransformerSolveMode::RatioFromSecondary;

  widgets.power_linear_variant->setEnabled(linear);
  widgets.power_linear_variant->setVisible(linear);
  widgets.transformer_mode->setEnabled(linear);
  widgets.transformer_waveform->setEnabled(linear);
  widgets.transformer_voltage_quantity->setEnabled(linear);
  widgets.transformer_primary_input->setEnabled(linear);
  widgets.transformer_primary_tol_input->setEnabled(linear);
  widgets.transformer_ratio_input->setEnabled(linear && !solve_ratio);
  widgets.transformer_secondary_input->setEnabled(linear && solve_ratio);
  widgets.vin_input->setReadOnly(true);
}

void sync_power_variant_controls(const FormWidgets &widgets, BlockVariant variant) {
  const QSignalBlocker family_blocker(widgets.power_family);
  const QSignalBlocker linear_variant_blocker(widgets.power_linear_variant);

  const int family_index =
      widgets.power_family->findData(static_cast<int>(power_family_for(variant)));
  if (family_index >= 0) {
    widgets.power_family->setCurrentIndex(family_index);
  }

  const bool is_linear = power_family_for(variant) == PowerFamily::Linear;
  const int linear_variant_index = widgets.power_linear_variant->findData(static_cast<int>(variant));
  if (is_linear && linear_variant_index >= 0) {
    widgets.power_linear_variant->setCurrentIndex(linear_variant_index);
  }

  update_transformer_controls(widgets);
}

BlockVariant selected_power_variant(const FormWidgets &widgets) {
  if (static_cast<PowerFamily>(widgets.power_family->currentData().toInt()) ==
      PowerFamily::Switching) {
    return BlockVariant::PsuSwitching;
  }
  return static_cast<BlockVariant>(widgets.power_linear_variant->currentData().toInt());
}

pep::modules::psu_basic::TransformerOutput compute_transformer_from_widgets(
    const FormWidgets &widgets) {
  return pep::modules::psu_basic::compute_transformer(pep::modules::psu_basic::TransformerInput{
      widgets.transformer_primary_input->text().toDouble(),
      widgets.transformer_secondary_input->text().toDouble(),
      widgets.transformer_ratio_input->text().toDouble(),
      to_psu_waveform(
          static_cast<SignalWaveform>(widgets.transformer_waveform->currentData().toInt())),
      to_psu_voltage_quantity(
          static_cast<VoltageQuantity>(widgets.transformer_voltage_quantity->currentData().toInt())),
      to_psu_transformer_mode(
          static_cast<TransformerSolveMode>(widgets.transformer_mode->currentData().toInt()))});
}

double regulator_input_min_from_power_block(const Block &power, SupplyRail rail) {
  if (power.kind != BlockKind::Power) {
    return 0.0;
  }
  if (rail == SupplyRail::Vee && power.variant != BlockVariant::PsuSymmetric) {
    return 0.0;
  }

  pep::modules::psu_basic::Input input;
  input.vin_ac_rms = power.vin_ac_rms;
  input.mains_hz = power.mains_hz;
  input.load_current = power.load_current;
  input.capacitor_uF = power.capacitor_uF;
  input.cap_tol_pct = power.capacitor_tol_pct;
  input.diode_drop = power.diode_drop;
  input.rectifier = pep::modules::psu_basic::RectifierType::FullWaveBridge;
  const auto output = pep::modules::psu_basic::compute(input);
  const double vmin = std::max(0.0, output.vmin);
  if (vmin > 0.0) {
    return vmin;
  }
  return std::max(0.0, output.vrect);
}

double regulator_required_input_min(const Block &regulator) {
  return std::abs(regulator.regulator_output_v) + std::max(0.0, regulator.regulator_dropout_v);
}

double regulator_headroom_margin(double available_source_min, const Block &regulator) {
  return available_source_min - regulator_required_input_min(regulator);
}

bool can_source_negative_rail_from_ground(const Block *block) {
  return block && block->kind == BlockKind::Power && block->variant != BlockVariant::PsuSymmetric;
}

} // namespace

void sync_active_to_form(const Block *active, const std::vector<Block> &blocks,
                         const std::vector<Connection> &connections, const FormWidgets &widgets,
                         bool *sync_flag,
                         const std::function<void()> &refresh_regulator_power_source_combo,
                         const std::function<void()> &refresh_amp_power_source_combo_pos,
                         const std::function<void()> &refresh_amp_power_source_combo_neg) {
  if (!active || !widgets_ready(widgets)) {
    return;
  }

  SyncGuard guard(sync_flag);

  if (active->kind == BlockKind::Power) {
    widgets.props_stack->setCurrentIndex(0);
    sync_power_variant_controls(widgets, active->variant);
    {
      const QSignalBlocker blocker_mode(widgets.transformer_mode);
      const QSignalBlocker blocker_waveform(widgets.transformer_waveform);
      const QSignalBlocker blocker_quantity(widgets.transformer_voltage_quantity);
      const int mode_idx =
          widgets.transformer_mode->findData(static_cast<int>(active->transformer_solve_mode));
      const int waveform_idx =
          widgets.transformer_waveform->findData(static_cast<int>(active->transformer_waveform));
      const int quantity_idx = widgets.transformer_voltage_quantity->findData(
          static_cast<int>(active->transformer_voltage_quantity));
      if (mode_idx >= 0) {
        widgets.transformer_mode->setCurrentIndex(mode_idx);
      }
      if (waveform_idx >= 0) {
        widgets.transformer_waveform->setCurrentIndex(waveform_idx);
      }
      if (quantity_idx >= 0) {
        widgets.transformer_voltage_quantity->setCurrentIndex(quantity_idx);
      }
    }
    update_transformer_controls(widgets);

    const int idx = widgets.variant->findData(static_cast<int>(active->variant));
    if (idx >= 0) {
      widgets.variant->setCurrentIndex(idx);
    }
    const int power_mode_idx =
        widgets.power_design_mode->findData(static_cast<int>(active->power_design_mode));
    widgets.power_design_mode->setCurrentIndex(power_mode_idx >= 0 ? power_mode_idx : 0);
    widgets.transformer_primary_input->setText(
        active->transformer_primary_v == 0.0 ? "" : QString::number(active->transformer_primary_v));
    widgets.transformer_primary_tol_input->setText(
        active->transformer_primary_tol_pct == 0.0
            ? ""
            : QString::number(active->transformer_primary_tol_pct));
    widgets.transformer_ratio_input->setText(
        active->transformer_turns_ratio == 0.0 ? ""
                                               : QString::number(active->transformer_turns_ratio));
    widgets.transformer_secondary_input->setText(
        active->transformer_secondary_v == 0.0
            ? ""
            : QString::number(active->transformer_secondary_v));
    widgets.vin_input->setText(active->vin_ac_rms == 0.0 ? ""
                                                         : QString::number(active->vin_ac_rms));
    widgets.diode_drop_input->setText(
        active->diode_drop == 0.0 ? "" : QString::number(active->diode_drop));
    widgets.freq_input->setText(active->mains_hz == 0.0 ? "" : QString::number(active->mains_hz));
  widgets.current_input->setText(
      active->load_current == 0.0 ? "" : QString::number(active->load_current));
  widgets.cap_input->setText(active->capacitor_uF == 0.0 ? ""
                                                         : QString::number(active->capacitor_uF));
  widgets.cap_tol_input->setText(active->capacitor_tol_pct == 0.0
                                     ? ""
                                     : QString::number(active->capacitor_tol_pct));
  widgets.max_ripple_input->setText(
      active->max_ripple_vpp == 0.0 ? "" : QString::number(active->max_ripple_vpp));
    return;
  }

  if (active->kind == BlockKind::Regulator) {
    widgets.props_stack->setCurrentIndex(1);
    refresh_regulator_power_source_combo();
    {
      const QSignalBlocker blocker_variant(widgets.regulator_variant);
      const int regulator_variant_index =
          widgets.regulator_variant->findData(static_cast<int>(active->variant));
      if (regulator_variant_index >= 0) {
        widgets.regulator_variant->setCurrentIndex(regulator_variant_index);
      }
    }
    {
      const QSignalBlocker blocker_rail(widgets.regulator_supply_rail);
      const int rail_index =
          widgets.regulator_supply_rail->findData(static_cast<int>(active->regulator_supply_rail));
      widgets.regulator_supply_rail->setCurrentIndex(rail_index >= 0 ? rail_index : 0);
    }
    int selected_source = active->regulator_input_source_id;
    if (selected_source == 0) {
      selected_source = connected_direct_supply_block_id(blocks, connections, active->id);
    }
    {
      const QSignalBlocker blocker_source(widgets.regulator_power_source);
      const int source_idx = widgets.regulator_power_source->findData(selected_source);
      widgets.regulator_power_source->setCurrentIndex(source_idx >= 0 ? source_idx : 0);
    }
    double available_source_min = 0.0;
    double suggested_output_current = active->regulator_output_current_a;
    if (const Block *power = find_block(blocks, selected_source);
        power && power->kind == BlockKind::Power) {
      const double source_vmin =
          regulator_input_min_from_power_block(*power, active->regulator_supply_rail);
      if (source_vmin > 0.0) {
        available_source_min = source_vmin;
      }
      if (power->load_current > 0.0) {
        suggested_output_current = power->load_current;
      }
    }
    widgets.regulator_source_min_input->setText(
        available_source_min == 0.0 ? "" : QString::number(available_source_min));
    widgets.regulator_input_min_input->setText(
        regulator_required_input_min(*active) == 0.0
            ? ""
            : QString::number(regulator_required_input_min(*active)));
    const double margin = regulator_headroom_margin(available_source_min, *active);
    widgets.regulator_margin_input->setText(
        available_source_min == 0.0 ? "" : QString::number(margin));
    widgets.regulator_output_input->setText(
        active->regulator_output_v == 0.0 ? "" : QString::number(active->regulator_output_v));
    widgets.regulator_current_input->setText(suggested_output_current == 0.0
                                                 ? ""
                                                 : QString::number(suggested_output_current));
    widgets.regulator_zener_current_input->setText(
        active->regulator_zener_current_a == 0.0
            ? ""
            : QString::number(active->regulator_zener_current_a));
    widgets.regulator_dropout_input->setText(active->regulator_dropout_v == 0.0
                                                 ? ""
                                                 : QString::number(active->regulator_dropout_v));
    return;
  }

  widgets.props_stack->setCurrentIndex(2);
  refresh_amp_power_source_combo_pos();
  refresh_amp_power_source_combo_neg();

  int selected_psu_pos = active->amp_power_pos_source_id;
  if (selected_psu_pos == 0) {
    selected_psu_pos = connected_direct_supply_block_id(blocks, connections, active->id);
  }
  int selected_psu_neg = active->amp_power_neg_source_id;
  if (selected_psu_neg == 0) {
    selected_psu_neg = connected_direct_supply_block_id(blocks, connections, active->id);
  }
  if (selected_psu_neg == 0) {
    const Block *selected_pos_block = find_block(blocks, selected_psu_pos);
    if (can_source_negative_rail_from_ground(selected_pos_block)) {
      selected_psu_neg = selected_psu_pos;
    }
  }
  const int power_pos_idx = widgets.amp_power_source_pos->findData(selected_psu_pos);
  {
    const QSignalBlocker blocker(widgets.amp_power_source_pos);
    widgets.amp_power_source_pos->setCurrentIndex(power_pos_idx >= 0 ? power_pos_idx : 0);
  }
  const int power_neg_idx = widgets.amp_power_source_neg->findData(selected_psu_neg);
  {
    const QSignalBlocker blocker(widgets.amp_power_source_neg);
    widgets.amp_power_source_neg->setCurrentIndex(power_neg_idx >= 0 ? power_neg_idx : 0);
  }

  const int waveform_idx =
      widgets.amp_waveform->findData(static_cast<int>(active->signal_waveform));
  if (waveform_idx >= 0) {
    widgets.amp_waveform->setCurrentIndex(waveform_idx);
  }
  const int design_idx =
      widgets.amp_design_mode->findData(static_cast<int>(active->amp_design_mode));
  widgets.amp_design_mode->setCurrentIndex(design_idx >= 0 ? design_idx : 0);
  widgets.amp_amp_input->setText(
      active->signal_amp_v == 0.0 ? "" : QString::number(active->signal_amp_v));
  widgets.amp_freq_input->setText(active->signal_hz == 0.0 ? ""
                                                           : QString::number(active->signal_hz));
  widgets.amp_gain_input->setText(active->gain == 0.0 ? "" : QString::number(active->gain));
  widgets.amp_load_input->setText(active->load_resistance_ohm == 0.0
                                      ? ""
                                      : QString::number(active->load_resistance_ohm));
  widgets.amp_power_input->setText(active->target_power_w == 0.0 ? ""
                                                                 : QString::number(active->target_power_w));
  widgets.amp_headroom_input->setText(active->supply_headroom_v == 0.0
                                          ? ""
                                          : QString::number(active->supply_headroom_v));
  widgets.amp_psrr_input->setText(active->psrr_db == 0.0 ? ""
                                                         : QString::number(active->psrr_db));
  widgets.amp_disturbance_rejection_input->setText(
      active->supply_disturbance_rejection_db == 0.0
          ? ""
          : QString::number(active->supply_disturbance_rejection_db));
  widgets.amp_disturbance_freq_input->setText(active->supply_disturbance_freq_hz == 0.0
                                                  ? ""
                                                  : QString::number(active->supply_disturbance_freq_hz));
  widgets.amp_cap_esr_input->setText(active->capacitor_esr_ohm == 0.0
                                         ? ""
                                         : QString::number(active->capacitor_esr_ohm));
  widgets.amp_transformer_res_input->setText(active->transformer_secondary_res_ohm == 0.0
                                                 ? ""
                                                 : QString::number(active->transformer_secondary_res_ohm));
  widgets.amp_max_ripple_input->setText(active->max_ripple_vpp == 0.0
                                            ? ""
                                            : QString::number(active->max_ripple_vpp));
}

void sync_form_to_active(Block *active, const FormWidgets &widgets) {
  if (!active || !widgets_ready(widgets)) {
    return;
  }

  if (active->kind == BlockKind::Power) {
    update_transformer_controls(widgets);
    active->variant = selected_power_variant(widgets);
    active->power_design_mode =
        static_cast<PowerDesignMode>(widgets.power_design_mode->currentData().toInt());
    active->transformer_solve_mode =
        static_cast<TransformerSolveMode>(widgets.transformer_mode->currentData().toInt());
    active->transformer_waveform =
        static_cast<SignalWaveform>(widgets.transformer_waveform->currentData().toInt());
    active->transformer_voltage_quantity =
        static_cast<VoltageQuantity>(widgets.transformer_voltage_quantity->currentData().toInt());
    active->transformer_primary_v = widgets.transformer_primary_input->text().toDouble();
    active->transformer_primary_tol_pct =
        read_or(widgets.transformer_primary_tol_input, 10.0);
    active->transformer_turns_ratio = widgets.transformer_ratio_input->text().toDouble();
    active->transformer_secondary_v = widgets.transformer_secondary_input->text().toDouble();

    const auto transformer = compute_transformer_from_widgets(widgets);
    active->vin_ac_rms = transformer.secondary_rms;
    active->transformer_turns_ratio = transformer.turns_ratio;

    const int idx = widgets.variant->findData(static_cast<int>(active->variant));
    if (idx >= 0) {
      widgets.variant->setCurrentIndex(idx);
    }
    widgets.vin_input->setText(active->vin_ac_rms == 0.0 ? ""
                                                         : QString::number(active->vin_ac_rms));
    widgets.transformer_ratio_input->setText(
        active->transformer_turns_ratio == 0.0 ? ""
                                               : QString::number(active->transformer_turns_ratio));

    active->diode_drop = read_or(widgets.diode_drop_input, 0.7);
    active->mains_hz = read_or(widgets.freq_input, 50.0);
    active->load_current = read_or(widgets.current_input, 0.0);
    active->capacitor_uF = read_or(widgets.cap_input, 0.0);
    active->capacitor_tol_pct = read_or(widgets.cap_tol_input, 20.0);
    active->max_ripple_vpp = read_or(widgets.max_ripple_input, 0.0);
    return;
  }

  if (active->kind == BlockKind::Regulator) {
    active->variant = static_cast<BlockVariant>(widgets.regulator_variant->currentData().toInt());
    active->regulator_input_source_id = widgets.regulator_power_source->currentData().toInt();
    active->regulator_supply_rail =
        static_cast<SupplyRail>(widgets.regulator_supply_rail->currentData().toInt());
    active->regulator_output_v = std::abs(read_or(widgets.regulator_output_input, 5.0));
    active->regulator_output_current_a = read_or(widgets.regulator_current_input, 0.02);
    active->regulator_zener_current_a = read_or(widgets.regulator_zener_current_input, 0.005);
    active->regulator_dropout_v = read_or(widgets.regulator_dropout_input, 2.0);
    active->regulator_input_min_v = regulator_required_input_min(*active);
    active->regulator_series_res_ohm =
        (active->regulator_input_min_v > active->regulator_output_v &&
         active->regulator_output_current_a + active->regulator_zener_current_a > 0.0)
            ? ((active->regulator_input_min_v - active->regulator_output_v) /
               (active->regulator_output_current_a + active->regulator_zener_current_a))
            : 0.0;
    return;
  }

  active->variant = BlockVariant::AmpModel1b;
  active->signal_waveform =
      static_cast<SignalWaveform>(widgets.amp_waveform->currentData().toInt());
  active->amp_design_mode =
      static_cast<AmpDesignMode>(widgets.amp_design_mode->currentData().toInt());
  active->amp_power_pos_source_id = widgets.amp_power_source_pos->currentData().toInt();
  active->amp_power_neg_source_id = widgets.amp_power_source_neg->currentData().toInt();
  active->signal_amp_v = widgets.amp_amp_input->text().toDouble();
  active->signal_hz = read_or(widgets.amp_freq_input, 1000.0);
  active->gain = read_or(widgets.amp_gain_input, 10.0);
  active->load_resistance_ohm = read_or(widgets.amp_load_input, 8.0);
  active->target_power_w = read_or(widgets.amp_power_input, 0.0);
  active->supply_headroom_v = read_or(widgets.amp_headroom_input, 4.0);
  active->psrr_db = read_or(widgets.amp_psrr_input, 80.0);
  active->supply_disturbance_rejection_db =
      read_or(widgets.amp_disturbance_rejection_input, 100.0);
  active->supply_disturbance_freq_hz = read_or(widgets.amp_disturbance_freq_input, 100.0);
  active->capacitor_esr_ohm = read_or(widgets.amp_cap_esr_input, 0.08);
  active->transformer_secondary_res_ohm = read_or(widgets.amp_transformer_res_input, 0.30);
  active->max_ripple_vpp = read_or(widgets.amp_max_ripple_input, 0.0);
}

} // namespace pep::modules::project_design
