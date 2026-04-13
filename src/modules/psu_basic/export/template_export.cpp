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
  // W tej chwili nie shipujemy już wbudowanego szablonu dla "PSU niesymetryczny".
  // Szablony są rozdzielone na elementy (np. zasilanie symetryczne i wzmacniacz) w module
  // "Projektowanie układu".
  return "";
}

TemplateExportResult export_schematic_from_asc_template(double vin_ac_rms, double mains_hz,
                                                        double load_current, double capacitor_uF,
                                                        double vin_tol_pct,
                                                        const std::string &step_param_name,
                                                        double cap_tol_pct,
                                                        const std::string &cap_step_param_name,
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
  const bool use_cap_step = (cap_tol_pct > 0.0 && !cap_step_param_name.empty());
  const std::string cap_value_base =
      (cap_uF > 0.0 ? (format_number(cap_uF, 6) + cap_suffix) : "");
  const std::string cap_value =
      (cap_uF > 0.0
           ? (use_cap_step
                  ? ("{" + cap_value_base + "*" + cap_step_param_name + "}")
                  : cap_value_base)
           : "");

  std::unordered_map<std::string, std::string> inst_values;
  const bool has_rload = asc_template.find("InstName Rload") != std::string::npos;
  const bool has_rl = asc_template.find("InstName RL") != std::string::npos;

  const bool use_step = (vin_tol_pct > 0.0 && !step_param_name.empty());
  if (vrms > 0.0) {
    if (use_step) {
      inst_values.emplace(
          "V1", "SINE(0 {" + format_number(vpk, 6) + "*" + step_param_name + "} " +
                     format_number(hz, 3) + ")");
    } else {
      inst_values.emplace("V1",
                          "SINE(0 " + format_number(vpk, 6) + " " + format_number(hz, 3) + ")");
    }
  } else {
    out.warnings.push_back("Brak Uwe (VAC) — nie nadpisuję V1.");
  }

  const bool has_c1 = asc_template.find("InstName C1") != std::string::npos;
  const bool has_c2 = asc_template.find("InstName C2") != std::string::npos;
  if (!cap_value.empty()) {
    if (has_c1) {
      inst_values.emplace("C1", cap_value);
    }
    if (has_c2) {
      inst_values.emplace("C2", cap_value); // jeśli istnieje w szablonie
    }
  } else if (has_c1 || has_c2) {
    out.warnings.push_back("Brak C (uF) — nie nadpisuję C1/C2.");
  }

  if (load_current > 0.0 && vrms > 0.0) {
    const double rload = vrms / load_current;
    if (has_rload) {
      inst_values.emplace("Rload", format_number(rload, 6));
    }
    if (has_rl) {
      inst_values.emplace("RL", format_number(rload, 6));
    }
  } else if (load_current <= 0.0 && (has_rload || has_rl)) {
    out.warnings.push_back("Brak I obciążenia (A) — nie nadpisuję Rload/RL.");
  }

  auto patched = pep::ltspice_patch_asc_values(asc_template, inst_values);
  out.asc = std::move(patched.asc);
  if (use_step) {
    const double k_down = 1.0 - vin_tol_pct / 100.0;
    const double k_up = 1.0 + vin_tol_pct / 100.0;
    out.directives.push_back(".param " + step_param_name + "=1");
    out.directives.push_back(".step param " + step_param_name + " list " +
                             format_number(k_down, 4) + " 1 " + format_number(k_up, 4));
  }
  if (use_cap_step) {
    const double k_down = 1.0 - cap_tol_pct / 100.0;
    const double k_up = 1.0 + cap_tol_pct / 100.0;
    out.directives.push_back(".param " + cap_step_param_name + "=1");
    out.directives.push_back(".step param " + cap_step_param_name + " list " +
                             format_number(k_down, 4) + " 1 " + format_number(k_up, 4));
  }
  out.warnings.insert(out.warnings.end(), patched.warnings.begin(), patched.warnings.end());
  return out;
}

} // namespace pep::modules::psu_basic
