#pragma once

#include "calculation_document.hpp"
#include "calculation_validation.hpp"

#include <string>
#include <vector>

namespace pep::ui::calculation {

enum class CalculationTheme { Light, Dark };

struct CalculationRenderResult {
  std::string html;
  std::vector<CalculationValidationIssue> validation_issues;
  CalculationValidationSummary validation_summary;
};

CalculationRenderResult render_calculation_document(const CalculationDocument &document,
                                                    CalculationTheme theme = CalculationTheme::Dark,
                                                    int active_section_index = 0);
std::string render_calculation_document_html(const CalculationDocument &document,
                                             CalculationTheme theme = CalculationTheme::Dark,
                                             int active_section_index = 0);

} // namespace pep::ui::calculation
