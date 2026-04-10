#pragma once

#include "calculation_document.hpp"

#include <string>

namespace pep::ui::calculation {

CalculationEntry make_entry(std::string label, std::string symbol, std::string value,
                            std::string unit, std::string formula_tex = {},
                            std::string substitution_tex = {}, std::string explanation = {});

CalculationNote make_info_note(std::string title, std::string body);
CalculationNote make_warning_note(std::string title, std::string body);
CalculationStep make_step(std::string title, std::string body);
CalculationChecklistItem make_checklist_item(std::string label, bool checked = false);

CalculationSection make_section(std::string title, std::string description = {});

std::string format_fixed(double value, int decimals);

} // namespace pep::ui::calculation
