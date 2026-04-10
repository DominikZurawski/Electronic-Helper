#include "form_sync.hpp"

#include "../model/model.hpp"

#include "../../psu_basic/model/model.hpp"

#include <QComboBox>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QStackedWidget>

namespace pep::modules::project_design {

namespace {

enum class PowerFamily { Linear, Switching };

double read_or(QLineEdit *edit, double fallback) {
  return edit->text().isEmpty() ? fallback : edit->text().toDouble();
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
         widgets.transformer_mode && widgets.transformer_waveform &&
         widgets.transformer_voltage_quantity && widgets.variant &&
         widgets.transformer_primary_input && widgets.transformer_ratio_input &&
         widgets.transformer_secondary_input && widgets.vin_input && widgets.freq_input &&
         widgets.current_input && widgets.cap_input &&
         widgets.amp_waveform && widgets.amp_power_source && widgets.amp_amp_input &&
         widgets.amp_freq_input && widgets.amp_gain_input;
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

int selected_psu_for_active(const Block &active, const std::vector<Block> &blocks,
                            const std::vector<Connection> &connections) {
  for (const auto &connection : connections) {
    const Endpoint a = connection.a;
    const Endpoint b = connection.b;
    const bool a_is_active = (a.block_id == active.id);
    const bool b_is_active = (b.block_id == active.id);
    if (!a_is_active && !b_is_active) {
      continue;
    }

    const Endpoint other = a_is_active ? b : a;
    const Block *other_block = find_block(blocks, other.block_id);
    if (!other_block || other_block->kind != BlockKind::Power) {
      continue;
    }

    const auto active_port = find_port(active, a_is_active ? a.port_id : b.port_id);
    if (!active_port.has_value()) {
      continue;
    }
    if (active_port->type != PortType::PowerPos && active_port->type != PortType::PowerNeg &&
        active_port->type != PortType::Ground) {
      continue;
    }

    return other_block->id;
  }

  return 0;
}

} // namespace

void sync_active_to_form(const Block *active, const std::vector<Block> &blocks,
                         const std::vector<Connection> &connections, const FormWidgets &widgets,
                         bool *sync_flag, const std::function<void()> &refresh_power_source_combo) {
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
    widgets.transformer_primary_input->setText(
        active->transformer_primary_v == 0.0 ? "" : QString::number(active->transformer_primary_v));
    widgets.transformer_ratio_input->setText(
        active->transformer_turns_ratio == 0.0 ? ""
                                               : QString::number(active->transformer_turns_ratio));
    widgets.transformer_secondary_input->setText(
        active->transformer_secondary_v == 0.0
            ? ""
            : QString::number(active->transformer_secondary_v));
    widgets.vin_input->setText(active->vin_ac_rms == 0.0 ? ""
                                                         : QString::number(active->vin_ac_rms));
    widgets.freq_input->setText(active->mains_hz == 0.0 ? "" : QString::number(active->mains_hz));
    widgets.current_input->setText(
        active->load_current == 0.0 ? "" : QString::number(active->load_current));
    widgets.cap_input->setText(active->capacitor_uF == 0.0 ? ""
                                                           : QString::number(active->capacitor_uF));
    return;
  }

  widgets.props_stack->setCurrentIndex(1);
  refresh_power_source_combo();

  const int selected_psu = selected_psu_for_active(*active, blocks, connections);
  const int power_idx = widgets.amp_power_source->findData(selected_psu);
  {
    const QSignalBlocker blocker(widgets.amp_power_source);
    widgets.amp_power_source->setCurrentIndex(power_idx >= 0 ? power_idx : 0);
  }

  const int waveform_idx =
      widgets.amp_waveform->findData(static_cast<int>(active->signal_waveform));
  if (waveform_idx >= 0) {
    widgets.amp_waveform->setCurrentIndex(waveform_idx);
  }
  widgets.amp_amp_input->setText(
      active->signal_amp_v == 0.0 ? "" : QString::number(active->signal_amp_v));
  widgets.amp_freq_input->setText(active->signal_hz == 0.0 ? ""
                                                           : QString::number(active->signal_hz));
  widgets.amp_gain_input->setText(active->gain == 0.0 ? "" : QString::number(active->gain));
}

void sync_form_to_active(Block *active, const FormWidgets &widgets) {
  if (!active || !widgets_ready(widgets)) {
    return;
  }

  if (active->kind == BlockKind::Power) {
    update_transformer_controls(widgets);
    active->variant = selected_power_variant(widgets);
    active->transformer_solve_mode =
        static_cast<TransformerSolveMode>(widgets.transformer_mode->currentData().toInt());
    active->transformer_waveform =
        static_cast<SignalWaveform>(widgets.transformer_waveform->currentData().toInt());
    active->transformer_voltage_quantity =
        static_cast<VoltageQuantity>(widgets.transformer_voltage_quantity->currentData().toInt());
    active->transformer_primary_v = widgets.transformer_primary_input->text().toDouble();
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

    active->mains_hz = read_or(widgets.freq_input, 50.0);
    active->load_current = read_or(widgets.current_input, 0.0);
    active->capacitor_uF = read_or(widgets.cap_input, 0.0);
    return;
  }

  active->variant = BlockVariant::AmpModel1b;
  active->signal_waveform =
      static_cast<SignalWaveform>(widgets.amp_waveform->currentData().toInt());
  active->signal_amp_v = widgets.amp_amp_input->text().toDouble();
  active->signal_hz = read_or(widgets.amp_freq_input, 1000.0);
  active->gain = read_or(widgets.amp_gain_input, 10.0);
}

} // namespace pep::modules::project_design
