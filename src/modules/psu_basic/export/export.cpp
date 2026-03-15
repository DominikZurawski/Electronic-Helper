#include "export.hpp"

#include "pep/ltspice_export.hpp"

namespace pep::modules::psu_basic {

std::string export_netlist(const ExportInput& input) {
  pep::LtspiceExporter exporter;
  pep::LtspiceExportRequest req;
  req.title = "PPE PSU Basic";
  req.vin_ac_rms = input.vin_ac_rms;
  req.mains_hz = input.mains_hz;
  req.load_current = input.load_current;
  req.capacitor_uF = input.capacitor_uF;
  return exporter.export_psu_basic_netlist(req);
}

std::string export_schematic(const ExportInput& input,
                             bool full_wave,
                             const std::string& cir_filename,
                             const std::string& symbol_transformer,
                             const std::string& symbol_bridge) {
  pep::LtspiceExporter exporter;
  pep::LtspiceExportRequest req;
  req.title = "PPE PSU Basic";
  req.vin_ac_rms = input.vin_ac_rms;
  req.mains_hz = input.mains_hz;
  req.load_current = input.load_current;
  req.capacitor_uF = input.capacitor_uF;
  req.full_wave = full_wave;
  return exporter.export_psu_basic_schematic(req, cir_filename, symbol_transformer, symbol_bridge);
}

} // namespace pep::modules::psu_basic
