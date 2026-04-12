#include "calculation_builders.hpp"

#include <iomanip>
#include <sstream>
#include <utility>

namespace pep::ui::calculation {

CalculationEntry make_entry(std::string label, std::string symbol, std::string value,
                            std::string unit, std::string formula_tex,
                            std::string substitution_tex, std::string explanation,
                            std::string value_secondary) {
  return CalculationEntry{std::move(label),       std::move(symbol),      std::move(value),
                          std::move(value_secondary), std::move(unit),
                          std::move(formula_tex), std::move(substitution_tex),
                          std::move(explanation)};
}

CalculationNote make_info_note(std::string title, std::string body) {
  return CalculationNote{std::move(title), std::move(body), CalculationNoteSeverity::Info};
}

CalculationNote make_warning_note(std::string title, std::string body) {
  return CalculationNote{std::move(title), std::move(body), CalculationNoteSeverity::Warning};
}

CalculationStep make_step(std::string title, std::string body) {
  return CalculationStep{std::move(title), std::move(body)};
}

CalculationChecklistItem make_checklist_item(std::string label, bool checked) {
  return CalculationChecklistItem{std::move(label), checked};
}

CalculationSection make_section(std::string title, std::string description) {
  return CalculationSection{std::move(title), std::move(description), {}, {}, {}, {}};
}

std::string format_fixed(double value, int decimals) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(decimals) << value;
  return out.str();
}

} // namespace pep::ui::calculation
