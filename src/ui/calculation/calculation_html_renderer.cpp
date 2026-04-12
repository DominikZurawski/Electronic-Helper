#include "calculation_html_renderer.hpp"
#include "calculation_validation.hpp"

#include <sstream>

namespace pep::ui::calculation {

namespace {

std::string escape_html(const std::string &text) {
  std::string out;
  out.reserve(text.size());
  for (const char ch : text) {
    switch (ch) {
    case '&':
      out += "&amp;";
      break;
    case '<':
      out += "&lt;";
      break;
    case '>':
      out += "&gt;";
      break;
    case '"':
      out += "&quot;";
      break;
    case '\'':
      out += "&#39;";
      break;
    default:
      out += ch;
      break;
    }
  }
  return out;
}

void append_math_block(std::ostringstream &html, const std::string &label, const std::string &tex) {
  if (tex.empty()) {
    return;
  }
  html << "<div class='calc-math-row'><span class='calc-math-label'>" << escape_html(label)
       << "</span><span class='calc-math'>\\(" << escape_html(tex) << "\\)</span></div>";
}

const char *note_severity_class(CalculationNoteSeverity severity) {
  switch (severity) {
  case CalculationNoteSeverity::Warning:
    return "calc-note-warning";
  default:
    return "calc-note-info";
  }
}

const char *validation_severity_class(CalculationValidationSeverity severity) {
  switch (severity) {
  case CalculationValidationSeverity::Error:
    return "calc-validation-error";
  default:
    return "calc-validation-warning";
  }
}

void append_step(std::ostringstream &html, const CalculationStep &step, int index) {
  html << "<div class='calc-step'><div class='calc-step-index'>" << index
       << "</div><div class='calc-step-content'>";
  if (!step.title.empty()) {
    html << "<div class='calc-step-title'>" << escape_html(step.title) << "</div>";
  }
  if (!step.body.empty()) {
    html << "<div class='calc-step-body'>" << escape_html(step.body) << "</div>";
  }
  html << "</div></div>";
}

void append_checklist_item(std::ostringstream &html, const CalculationChecklistItem &item) {
  html << "<div class='calc-checklist-item'><span class='calc-checklist-mark'>"
       << (item.checked ? "&#10003;" : "&#9633;") << "</span>"
       << "<span class='calc-checklist-label'>" << escape_html(item.label) << "</span></div>";
}

const char *theme_class(CalculationTheme theme) {
  return theme == CalculationTheme::Light ? "theme-light" : "theme-dark";
}

int clamp_active_section_index(const CalculationDocument &document, int active_section_index) {
  if (document.sections.empty()) {
    return 0;
  }
  if (active_section_index < 0) {
    return 0;
  }
  const int last_index = static_cast<int>(document.sections.size()) - 1;
  return active_section_index > last_index ? last_index : active_section_index;
}

} // namespace

CalculationRenderResult render_calculation_document(const CalculationDocument &document,
                                                    CalculationTheme theme,
                                                    int active_section_index) {
  const auto validation_issues = validate_calculation_document(document);
  const auto validation_summary = summarize_validation_issues(validation_issues);
  const int clamped_active_section_index =
      clamp_active_section_index(document, active_section_index);

  std::ostringstream html;
  html << "<!doctype html><html><head><meta charset='utf-8'>"
       << "<meta name='viewport' content='width=device-width, initial-scale=1'>"
       << "<link rel='stylesheet' href='qrc:/calculation/calculation.css'>"
       << "<link rel='stylesheet' href='qrc:/katex/katex.min.css'>"
       << "<script defer src='qrc:/katex/katex.min.js'></script>"
       << "<script defer src='qrc:/katex/auto-render.min.js'></script>"
       << "</head><body class='" << theme_class(theme) << "'><div class='calc-document'>";

  if (!document.title.empty()) {
    html << "<header class='calc-header'><h1>" << escape_html(document.title) << "</h1>";
    if (!document.intro.empty()) {
      html << "<p class='calc-intro'>" << escape_html(document.intro) << "</p>";
    }
    html << "</header>";
  }

  if (!validation_issues.empty()) {
    html << "<section class='calc-validation'><div class='calc-validation-title'>"
         << "Uwagi do dokumentu obliczen</div>";
    for (const auto &issue : validation_issues) {
      html << "<div class='calc-validation-item " << validation_severity_class(issue.severity)
           << "'>" << escape_html(issue.message) << "</div>";
    }
    html << "</section>";
  }

  if (document.sections.size() > 1) {
    html << "<nav class='calc-section-tabs' aria-label='Sekcje kalkulatora'>";
    for (std::size_t index = 0; index < document.sections.size(); ++index) {
      html << "<button type='button' class='calc-section-tab";
      if (static_cast<int>(index) == clamped_active_section_index) {
        html << " is-active";
      }
      html << "' data-calc-tab='" << index << "'>" << escape_html(document.sections[index].title)
           << "</button>";
    }
    html << "</nav>";
  }

  for (std::size_t index = 0; index < document.sections.size(); ++index) {
    const auto &section = document.sections[index];
    html << "<section class='calc-section";
    if (static_cast<int>(index) == clamped_active_section_index) {
      html << " is-active";
    }
    html << "' data-calc-section='" << index << "'><h2>" << escape_html(section.title)
         << "</h2>";
    if (!section.description.empty()) {
      html << "<p class='calc-section-description'>" << escape_html(section.description) << "</p>";
    }

    for (const auto &entry : section.entries) {
      html << "<article class='calc-entry'><div class='calc-entry-header'><div class='calc-entry-title'>"
           << escape_html(entry.label) << "</div>";
      if (!entry.symbol.empty()) {
        html << "<div class='calc-entry-symbol'>" << escape_html(entry.symbol) << "</div>";
      }
      html << "</div><div class='calc-entry-result'>";
      if (!entry.value.empty()) {
        html << "<span class='calc-entry-value'>" << escape_html(entry.value) << "</span>";
      }
      if (!entry.unit.empty()) {
        html << "<span class='calc-entry-unit'>" << escape_html(entry.unit) << "</span>";
      }
      if (!entry.value_secondary.empty()) {
        html << "<span class='calc-entry-range'>" << escape_html(entry.value_secondary) << "</span>";
      }
      html << "</div>";
      append_math_block(html, "Wzór", entry.formula_tex);
      append_math_block(html, "Podstawienie", entry.substitution_tex);
      if (!entry.explanation.empty()) {
        html << "<p class='calc-entry-explanation'>" << escape_html(entry.explanation) << "</p>";
      }
      html << "</article>";
    }

    for (const auto &note : section.notes) {
      html << "<aside class='calc-note " << note_severity_class(note.severity) << "'>";
      if (!note.title.empty()) {
        html << "<div class='calc-note-title'>" << escape_html(note.title) << "</div>";
      }
      if (!note.body.empty()) {
        html << "<div class='calc-note-body'>" << escape_html(note.body) << "</div>";
      }
      html << "</aside>";
    }

    if (!section.steps.empty()) {
      html << "<div class='calc-steps'><div class='calc-block-title'>Kroki</div>";
      int index = 1;
      for (const auto &step : section.steps) {
        append_step(html, step, index++);
      }
      html << "</div>";
    }

    if (!section.checklist_items.empty()) {
      html << "<div class='calc-checklist'><div class='calc-block-title'>Checklista</div>";
      for (const auto &item : section.checklist_items) {
        append_checklist_item(html, item);
      }
      html << "</div>";
    }

    html << "</section>";
  }

  html << "</div><script>"
       << "document.addEventListener('DOMContentLoaded', function() {"
       << "if (window.renderMathInElement) {"
       << "renderMathInElement(document.body, {delimiters:[{left:'\\\\(',right:'\\\\)',display:false},{left:'\\\\[',right:'\\\\]',display:true}]});"
       << "}"
       << "const tabButtons = document.querySelectorAll('[data-calc-tab]');"
       << "const sections = document.querySelectorAll('[data-calc-section]');"
       << "const activateSection = function(sectionIndex) {"
       << "tabButtons.forEach(function(button) {"
       << "button.classList.toggle('is-active', button.getAttribute('data-calc-tab') === sectionIndex);"
       << "});"
       << "sections.forEach(function(section) {"
       << "section.classList.toggle('is-active', section.getAttribute('data-calc-section') === sectionIndex);"
       << "});"
       << "};"
       << "tabButtons.forEach(function(button) {"
       << "button.addEventListener('click', function() {"
       << "activateSection(button.getAttribute('data-calc-tab'));"
       << "});"
       << "});"
       << "});"
       << "</script></body></html>";
  return CalculationRenderResult{html.str(), validation_issues, validation_summary};
}

std::string render_calculation_document_html(const CalculationDocument &document,
                                             CalculationTheme theme,
                                             int active_section_index) {
  return render_calculation_document(document, theme, active_section_index).html;
}

} // namespace pep::ui::calculation
