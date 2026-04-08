#include "pep/ltspice_export.hpp"

#include <sstream>

namespace pep {

std::string LtspiceExporter::export_psu_basic_netlist(const LtspiceExportRequest &req) const {
  std::ostringstream out;
  out << "* " << (req.title.empty() ? "PPE PSU Basic" : req.title) << "\n";
  out << "V1 in 0 SINE(0 " << req.vin_ac_rms << " " << req.mains_hz << ")\n";
  out << "D1 in out D\n";
  out << "D2 0 out D\n";
  out << "C1 out 0 " << req.capacitor_uF << "u\n";

  if (req.load_current > 0.0) {
    const double rload = req.vin_ac_rms / req.load_current;
    out << "Rload out 0 " << rload << "\n";
  } else {
    out << "Rload out 0 1000\n";
  }

  out << ".model D D\n";
  out << ".tran 0 200m 0 1m\n";
  out << ".end\n";
  return out.str();
}

std::string LtspiceExporter::export_psu_basic_schematic(const LtspiceExportRequest &req,
                                                        const std::string &cir_filename,
                                                        const std::string &symbol_transformer,
                                                        const std::string &symbol_bridge) const {
  std::ostringstream out;
  out << "Version 4\n";
  out << "SHEET 1 880 680\n";
  out << "TEXT 64 56 Left 2 !.tran 0 200m 0 1m\n";
  out << "TEXT 64 80 Left 2 !.backanno\n";

  // Visual schematic using standard LTspice symbols with optional custom symbols.
  if (!symbol_transformer.empty()) {
    out << "SYMBOL " << symbol_transformer << " 64 144 R0\n";
    out << "SYMATTR InstName T1\n";
    out << "TEXT 40 112 Left 2 Transformator\n";
  } else {
    out << "SYMBOL voltage 64 160 R0\n";
    out << "SYMATTR InstName V1\n";
    out << "SYMATTR Value SINE(0 " << req.vin_ac_rms << " " << req.mains_hz << ")\n";
    out << "FLAG 64 256 0\n";
    out << "TEXT 24 120 Left 2 Transformator\n";
  }

  if (req.full_wave && !symbol_bridge.empty()) {
    out << "SYMBOL " << symbol_bridge << " 256 160 R0\n";
    out << "SYMATTR InstName BR1\n";
  } else if (req.full_wave) {
    // Full-wave bridge (4 diodes), connected
    out << "SYMBOL diode 240 160 R0\n";
    out << "SYMATTR InstName D1\n";
    out << "SYMBOL diode 240 192 R180\n";
    out << "SYMATTR InstName D2\n";
    out << "SYMBOL diode 320 160 R0\n";
    out << "SYMATTR InstName D3\n";
    out << "SYMBOL diode 320 192 R180\n";
    out << "SYMATTR InstName D4\n";
    out << "TEXT 248 120 Left 2 Mostek\n";

    // AC rails from source
    out << "WIRE 96 176 240 176\n"; // AC1 to D1/D2
    out << "WIRE 96 208 240 208\n"; // AC2 to D2/D1

    // Bridge internal connections
    out << "WIRE 240 176 240 208\n"; // left vertical
    out << "WIRE 320 176 320 208\n"; // right vertical
    out << "WIRE 352 176 416 176\n"; // + output
    out << "WIRE 352 208 416 208\n"; // - output
  } else {
    out << "SYMBOL diode 256 160 R0\n";
    out << "SYMATTR InstName D1\n";
    out << "TEXT 240 120 Left 2 Prostownik\n";
    out << "WIRE 96 176 256 176\n";
    out << "WIRE 336 176 416 176\n";
  }

  const double cap_u = (req.capacitor_uF > 0.0 ? req.capacitor_uF : 1000.0);
  out << "SYMBOL cap 416 160 R0\n";
  out << "SYMATTR InstName C1\n";
  out << "SYMATTR Value " << cap_u << "u\n";

  out << "SYMBOL res 576 160 R0\n";
  out << "SYMATTR InstName Rload\n";
  out << "SYMATTR Value " << (req.load_current > 0.0 ? (req.vin_ac_rms / req.load_current) : 1000.0)
      << "\n";

  out << "WIRE 496 176 576 176\n";
  out << "WIRE 656 176 720 176\n";

  out << "FLAG 416 256 0\n";
  out << "FLAG 576 256 0\n";

  return out.str();
}

} // namespace pep
