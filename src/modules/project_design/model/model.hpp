#pragma once

#include <QPointF>
#include <QString>

#include <optional>
#include <vector>

namespace pep::modules::project_design {

enum class PortType { PowerPos, PowerNeg, Ground, AnalogIn, AnalogOut, Unknown };

struct FlagRef {
  int x = 0;
  int y = 0;
};

struct PortDef {
  QString id;
  QString label;
  PortType type = PortType::Unknown;
  std::vector<FlagRef> flags;
};

struct Block {
  int id = 0;
  QString kind;     // "power" | "amplifier"
  QString variant;  // e.g. "psu_symmetric" | "psu_unregulated" | "model1b"
  QString title;
  QPointF canvas_pos{0.0, 0.0};

  // Parameters (only meaningful for some kinds/variants).
  double vin_ac_rms = 0.0;
  double mains_hz = 50.0;
  double load_current = 0.0;
  double capacitor_uF = 0.0;
  double diode_drop = 0.9;

  // Amplifier/signal parameters.
  double signal_amp_v = 1.0;
  double signal_hz = 1000.0;
  double gain = 10.0;
  QString signal_waveform = "sine"; // "sine" | "square" | "triangle"

  // Additional power rails (dynamic, based on user/calculator fields).
  QString extra_pos_rails_csv; // e.g. "+5V,+12V"
};

struct Endpoint {
  int block_id = 0;
  QString port_id;
};

struct Connection {
  Endpoint a;
  Endpoint b;
};

QString port_type_label(PortType t);
bool ports_compatible(PortType a, PortType b);

Block make_power_block(int id, const QString& title, const QString& variant);
Block make_amp_model1b_block(int id, const QString& title);

std::vector<PortDef> ports_for(const Block& b);
std::optional<PortDef> find_port(const Block& b, const QString& port_id);
Block* find_block(std::vector<Block>& blocks, int id);

} // namespace pep::modules::project_design
