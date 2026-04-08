#pragma once

#include <string>

namespace pep {

struct LtspiceExportRequest {
  std::string title;
  double vin_ac_rms = 0.0;
  double mains_hz = 50.0;
  double load_current = 0.0;
  double capacitor_uF = 0.0;
  bool full_wave = true;
};

class LtspiceExporter {
public:
  std::string export_psu_basic_netlist(const LtspiceExportRequest &req) const;
  std::string export_psu_basic_schematic(const LtspiceExportRequest &req,
                                         const std::string &cir_filename,
                                         const std::string &symbol_transformer,
                                         const std::string &symbol_bridge) const;
};

} // namespace pep
