#include "calculation_validation.hpp"

#include <sstream>
#include <utility>

namespace pep::ui::calculation {

namespace {

void append_issue(std::vector<CalculationValidationIssue> &issues,
                  CalculationValidationSeverity severity, std::string message) {
  issues.push_back(CalculationValidationIssue{severity, std::move(message)});
}

} // namespace

std::vector<CalculationValidationIssue>
validate_calculation_document(const CalculationDocument &document) {
  std::vector<CalculationValidationIssue> issues;

  if (document.sections.empty()) {
    append_issue(issues, CalculationValidationSeverity::Error,
                 "Dokument obliczen nie zawiera zadnych sekcji.");
    return issues;
  }

  for (std::size_t section_index = 0; section_index < document.sections.size(); ++section_index) {
    const auto &section = document.sections[section_index];
    const auto section_number = std::to_string(section_index + 1);

    if (section.title.empty()) {
      append_issue(issues, CalculationValidationSeverity::Error,
                   "Sekcja " + section_number + " nie ma tytulu.");
    }

    const bool has_any_content = !section.entries.empty() || !section.notes.empty()
                                 || !section.steps.empty() || !section.checklist_items.empty();
    if (!has_any_content) {
      append_issue(issues, CalculationValidationSeverity::Warning,
                   "Sekcja \"" + section.title + "\" nie zawiera zadnej tresci.");
    }

    for (std::size_t entry_index = 0; entry_index < section.entries.size(); ++entry_index) {
      const auto &entry = section.entries[entry_index];
      const auto entry_number = std::to_string(entry_index + 1);

      if (entry.label.empty()) {
        append_issue(issues, CalculationValidationSeverity::Error,
                     "Sekcja \"" + section.title + "\", wpis " + entry_number
                         + " nie ma etykiety.");
      }

      if (entry.value.empty()) {
        append_issue(issues, CalculationValidationSeverity::Warning,
                     "Sekcja \"" + section.title + "\", wpis \"" + entry.label
                         + "\" nie ma wartosci wyniku.");
      }

      if (entry.formula_tex.empty() && entry.substitution_tex.empty()) {
        append_issue(issues, CalculationValidationSeverity::Warning,
                     "Sekcja \"" + section.title + "\", wpis \"" + entry.label
                         + "\" nie ma wzoru ani podstawienia.");
      }
    }
  }

  return issues;
}

CalculationValidationSummary summarize_validation_issues(
    const std::vector<CalculationValidationIssue> &issues) {
  CalculationValidationSummary summary;
  for (const auto &issue : issues) {
    if (issue.severity == CalculationValidationSeverity::Error) {
      ++summary.error_count;
    } else {
      ++summary.warning_count;
    }
  }
  return summary;
}

std::string format_validation_compact_summary(const CalculationValidationSummary &summary) {
  std::ostringstream out;
  out << summary.error_count << " bledow, " << summary.warning_count << " ostrzezen";
  return out.str();
}

std::string format_validation_summary(const std::vector<CalculationValidationIssue> &issues,
                                      const std::string &title) {
  if (issues.empty()) {
    return {};
  }

  const auto summary = summarize_validation_issues(issues);
  std::ostringstream out;
  if (!title.empty()) {
    out << title;
    if (summary.error_count > 0 || summary.warning_count > 0) {
      out << " (" << format_validation_compact_summary(summary) << ")";
    }
  }

  for (const auto &issue : issues) {
    if (out.tellp() > 0) {
      out << '\n';
    }

    out << "- ";
    if (issue.severity == CalculationValidationSeverity::Error) {
      out << "[blad] ";
    }
    out << issue.message;
  }

  return out.str();
}

} // namespace pep::ui::calculation
