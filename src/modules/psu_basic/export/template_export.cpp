#include "template_export.hpp"

#include "pep/ltspice_template.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace pep::modules::psu_basic {

namespace {

std::string format_number(double v, int precision = 10) {
  std::ostringstream out;
  out.setf(std::ios::fixed);
  out << std::setprecision(precision) << v;
  std::string s = out.str();
  while (s.size() > 1 && s.find('.') != std::string::npos && s.back() == '0') {
    s.pop_back();
  }
  if (!s.empty() && s.back() == '.') {
    s.pop_back();
  }
  return s;
}

} // namespace

std::string built_in_psu_basic_asc_template() {
  // W tej chwili nie shipujemy już wbudowanego szablonu dla "PSU niestabilizowany".
  // Szablony są rozdzielone na elementy (np. zasilanie symetryczne i wzmacniacz) w module
  // "Projektowanie układu".
  return "";
}

TemplateExportResult export_schematic_from_asc_template(double vin_ac_rms, double mains_hz,
                                                        double load_current, double capacitor_uF,
                                                        const std::string &asc_template) {
  TemplateExportResult out;
  if (asc_template.empty()) {
    out.warnings.push_back("Pusty szablon .asc.");
    return out;
  }

  const double hz = (mains_hz > 0.0 ? mains_hz : 50.0);
  const double vrms = (vin_ac_rms > 0.0 ? vin_ac_rms : 0.0);
  const double vpk = vrms * std::sqrt(2.0);

  const double cap_uF = (capacitor_uF > 0.0 ? capacitor_uF : 0.0);
  std::string cap_suffix = pep::ltspice_detect_value_suffix(asc_template, "C1");
  // LTspice typically accepts "u". We force ASCII "u" to avoid locale/encoding artifacts (µ/�).
  cap_suffix = "u";
  const std::string cap_value = (cap_uF > 0.0 ? (format_number(cap_uF, 6) + cap_suffix) : "");

  std::unordered_map<std::string, std::string> inst_values;

  if (vrms > 0.0) {
    inst_values.emplace("V1", "SINE(0 " + format_number(vpk, 6) + " " + format_number(hz, 3) + ")");
  } else {
    out.warnings.push_back("Brak Uwe (VAC) — nie nadpisuję V1.");
  }

  if (!cap_value.empty()) {
    inst_values.emplace("C1", cap_value);
    inst_values.emplace("C2", cap_value); // jeśli istnieje w szablonie
  } else {
    out.warnings.push_back("Brak C (uF) — nie nadpisuję C1/C2.");
  }

  if (load_current > 0.0 && vrms > 0.0) {
    const double rload = vrms / load_current;
    inst_values.emplace("Rload", format_number(rload, 6));
    inst_values.emplace("RL", format_number(rload, 6));
  } else if (load_current <= 0.0) {
    out.warnings.push_back("Brak I obciążenia (A) — nie nadpisuję Rload/RL.");
  }

  auto patched = pep::ltspice_patch_asc_values(asc_template, inst_values);
  out.asc = std::move(patched.asc);
  out.warnings.insert(out.warnings.end(), patched.warnings.begin(), patched.warnings.end());
  return out;
}

} // namespace pep::modules::psu_basic
