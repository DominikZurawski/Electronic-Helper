#include "src/ui/calculation/calculation_builders.hpp"
#include "src/ui/calculation/calculation_validation.hpp"

#include <algorithm>
#include <cassert>
#include <string>

namespace calc = pep::ui::calculation;

namespace {

bool contains_message(const std::vector<calc::CalculationValidationIssue> &issues,
                      const std::string &needle) {
  return std::any_of(issues.begin(), issues.end(), [&](const auto &issue) {
    return issue.message.find(needle) != std::string::npos;
  });
}

void test_rejects_document_without_sections() {
  calc::CalculationDocument document;
  const auto issues = calc::validate_calculation_document(document);

  assert(!issues.empty());
  assert(issues.front().severity == calc::CalculationValidationSeverity::Error);
  assert(contains_message(issues, "nie zawiera zadnych sekcji"));
}

void test_warns_about_incomplete_entry() {
  calc::CalculationDocument document;
  auto section = calc::make_section("Transformator");
  section.entries.push_back(
      calc::make_entry("Napiecie wtórne", "V_s", "", "V", "", "", "Opis"));
  document.sections.push_back(section);

  const auto issues = calc::validate_calculation_document(document);

  assert(contains_message(issues, "nie ma wartosci wyniku"));
  assert(contains_message(issues, "nie ma wzoru ani podstawienia"));
}

void test_accepts_document_with_complete_section() {
  calc::CalculationDocument document;
  auto section = calc::make_section("Transformator", "Opis sekcji");
  section.entries.push_back(calc::make_entry("Napiecie wtórne", "V_s", "12.00", "V",
                                             "V_s = V_p / n", "230 / 19.17 = 12.00", "Opis"));
  section.notes.push_back(calc::make_info_note("Info", "To jest wskazowka."));
  section.steps.push_back(calc::make_step("Krok", "Opis kroku."));
  section.checklist_items.push_back(calc::make_checklist_item("Czy wynik ma sens?", true));
  document.sections.push_back(section);

  const auto issues = calc::validate_calculation_document(document);

  assert(issues.empty());
}

void test_formats_validation_summary() {
  std::vector<calc::CalculationValidationIssue> issues{
      {calc::CalculationValidationSeverity::Warning, "Brakuje wzoru."},
      {calc::CalculationValidationSeverity::Error, "Sekcja nie ma tytulu."}};

  const auto summary = calc::format_validation_summary(issues);

  assert(summary.find("Uwagi do dokumentu obliczen") != std::string::npos);
  assert(summary.find("(1 bledow, 1 ostrzezen)") != std::string::npos);
  assert(summary.find("- Brakuje wzoru.") != std::string::npos);
  assert(summary.find("- [blad] Sekcja nie ma tytulu.") != std::string::npos);
}

void test_summarizes_issue_counts() {
  std::vector<calc::CalculationValidationIssue> issues{
      {calc::CalculationValidationSeverity::Warning, "Brakuje wzoru."},
      {calc::CalculationValidationSeverity::Warning, "Brakuje jednostki."},
      {calc::CalculationValidationSeverity::Error, "Sekcja nie ma tytulu."}};

  const auto summary = calc::summarize_validation_issues(issues);

  assert(summary.warning_count == 2);
  assert(summary.error_count == 1);
}

void test_formats_compact_validation_summary() {
  const calc::CalculationValidationSummary summary{2, 1};
  const auto text = calc::format_validation_compact_summary(summary);

  assert(text == "1 bledow, 2 ostrzezen");
}

} // namespace

int main() {
  test_rejects_document_without_sections();
  test_warns_about_incomplete_entry();
  test_accepts_document_with_complete_section();
  test_formats_validation_summary();
  test_summarizes_issue_counts();
  test_formats_compact_validation_summary();
  return 0;
}
