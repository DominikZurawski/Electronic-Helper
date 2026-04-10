#pragma once

#include "calculation_document.hpp"

#include <string>
#include <vector>

namespace pep::ui::calculation {

enum class CalculationValidationSeverity { Warning, Error };

struct CalculationValidationIssue {
  CalculationValidationSeverity severity = CalculationValidationSeverity::Warning;
  std::string message;
};

struct CalculationValidationSummary {
  int warning_count = 0;
  int error_count = 0;
};

std::vector<CalculationValidationIssue>
validate_calculation_document(const CalculationDocument &document);

CalculationValidationSummary summarize_validation_issues(
    const std::vector<CalculationValidationIssue> &issues);

std::string format_validation_compact_summary(const CalculationValidationSummary &summary);

std::string format_validation_summary(
    const std::vector<CalculationValidationIssue> &issues,
    const std::string &title = "Uwagi do dokumentu obliczen");

} // namespace pep::ui::calculation
