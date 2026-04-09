#include "active_block_view.hpp"

#include "../model/model.hpp"
#include "enum_ui.hpp"

#include "../../psu_basic/model/model.hpp"

#include <QLabel>
#include <QTabWidget>

#include <vector>

namespace pep::modules::project_design {

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

void update_power_view(const Block &active, const ActiveBlockWidgets &widgets) {
  pep::modules::psu_basic::Input input;
  input.vin_ac_rms = active.vin_ac_rms;
  input.mains_hz = active.mains_hz;
  input.load_current = active.load_current;
  input.capacitor_uF = active.capacitor_uF;
  input.diode_drop = 0.9;
  input.rectifier = pep::modules::psu_basic::RectifierType::FullWaveBridge;

  const auto output = pep::modules::psu_basic::compute(input);
  widgets.compute_result->setText(QString("Podgląd zasilania:\n"
                                          "Szczytowe napięcie po transformatorze (Vpeak) = %1 V\n"
                                          "Napięcie po prostowaniu (Vrect) = %2 V\n"
                                          "Średnie napięcie DC pod obciążeniem (Vdc) = %3 V\n"
                                          "Tętnienia szczyt‑szczyt (Ripple Vpp) = %4 V\n\n"
                                          "Wzory (uproszczone):\n"
                                          "Vpeak ≈ Vin_rms · √2\n"
                                          "Vrect ≈ Vpeak − 2·Vd\n"
                                          "Ripple ≈ Iload / (f · C)\n")
                                      .arg(output.vpeak, 0, 'f', 2)
                                      .arg(output.vrect, 0, 'f', 2)
                                      .arg(output.vdc_loaded, 0, 'f', 2)
                                      .arg(output.ripple_vpp, 0, 'f', 3));

  pep::modules::psu_basic::WaveformParams input_waveform;
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

  widgets.compute_result->setText(QString("Wzmacniacz model 1(B):\n"
                                          "Amplituda wejściowa = %1 V\n"
                                          "Częstotliwość = %2 Hz\n"
                                          "Wzmocnienie napięciowe = %3 V/V\n"
                                          "Amplituda wyjściowa ≈ Vin · Av = %4 V\n")
                                      .arg(amp, 0, 'f', 3)
                                      .arg(hz, 0, 'f', 1)
                                      .arg(gain, 0, 'f', 2)
                                      .arg(amp * gain, 0, 'f', 3));

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
  if (!widgets.active_title || !widgets.compute_result || !widgets.validation ||
      !widgets.ports_label || !widgets.waveform_in || !widgets.waveform_out) {
    return;
  }

  if (!active) {
    widgets.active_title->setText("Aktywny element: —");
    widgets.compute_result->setText("Wyniki pojawią się tutaj");
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

  update_validation(blocks, connections, widgets.validation);

  if (widgets.bottom_tabs && widgets.waveform_tab_index >= 0) {
    widgets.bottom_tabs->setCurrentIndex(widgets.waveform_tab_index);
  }
}

} // namespace pep::modules::project_design
