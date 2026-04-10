#include "pep/modules/project_design/ui/widget.hpp"
#include "src/ui/calculation/calculation_view.hpp"

#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

#include <cassert>
#include <cstdlib>
#include <initializer_list>
#include <string>
#include <vector>

namespace {

QPushButton *find_button(QWidget &widget, const QString &text) {
  const auto buttons = widget.findChildren<QPushButton *>();
  for (auto *button : buttons) {
    if (button && button->text() == text) {
      return button;
    }
  }
  return nullptr;
}

QComboBox *find_combo_with_items(QWidget &widget, std::initializer_list<QString> items) {
  const auto combos = widget.findChildren<QComboBox *>();
  for (auto *combo : combos) {
    if (!combo || combo->count() != static_cast<int>(items.size())) {
      continue;
    }

    bool matches = true;
    int index = 0;
    for (const auto &item : items) {
      if (combo->itemText(index++) != item) {
        matches = false;
        break;
      }
    }

    if (matches) {
      return combo;
    }
  }
  return nullptr;
}

QLabel *find_label_containing(QWidget &widget, const QString &text) {
  const auto labels = widget.findChildren<QLabel *>();
  for (auto *label : labels) {
    if (label && label->text().contains(text)) {
      return label;
    }
  }
  return nullptr;
}

QLabel *find_label_by_object_name(QWidget &widget, const QString &object_name) {
  const auto labels = widget.findChildren<QLabel *>();
  for (auto *label : labels) {
    if (label && label->objectName() == object_name) {
      return label;
    }
  }
  return nullptr;
}

pep::ui::calculation::CalculationView *find_calculation_view(QWidget &widget) {
  const auto children = widget.findChildren<QWidget *>();
  pep::ui::calculation::CalculationView *best = nullptr;
  for (auto *child : children) {
    if (auto *view = dynamic_cast<pep::ui::calculation::CalculationView *>(child)) {
      if (!best || view->height() > best->height()) {
        best = view;
      }
    }
  }
  return best;
}

void test_can_add_power_block_without_crash() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.show();
  app.processEvents();

  auto *button = find_button(widget, "Dodaj zasilanie");
  assert(button != nullptr);

  button->click();
  app.processEvents();
}

void test_power_type_exposes_linear_and_switching_choices() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.show();
  app.processEvents();

  auto *power_type = find_combo_with_items(widget, {"Liniowy", "Impulsowy"});
  assert(power_type != nullptr);

  auto *linear_subtype = find_combo_with_items(
      widget, {"Symetryczny", "Niestabilizowany (brak schematu)"});
  assert(linear_subtype != nullptr);
  assert(linear_subtype->isVisible());
  assert(linear_subtype->isEnabled());

  power_type->setCurrentIndex(1);
  app.processEvents();

  assert(!linear_subtype->isVisible());
  assert(!linear_subtype->isEnabled());

  power_type->setCurrentIndex(0);
  app.processEvents();

  assert(linear_subtype->isVisible());
  assert(linear_subtype->isEnabled());
}

void test_symmetric_transformer_hint_is_visible() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.show();
  app.processEvents();

  auto *mode_combo = find_combo_with_items(
      widget, {"Napięcie wtórne z przekładni", "Przekładnia z wymaganego napięcia"});
  assert(mode_combo != nullptr);
  mode_combo->setCurrentIndex(1);
  app.processEvents();

  auto *hint = find_label_by_object_name(widget, "transformerHint");
  assert(hint != nullptr);
  assert(hint->isVisible());
}

void test_transformer_mode_hides_irrelevant_input_row() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.show();
  app.processEvents();

  auto *mode_combo = find_combo_with_items(
      widget, {"Napięcie wtórne z przekładni", "Przekładnia z wymaganego napięcia"});
  assert(mode_combo != nullptr);

  auto *ratio_label = find_label_containing(widget, "Przekładnia transformatora");
  auto *secondary_label = find_label_containing(widget, "Wymagane napięcie wtórne");
  assert(ratio_label != nullptr);
  assert(secondary_label != nullptr);

  mode_combo->setCurrentIndex(0);
  app.processEvents();
  assert(ratio_label->isVisible());
  assert(!secondary_label->isVisible());

  mode_combo->setCurrentIndex(1);
  app.processEvents();
  assert(!ratio_label->isVisible());
  assert(secondary_label->isVisible());
  auto *hint = find_label_by_object_name(widget, "transformerHint");
  assert(hint != nullptr);
  assert(hint->isVisible());

  mode_combo->setCurrentIndex(0);
  app.processEvents();
  assert(!hint->isVisible());
}

void test_calculation_panel_is_visible_and_has_space() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.resize(1200, 900);
  widget.show();
  app.processEvents();

  auto *view = find_calculation_view(widget);
  assert(view != nullptr);
  assert(view->height() >= 200);
}

} // namespace

int main() {
  test_can_add_power_block_without_crash();
  test_power_type_exposes_linear_and_switching_choices();
  test_symmetric_transformer_hint_is_visible();
  test_transformer_mode_hides_irrelevant_input_row();
  test_calculation_panel_is_visible_and_has_space();
  return 0;
}
