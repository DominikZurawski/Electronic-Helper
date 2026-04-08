#include "model.hpp"

#include <QStringList>

#include <utility>

namespace pep::modules::project_design {

QString port_type_label(PortType t) {
  switch (t) {
  case PortType::PowerPos:
    return "Power +";
  case PortType::PowerNeg:
    return "Power -";
  case PortType::Ground:
    return "GND";
  case PortType::AnalogIn:
    return "Audio IN";
  case PortType::AnalogOut:
    return "Audio OUT";
  default:
    return "Unknown";
  }
}

bool ports_compatible(PortType a, PortType b) {
  if (a == PortType::Unknown || b == PortType::Unknown) {
    return true;
  }
  if (a == PortType::Ground || b == PortType::Ground) {
    return a == b;
  }
  if ((a == PortType::PowerPos || a == PortType::PowerNeg) ||
      (b == PortType::PowerPos || b == PortType::PowerNeg)) {
    return a == b;
  }
  if (a == PortType::AnalogOut) {
    return b == PortType::AnalogIn;
  }
  if (b == PortType::AnalogOut) {
    return a == PortType::AnalogIn;
  }
  return true;
}

Block make_power_block(int id, const QString &title, const QString &variant) {
  Block b;
  b.id = id;
  b.kind = "power";
  b.variant = variant;
  b.title = title;
  b.canvas_pos = QPointF(0, 0);
  b.mains_hz = 50.0;
  b.signal_waveform = "sine";
  return b;
}

Block make_amp_model1b_block(int id, const QString &title) {
  Block b;
  b.id = id;
  b.kind = "amplifier";
  b.variant = "model1b";
  b.title = title;
  b.canvas_pos = QPointF(0, 0);
  b.mains_hz = 50.0;
  b.signal_waveform = "sine";
  b.signal_amp_v = 1.0;
  b.signal_hz = 1000.0;
  b.gain = 10.0;
  return b;
}

std::vector<PortDef> ports_for(const Block &b) {
  if (b.kind == "power") {
    if (b.variant == "psu_symmetric") {
      // Port flags based on provided template (assets/ltspice/power.asc).
      std::vector<PortDef> ports = {
          {"vcc", "Vcc", PortType::PowerPos, {FlagRef{672, -352}}},
          {"vee", "Vee", PortType::PowerNeg, {FlagRef{672, -64}}},
          {"gnd", "0", PortType::Ground, {}},
      };
      // Dynamic extra + rails (no export flags yet).
      const QStringList items = b.extra_pos_rails_csv.split(',', Qt::SkipEmptyParts);
      int i = 0;
      for (QString s : items) {
        s = s.trimmed();
        if (s.isEmpty()) {
          continue;
        }
        ports.push_back({QString("pos%1").arg(i++), s, PortType::PowerPos, {}});
      }
      return ports;
    }
    // Unregulated PSU currently has no exported template, but still participates in typed
    // connections.
    std::vector<PortDef> ports = {
        {"vcc", "Vcc", PortType::PowerPos, {}},
        {"vee", "Vee", PortType::PowerNeg, {}},
        {"gnd", "0", PortType::Ground, {}},
    };
    const QStringList items = b.extra_pos_rails_csv.split(',', Qt::SkipEmptyParts);
    int i = 0;
    for (QString s : items) {
      s = s.trimmed();
      if (s.isEmpty()) {
        continue;
      }
      ports.push_back({QString("pos%1").arg(i++), s, PortType::PowerPos, {}});
    }
    return ports;
  }

  // Amplifier model 1(B) (template: assets/ltspice/opamp_model1b.asc).
  return {
      {"vcc", "Vcc", PortType::PowerPos, {FlagRef{1136, -512}, FlagRef{944, -208}}},
      {"vee", "Vee", PortType::PowerNeg, {FlagRef{1136, -32}, FlagRef{944, -400}}},
      {"gnd", "0", PortType::Ground, {}},
      {"in", "IN", PortType::AnalogIn, {FlagRef{784, -320}}},
      {"out", "OUT", PortType::AnalogOut, {FlagRef{1360, -304}}},
  };
}

std::optional<PortDef> find_port(const Block &b, const QString &port_id) {
  for (const auto &p : ports_for(b)) {
    if (p.id == port_id) {
      return p;
    }
  }
  return std::nullopt;
}

Block *find_block(std::vector<Block> &blocks, int id) {
  for (auto &b : blocks) {
    if (b.id == id) {
      return &b;
    }
  }
  return nullptr;
}

} // namespace pep::modules::project_design
