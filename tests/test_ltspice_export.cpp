#include "pep/ltspice_export.hpp"
#include "src/ltspice/ltspice_export_render.hpp"

#include <cassert>
#include <string>

namespace {

bool contains(const std::string &text, const std::string &needle) {
  return text.find(needle) != std::string::npos;
}

void test_netlist_uses_default_title_and_default_rload() {
  pep::LtspiceExporter exporter;
  pep::LtspiceExportRequest req;
  req.vin_ac_rms = 12.0;
  req.mains_hz = 50.0;
  req.capacitor_uF = 2200.0;
  req.load_current = 0.0;

  const auto netlist = exporter.export_psu_basic_netlist(req);

  assert(contains(netlist, "* PPE PSU Basic"));
  assert(contains(netlist, "V1 in 0 SINE(0 12 50)"));
  assert(contains(netlist, "C1 out 0 2200u"));
  assert(contains(netlist, "Rload out 0 1000"));
  assert(contains(netlist, ".tran 0 200m 0 1m"));
}

void test_netlist_uses_computed_rload_when_current_is_present() {
  pep::LtspiceExporter exporter;
  pep::LtspiceExportRequest req;
  req.title = "Custom PSU";
  req.vin_ac_rms = 15.0;
  req.mains_hz = 60.0;
  req.capacitor_uF = 4700.0;
  req.load_current = 0.5;

  const auto netlist = exporter.export_psu_basic_netlist(req);

  assert(contains(netlist, "* Custom PSU"));
  assert(contains(netlist, "V1 in 0 SINE(0 15 60)"));
  assert(contains(netlist, "Rload out 0 30"));
}

void test_schematic_uses_bridge_symbol_when_provided() {
  pep::LtspiceExporter exporter;
  pep::LtspiceExportRequest req;
  req.vin_ac_rms = 12.0;
  req.load_current = 0.2;
  req.capacitor_uF = 3300.0;
  req.full_wave = true;

  const auto schematic =
      exporter.export_psu_basic_schematic(req, "power.cir", "transformer.asy", "bridge.asy");

  assert(contains(schematic, "SYMBOL transformer.asy 64 144 R0"));
  assert(contains(schematic, "SYMBOL bridge.asy 256 160 R0"));
  assert(!contains(schematic, "SYMBOL diode 240 160 R0"));
}

void test_schematic_uses_single_diode_for_half_wave() {
  pep::LtspiceExporter exporter;
  pep::LtspiceExportRequest req;
  req.vin_ac_rms = 9.0;
  req.load_current = 0.1;
  req.capacitor_uF = 1000.0;
  req.full_wave = false;

  const auto schematic = exporter.export_psu_basic_schematic(req, "", "", "");

  assert(contains(schematic, "SYMBOL voltage 64 160 R0"));
  assert(contains(schematic, "SYMBOL diode 256 160 R0"));
  assert(!contains(schematic, "SYMATTR InstName BR1"));
}

void test_render_helpers_use_fallback_title_and_default_rload() {
  pep::LtspiceExportRequest req;
  req.vin_ac_rms = 12.0;

  assert(pep::default_export_title("", "Fallback PSU") == "Fallback PSU");
  assert(pep::exported_rload(req) == 1000.0);
}

void test_render_helpers_use_explicit_title_and_computed_rload() {
  pep::LtspiceExportRequest req;
  req.title = "Named PSU";
  req.vin_ac_rms = 18.0;
  req.load_current = 0.6;

  assert(pep::default_export_title(req.title, "Fallback PSU") == "Named PSU");
  assert(pep::exported_rload(req) == 30.0);
}

} // namespace

int main() {
  test_netlist_uses_default_title_and_default_rload();
  test_netlist_uses_computed_rload_when_current_is_present();
  test_schematic_uses_bridge_symbol_when_provided();
  test_schematic_uses_single_diode_for_half_wave();
  test_render_helpers_use_fallback_title_and_default_rload();
  test_render_helpers_use_explicit_title_and_computed_rload();
  return 0;
}
