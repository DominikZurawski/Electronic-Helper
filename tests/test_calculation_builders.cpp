#include "src/ui/calculation/calculation_builders.hpp"

#include <cassert>

namespace calc = pep::ui::calculation;

namespace {

void test_make_entry_builds_complete_entry() {
  const auto entry =
      calc::make_entry("Napiecie", "V", "12.00", "V", "V = I \\cdot R", "1 \\cdot 12", "Opis");

  assert(entry.label == "Napiecie");
  assert(entry.symbol == "V");
  assert(entry.value == "12.00");
  assert(entry.unit == "V");
  assert(entry.formula_tex == "V = I \\cdot R");
  assert(entry.substitution_tex == "1 \\cdot 12");
  assert(entry.explanation == "Opis");
}

void test_make_notes_set_expected_severity() {
  const auto info = calc::make_info_note("Info", "Tresc");
  const auto warning = calc::make_warning_note("Warn", "Tresc");

  assert(info.severity == calc::CalculationNoteSeverity::Info);
  assert(warning.severity == calc::CalculationNoteSeverity::Warning);
}

void test_format_fixed_uses_requested_precision() {
  assert(calc::format_fixed(12.3456, 2) == "12.35");
  assert(calc::format_fixed(12.0, 3) == "12.000");
}

void test_make_step_and_checklist_item_fill_expected_fields() {
  const auto step = calc::make_step("Krok 1", "Opis kroku");
  const auto item = calc::make_checklist_item("Sprawdz napiecie", true);

  assert(step.title == "Krok 1");
  assert(step.body == "Opis kroku");
  assert(item.label == "Sprawdz napiecie");
  assert(item.checked);
}

} // namespace

int main() {
  test_make_entry_builds_complete_entry();
  test_make_notes_set_expected_severity();
  test_format_fixed_uses_requested_precision();
  test_make_step_and_checklist_item_fill_expected_fields();
  return 0;
}
