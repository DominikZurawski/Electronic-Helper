#include "src/ui/calculation/calculation_document.hpp"
#include "src/ui/calculation/calculation_html_renderer.hpp"

#include <cassert>
#include <string>

namespace calc = pep::ui::calculation;

namespace {

bool contains(const std::string &haystack, const std::string &needle) {
  return haystack.find(needle) != std::string::npos;
}

void test_renders_document_title_sections_and_entries() {
  calc::CalculationDocument document;
  document.title = "Kalkulator testowy";
  document.intro = "Intro";

  calc::CalculationSection section;
  section.title = "Transformator";
  section.description = "Opis sekcji";
  section.entries.push_back(calc::CalculationEntry{
      "Napiecie wtórne",
      "V_{s,rms}",
      "12.00",
      "V",
      "V_{s,rms} = \\frac{V_{p,rms}}{n}",
      "230 / 19.17 = 12.00",
      "Wyjasnienie"});
  document.sections.push_back(section);

  const std::string html = calc::render_calculation_document_html(document);

  assert(contains(html, "Kalkulator testowy"));
  assert(contains(html, "Transformator"));
  assert(contains(html, "Napiecie wtórne"));
  assert(contains(html, "V_{s,rms}"));
  assert(contains(html, "230 / 19.17 = 12.00"));
}

void test_escapes_html_in_user_visible_content() {
  calc::CalculationDocument document;
  calc::CalculationSection section;
  section.title = "<sekcja>";
  section.entries.push_back(calc::CalculationEntry{
      "A < B",
      "",
      "5",
      "V",
      "",
      "",
      "x && y"});
  document.sections.push_back(section);

  const std::string html = calc::render_calculation_document_html(document);

  assert(contains(html, "&lt;sekcja&gt;"));
  assert(contains(html, "A &lt; B"));
  assert(contains(html, "x &amp;&amp; y"));
}

void test_renders_notes_with_severity_classes() {
  calc::CalculationDocument document;
  calc::CalculationSection section;
  section.title = "Uwagi";
  section.notes.push_back(calc::CalculationNote{
      "Wskazowka", "To jest informacja.", calc::CalculationNoteSeverity::Info});
  section.notes.push_back(calc::CalculationNote{
      "Uwaga", "To jest ostrzezenie.", calc::CalculationNoteSeverity::Warning});
  document.sections.push_back(section);

  const std::string html = calc::render_calculation_document_html(document);

  assert(contains(html, "calc-note"));
  assert(contains(html, "calc-note-info"));
  assert(contains(html, "calc-note-warning"));
  assert(contains(html, "To jest ostrzezenie."));
}

void test_renders_steps_and_checklist() {
  calc::CalculationDocument document;
  calc::CalculationSection section;
  section.title = "Instrukcja";
  section.steps.push_back(calc::CalculationStep{"Krok 1", "Zrob to najpierw."});
  section.checklist_items.push_back(calc::CalculationChecklistItem{"Czy wynik ma sens?", true});
  document.sections.push_back(section);

  const std::string html = calc::render_calculation_document_html(document);

  assert(contains(html, "calc-steps"));
  assert(contains(html, "Krok 1"));
  assert(contains(html, "Zrob to najpierw."));
  assert(contains(html, "calc-checklist"));
  assert(contains(html, "Czy wynik ma sens?"));
}

void test_renders_validation_summary_for_incomplete_document() {
  calc::CalculationDocument document;
  calc::CalculationSection section;
  section.title = "Transformator";
  section.entries.push_back(calc::CalculationEntry{
      "Napiecie wtórne",
      "V_{s,rms}",
      "",
      "V",
      "",
      "",
      "Wyjasnienie"});
  document.sections.push_back(section);

  const std::string html = calc::render_calculation_document_html(document);

  assert(contains(html, "Uwagi do dokumentu obliczen"));
  assert(contains(html, "calc-validation"));
  assert(contains(html, "calc-validation-warning"));
  assert(contains(html, "nie ma wartosci wyniku"));
}

void test_render_result_exposes_validation_issues() {
  calc::CalculationDocument document;
  calc::CalculationSection section;
  section.title = "Transformator";
  section.entries.push_back(calc::CalculationEntry{
      "Napiecie wtórne",
      "V_{s,rms}",
      "",
      "V",
      "",
      "",
      "Wyjasnienie"});
  document.sections.push_back(section);

  const auto rendered = calc::render_calculation_document(document);

  assert(!rendered.validation_issues.empty());
  assert(rendered.validation_summary.warning_count >= 1);
  assert(contains(rendered.html, "Uwagi do dokumentu obliczen"));
}

void test_renderer_emits_light_and_dark_theme_classes() {
  calc::CalculationDocument document;
  calc::CalculationSection section;
  section.title = "Transformator";
  section.entries.push_back(calc::CalculationEntry{
      "Napiecie wtórne",
      "V_{s,rms}",
      "12.00",
      "V",
      "V_{s,rms} = \\frac{V_{p,rms}}{n}",
      "230 / 19.17 = 12.00",
      "Wyjasnienie"});
  document.sections.push_back(section);

  const auto light_html =
      calc::render_calculation_document_html(document, calc::CalculationTheme::Light);
  const auto dark_html =
      calc::render_calculation_document_html(document, calc::CalculationTheme::Dark);

  assert(contains(light_html, "body class='theme-light'"));
  assert(contains(dark_html, "body class='theme-dark'"));
}

void test_renderer_emits_section_tabs_and_marks_active_section() {
  calc::CalculationDocument document;
  document.title = "Kalkulator";
  document.sections.push_back(calc::CalculationSection{"Transformator"});
  document.sections.push_back(calc::CalculationSection{"Prostownik"});

  const auto html = calc::render_calculation_document_html(document, calc::CalculationTheme::Dark, 1);

  assert(contains(html, "calc-section-tabs"));
  assert(contains(html, "data-calc-tab='0'"));
  assert(contains(html, "data-calc-tab='1'"));
  assert(contains(html, "data-calc-section='1'><h2>Prostownik</h2>"));
  assert(contains(html, "calc-section-tab is-active"));
}

} // namespace

int main() {
  test_renders_document_title_sections_and_entries();
  test_escapes_html_in_user_visible_content();
  test_renders_notes_with_severity_classes();
  test_renders_steps_and_checklist();
  test_renders_validation_summary_for_incomplete_document();
  test_render_result_exposes_validation_issues();
  test_renderer_emits_light_and_dark_theme_classes();
  test_renderer_emits_section_tabs_and_marks_active_section();
  return 0;
}
