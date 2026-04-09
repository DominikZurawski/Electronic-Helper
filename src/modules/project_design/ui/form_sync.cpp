#include "form_sync.hpp"

#include "../model/model.hpp"

#include <QComboBox>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QStackedWidget>

namespace pep::modules::project_design {

namespace {

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
  return widgets.props_stack && widgets.variant && widgets.vin_input && widgets.freq_input &&
         widgets.current_input && widgets.cap_input && widgets.extra_rails_input &&
         widgets.amp_waveform && widgets.amp_power_source && widgets.amp_amp_input &&
         widgets.amp_freq_input && widgets.amp_gain_input;
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
    const int idx = widgets.variant->findData(static_cast<int>(active->variant));
    if (idx >= 0) {
      widgets.variant->setCurrentIndex(idx);
    }
    widgets.vin_input->setText(active->vin_ac_rms == 0.0 ? ""
                                                         : QString::number(active->vin_ac_rms));
    widgets.freq_input->setText(active->mains_hz == 0.0 ? "" : QString::number(active->mains_hz));
    widgets.current_input->setText(
        active->load_current == 0.0 ? "" : QString::number(active->load_current));
    widgets.cap_input->setText(active->capacitor_uF == 0.0 ? ""
                                                           : QString::number(active->capacitor_uF));
    widgets.extra_rails_input->setText(QString::fromStdString(active->extra_pos_rails_csv));
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
    active->variant = static_cast<BlockVariant>(widgets.variant->currentData().toInt());
    active->vin_ac_rms = widgets.vin_input->text().toDouble();
    active->mains_hz = read_or(widgets.freq_input, 50.0);
    active->load_current = read_or(widgets.current_input, 0.0);
    active->capacitor_uF = read_or(widgets.cap_input, 0.0);
    active->extra_pos_rails_csv = widgets.extra_rails_input->text().toStdString();
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
