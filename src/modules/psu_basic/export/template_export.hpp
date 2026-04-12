#pragma once

#include <string>
#include <vector>

namespace pep::modules::psu_basic {

struct TemplateExportResult {
  std::string asc;
  std::vector<std::string> warnings;
  std::vector<std::string> directives;
};

TemplateExportResult export_schematic_from_asc_template(double vin_ac_rms, double mains_hz,
                                                        double load_current, double capacitor_uF,
                                                        double vin_tol_pct,
                                                        const std::string &step_param_name,
                                                        const std::string &asc_template);

// Exact built-in template (same content as the bundled resource), used only as a fallback
// if Qt resources fail to load at runtime.
std::string built_in_psu_basic_asc_template();

} // namespace pep::modules::psu_basic
