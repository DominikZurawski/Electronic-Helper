#pragma once

#include <QString>

#include <functional>
#include <string>

class QComboBox;
class QGraphicsScene;
class QLineEdit;
class QObject;
class QPushButton;

namespace pep::modules::project_design {

class CanvasView;

struct WidgetBindings {
  QObject *owner = nullptr;
  QGraphicsScene *scene = nullptr;
  CanvasView *view = nullptr;

  QPushButton *btn_add_power = nullptr;
  QPushButton *btn_add_amp = nullptr;
  QPushButton *btn_auto_layout = nullptr;
  QPushButton *add_conn = nullptr;
  QPushButton *remove_conn = nullptr;
  QPushButton *export_active = nullptr;
  QPushButton *export_project = nullptr;

  QComboBox *power_family = nullptr;
  QComboBox *power_linear_variant = nullptr;
  QComboBox *power_design_mode = nullptr;
  QComboBox *transformer_mode = nullptr;
  QComboBox *transformer_waveform = nullptr;
  QComboBox *transformer_voltage_quantity = nullptr;
  QComboBox *variant = nullptr;
  QComboBox *regulator_variant = nullptr;
  QComboBox *regulator_power_source = nullptr;
  QComboBox *regulator_supply_rail = nullptr;
  QComboBox *amp_waveform = nullptr;
  QComboBox *amp_design_mode = nullptr;
  QComboBox *amp_power_source_pos = nullptr;
  QComboBox *amp_power_source_neg = nullptr;
  QComboBox *conn_from_block = nullptr;
  QComboBox *conn_from_port = nullptr;
  QComboBox *conn_to_block = nullptr;
  QComboBox *conn_to_port = nullptr;

  QLineEdit *transformer_primary_input = nullptr;
  QLineEdit *transformer_primary_tol_input = nullptr;
  QLineEdit *transformer_ratio_input = nullptr;
  QLineEdit *transformer_secondary_input = nullptr;
  QLineEdit *vin_input = nullptr;
  QLineEdit *diode_drop_input = nullptr;
  QLineEdit *freq_input = nullptr;
  QLineEdit *current_input = nullptr;
  QLineEdit *cap_input = nullptr;
  QLineEdit *cap_tol_input = nullptr;
  QLineEdit *max_ripple_input = nullptr;
  QLineEdit *regulator_source_min_input = nullptr;
  QLineEdit *regulator_input_min_input = nullptr;
  QLineEdit *regulator_margin_input = nullptr;
  QLineEdit *regulator_output_input = nullptr;
  QLineEdit *regulator_current_input = nullptr;
  QLineEdit *regulator_zener_current_input = nullptr;
  QLineEdit *regulator_dropout_input = nullptr;
  QLineEdit *amp_amp_input = nullptr;
  QLineEdit *amp_freq_input = nullptr;
  QLineEdit *amp_gain_input = nullptr;
  QLineEdit *amp_load_input = nullptr;
  QLineEdit *amp_power_input = nullptr;
  QLineEdit *amp_headroom_input = nullptr;
  QLineEdit *amp_psrr_input = nullptr;
  QLineEdit *amp_disturbance_rejection_input = nullptr;
  QLineEdit *amp_disturbance_freq_input = nullptr;
  QLineEdit *amp_cap_esr_input = nullptr;
  QLineEdit *amp_transformer_res_input = nullptr;
  QLineEdit *amp_max_ripple_input = nullptr;

  std::function<void()> on_add_power;
  std::function<void()> on_add_amp;
  std::function<void()> on_delete_selected;
  std::function<void()> on_auto_layout;
  std::function<void(int, const std::string &)> on_port_clicked;
  std::function<void()> on_scene_selection_changed;

  std::function<void()> on_power_family_changed;
  std::function<void()> on_power_linear_variant_changed;
  std::function<void()> on_power_design_mode_changed;
  std::function<void()> on_transformer_mode_changed;
  std::function<void()> on_transformer_waveform_changed;
  std::function<void()> on_transformer_voltage_quantity_changed;
  std::function<void()> on_variant_changed;
  std::function<void()> on_regulator_variant_changed;
  std::function<void()> on_regulator_power_source_changed;
  std::function<void()> on_regulator_supply_rail_changed;
  std::function<void()> on_regulator_input_changed;
  std::function<void()> on_power_input_changed;
  std::function<void()> on_amp_power_source_pos_changed;
  std::function<void()> on_amp_power_source_neg_changed;
  std::function<void()> on_amp_waveform_changed;
  std::function<void()> on_amp_design_mode_changed;
  std::function<void()> on_amp_amp_changed;
  std::function<void()> on_amp_freq_changed;
  std::function<void()> on_amp_gain_changed;
  std::function<void()> on_amp_specs_changed;

  std::function<void(QComboBox *, QComboBox *)> refresh_ports_combo;
  std::function<void()> on_add_connection;
  std::function<void()> on_remove_connection;
  std::function<void()> on_export_active;
  std::function<void()> on_export_project;
};

void bind_widget_interactions(const WidgetBindings &bindings);

} // namespace pep::modules::project_design
