#include "pep/modules/project_design/ui/widget.hpp"

#include "../export/export.hpp"
#include "../graph/graph.hpp"
#include "../model/connection_ops.hpp"
#include "../model/model.hpp"
#include "active_block_view.hpp"
#include "canvas_scene.hpp"
#include "connection_ui.hpp"
#include "export_actions.hpp"
#include "form_sync.hpp"
#include "widget_actions.hpp"
#include "widget_bindings.hpp"

#include "../../psu_basic/model/model.hpp"
#include "../../psu_basic/ui/waveform_widget.hpp"
#include "../../../ui/calculation/calculation_module_widget.hpp"
#include "../../../ui/calculation/calculation_view.hpp"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QFormLayout>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHash>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSettings>
#include <QSizePolicy>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pep::modules::project_design {

namespace {

constexpr int kLeftPanelHorizontalPadding = 8;
constexpr int kFormLabelMinWidth = 230;

pep::modules::psu_basic::WaveformShape to_psu_waveform(SignalWaveform waveform) {
  switch (waveform) {
  case SignalWaveform::Square:
    return pep::modules::psu_basic::WaveformShape::Square;
  case SignalWaveform::Triangle:
    return pep::modules::psu_basic::WaveformShape::Triangle;
  default:
    return pep::modules::psu_basic::WaveformShape::Sine;
  }
}

double convert_transformer_voltage_value(double value, VoltageQuantity from, VoltageQuantity to,
                                         SignalWaveform waveform) {
  if (from == to) {
    return value;
  }

  const auto psu_waveform = to_psu_waveform(waveform);
  if (from == VoltageQuantity::Rms && to == VoltageQuantity::Peak) {
    return pep::modules::psu_basic::rms_to_peak(value, psu_waveform);
  }
  return pep::modules::psu_basic::peak_to_rms(value, psu_waveform);
}

void set_form_row_visible(QWidget *label, QWidget *field, bool visible) {
  if (label) {
    label->setVisible(visible);
  }
  if (field) {
    field->setVisible(visible);
  }
}

void tune_form_layout(QFormLayout *layout) {
  if (!layout) {
    return;
  }

  layout->setContentsMargins(0, 0, 0, 0);
  layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  layout->setRowWrapPolicy(QFormLayout::DontWrapRows);
  layout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  layout->setHorizontalSpacing(16);
  layout->setVerticalSpacing(10);

  for (int row = 0; row < layout->rowCount(); ++row) {
    if (auto *item = layout->itemAt(row, QFormLayout::LabelRole)) {
      if (auto *widget = item->widget()) {
        if (widget->objectName() == "compactLabel") {
          widget->setMinimumWidth(0);
          widget->setMaximumWidth(160);
        } else {
          widget->setMinimumWidth(kFormLabelMinWidth);
          widget->setMaximumWidth(kFormLabelMinWidth);
        }
        widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        if (auto *label = qobject_cast<QLabel *>(widget)) {
          label->setWordWrap(true);
          label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        }
      }
    }
  }
}

void apply_lightweight_tabs_style(QTabWidget *tabs) {
  if (!tabs) {
    return;
  }

  if (auto *bar = tabs->tabBar()) {
    bar->setDrawBase(false);
  }

  tabs->setStyleSheet(
      "QTabWidget::pane { border: 0; background: transparent; margin-top: 2px; }"
      "QTabBar::tab {"
      "  background: palette(button);"
      "  color: palette(window-text);"
      "  border: 1px solid palette(mid);"
      "  border-radius: 10px;"
      "  padding: 7px 14px;"
      "  margin-right: 8px;"
      "  margin-top: 0;"
      "}"
      "QTabBar::tab:selected {"
      "  background: palette(base);"
      "  border-color: palette(highlight);"
      "  font-weight: 600;"
      "}"
      "QTabBar::tab:hover:!selected {"
      "  background: palette(alternate-base);"
      "}");
}

} // namespace

struct Widget::Impl {
  std::vector<Block> blocks;
  std::vector<Connection> connections;
  int next_block_id = 1;
  std::optional<int> active_block_id;
  bool refreshing_canvas = false;
  std::optional<Endpoint> pending_connection;
  bool manual_layout = false;
  bool signal_lines_update_scheduled = false;
  bool syncing_active_to_form = false;

  // UI pointers (owned by QWidget parent tree).
  QGraphicsScene *scene = nullptr;
  std::vector<CanvasSignalLine> signal_lines;
  QListWidget *connections_list = nullptr;
  QComboBox *conn_from_block = nullptr;
  QComboBox *conn_from_port = nullptr;
  QComboBox *conn_to_block = nullptr;
  QComboBox *conn_to_port = nullptr;
  QTabWidget *bottom_tabs = nullptr;
  int waveform_tab_index = -1;

  QLabel *active_title = nullptr;
  QLabel *ports_label = nullptr;
  QLabel *transformer_ratio_label = nullptr;
  QLabel *transformer_secondary_label = nullptr;
  QLabel *transformer_hint = nullptr;
  QLabel *transformer_secondary_note = nullptr;
  QStackedWidget *props_stack = nullptr;
  QComboBox *power_family = nullptr;
  QComboBox *power_linear_variant = nullptr;
  QComboBox *power_design_mode = nullptr;
  QComboBox *transformer_mode = nullptr;
  QComboBox *transformer_waveform = nullptr;
  QComboBox *transformer_voltage_quantity = nullptr;
  QComboBox *variant = nullptr;
  QLineEdit *transformer_primary_input = nullptr;
  QLineEdit *transformer_primary_tol_input = nullptr;
  QLineEdit *transformer_ratio_input = nullptr;
  QLineEdit *transformer_secondary_input = nullptr;
  QLineEdit *vin_input = nullptr;
  QLineEdit *diode_drop_input = nullptr;
  QLineEdit *freq_input = nullptr;
  QLineEdit *current_input = nullptr;
  QLineEdit *cap_input = nullptr;
  QLineEdit *max_ripple_input = nullptr;
  QComboBox *amp_waveform = nullptr;
  QComboBox *amp_design_mode = nullptr;
  QComboBox *amp_power_source = nullptr;
  QLineEdit *amp_amp_input = nullptr;
  QLineEdit *amp_freq_input = nullptr;
  QLineEdit *amp_gain_input = nullptr;
  QLineEdit *amp_load_input = nullptr;
  QLineEdit *amp_power_input = nullptr;
  QLineEdit *amp_headroom_input = nullptr;
  QLineEdit *amp_max_ripple_input = nullptr;
  QWidget *amp_signal_group = nullptr;
  QWidget *amp_power_group = nullptr;
  QStackedWidget *calculator_input_stack = nullptr;
  QTabWidget *power_calculator_tabs = nullptr;
  int active_power_calculator_tab_index = 0;
  std::vector<pep::ui::calculation::CalculationView *> power_compute_results;
  pep::ui::calculation::CalculationView *compute_result = nullptr;
  QLabel *calculator_placeholder = nullptr;
  QLabel *validation = nullptr;

  pep::modules::psu_basic::WaveformWidget *waveform_in = nullptr;
  pep::modules::psu_basic::WaveformWidget *waveform_out = nullptr;

  // Helpers (stable, stored here to avoid capturing references to locals).
  std::function<void()> refresh_canvas;
  std::function<void()> refresh_connections_ui;
  std::function<void()> refresh_connection_form;
  std::function<void(QComboBox *, QComboBox *)> refresh_ports_combo;
  std::function<void()> sync_active_to_form;
  std::function<void()> sync_form_to_active;
  std::function<void()> recompute_and_validate;
  std::function<void()> apply_amp_requirements_to_power;
  std::function<void()> update_transformer_labels;
  std::function<void()> update_power_mode_visibility;
  std::function<void()> update_amp_mode_visibility;
  std::function<void()> update_calculator_panels;
  std::function<void()> update_props_stack_height;
  std::function<void(int)> set_active;
  VoltageQuantity last_transformer_voltage_quantity = VoltageQuantity::Rms;
  bool syncing_filter_fields = false;
  enum class FilterInput { None, Cap, Ripple };
  FilterInput last_filter_input = FilterInput::None;
};

Widget::Widget(QWidget *parent) : QWidget(parent) {
  impl_ = std::make_unique<Impl>();

  auto *root = new QHBoxLayout(this);
  root->setContentsMargins(12, 12, 12, 12);
  root->setSpacing(12);

  // Seed with one power block.
  impl_->blocks.push_back(
      make_power_block(impl_->next_block_id++, "Zasilanie #1", BlockVariant::PsuSymmetric));
  impl_->active_block_id = impl_->blocks.front().id;

  auto *left = new QWidget(this);  // calculators
  auto *right = new QWidget(this); // waveforms + graph
  left->setMinimumWidth(420);
  right->setMinimumWidth(360);
  auto *left_layout = new QVBoxLayout(left);
  auto *right_layout = new QVBoxLayout(right);
  left_layout->setContentsMargins(0, 0, 0, 0);

  auto *scene = new QGraphicsScene(right);
  impl_->scene = scene;
  auto *view = new CanvasView(scene, right);
  view->setMinimumHeight(280);
  view->setRenderHint(QPainter::Antialiasing, true);
  view->setFocusPolicy(Qt::StrongFocus);

  auto *canvas_toolbar = new QWidget(right);
  auto *canvas_toolbar_l = new QHBoxLayout(canvas_toolbar);
  canvas_toolbar_l->setContentsMargins(0, 0, 0, 0);
  auto *btn_add_element = new QPushButton("Dodaj element", canvas_toolbar);
  auto *add_element_menu = new QMenu(btn_add_element);
  auto *action_add_power = add_element_menu->addAction("Zasilacz liniowy");
  auto *action_add_switching = add_element_menu->addAction("Zasilacz impulsowy");
  auto *action_add_amp = add_element_menu->addAction("Wzmacniacz klasy B (audio)");
  btn_add_element->setMenu(add_element_menu);
  auto *btn_remove_selected = new QPushButton("Usuń zaznaczony", canvas_toolbar);
  auto *btn_auto_layout = new QPushButton("Auto-rozmieszczenie", canvas_toolbar);
  canvas_toolbar_l->addWidget(btn_add_element);
  canvas_toolbar_l->addWidget(btn_remove_selected);
  canvas_toolbar_l->addWidget(btn_auto_layout);
  canvas_toolbar_l->addStretch(1);

  auto *waveforms_box = new QGroupBox("Wykresy", right);
  auto *waveforms_layout = new QVBoxLayout(waveforms_box);
  auto *w_in_label = new QLabel("Sygnał wejściowy", waveforms_box);
  auto *w_out_label = new QLabel("Sygnał wyjściowy", waveforms_box);
  auto *waveform_in = new pep::modules::psu_basic::WaveformWidget(waveforms_box);
  auto *waveform_out = new pep::modules::psu_basic::WaveformWidget(waveforms_box);
  impl_->waveform_in = waveform_in;
  impl_->waveform_out = waveform_out;
  waveform_in->setObjectName("waveformInput");
  waveform_out->setObjectName("waveformOutput");
  waveform_in->setMinimumHeight(160);
  waveform_out->setMinimumHeight(160);
  waveforms_layout->addWidget(w_in_label);
  waveforms_layout->addWidget(waveform_in);
  waveforms_layout->addWidget(w_out_label);
  waveforms_layout->addWidget(waveform_out);

  auto *connections_tab = new QWidget(right);
  auto *connections_layout = new QVBoxLayout(connections_tab);
  auto *connections_list = new QListWidget(connections_tab);
  connections_list->setSelectionMode(QAbstractItemView::SingleSelection);
  impl_->connections_list = connections_list;
  connections_layout->addWidget(connections_list);

  auto *conn_form = new QFormLayout();
  auto *conn_from_block = new QComboBox(connections_tab);
  auto *conn_from_port = new QComboBox(connections_tab);
  auto *conn_to_block = new QComboBox(connections_tab);
  auto *conn_to_port = new QComboBox(connections_tab);
  impl_->conn_from_block = conn_from_block;
  impl_->conn_from_port = conn_from_port;
  impl_->conn_to_block = conn_to_block;
  impl_->conn_to_port = conn_to_port;
  conn_form->addRow("Z", conn_from_block);
  conn_form->addRow("Port", conn_from_port);
  conn_form->addRow("Do", conn_to_block);
  conn_form->addRow("Port", conn_to_port);
  auto *add_conn = new QPushButton("Dodaj połączenie", connections_tab);
  auto *remove_conn = new QPushButton("Usuń zaznaczone połączenie", connections_tab);

  connections_layout->addLayout(conn_form);
  connections_layout->addWidget(add_conn);
  connections_layout->addWidget(remove_conn);

  auto *active_title = new QLabel("Aktywny element: —", right);
  {
    QFont font = active_title->font();
    font.setBold(true);
    font.setPointSize(14);
    active_title->setFont(font);
  }
  impl_->active_title = active_title;

  auto *ports_label = new QLabel(right);
  ports_label->setWordWrap(true);
  impl_->ports_label = ports_label;

  auto *props_stack = new QStackedWidget(left);
  impl_->props_stack = props_stack;
  props_stack->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  auto *power_props = new QWidget(props_stack);
  power_props->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  auto *power_form = new QFormLayout(power_props);
  tune_form_layout(power_form);
  auto *transformer_ratio_label = new QLabel(power_props);
  auto *transformer_secondary_label = new QLabel(power_props);
  auto *transformer_hint = new QLabel(power_props);
  auto *transformer_secondary_note = new QLabel(power_props);
  auto *power_family = new QComboBox(power_props);
  auto *power_linear_variant = new QComboBox(power_props);
  auto *power_design_mode = new QComboBox(power_props);
  auto *transformer_mode = new QComboBox(power_props);
  auto *transformer_waveform = new QComboBox(power_props);
  auto *transformer_voltage_quantity = new QComboBox(power_props);
  auto *variant = new QComboBox(power_props);
  impl_->power_family = power_family;
  impl_->power_linear_variant = power_linear_variant;
  impl_->power_design_mode = power_design_mode;
  impl_->transformer_ratio_label = transformer_ratio_label;
  impl_->transformer_secondary_label = transformer_secondary_label;
  impl_->transformer_hint = transformer_hint;
  impl_->transformer_secondary_note = transformer_secondary_note;
  impl_->transformer_mode = transformer_mode;
  impl_->transformer_waveform = transformer_waveform;
  impl_->transformer_voltage_quantity = transformer_voltage_quantity;
  impl_->variant = variant;
  power_family->addItem("Liniowy", 0);
  power_family->addItem("Impulsowy", 1);
  power_linear_variant->addItem("Symetryczny", static_cast<int>(BlockVariant::PsuSymmetric));
  power_linear_variant->addItem("Niesymetryczny",
                                static_cast<int>(BlockVariant::PsuUnregulated));
  power_design_mode->addItem("Zasilacz do obciążenia",
                             static_cast<int>(PowerDesignMode::SupplyForLoad));
  power_design_mode->addItem("Obciążenie do zasilacza",
                             static_cast<int>(PowerDesignMode::LoadForSupply));
  power_design_mode->setVisible(false);
  power_design_mode->setEnabled(false);
  transformer_mode->addItem("Napięcie wtórne z przekładni",
                            static_cast<int>(TransformerSolveMode::SecondaryFromRatio));
  transformer_mode->addItem("Przekładnia z wymaganego napięcia",
                            static_cast<int>(TransformerSolveMode::RatioFromSecondary));
  transformer_waveform->addItem("Sinus", static_cast<int>(SignalWaveform::Sine));
  transformer_waveform->addItem("Prostokąt", static_cast<int>(SignalWaveform::Square));
  transformer_waveform->addItem("Trójkąt", static_cast<int>(SignalWaveform::Triangle));
  transformer_voltage_quantity->addItem("Skuteczne (RMS)", static_cast<int>(VoltageQuantity::Rms));
  transformer_voltage_quantity->addItem("Szczytowe (peak)",
                                        static_cast<int>(VoltageQuantity::Peak));
  variant->addItem("Zasilacz symetryczny", static_cast<int>(BlockVariant::PsuSymmetric));
  variant->addItem("Zasilacz niesymetryczny",
                   static_cast<int>(BlockVariant::PsuUnregulated));
  variant->addItem("Zasilacz impulsowy (brak schematu)",
                   static_cast<int>(BlockVariant::PsuSwitching));
  variant->setVisible(false);

  auto *transformer_primary_input = new QLineEdit(power_props);
  auto *transformer_primary_tol_input = new QLineEdit(power_props);
  auto *transformer_ratio_input = new QLineEdit(power_props);
  auto *transformer_secondary_input = new QLineEdit(power_props);
  auto *vin_input = new QLineEdit(power_props);
  auto *diode_drop_input = new QLineEdit(power_props);
  auto *freq_input = new QLineEdit(power_props);
  auto *current_input = new QLineEdit(power_props);
  auto *cap_input = new QLineEdit(power_props);
  auto *max_ripple_input = new QLineEdit(power_props);
  impl_->transformer_primary_input = transformer_primary_input;
  impl_->transformer_primary_tol_input = transformer_primary_tol_input;
  impl_->transformer_ratio_input = transformer_ratio_input;
  impl_->transformer_secondary_input = transformer_secondary_input;
  impl_->vin_input = vin_input;
  impl_->diode_drop_input = diode_drop_input;
  impl_->freq_input = freq_input;
  impl_->current_input = current_input;
  impl_->cap_input = cap_input;
  impl_->max_ripple_input = max_ripple_input;
  transformer_primary_input->setPlaceholderText("np. 230");
  transformer_primary_input->setObjectName("transformerPrimaryInput");
  transformer_primary_tol_input->setPlaceholderText("10");
  transformer_primary_tol_input->setObjectName("transformerPrimaryTolInput");
  transformer_primary_tol_input->setMaximumWidth(60);
  transformer_primary_tol_input->setText("10");
  transformer_mode->setObjectName("transformerMode");
  transformer_waveform->setObjectName("transformerWaveform");
  transformer_voltage_quantity->setObjectName("transformerVoltageQuantity");
  transformer_ratio_input->setPlaceholderText("Np:Ns, np. 19.17");
  transformer_ratio_input->setObjectName("transformerRatioInput");
  transformer_secondary_input->setPlaceholderText("np. 12 RMS");
  transformer_secondary_input->setObjectName("transformerSecondaryInput");
  vin_input->setPlaceholderText("VAC RMS");
  vin_input->setReadOnly(true);
  diode_drop_input->setPlaceholderText("V");
  diode_drop_input->setObjectName("diodeDropInput");
  freq_input->setPlaceholderText("50 / 60");
  freq_input->setObjectName("mainsFrequencyInput");
  current_input->setPlaceholderText("A");
  current_input->setObjectName("currentInput");
  cap_input->setPlaceholderText("uF");
  cap_input->setObjectName("capacitorInput");
  max_ripple_input->setPlaceholderText("Vpp");
  max_ripple_input->setObjectName("maxRippleInput");
  transformer_hint->setWordWrap(true);
  transformer_hint->setObjectName("transformerHint");
  transformer_hint->setStyleSheet(
      "QLabel#transformerHint { color: palette(window-text); font-size: 12px; font-style: italic; }");
  transformer_secondary_note->setWordWrap(true);
  transformer_secondary_note->setObjectName("transformerSecondaryNote");
  transformer_secondary_note->setStyleSheet(
      "QLabel#transformerSecondaryNote { color: palette(window-text); font-size: 11px; font-style: italic; }");
  auto *power_variant_row = new QWidget(power_props);
  auto *power_variant_row_layout = new QHBoxLayout(power_variant_row);
  power_variant_row_layout->setContentsMargins(0, 0, 0, 0);
  power_variant_row_layout->setSpacing(8);
  power_variant_row_layout->addWidget(power_family, 1);
  power_variant_row_layout->addWidget(power_linear_variant, 1);
  auto *transformer_secondary_with_hint = new QWidget(power_props);
  auto *transformer_secondary_with_hint_layout = new QVBoxLayout(transformer_secondary_with_hint);
  transformer_secondary_with_hint_layout->setContentsMargins(0, 0, 0, 0);
  transformer_secondary_with_hint_layout->setSpacing(4);
  transformer_secondary_with_hint_layout->addWidget(transformer_secondary_input);
  transformer_secondary_with_hint_layout->addWidget(transformer_hint);
  transformer_secondary_with_hint_layout->addWidget(transformer_secondary_note);
  auto *transformer_primary_with_quantity = new QWidget(power_props);
  auto *transformer_primary_with_quantity_layout =
      new QHBoxLayout(transformer_primary_with_quantity);
  transformer_primary_with_quantity_layout->setContentsMargins(0, 0, 0, 0);
  transformer_primary_with_quantity_layout->setSpacing(8);
  transformer_primary_with_quantity_layout->addWidget(transformer_voltage_quantity, 0);
  transformer_primary_with_quantity_layout->addWidget(transformer_primary_input, 1);
  transformer_primary_with_quantity_layout->addWidget(transformer_primary_tol_input, 0);
  transformer_primary_with_quantity_layout->addWidget(new QLabel("%", power_props), 0);
  power_form->addRow("Rodzaj zasilacza", power_variant_row);
  power_variant_row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  tune_form_layout(power_form);

  auto *amp_props = new QWidget(props_stack);
  amp_props->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  auto *amp_root = new QVBoxLayout(amp_props);
  amp_root->setContentsMargins(0, 0, 0, 0);
  amp_root->setSpacing(12);
  auto *amp_header = new QWidget(amp_props);
  auto *amp_header_form = new QFormLayout(amp_header);
  tune_form_layout(amp_header_form);
  auto *amp_model = new QLabel("Wzmacniacz klasy B (audio)", amp_props);
  auto *amp_design_mode = new QComboBox(amp_props);
  auto *amp_power_source = new QComboBox(amp_props);
  auto *amp_waveform = new QComboBox(amp_props);
  amp_waveform->addItem("Sinus", static_cast<int>(SignalWaveform::Sine));
  amp_waveform->addItem("Prostokąt", static_cast<int>(SignalWaveform::Square));
  amp_waveform->addItem("Trójkąt", static_cast<int>(SignalWaveform::Triangle));
  amp_design_mode->addItem("Projektowanie zasilacza do wzmacniacza",
                           static_cast<int>(AmpDesignMode::SupplyForAmp));
  amp_design_mode->addItem("Wzmacniacz", static_cast<int>(AmpDesignMode::AmpForSupply));
  auto *amp_amp_input = new QLineEdit(amp_props);
  auto *amp_freq_input = new QLineEdit(amp_props);
  auto *amp_gain_input = new QLineEdit(amp_props);
  auto *amp_load_input = new QLineEdit(amp_props);
  auto *amp_power_input = new QLineEdit(amp_props);
  auto *amp_headroom_input = new QLineEdit(amp_props);
  auto *amp_max_ripple_input = new QLineEdit(amp_props);
  amp_amp_input->setPlaceholderText("V (amplituda)");
  amp_freq_input->setPlaceholderText("Hz");
  amp_gain_input->setPlaceholderText("np. 10");
  amp_load_input->setPlaceholderText("Ohm");
  amp_power_input->setPlaceholderText("W");
  amp_headroom_input->setPlaceholderText("V");
  amp_max_ripple_input->setPlaceholderText("Vpp");
  amp_load_input->setObjectName("ampLoadInput");
  amp_power_input->setObjectName("ampPowerInput");
  amp_headroom_input->setObjectName("ampHeadroomInput");
  amp_max_ripple_input->setObjectName("ampMaxRippleInput");
  impl_->amp_waveform = amp_waveform;
  impl_->amp_design_mode = amp_design_mode;
  impl_->amp_power_source = amp_power_source;
  impl_->amp_amp_input = amp_amp_input;
  impl_->amp_freq_input = amp_freq_input;
  impl_->amp_gain_input = amp_gain_input;
  impl_->amp_load_input = amp_load_input;
  impl_->amp_power_input = amp_power_input;
  impl_->amp_headroom_input = amp_headroom_input;
  impl_->amp_max_ripple_input = amp_max_ripple_input;
  amp_header_form->addRow("Model wzmacniacza", amp_model);
  amp_header_form->addRow("Tryb projektu", amp_design_mode);
  amp_header_form->addRow("Źródło zasilania", amp_power_source);
  amp_header_form->addRow("Rodzaj przebiegu wejściowego", amp_waveform);
  amp_root->addWidget(amp_header);

  auto *amp_columns = new QWidget(amp_props);
  auto *amp_columns_layout = new QHBoxLayout(amp_columns);
  amp_columns_layout->setContentsMargins(0, 0, 0, 0);
  amp_columns_layout->setSpacing(12);
  auto *amp_signal_group = new QGroupBox("Parametry sygnału", amp_columns);
  auto *amp_signal_form = new QFormLayout(amp_signal_group);
  tune_form_layout(amp_signal_form);
  amp_signal_form->addRow("Amplituda wejściowa (V)", amp_amp_input);
  amp_signal_form->addRow("Częstotliwość sygnału (Hz)", amp_freq_input);
  amp_signal_form->addRow("Wzmocnienie napięciowe (V/V)", amp_gain_input);
  auto *amp_power_group = new QGroupBox("Obciążenie i zasilanie", amp_columns);
  auto *amp_power_form = new QFormLayout(amp_power_group);
  tune_form_layout(amp_power_form);
  amp_power_form->addRow("Obciążenie (Ohm)", amp_load_input);
  amp_power_form->addRow("Moc na obciążeniu (W)", amp_power_input);
  amp_power_form->addRow("Zapas napięcia zasilania (V)", amp_headroom_input);
  amp_power_form->addRow("Maksymalne tętnienia (Vpp)", amp_max_ripple_input);
  amp_columns_layout->addWidget(amp_signal_group, 1);
  amp_columns_layout->addWidget(amp_power_group, 1);
  amp_root->addWidget(amp_columns);
  impl_->amp_signal_group = amp_signal_group;
  impl_->amp_power_group = amp_power_group;

  props_stack->addWidget(power_props); // index 0
  props_stack->addWidget(amp_props);   // index 1

  auto *calculator_input_stack = new QStackedWidget(left);
  impl_->calculator_input_stack = calculator_input_stack;
  calculator_input_stack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  auto *power_calculator_tabs = new QTabWidget(left);
  impl_->power_calculator_tabs = power_calculator_tabs;
  power_calculator_tabs->setObjectName("powerCalculatorTabs");
  power_calculator_tabs->setDocumentMode(true);
  power_calculator_tabs->setElideMode(Qt::ElideNone);
  power_calculator_tabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  apply_lightweight_tabs_style(power_calculator_tabs);

  auto *transformer_module = new pep::ui::calculation::CalculationModuleWidget(power_calculator_tabs);
  transformer_module->form_layout()->addRow("Co chcesz obliczyć", transformer_mode);
  transformer_module->form_layout()->addRow("Przebieg napięcia", transformer_waveform);
  transformer_module->form_layout()->addRow("Napięcie uzwojenia pierwotnego",
                                            transformer_primary_with_quantity);
  transformer_module->form_layout()->addRow(transformer_ratio_label, transformer_ratio_input);
  transformer_module->form_layout()->addRow(transformer_secondary_label, transformer_secondary_with_hint);
  tune_form_layout(transformer_module->form_layout());
  impl_->power_compute_results.push_back(transformer_module->result_view());
  power_calculator_tabs->addTab(transformer_module, "Transformator");

  auto *rectifier_module = new pep::ui::calculation::CalculationModuleWidget(power_calculator_tabs);
  rectifier_module->form_layout()->addRow("Wtórne do prostownika (VAC RMS)", vin_input);
  rectifier_module->form_layout()->addRow("Spadek na jednej diodzie (V)", diode_drop_input);
  rectifier_module->form_layout()->addRow("Częstotliwość sieci (Hz)", freq_input);
  tune_form_layout(rectifier_module->form_layout());
  impl_->power_compute_results.push_back(rectifier_module->result_view());
  power_calculator_tabs->addTab(rectifier_module, "Prostownik");

  auto *filter_module = new pep::ui::calculation::CalculationModuleWidget(power_calculator_tabs);
  filter_module->form_layout()->addRow("Prąd obciążenia (A)", current_input);
  filter_module->form_layout()->addRow("Maksymalne tętnienia (Vpp)", max_ripple_input);
  filter_module->form_layout()->addRow("Pojemność kondensatora (uF)", cap_input);
  tune_form_layout(filter_module->form_layout());
  impl_->power_compute_results.push_back(filter_module->result_view());
  power_calculator_tabs->addTab(filter_module, "Filtracja");

  auto *load_module = new pep::ui::calculation::CalculationModuleWidget(power_calculator_tabs);
  auto *load_info = new QLabel(load_module);
  load_info->setWordWrap(true);
  load_info->setText(
      "Ten moduł pokazuje moc i obciążenie po filtracji. Prąd pobieramy z zakładki Filtracja.");
  load_module->form_layout()->addRow(load_info);
  tune_form_layout(load_module->form_layout());
  impl_->power_compute_results.push_back(load_module->result_view());
  power_calculator_tabs->addTab(load_module, "Obciążenie");

  calculator_input_stack->addWidget(power_calculator_tabs);

  auto *calculator_placeholder = new QLabel(left);
  calculator_placeholder->setWordWrap(true);
  calculator_placeholder->setText(
      "Ten wariant elementu będzie miał własne moduły kalkulatora. Po przełączeniu na zasilacz impulsowy nie pokazujemy pól liniowego transformatora i prostownika.");
  impl_->calculator_placeholder = calculator_placeholder;
  calculator_input_stack->addWidget(calculator_placeholder);

  auto *compute_result = new pep::ui::calculation::CalculationView(left);
  impl_->compute_result = compute_result;
  compute_result->setMinimumHeight(260);
  compute_result->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  auto *validation = new QLabel(left);
  validation->setWordWrap(true);
  impl_->validation = validation;

  auto *export_active = new QPushButton("Eksport aktywnego elementu do LtSpice (.asc)", right);
  auto *export_project = new QPushButton("Eksport całego układu do LtSpice (.asc)", right);

  auto *left_content = new QWidget(left);
  auto *left_content_layout = new QVBoxLayout(left_content);
  left_content_layout->setContentsMargins(kLeftPanelHorizontalPadding, 0,
                                          kLeftPanelHorizontalPadding, 0);
  left_content_layout->setSpacing(10);
  left_content_layout->setAlignment(Qt::AlignTop);
  left_content_layout->addWidget(props_stack);
  left_content_layout->addWidget(calculator_input_stack);
  left_content_layout->addWidget(compute_result, 1);

  auto *left_scroll = new QScrollArea(left);
  left_scroll->setWidgetResizable(true);
  left_scroll->setFrameShape(QFrame::NoFrame);
  left_scroll->setWidget(left_content);

  left_layout->addWidget(left_scroll, 1);
  // validation moved to Export tab

  auto *info_tab = new QWidget(right);
  auto *info_layout = new QVBoxLayout(info_tab);
  info_layout->addWidget(active_title);
  info_layout->addWidget(ports_label);
  info_layout->addStretch(1);

  auto *export_tab = new QWidget(right);
  auto *export_layout = new QVBoxLayout(export_tab);
  export_layout->addWidget(validation);
  export_layout->addWidget(export_active);
  export_layout->addWidget(export_project);
  export_layout->addStretch(1);

  auto *waveforms_tab = new QWidget(right);
  auto *waveforms_tab_layout = new QVBoxLayout(waveforms_tab);
  waveforms_tab_layout->addWidget(waveforms_box);

  auto *bottom_tabs = new QTabWidget(right);
  impl_->bottom_tabs = bottom_tabs;
  impl_->waveform_tab_index = bottom_tabs->addTab(waveforms_tab, "Wykresy");
  bottom_tabs->addTab(connections_tab, "Połączenia portów");
  bottom_tabs->addTab(info_tab, "Aktywny element");
  bottom_tabs->addTab(export_tab, "Eksport");

  auto *graph_help =
      new QLabel("Schemat blokowy (aby połączyć porty, kliknij najpierw pierwszy port, a potem "
                 "drugi; Ctrl+scroll = zoom; Ctrl+0 reset). "
                 "Blok zasilania jest widoczny na schemacie, ale nie jest łączony liniami. "
                 "Zasilanie wzmacniaczy wybiera się z listy, więc ich piny zasilania są ukryte.",
                 right);
  graph_help->setWordWrap(true);
  right_layout->addWidget(graph_help);

  auto *graph_legend = new QLabel("Legenda: porty sygnałowe są zielone, żółty oznacza wybrany "
                                  "port do połączenia. Linie zielone oznaczają poprawne "
                                  "połączenie, a czerwone niezgodne typy portów. Bloki z tym "
                                  "samym zasilaniem mają zbliżony kolor tła.",
                                  right);
  graph_legend->setWordWrap(true);
  right_layout->addWidget(graph_legend);
  right_layout->addWidget(canvas_toolbar);
  right_layout->addWidget(view);
  right_layout->addWidget(bottom_tabs);

  auto *splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setChildrenCollapsible(false);
  splitter->setHandleWidth(8);
  splitter->addWidget(left);
  splitter->addWidget(right);
  splitter->setStretchFactor(0, 6);
  splitter->setStretchFactor(1, 4);
  splitter->setSizes({840, 560});
  root->addWidget(splitter, 1);

  auto active_block = [this]() -> Block * {
    if (!impl_->active_block_id.has_value()) {
      return nullptr;
    }
    return find_block(impl_->blocks, *impl_->active_block_id);
  };

  auto refresh_connection_form = [this]() {
    pep::modules::project_design::refresh_connection_form(impl_->blocks, impl_->conn_from_block,
                                                          impl_->conn_to_block);
  };

  auto refresh_ports_combo = [this](QComboBox *block_combo, QComboBox *port_combo) {
    pep::modules::project_design::refresh_ports_combo(impl_->blocks, block_combo, port_combo);
  };

  auto refresh_power_source_combo = [this]() {
    pep::modules::project_design::refresh_power_source_combo(impl_->blocks,
                                                             impl_->amp_power_source);
  };

  auto refresh_connections_ui = [this]() {
    pep::modules::project_design::refresh_connections_ui(impl_->blocks, impl_->connections,
                                                         impl_->connections_list);
  };

  auto refresh_canvas = [this]() {
    pep::modules::project_design::refresh_canvas_scene(
        CanvasSceneState{impl_->scene, &impl_->signal_lines, &impl_->blocks, &impl_->connections,
                         impl_->active_block_id, impl_->pending_connection, impl_->manual_layout,
                         &impl_->refreshing_canvas, &impl_->signal_lines_update_scheduled, this,
                         [this](int, const QPointF &) { impl_->manual_layout = true; }});
  };

  auto sync_active_to_form = [this, active_block, refresh_power_source_combo]() {
    const FormWidgets widgets{
        impl_->props_stack,       impl_->power_family,   impl_->power_linear_variant,
        impl_->power_design_mode, impl_->transformer_mode,  impl_->transformer_waveform,
        impl_->transformer_voltage_quantity, impl_->variant,
        impl_->transformer_primary_input, impl_->transformer_primary_tol_input,
        impl_->transformer_ratio_input,
        impl_->transformer_secondary_input, impl_->vin_input, impl_->diode_drop_input,
        impl_->freq_input,
        impl_->current_input,     impl_->cap_input, impl_->max_ripple_input,
        impl_->amp_waveform,      impl_->amp_design_mode, impl_->amp_power_source,
        impl_->amp_amp_input,     impl_->amp_freq_input, impl_->amp_gain_input,
        impl_->amp_load_input,    impl_->amp_power_input, impl_->amp_headroom_input,
        impl_->amp_max_ripple_input};
    pep::modules::project_design::sync_active_to_form(
        active_block(), impl_->blocks, impl_->connections, widgets, &impl_->syncing_active_to_form,
        refresh_power_source_combo);
    const Block *active = active_block();
    if (active && active->kind == BlockKind::Amplifier &&
        active->amp_design_mode == AmpDesignMode::SupplyForAmp &&
        impl_->apply_amp_requirements_to_power) {
      impl_->apply_amp_requirements_to_power();
    }
    if (impl_->update_transformer_labels) {
      impl_->update_transformer_labels();
    }
    if (impl_->update_power_mode_visibility) {
      impl_->update_power_mode_visibility();
    }
    if (impl_->update_amp_mode_visibility) {
      impl_->update_amp_mode_visibility();
    }
    if (impl_->transformer_voltage_quantity) {
      impl_->last_transformer_voltage_quantity =
          static_cast<VoltageQuantity>(impl_->transformer_voltage_quantity->currentData().toInt());
    }
    if (impl_->update_calculator_panels) {
      impl_->update_calculator_panels();
    }
    if (impl_->update_props_stack_height) {
      impl_->update_props_stack_height();
    }
  };

  auto sync_form_to_active = [this, active_block]() {
    const FormWidgets widgets{
        impl_->props_stack,       impl_->power_family,   impl_->power_linear_variant,
        impl_->power_design_mode, impl_->transformer_mode,  impl_->transformer_waveform,
        impl_->transformer_voltage_quantity, impl_->variant,
        impl_->transformer_primary_input, impl_->transformer_primary_tol_input,
        impl_->transformer_ratio_input,
        impl_->transformer_secondary_input, impl_->vin_input, impl_->diode_drop_input,
        impl_->freq_input,
        impl_->current_input,     impl_->cap_input, impl_->max_ripple_input,
        impl_->amp_waveform,      impl_->amp_design_mode, impl_->amp_power_source,
        impl_->amp_amp_input,     impl_->amp_freq_input, impl_->amp_gain_input,
        impl_->amp_load_input,    impl_->amp_power_input, impl_->amp_headroom_input,
        impl_->amp_max_ripple_input};
    pep::modules::project_design::sync_form_to_active(active_block(), widgets);
  };

  auto recompute_and_validate = [this, active_block]() {
    const ActiveBlockWidgets widgets{
        impl_->active_title,          impl_->validation,
        impl_->ports_label,           impl_->compute_result, impl_->power_compute_results,
        impl_->waveform_in,           impl_->waveform_out,
        impl_->active_power_calculator_tab_index,
        impl_->bottom_tabs,           impl_->waveform_tab_index};
    const Block *active = active_block();
    if (active && active->kind == BlockKind::Power) {
      for (const auto &block : impl_->blocks) {
        if (block.kind != BlockKind::Amplifier ||
            block.amp_design_mode != AmpDesignMode::SupplyForAmp) {
          continue;
        }
        int psu_id = block.amp_power_source_id;
        if (psu_id == 0) {
          psu_id = pep::modules::project_design::connected_power_block_id(impl_->blocks,
                                                                          impl_->connections,
                                                                          block.id);
        }
        if (psu_id != active->id) {
          continue;
        }
        pep::modules::project_design::apply_amp_requirements_to_power(
            impl_->blocks, impl_->connections, block.id, psu_id);
      }
    }
    if (active && active->kind == BlockKind::Amplifier &&
        active->amp_design_mode == AmpDesignMode::SupplyForAmp &&
        impl_->apply_amp_requirements_to_power) {
      impl_->apply_amp_requirements_to_power();
    }
    pep::modules::project_design::recompute_and_validate(active_block(), impl_->blocks,
                                                         impl_->connections, widgets);
  };
  impl_->recompute_and_validate = recompute_and_validate;

  auto set_active = [this, active_block, sync_active_to_form, recompute_and_validate](int id) {
    impl_->active_block_id = id;
    Block *active = active_block();
    if (active && active->kind == BlockKind::Power) {
      for (const auto &block : impl_->blocks) {
        if (block.kind != BlockKind::Amplifier ||
            block.amp_design_mode != AmpDesignMode::SupplyForAmp) {
          continue;
        }
        int psu_id = block.amp_power_source_id;
        if (psu_id == 0) {
          psu_id = pep::modules::project_design::connected_power_block_id(impl_->blocks,
                                                                          impl_->connections,
                                                                          block.id);
        }
        if (psu_id != id) {
          continue;
        }
        pep::modules::project_design::apply_amp_requirements_to_power(
            impl_->blocks, impl_->connections, block.id, psu_id);
      }
    }
    sync_active_to_form();
    recompute_and_validate();
  };

  auto update_transformer_labels = [this]() {
    if (!impl_->transformer_ratio_label || !impl_->transformer_secondary_label ||
        !impl_->transformer_hint || !impl_->transformer_secondary_note ||
        !impl_->power_family || !impl_->power_linear_variant || !impl_->transformer_mode ||
        !impl_->transformer_voltage_quantity) {
      return;
    }

    const bool linear = impl_->power_family->currentData().toInt() == 0;
    const bool symmetric =
        impl_->power_linear_variant->currentData().toInt() == static_cast<int>(BlockVariant::PsuSymmetric);
    const bool solve_ratio = impl_->transformer_mode->currentData().toInt() ==
                             static_cast<int>(TransformerSolveMode::RatioFromSecondary);
    const bool peak = impl_->transformer_voltage_quantity->currentData().toInt() ==
                      static_cast<int>(VoltageQuantity::Peak);

    impl_->transformer_ratio_label->setText("Przekładnia transformatora (Np:Ns)");
    impl_->transformer_secondary_label->setText(
        QString("Wymagane napięcie wtórne (%1)").arg(peak ? "szczytowe" : "skuteczne RMS"));

    set_form_row_visible(impl_->transformer_ratio_label, impl_->transformer_ratio_input,
                         linear && !solve_ratio);
    set_form_row_visible(impl_->transformer_secondary_label, impl_->transformer_secondary_input,
                         linear && solve_ratio);

    const bool show_hint = linear && symmetric && solve_ratio;
    impl_->transformer_hint->setVisible(show_hint);
    impl_->transformer_hint->setText(
        show_hint
            ? "Transformator 2x12 V oznacza dwa oddzielne uzwojenia po 12 V. W tym polu wpisz 12 V."
            : "");

    const bool show_note = linear && solve_ratio;
    impl_->transformer_secondary_note->setVisible(show_note);
    impl_->transformer_secondary_note->setText(
        show_note
            ? "Vdc ≈ Vrms·1.414 − 2·Vd − ΔVpp/2 (po mostku i filtracji)."
            : "");
  };
  impl_->update_transformer_labels = update_transformer_labels;

  auto update_power_mode_visibility = [this]() {
    if (!impl_->power_design_mode || !impl_->power_calculator_tabs) {
      return;
    }
    auto *tabs = impl_->power_calculator_tabs;
    auto *bar = tabs->tabBar();
    const auto set_tab = [&](int index, bool visible) {
      if (index < 0 || index >= tabs->count()) {
        return;
      }
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      if (bar) {
        bar->setTabVisible(index, visible);
      }
#else
      tabs->setTabEnabled(index, visible);
#endif
    };
    for (int i = 0; i < tabs->count(); ++i) {
      set_tab(i, true);
    }
  };
  impl_->update_power_mode_visibility = update_power_mode_visibility;

  auto update_amp_mode_visibility = [this]() {
    if (!impl_->amp_design_mode || !impl_->amp_signal_group || !impl_->amp_power_group) {
      return;
    }
    const auto mode =
        static_cast<AmpDesignMode>(impl_->amp_design_mode->currentData().toInt());
    const bool show_signal = mode == AmpDesignMode::AmpForSupply ||
                             mode == AmpDesignMode::AllFields;
    const bool show_power = mode == AmpDesignMode::SupplyForAmp ||
                            mode == AmpDesignMode::AllFields;
    impl_->amp_signal_group->setVisible(show_signal);
    impl_->amp_power_group->setVisible(show_power);
    if (impl_->update_props_stack_height) {
      impl_->update_props_stack_height();
    }
  };
  impl_->update_amp_mode_visibility = update_amp_mode_visibility;

  auto update_calculator_panels = [this, active_block]() {
    if (!impl_->calculator_input_stack || !impl_->power_calculator_tabs || !impl_->compute_result ||
        !impl_->calculator_placeholder) {
      return;
    }

    const Block *active = active_block();
    if (!active || active->kind != BlockKind::Power) {
      impl_->calculator_placeholder->setText(
          "Ten element nie ma jeszcze osobnych modułów wejściowych w dolnej sekcji. Wyniki pozostają widoczne poniżej we wspólnym panelu kalkulatora.");
      impl_->calculator_input_stack->setVisible(false);
      impl_->compute_result->setVisible(true);
      impl_->calculator_input_stack->setCurrentIndex(1);
      return;
    }

    const bool linear = active->variant != BlockVariant::PsuSwitching;
    impl_->calculator_input_stack->setVisible(true);
    impl_->compute_result->setVisible(false);
    impl_->calculator_input_stack->setCurrentIndex(linear ? 0 : 1);
    impl_->power_calculator_tabs->setVisible(linear);
    if (linear) {
      impl_->active_power_calculator_tab_index = impl_->power_calculator_tabs->currentIndex();
      impl_->compute_result->set_active_section_index(impl_->power_calculator_tabs->currentIndex());
      if (impl_->update_power_mode_visibility) {
        impl_->update_power_mode_visibility();
      }
    } else {
      impl_->active_power_calculator_tab_index = 0;
      impl_->calculator_placeholder->setText(
          "Zasilacz impulsowy dostanie własne moduły kalkulatora. Nie pokazujemy tutaj wejść liniowego transformatora ani prostownika.");
      impl_->compute_result->set_active_section_index(0);
    }
  };
  impl_->update_calculator_panels = update_calculator_panels;

  auto update_props_stack_height = [this]() {
    if (!impl_->props_stack) {
      return;
    }

    QWidget *current = impl_->props_stack->currentWidget();
    if (!current) {
      impl_->props_stack->setMaximumHeight(QWIDGETSIZE_MAX);
      return;
    }

    current->adjustSize();
    const int target_height = current->sizeHint().height();
    impl_->props_stack->setMinimumHeight(target_height);
    impl_->props_stack->setMaximumHeight(target_height);
    impl_->props_stack->updateGeometry();
  };
  impl_->update_props_stack_height = update_props_stack_height;

  auto add_block = [this, refresh_canvas, refresh_connection_form, refresh_ports_combo, set_active,
                    recompute_and_validate, refresh_connections_ui,
                    refresh_power_source_combo](Block block) {
    const int id = block.id;
    const std::optional<int> prev_active = impl_->active_block_id;
    impl_->blocks.push_back(std::move(block));
    pep::modules::project_design::connect_new_block(impl_->connections, impl_->blocks,
                                                    impl_->pending_connection, prev_active, id);
    impl_->pending_connection.reset();

    refresh_connection_form();
    refresh_power_source_combo();
    refresh_ports_combo(impl_->conn_from_block, impl_->conn_from_port);
    refresh_ports_combo(impl_->conn_to_block, impl_->conn_to_port);
    set_active(id);
    refresh_connections_ui();
    recompute_and_validate();
    refresh_canvas();
  };

  auto refresh_connection_controls = [refresh_connection_form, refresh_ports_combo, conn_from_block,
                                      conn_from_port, conn_to_block, conn_to_port]() {
    refresh_connection_form();
    refresh_ports_combo(conn_from_block, conn_from_port);
    refresh_ports_combo(conn_to_block, conn_to_port);
  };

  auto add_power_block = [this, add_block]() {
    const int id = impl_->next_block_id++;
    add_block(make_power_block(id, QString("Zasilanie #%1").arg(id).toStdString(),
                               BlockVariant::PsuSymmetric));
  };
  auto add_switching_power_block = [this, add_block]() {
    const int id = impl_->next_block_id++;
    add_block(make_power_block(id, QString("Zasilanie #%1").arg(id).toStdString(),
                               BlockVariant::PsuSwitching));
  };
  auto add_amp_block = [this, add_block]() {
    const int id = impl_->next_block_id++;
    add_block(make_amp_model1b_block(id, QString("Wzmacniacz klasy B #%1").arg(id).toStdString()));
  };

  auto delete_selected_block = [this, refresh_connections_ui, refresh_connection_controls,
                                set_active, refresh_canvas, recompute_and_validate,
                                refresh_power_source_combo]() {
    pep::modules::project_design::delete_selected_block(
        impl_->scene, impl_->blocks, impl_->connections, impl_->active_block_id,
        refresh_connections_ui, refresh_power_source_combo, refresh_connection_controls, set_active,
        refresh_canvas, recompute_and_validate);
  };

  auto handle_scene_selection = [this, set_active]() {
    if (impl_->refreshing_canvas) {
      return;
    }
    const auto sel = impl_->scene->selectedItems();
    if (sel.empty()) {
      return;
    }
    auto *item = dynamic_cast<BlockItem *>(sel.front());
    if (!item) {
      return;
    }
    set_active(item->block_id());
  };

  auto handle_auto_layout = [this, refresh_canvas]() {
    impl_->manual_layout = false;
    refresh_canvas();
  };

  QObject::connect(action_add_power, &QAction::triggered, this,
                   [add_power_block]() { add_power_block(); });
  QObject::connect(action_add_switching, &QAction::triggered, this,
                   [add_switching_power_block]() { add_switching_power_block(); });
  QObject::connect(action_add_amp, &QAction::triggered, this,
                   [add_amp_block]() { add_amp_block(); });
  QObject::connect(btn_remove_selected, &QPushButton::clicked, this,
                   [delete_selected_block]() { delete_selected_block(); });

  auto handle_port_clicked = [this, refresh_connections_ui, recompute_and_validate,
                              refresh_canvas](int block_id, const std::string &port_id) {
    pep::modules::project_design::handle_port_click(
        impl_->pending_connection, impl_->validation, impl_->connections, block_id, port_id,
        refresh_connections_ui, recompute_and_validate, refresh_canvas);
  };

  const auto amp_to_power_mode = [](AmpDesignMode mode) {
    if (mode == AmpDesignMode::AmpForSupply) {
      return PowerDesignMode::LoadForSupply;
    }
    if (mode == AmpDesignMode::AllFields) {
      return PowerDesignMode::AllFields;
    }
    return PowerDesignMode::SupplyForLoad;
  };
  const auto power_to_amp_mode = [](PowerDesignMode mode) {
    if (mode == PowerDesignMode::LoadForSupply) {
      return AmpDesignMode::AmpForSupply;
    }
    if (mode == PowerDesignMode::AllFields) {
      return AmpDesignMode::AllFields;
    }
    return AmpDesignMode::SupplyForAmp;
  };

  auto handle_variant_changed = [=]() {
    sync_form_to_active();
    update_transformer_labels();
    update_calculator_panels();
    refresh_connection_controls();
    refresh_connections_ui();
    recompute_and_validate();
  };

  auto handle_power_family_changed = [=]() {
    sync_form_to_active();
    sync_active_to_form();
    update_transformer_labels();
    update_calculator_panels();
    refresh_connection_controls();
    refresh_connections_ui();
    recompute_and_validate();
  };

  auto handle_power_linear_variant_changed = [=]() {
    handle_variant_changed();
  };
  auto handle_power_design_mode_changed = [=, this]() {
    sync_form_to_active();
    const auto mode = power_to_amp_mode(active_block()->power_design_mode);
    for (auto &block : impl_->blocks) {
      if (block.kind == BlockKind::Amplifier) {
        block.amp_design_mode = mode;
      }
    }
    if (impl_->amp_design_mode) {
      const int idx = impl_->amp_design_mode->findData(static_cast<int>(mode));
      if (idx >= 0) {
        const QSignalBlocker blocker(*impl_->amp_design_mode);
        impl_->amp_design_mode->setCurrentIndex(idx);
      }
    }
    if (impl_->update_power_mode_visibility) {
      impl_->update_power_mode_visibility();
    }
    recompute_and_validate();
  };

  auto handle_transformer_mode_changed = [=]() {
    sync_form_to_active();
    sync_active_to_form();
    update_transformer_labels();
    recompute_and_validate();
  };

  auto handle_transformer_waveform_changed = [=]() {
    sync_form_to_active();
    sync_active_to_form();
    recompute_and_validate();
  };

  auto handle_transformer_voltage_quantity_changed = [=, this]() {
    if (!impl_->syncing_active_to_form) {
      const auto previous_quantity = impl_->last_transformer_voltage_quantity;
      const auto new_quantity =
          static_cast<VoltageQuantity>(impl_->transformer_voltage_quantity->currentData().toInt());
      const auto waveform =
          static_cast<SignalWaveform>(impl_->transformer_waveform->currentData().toInt());

      const auto convert_line_edit = [&](QLineEdit *edit) {
        if (!edit || edit->text().trimmed().isEmpty()) {
          return;
        }
        const double converted =
            convert_transformer_voltage_value(edit->text().toDouble(), previous_quantity,
                                              new_quantity, waveform);
        edit->setText(QString::number(converted, 'f', 3));
      };

      convert_line_edit(impl_->transformer_primary_input);
      convert_line_edit(impl_->transformer_secondary_input);
      impl_->last_transformer_voltage_quantity = new_quantity;
    }

    sync_form_to_active();
    sync_active_to_form();
    update_transformer_labels();
    recompute_and_validate();
  };

  auto handle_power_input_changed = [=, this]() {
    if (!impl_->syncing_filter_fields) {
      Block *active = active_block();
      if (active && active->kind == BlockKind::Power) {
        const double load_current = impl_->current_input
                                        ? impl_->current_input->text().toDouble()
                                        : 0.0;
        const double mains_hz =
            impl_->freq_input ? impl_->freq_input->text().toDouble() : 0.0;
        const double ripple_hz = mains_hz > 0.0 ? mains_hz * 2.0 : 0.0;

        const auto update_from_ripple = [&]() {
          if (!impl_->max_ripple_input || !impl_->cap_input) {
            return;
          }
          const double ripple_vpp = impl_->max_ripple_input->text().toDouble();
          if (load_current <= 0.0 || ripple_hz <= 0.0 || ripple_vpp <= 0.0) {
            return;
          }
          const double required_cap =
              pep::modules::psu_basic::required_capacitance_uF(load_current, ripple_vpp, ripple_hz);
          if (required_cap <= 0.0) {
            return;
          }
          const QSignalBlocker blocker(*impl_->cap_input);
          impl_->syncing_filter_fields = true;
          impl_->cap_input->setText(QString::number(required_cap, 'f', 1));
          impl_->syncing_filter_fields = false;
        };

        const auto update_from_cap = [&]() {
          if (!impl_->max_ripple_input || !impl_->cap_input) {
            return;
          }
          const double cap_uF = impl_->cap_input->text().toDouble();
          if (load_current <= 0.0 || ripple_hz <= 0.0 || cap_uF <= 0.0) {
            return;
          }
          const double ripple_vpp =
              load_current / (ripple_hz * cap_uF * 1e-6);
          if (ripple_vpp <= 0.0) {
            return;
          }
          const QSignalBlocker blocker(*impl_->max_ripple_input);
          impl_->syncing_filter_fields = true;
          impl_->max_ripple_input->setText(QString::number(ripple_vpp, 'f', 3));
          impl_->syncing_filter_fields = false;
        };

        if (impl_->last_filter_input == Impl::FilterInput::Ripple) {
          update_from_ripple();
        } else if (impl_->last_filter_input == Impl::FilterInput::Cap) {
          update_from_cap();
        }
      }
    }
    sync_form_to_active();
    recompute_and_validate();
  };

  QObject::connect(impl_->cap_input, &QLineEdit::textChanged, this, [=, this]() {
    if (!impl_->syncing_filter_fields) {
      impl_->last_filter_input = Impl::FilterInput::Cap;
      handle_power_input_changed();
    }
  });
  QObject::connect(impl_->max_ripple_input, &QLineEdit::textChanged, this, [=, this]() {
    if (!impl_->syncing_filter_fields) {
      impl_->last_filter_input = Impl::FilterInput::Ripple;
      handle_power_input_changed();
    }
  });

  auto apply_amp_requirements_to_power = [=, this]() {
    Block *active = active_block();
    if (!active || active->kind != BlockKind::Amplifier) {
      return;
    }

    int psu_id = 0;
    if (active->amp_power_source_id != 0) {
      psu_id = active->amp_power_source_id;
    } else if (impl_->amp_power_source) {
      psu_id = impl_->amp_power_source->currentData().toInt();
    }
    if (psu_id == 0) {
      psu_id = pep::modules::project_design::connected_power_block_id(impl_->blocks,
                                                                      impl_->connections,
                                                                      active->id);
      if (psu_id != 0 && impl_->amp_power_source) {
        const int idx = impl_->amp_power_source->findData(psu_id);
        if (idx >= 0) {
          const QSignalBlocker blocker(*impl_->amp_power_source);
          impl_->amp_power_source->setCurrentIndex(idx);
        }
      }
    }

    pep::modules::project_design::apply_amp_requirements_to_power(
        impl_->blocks, impl_->connections, active->id, psu_id);
  };
  impl_->apply_amp_requirements_to_power = apply_amp_requirements_to_power;

  auto handle_amp_power_source_changed = [=, this]() {
    if (impl_->refreshing_canvas) {
      return;
    }
    if (impl_->syncing_active_to_form) {
      return;
    }
    Block *active = active_block();
    if (!active || active->kind == BlockKind::Power) {
      return;
    }

    sync_form_to_active();
    const int psu_id = impl_->amp_power_source->currentData().toInt();
    pep::modules::project_design::remap_block_power_connections(impl_->blocks, impl_->connections,
                                                                active->id, psu_id);
    apply_amp_requirements_to_power();

    refresh_connections_ui();
    refresh_connection_controls();
    recompute_and_validate();
    refresh_canvas();
  };
  auto handle_amp_design_mode_changed = [=, this]() {
    sync_form_to_active();
    const auto mode = amp_to_power_mode(active_block()->amp_design_mode);
    for (auto &block : impl_->blocks) {
      if (block.kind == BlockKind::Power) {
        block.power_design_mode = mode;
      }
    }
    if (impl_->power_design_mode) {
      const int idx = impl_->power_design_mode->findData(static_cast<int>(mode));
      if (idx >= 0) {
        const QSignalBlocker blocker(*impl_->power_design_mode);
        impl_->power_design_mode->setCurrentIndex(idx);
      }
    }
    if (impl_->update_amp_mode_visibility) {
      impl_->update_amp_mode_visibility();
    }
    apply_amp_requirements_to_power();
    recompute_and_validate();
  };
  auto handle_amp_waveform_changed = [=, this]() {
    sync_form_to_active();
    apply_amp_requirements_to_power();
    recompute_and_validate();
  };
  auto handle_amp_amp_changed = [=, this]() {
    sync_form_to_active();
    apply_amp_requirements_to_power();
    recompute_and_validate();
  };
  auto handle_amp_freq_changed = [=, this]() {
    sync_form_to_active();
    recompute_and_validate();
  };
  auto handle_amp_gain_changed = [=, this]() {
    sync_form_to_active();
    apply_amp_requirements_to_power();
    recompute_and_validate();
  };
  auto handle_amp_specs_changed = [=, this]() {
    sync_form_to_active();
    apply_amp_requirements_to_power();
    recompute_and_validate();
  };

  auto handle_add_connection = [=, this]() {
    if (!pep::modules::project_design::add_connection_from_form(impl_->blocks, impl_->connections,
                                                                conn_from_block, conn_from_port,
                                                                conn_to_block, conn_to_port)) {
      return;
    }
    refresh_connections_ui();
    recompute_and_validate();
  };

  auto handle_remove_connection = [=, this]() {
    if (!pep::modules::project_design::remove_selected_connection(impl_->connections,
                                                                  impl_->connections_list)) {
      return;
    }
    refresh_connections_ui();
    recompute_and_validate();
  };

  auto handle_export_active = [=, this]() {
    pep::modules::project_design::export_active_block(this, impl_->active_block_id, impl_->blocks,
                                                      impl_->connections);
  };

  auto handle_export_project = [=, this]() {
    export_ltspice_asc(this, impl_->blocks, impl_->connections);
  };

  bind_widget_interactions(WidgetBindings{this,
                                          scene,
                                          view,
                                          nullptr,
                                          nullptr,
                                          btn_auto_layout,
                                          add_conn,
                                          remove_conn,
                                          export_active,
                                          export_project,
                                          power_family,
                                          power_linear_variant,
                                          power_design_mode,
                                          transformer_mode,
                                          transformer_waveform,
                                          transformer_voltage_quantity,
                                          variant,
                                          impl_->amp_waveform,
                                          impl_->amp_design_mode,
                                          impl_->amp_power_source,
                                          conn_from_block,
                                          conn_from_port,
                                          conn_to_block,
                                          conn_to_port,
                                          transformer_primary_input,
                                          transformer_primary_tol_input,
                                          transformer_ratio_input,
                                          transformer_secondary_input,
                                          vin_input,
                                          diode_drop_input,
                                          freq_input,
                                          current_input,
                                          cap_input,
                                          max_ripple_input,
                                          impl_->amp_amp_input,
                                          impl_->amp_freq_input,
                                          impl_->amp_gain_input,
                                          impl_->amp_load_input,
                                          impl_->amp_power_input,
                                          impl_->amp_headroom_input,
                                          impl_->amp_max_ripple_input,
                                          add_power_block,
                                          add_amp_block,
                                          delete_selected_block,
                                          handle_auto_layout,
                                          handle_port_clicked,
                                          handle_scene_selection,
                                          handle_power_family_changed,
                                          handle_power_linear_variant_changed,
                                          handle_power_design_mode_changed,
                                          handle_transformer_mode_changed,
                                          handle_transformer_waveform_changed,
                                          handle_transformer_voltage_quantity_changed,
                                          handle_variant_changed,
                                          handle_power_input_changed,
                                          handle_amp_power_source_changed,
                                          handle_amp_waveform_changed,
                                          handle_amp_design_mode_changed,
                                          handle_amp_amp_changed,
                                          handle_amp_freq_changed,
                                          handle_amp_gain_changed,
                                          handle_amp_specs_changed,
                                          refresh_ports_combo,
                                          handle_add_connection,
                                          handle_remove_connection,
                                          handle_export_active,
                                          handle_export_project});

  QObject::connect(power_calculator_tabs, &QTabWidget::currentChanged, this,
                   [this](int index) {
                     impl_->active_power_calculator_tab_index = index;
                     if (impl_->compute_result) {
                       impl_->compute_result->set_active_section_index(index);
                     }
                     if (impl_->recompute_and_validate) {
                       impl_->recompute_and_validate();
                     }
                   });
  QObject::connect(props_stack, &QStackedWidget::currentChanged, this, [this](int) {
    if (impl_->update_props_stack_height) {
      impl_->update_props_stack_height();
    }
  });

  refresh_connection_form();
  refresh_ports_combo(conn_from_block, conn_from_port);
  refresh_ports_combo(conn_to_block, conn_to_port);
  refresh_connections_ui();
  refresh_canvas();
  sync_active_to_form();
  update_transformer_labels();
  if (impl_->update_amp_mode_visibility) {
    impl_->update_amp_mode_visibility();
  }
  update_calculator_panels();
  update_props_stack_height();
  recompute_and_validate();
}

Widget::~Widget() {
  if (impl_ && impl_->scene) {
    QObject::disconnect(impl_->scene, nullptr, this, nullptr);
  }
}

} // namespace pep::modules::project_design
