#include "ltspice_export_render.hpp"

namespace pep {

std::string default_export_title(std::string_view requested_title,
                                 std::string_view fallback_title) {
  if (!requested_title.empty()) {
    return std::string(requested_title);
  }
  return std::string(fallback_title);
}

double exported_rload(const LtspiceExportRequest &req) {
  if (req.load_current > 0.0) {
    return req.vin_ac_rms / req.load_current;
  }
  return 1000.0;
}

void append_psu_basic_netlist(std::ostream &out, const LtspiceExportRequest &req) {
  out << "* " << default_export_title(req.title, "PPE PSU Basic") << "\n";
  out << "V1 in 0 SINE(0 " << req.vin_ac_rms << " " << req.mains_hz << ")\n";
  out << "D1 in out D\n";
  out << "D2 0 out D\n";
  out << "C1 out 0 " << req.capacitor_uF << "u\n";
  out << "Rload out 0 " << exported_rload(req) << "\n";
  out << ".model D D\n";
  out << ".tran 0 200m 0 1m\n";
  out << ".end\n";
}

void append_psu_basic_schematic_header(std::ostream &out) {
  out << "Version 4\n";
  out << "SHEET 1 880 680\n";
  out << "TEXT 64 56 Left 2 !.tran 0 200m 0 1m\n";
  out << "TEXT 64 80 Left 2 !.backanno\n";
}

void append_psu_basic_source(std::ostream &out, const LtspiceExportRequest &req,
                             const std::string &symbol_transformer) {
  if (!symbol_transformer.empty()) {
    out << "SYMBOL " << symbol_transformer << " 64 144 R0\n";
    out << "SYMATTR InstName T1\n";
    out << "TEXT 40 112 Left 2 Transformator\n";
    return;
  }

  out << "SYMBOL voltage 64 160 R0\n";
  out << "SYMATTR InstName V1\n";
  out << "SYMATTR Value SINE(0 " << req.vin_ac_rms << " " << req.mains_hz << ")\n";
  out << "FLAG 64 256 0\n";
  out << "TEXT 24 120 Left 2 Transformator\n";
}

void append_psu_basic_rectifier(std::ostream &out, const LtspiceExportRequest &req,
                                const std::string &symbol_bridge) {
  if (req.full_wave && !symbol_bridge.empty()) {
    out << "SYMBOL " << symbol_bridge << " 256 160 R0\n";
    out << "SYMATTR InstName BR1\n";
    return;
  }

  if (req.full_wave) {
    out << "SYMBOL diode 240 160 R0\n";
    out << "SYMATTR InstName D1\n";
    out << "SYMBOL diode 240 192 R180\n";
    out << "SYMATTR InstName D2\n";
    out << "SYMBOL diode 320 160 R0\n";
    out << "SYMATTR InstName D3\n";
    out << "SYMBOL diode 320 192 R180\n";
    out << "SYMATTR InstName D4\n";
    out << "TEXT 248 120 Left 2 Mostek\n";
    out << "WIRE 96 176 240 176\n";
    out << "WIRE 96 208 240 208\n";
    out << "WIRE 240 176 240 208\n";
    out << "WIRE 320 176 320 208\n";
    out << "WIRE 352 176 416 176\n";
    out << "WIRE 352 208 416 208\n";
    return;
  }

  out << "SYMBOL diode 256 160 R0\n";
  out << "SYMATTR InstName D1\n";
  out << "TEXT 240 120 Left 2 Prostownik\n";
  out << "WIRE 96 176 256 176\n";
  out << "WIRE 336 176 416 176\n";
}

void append_psu_basic_filter_and_load(std::ostream &out, const LtspiceExportRequest &req) {
  const double cap_u = (req.capacitor_uF > 0.0 ? req.capacitor_uF : 1000.0);
  out << "SYMBOL cap 416 160 R0\n";
  out << "SYMATTR InstName C1\n";
  out << "SYMATTR Value " << cap_u << "u\n";

  out << "SYMBOL res 576 160 R0\n";
  out << "SYMATTR InstName Rload\n";
  out << "SYMATTR Value " << exported_rload(req) << "\n";
  out << "WIRE 496 176 576 176\n";
  out << "WIRE 656 176 720 176\n";
  out << "FLAG 416 256 0\n";
  out << "FLAG 576 256 0\n";
}

} // namespace pep
