#include "pep/modules/project_design/ui/widget.hpp"
#include "src/modules/project_design/graph/graph.hpp"
#include "src/modules/project_design/model/model.hpp"
#include "src/modules/psu_basic/ui/waveform_widget.hpp"
#include "src/ui/calculation/calculation_view.hpp"

#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTextBrowser>

#include <cassert>
#include <cmath>
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

QAction *find_action(QMenu *menu, const QString &text) {
  if (!menu) {
    return nullptr;
  }
  for (auto *action : menu->actions()) {
    if (action && action->text() == text) {
      return action;
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

QLineEdit *find_line_edit_by_object_name(QWidget &widget, const QString &object_name) {
  const auto edits = widget.findChildren<QLineEdit *>();
  for (auto *edit : edits) {
    if (edit && edit->objectName() == object_name) {
      return edit;
    }
  }
  return nullptr;
}

QComboBox *find_combo_by_object_name(QWidget &widget, const QString &object_name) {
  const auto combos = widget.findChildren<QComboBox *>();
  for (auto *combo : combos) {
    if (combo && combo->objectName() == object_name) {
      return combo;
    }
  }
  return nullptr;
}

QGraphicsScene *find_graphics_scene(QWidget &widget) {
  auto *view = widget.findChild<QGraphicsView *>();
  return view ? view->scene() : nullptr;
}

pep::modules::project_design::BlockItem *find_block_item(QGraphicsScene *scene, int block_id) {
  if (!scene) {
    return nullptr;
  }

  const auto items = scene->items();
  for (auto *item : items) {
    if (auto *block = dynamic_cast<pep::modules::project_design::BlockItem *>(item)) {
      if (block->block_id() == block_id) {
        return block;
      }
    }
  }

  return nullptr;
}

QTextBrowser *find_text_browser(QWidget &widget) {
  const auto views = widget.findChildren<QTextBrowser *>();
  if (views.empty()) {
    return nullptr;
  }
  return views.front();
}

pep::modules::psu_basic::WaveformWidget *find_waveform_by_object_name(QWidget &widget,
                                                                      const QString &object_name) {
  const auto children = widget.findChildren<QWidget *>();
  for (auto *child : children) {
    if (auto *waveform = dynamic_cast<pep::modules::psu_basic::WaveformWidget *>(child)) {
      if (waveform->objectName() == object_name) {
        return waveform;
      }
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

  auto *button = find_button(widget, "Dodaj element");
  assert(button != nullptr);
  auto *menu = button->menu();
  assert(menu != nullptr);
  auto actions = menu->actions();
  assert(!actions.empty());
  actions.front()->trigger();
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

  auto *linear_subtype = find_combo_with_items(
      widget, {"Symetryczny", "Niesymetryczny"});
  assert(linear_subtype != nullptr);
  assert(linear_subtype->isVisible());
  assert(linear_subtype->isEnabled());

  auto *power_type = find_combo_with_items(widget, {"Liniowy", "Impulsowy"});
  assert(power_type != nullptr);
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
  assert(hint->text().contains("Transformator 2x12 V"));
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

void test_nonsymmetric_variant_keeps_calculators_and_hides_symmetric_hint() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.show();
  app.processEvents();

  auto *linear_variant = find_combo_with_items(
      widget, {"Symetryczny", "Niesymetryczny"});
  auto *tabs = widget.findChild<QTabWidget *>("powerCalculatorTabs");
  auto *mode_combo = find_combo_with_items(
      widget, {"Napięcie wtórne z przekładni", "Przekładnia z wymaganego napięcia"});
  auto *hint = find_label_by_object_name(widget, "transformerHint");
  assert(linear_variant != nullptr);
  assert(tabs != nullptr);
  assert(mode_combo != nullptr);
  assert(hint != nullptr);

  linear_variant->setCurrentIndex(1);
  mode_combo->setCurrentIndex(1);
  app.processEvents();

  assert(tabs->count() == 4);
  assert(tabs->tabText(0) == "Transformator");
  assert(tabs->tabText(1) == "Prostownik");
  assert(tabs->tabText(2) == "Filtracja");
  assert(tabs->tabText(3) == "Obciążenie");
  assert(!hint->isVisible());
}

void test_voltage_quantity_switch_converts_primary_value() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.show();
  app.processEvents();

  auto *quantity_combo = find_combo_by_object_name(widget, "transformerVoltageQuantity");
  auto *primary_input = find_line_edit_by_object_name(widget, "transformerPrimaryInput");
  assert(quantity_combo != nullptr);
  assert(primary_input != nullptr);

  primary_input->setText("230");
  app.processEvents();

  quantity_combo->setCurrentIndex(1);
  app.processEvents();
  const double peak_value = primary_input->text().toDouble();
  assert(peak_value > 325.0 && peak_value < 325.5);

  quantity_combo->setCurrentIndex(0);
  app.processEvents();
  const double rms_value = primary_input->text().toDouble();
  assert(rms_value > 229.9 && rms_value < 230.1);
}

void test_rectifier_diode_drop_defaults_to_0_7_and_updates_calculation() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.resize(1200, 900);
  widget.show();
  app.processEvents();

  auto *diode_drop_input = find_line_edit_by_object_name(widget, "diodeDropInput");
  auto *text_browser = find_text_browser(widget);
  assert(diode_drop_input != nullptr);
  assert(text_browser != nullptr);
  assert(diode_drop_input->text() == "0.7");

  auto *tabs = widget.findChild<QTabWidget *>("powerCalculatorTabs");
  assert(tabs != nullptr);
  tabs->setCurrentIndex(1);
  app.processEvents();

  const QString default_text = text_browser->toPlainText();
  assert(!default_text.isEmpty());

  diode_drop_input->setText("1.1");
  app.processEvents();

  const QString updated_text = text_browser->toPlainText();
  assert(!updated_text.isEmpty());
  assert(updated_text != default_text);
}

void test_filtration_can_compute_required_capacitance_from_max_ripple() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.resize(1200, 900);
  widget.show();
  app.processEvents();

  auto *tabs = widget.findChild<QTabWidget *>("powerCalculatorTabs");
  auto *current_input = find_line_edit_by_object_name(widget, "currentInput");
  auto *ripple_input = find_line_edit_by_object_name(widget, "maxRippleInput");
  auto *cap_input = find_line_edit_by_object_name(widget, "capacitorInput");
  assert(tabs != nullptr);
  assert(current_input != nullptr);
  assert(ripple_input != nullptr);
  assert(cap_input != nullptr);

  tabs->setCurrentIndex(2);
  app.processEvents();

  current_input->setText("0.5");
  ripple_input->setText("2");
  app.processEvents();

  const double cap_value = cap_input->text().toDouble();
  assert(cap_value > 2400.0 && cap_value < 2600.0);
}

void test_power_variant_is_preserved_per_block_after_switching_active_selection() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.resize(1200, 900);
  widget.show();
  app.processEvents();

  auto *add_button = find_button(widget, "Dodaj element");
  auto *linear_variant = find_combo_with_items(
      widget, {"Symetryczny", "Niesymetryczny"});
  auto *scene = find_graphics_scene(widget);
  assert(add_button != nullptr);
  assert(add_button->menu() != nullptr);
  assert(linear_variant != nullptr);
  assert(scene != nullptr);

  linear_variant->setCurrentIndex(1);
  app.processEvents();

  auto *add_power = find_action(add_button->menu(), "Zasilacz liniowy");
  auto *add_amp = find_action(add_button->menu(), "Wzmacniacz klasy B (audio)");
  assert(add_power != nullptr);
  assert(add_amp != nullptr);
  add_power->trigger();
  app.processEvents();

  linear_variant->setCurrentIndex(1);
  app.processEvents();

  add_amp->trigger();
  app.processEvents();

  auto *block1 = find_block_item(scene, 1);
  auto *block2 = find_block_item(scene, 2);
  auto *block3 = find_block_item(scene, 3);
  assert(block1 != nullptr);
  assert(block2 != nullptr);
  assert(block3 != nullptr);

  block1->setSelected(true);
  app.processEvents();
  assert(linear_variant->currentData().toInt() ==
         static_cast<int>(pep::modules::project_design::BlockVariant::PsuUnregulated));

  block2->setSelected(true);
  app.processEvents();
  assert(linear_variant->currentData().toInt() ==
         static_cast<int>(pep::modules::project_design::BlockVariant::PsuUnregulated));

  block3->setSelected(true);
  app.processEvents();

  block1->setSelected(true);
  app.processEvents();
  assert(linear_variant->currentData().toInt() ==
         static_cast<int>(pep::modules::project_design::BlockVariant::PsuUnregulated));
}

void test_amp_supply_mode_shows_disturbance_inputs_and_amp_mode_hides_them() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.resize(1200, 900);
  widget.show();
  app.processEvents();

  auto *add_button = find_button(widget, "Dodaj element");
  assert(add_button != nullptr);
  assert(add_button->menu() != nullptr);
  auto *add_amp = find_action(add_button->menu(), "Wzmacniacz klasy B (audio)");
  assert(add_amp != nullptr);
  add_amp->trigger();
  app.processEvents();

  auto *design_mode =
      find_combo_with_items(widget, {"Projektowanie zasilacza do wzmacniacza", "Wzmacniacz"});
  auto *psrr_input = find_line_edit_by_object_name(widget, "ampPsrrInput");
  auto *disturbance_input =
      find_line_edit_by_object_name(widget, "ampDisturbanceRejectionInput");
  auto *disturbance_freq_input =
      find_line_edit_by_object_name(widget, "ampDisturbanceFreqInput");
  auto *cap_esr_input = find_line_edit_by_object_name(widget, "ampCapEsrInput");
  auto *transformer_res_input =
      find_line_edit_by_object_name(widget, "ampTransformerResInput");
  assert(design_mode != nullptr);
  assert(psrr_input != nullptr);
  assert(disturbance_input != nullptr);
  assert(disturbance_freq_input != nullptr);
  assert(cap_esr_input != nullptr);
  assert(transformer_res_input != nullptr);

  design_mode->setCurrentIndex(0);
  app.processEvents();
  assert(psrr_input->isVisible());
  assert(disturbance_input->isVisible());
  assert(disturbance_freq_input->isVisible());
  assert(cap_esr_input->isVisible());
  assert(transformer_res_input->isVisible());

  design_mode->setCurrentIndex(1);
  app.processEvents();
  assert(!psrr_input->isVisible());
  assert(!disturbance_input->isVisible());
  assert(!disturbance_freq_input->isVisible());
  assert(!cap_esr_input->isVisible());
  assert(!transformer_res_input->isVisible());
}

void test_regulator_source_combo_lists_only_power_blocks() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.resize(1200, 900);
  widget.show();
  app.processEvents();

  auto *add_button = find_button(widget, "Dodaj element");
  assert(add_button != nullptr);
  auto *add_power = find_action(add_button->menu(), "Zasilacz liniowy");
  auto *add_regulator = find_action(add_button->menu(), "Stabilizator napięcia");
  assert(add_power != nullptr);
  assert(add_regulator != nullptr);

  add_power->trigger();
  add_regulator->trigger();
  app.processEvents();

  auto *scene = find_graphics_scene(widget);
  assert(scene != nullptr);
  auto *regulator = find_block_item(scene, 3);
  assert(regulator != nullptr);
  regulator->setSelected(true);
  app.processEvents();

  auto *combo = find_combo_by_object_name(widget, "regulatorPowerSource");
  assert(combo != nullptr);
  assert(combo->count() == 3);
  assert(combo->itemText(0) == "— (brak)");
  assert(combo->itemText(1).contains("Zasilanie #1"));
  assert(combo->itemText(2).contains("Zasilanie #2"));
}

void test_amp_source_combo_lists_power_and_regulator_blocks() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.resize(1200, 900);
  widget.show();
  app.processEvents();

  auto *add_button = find_button(widget, "Dodaj element");
  assert(add_button != nullptr);
  auto *add_regulator = find_action(add_button->menu(), "Stabilizator napięcia");
  auto *add_amp = find_action(add_button->menu(), "Wzmacniacz klasy B (audio)");
  assert(add_regulator != nullptr);
  assert(add_amp != nullptr);

  add_regulator->trigger();
  add_amp->trigger();
  app.processEvents();

  auto *scene = find_graphics_scene(widget);
  assert(scene != nullptr);
  auto *amp = find_block_item(scene, 3);
  assert(amp != nullptr);
  amp->setSelected(true);
  app.processEvents();

  auto *combo_pos = find_combo_by_object_name(widget, "ampPowerSourcePos");
  auto *combo_neg = find_combo_by_object_name(widget, "ampPowerSourceNeg");
  assert(combo_pos != nullptr);
  assert(combo_neg != nullptr);
  assert(combo_pos->count() == 3);
  assert(combo_pos->itemText(0) == "— (brak)");
  assert(combo_pos->itemText(1).contains("Zasilanie #1"));
  assert(combo_pos->itemText(2).contains("Stabilizator #2"));
  assert(combo_neg->count() == 2);
  assert(combo_neg->itemText(0) == "— (brak)");
  assert(combo_neg->itemText(1).contains("Zasilanie #1"));
}

void test_regulator_form_hides_series_resistor_and_prefills_from_power() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.resize(1200, 900);
  widget.show();
  app.processEvents();

  auto *add_button = find_button(widget, "Dodaj element");
  auto *add_regulator = find_action(add_button->menu(), "Stabilizator napięcia");
  assert(add_regulator != nullptr);
  add_regulator->trigger();
  app.processEvents();

  auto *scene = find_graphics_scene(widget);
  auto *regulator = find_block_item(scene, 2);
  assert(regulator != nullptr);
  regulator->setSelected(true);
  app.processEvents();

  auto *source_combo = find_combo_by_object_name(widget, "regulatorPowerSource");
  auto *rail_combo = find_combo_by_object_name(widget, "regulatorSupplyRail");
  auto *source_min_input = find_line_edit_by_object_name(widget, "regulatorSourceMinInput");
  auto *vin_input = find_line_edit_by_object_name(widget, "regulatorInputMinInput");
  auto *margin_input = find_line_edit_by_object_name(widget, "regulatorMarginInput");
  auto *output_input = find_line_edit_by_object_name(widget, "regulatorOutputInput");
  auto *dropout_input = find_line_edit_by_object_name(widget, "regulatorDropoutInput");
  auto *current_input = find_line_edit_by_object_name(widget, "regulatorCurrentInput");
  assert(source_combo != nullptr);
  assert(rail_combo != nullptr);
  assert(source_min_input != nullptr);
  assert(vin_input != nullptr);
  assert(margin_input != nullptr);
  assert(output_input != nullptr);
  assert(dropout_input != nullptr);
  assert(current_input != nullptr);
  assert(source_min_input->isReadOnly());
  assert(vin_input->isReadOnly());
  assert(margin_input->isReadOnly());
  assert(find_line_edit_by_object_name(widget, "regulatorSeriesResInput") == nullptr);
  assert(find_label_containing(widget, "Dla wariantu Zenera rezystor szeregowy") == nullptr);

  source_combo->setCurrentIndex(1);
  app.processEvents();

  assert(!vin_input->text().isEmpty());
  assert(vin_input->text().toDouble() > 0.0);
  if (!margin_input->text().isEmpty()) {
    assert(std::abs(margin_input->text().toDouble()) >= 0.0);
  }
  assert(!current_input->text().isEmpty());
  assert(current_input->text().toDouble() > 0.0);

  output_input->setText("5");
  dropout_input->setText("3");
  app.processEvents();
  assert(vin_input->text().toDouble() > 7.9 && vin_input->text().toDouble() < 8.1);

  output_input->setText("5,5");
  dropout_input->setText("2,5");
  app.processEvents();
  assert(vin_input->text().toDouble() > 7.9 && vin_input->text().toDouble() < 8.1);

  rail_combo->setCurrentIndex(1);
  app.processEvents();
  assert(rail_combo->currentText().contains("Vee"));
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

void test_transformer_waveforms_show_primary_and_secondary_signals() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.resize(1200, 900);
  widget.show();
  app.processEvents();

  auto *input_waveform = find_waveform_by_object_name(widget, "waveformInput");
  auto *output_waveform = find_waveform_by_object_name(widget, "waveformOutput");
  assert(input_waveform != nullptr);
  assert(output_waveform != nullptr);

  const auto default_input = input_waveform->params();
  const auto default_output = output_waveform->params();
  assert(default_input.type == pep::modules::psu_basic::WaveformType::Sinus);
  assert(std::abs(default_input.vpeak - 325.269) < 0.5);
  assert(std::abs(default_output.vpeak) < 1e-9);

  auto *waveform_combo = find_combo_by_object_name(widget, "transformerWaveform");
  auto *ratio_input = find_line_edit_by_object_name(widget, "transformerRatioInput");
  assert(waveform_combo != nullptr);
  assert(ratio_input != nullptr);

  waveform_combo->setCurrentIndex(1);
  ratio_input->setText("20");
  app.processEvents();

  const auto square_input = input_waveform->params();
  const auto square_output = output_waveform->params();
  assert(square_input.type == pep::modules::psu_basic::WaveformType::Square);
  assert(square_output.type == pep::modules::psu_basic::WaveformType::Square);
  assert(std::abs(square_input.vpeak - 230.0) < 0.1);
  assert(std::abs(square_output.vpeak - 11.5) < 0.1);
}

void test_power_waveforms_follow_active_calculator_tab() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::project_design::Widget widget;
  widget.resize(1200, 900);
  widget.show();
  app.processEvents();

  auto *tabs = widget.findChild<QTabWidget *>("powerCalculatorTabs");
  auto *ratio_input = find_line_edit_by_object_name(widget, "transformerRatioInput");
  auto *input_waveform = find_waveform_by_object_name(widget, "waveformInput");
  auto *output_waveform = find_waveform_by_object_name(widget, "waveformOutput");
  assert(tabs != nullptr);
  assert(ratio_input != nullptr);
  assert(input_waveform != nullptr);
  assert(output_waveform != nullptr);

  ratio_input->setText("20");
  app.processEvents();

  const auto transformer_input = input_waveform->params();
  const auto transformer_output = output_waveform->params();
  assert(transformer_input.type == pep::modules::psu_basic::WaveformType::Sinus);
  assert(transformer_output.type == pep::modules::psu_basic::WaveformType::Sinus);
  assert(std::abs(transformer_input.vpeak - 325.269) < 0.5);
  assert(std::abs(transformer_output.vpeak - 16.263) < 0.2);

  tabs->setCurrentIndex(1);
  app.processEvents();

  const auto rectifier_input = input_waveform->params();
  const auto rectifier_output = output_waveform->params();
  assert(rectifier_input.type == pep::modules::psu_basic::WaveformType::Sinus);
  assert(rectifier_output.type == pep::modules::psu_basic::WaveformType::FullWave);
  assert(std::abs(rectifier_input.vpeak - 16.263) < 0.2);
  assert(std::abs(rectifier_output.vrect - 14.863) < 0.3);

  tabs->setCurrentIndex(2);
  app.processEvents();

  const auto filtration_input = input_waveform->params();
  const auto filtration_output = output_waveform->params();
  assert(filtration_input.type == pep::modules::psu_basic::WaveformType::FullWave);
  assert(filtration_output.type == pep::modules::psu_basic::WaveformType::Filtered);
  assert(std::abs(filtration_input.vrect - 14.863) < 0.3);
  assert(std::abs(filtration_output.vrect - 14.863) < 0.3);

  tabs->setCurrentIndex(0);
  app.processEvents();

  const auto transformer_again_input = input_waveform->params();
  const auto transformer_again_output = output_waveform->params();
  assert(transformer_again_input.type == pep::modules::psu_basic::WaveformType::Sinus);
  assert(transformer_again_output.type == pep::modules::psu_basic::WaveformType::Sinus);
  assert(std::abs(transformer_again_input.vpeak - 325.269) < 0.5);
  assert(std::abs(transformer_again_output.vpeak - 16.263) < 0.2);
}

void test_waveform_widget_scales_filtered_signal_to_visible_range() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::psu_basic::WaveformWidget widget;
  widget.set_params(pep::modules::psu_basic::WaveformParams{
      14.463, 14.463, 32.0, 50.0, pep::modules::psu_basic::WaveformType::Filtered});

  const auto [vmin, vmax] = widget.visible_voltage_range();
  assert(vmin < -17.0);
  assert(vmax > 14.4);
}

} // namespace

int main() {
  test_can_add_power_block_without_crash();
  test_power_type_exposes_linear_and_switching_choices();
  test_symmetric_transformer_hint_is_visible();
  test_transformer_mode_hides_irrelevant_input_row();
  test_nonsymmetric_variant_keeps_calculators_and_hides_symmetric_hint();
  test_voltage_quantity_switch_converts_primary_value();
  test_rectifier_diode_drop_defaults_to_0_7_and_updates_calculation();
  test_filtration_can_compute_required_capacitance_from_max_ripple();
  test_power_variant_is_preserved_per_block_after_switching_active_selection();
  test_amp_supply_mode_shows_disturbance_inputs_and_amp_mode_hides_them();
  test_regulator_source_combo_lists_only_power_blocks();
  test_amp_source_combo_lists_power_and_regulator_blocks();
  test_regulator_form_hides_series_resistor_and_prefills_from_power();
  test_calculation_panel_is_visible_and_has_space();
  test_transformer_waveforms_show_primary_and_secondary_signals();
  test_power_waveforms_follow_active_calculator_tab();
  test_waveform_widget_scales_filtered_signal_to_visible_range();
  return 0;
}
