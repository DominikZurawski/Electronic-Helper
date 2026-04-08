#pragma once

#include <string>

namespace pep::modules::psu_basic {

struct ExportInput {
  double vin_ac_rms = 0.0;
  double mains_hz = 50.0;
  double load_current = 0.0;
  double capacitor_uF = 0.0;
};

std::string export_netlist(const ExportInput &input);
std::string export_schematic(const ExportInput &input, bool full_wave,
                             const std::string &cir_filename, const std::string &symbol_transformer,
                             const std::string &symbol_bridge);

} // namespace pep::modules::psu_basic
