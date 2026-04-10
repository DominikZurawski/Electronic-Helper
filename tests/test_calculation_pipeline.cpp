#include "src/ui/calculation/calculation_builders.hpp"
#include "src/ui/calculation/calculation_html_renderer.hpp"

#include <cassert>
#include <string>

namespace calc = pep::ui::calculation;

namespace {

bool contains(const std::string &haystack, const std::string &needle) {
  return haystack.find(needle) != std::string::npos;
}

void test_complete_document_flows_from_builders_to_renderer_without_issues() {
  calc::CalculationDocument document;
  document.title = "Kalkulator zasilacza";
  document.intro = "Test pelnego przeplywu dokumentu.";

  auto section = calc::make_section("Transformator", "Sekcja zbudowana builderami.");
  section.entries.push_back(calc::make_entry(
      "Napiecie wtórne", "V_{s,rms}", "12.00", "V", "V_{s,rms} = V_{p,rms} / n",
      "230 / 19.17 = 12.00", "Wartosc skuteczna po stronie wtórnej."));
  section.notes.push_back(calc::make_info_note("Info", "Dokument jest kompletny."));
  section.steps.push_back(calc::make_step("Krok 1", "Zweryfikuj dane transformatora."));
  section.checklist_items.push_back(
      calc::make_checklist_item("Czy wynik ma sens fizyczny?", true));
  document.sections.push_back(section);

  const auto rendered = calc::render_calculation_document(document);

  assert(rendered.validation_issues.empty());
  assert(contains(rendered.html, "Kalkulator zasilacza"));
  assert(contains(rendered.html, "Transformator"));
  assert(contains(rendered.html, "Napiecie wtórne"));
  assert(contains(rendered.html, "V_{s,rms}"));
  assert(contains(rendered.html, "230 / 19.17 = 12.00"));
  assert(contains(rendered.html, "Czy wynik ma sens fizyczny?"));
  assert(!contains(rendered.html, "Uwagi do dokumentu obliczen"));
}

void test_incomplete_document_flows_to_validation_banner_and_issue_list() {
  calc::CalculationDocument document;
  document.title = "Kalkulator anten";

  auto section = calc::make_section("Promiennik");
  section.entries.push_back(
      calc::make_entry("Dlugosc fali", "\\lambda", "", "m", "", "", "Brak danych wejściowych."));
  document.sections.push_back(section);

  const auto rendered = calc::render_calculation_document(document);

  assert(!rendered.validation_issues.empty());
  assert(contains(rendered.html, "Uwagi do dokumentu obliczen"));
  assert(contains(rendered.html, "nie ma wartosci wyniku"));
  assert(contains(rendered.html, "nie ma wzoru ani podstawienia"));
}

} // namespace

int main() {
  test_complete_document_flows_from_builders_to_renderer_without_issues();
  test_incomplete_document_flows_to_validation_banner_and_issue_list();
  return 0;
}
