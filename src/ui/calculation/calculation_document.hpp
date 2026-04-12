#pragma once

#include <string>
#include <vector>

namespace pep::ui::calculation {

enum class CalculationNoteSeverity { Info, Warning };

struct CalculationEntry {
  std::string label;
  std::string symbol;
  std::string value;
  std::string value_secondary;
  std::string unit;
  std::string formula_tex;
  std::string substitution_tex;
  std::string explanation;
};

struct CalculationNote {
  std::string title;
  std::string body;
  CalculationNoteSeverity severity = CalculationNoteSeverity::Info;
};

struct CalculationStep {
  std::string title;
  std::string body;
};

struct CalculationChecklistItem {
  std::string label;
  bool checked = false;
};

struct CalculationSection {
  std::string title;
  std::string description;
  std::vector<CalculationEntry> entries;
  std::vector<CalculationNote> notes;
  std::vector<CalculationStep> steps;
  std::vector<CalculationChecklistItem> checklist_items;
};

struct CalculationDocument {
  std::string title;
  std::string intro;
  std::vector<CalculationSection> sections;
};

} // namespace pep::ui::calculation
