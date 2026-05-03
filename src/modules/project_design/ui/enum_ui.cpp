#include "enum_ui.hpp"

#include <QString>

namespace pep::modules::project_design {

QString port_type_label(PortType type) {
  switch (type) {
  case PortType::PowerPos:
    return "Power +";
  case PortType::PowerNeg:
    return "Power -";
  case PortType::PowerInPos:
    return "Power IN";
  case PortType::PowerOutPos:
    return "Power OUT";
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

} // namespace pep::modules::project_design
