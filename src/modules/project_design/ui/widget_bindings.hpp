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

  QComboBox *variant = nullptr;
  QComboBox *amp_waveform = nullptr;
  QComboBox *amp_power_source = nullptr;
  QComboBox *conn_from_block = nullptr;
  QComboBox *conn_from_port = nullptr;
  QComboBox *conn_to_block = nullptr;
  QComboBox *conn_to_port = nullptr;

  QLineEdit *vin_input = nullptr;
  QLineEdit *freq_input = nullptr;
  QLineEdit *current_input = nullptr;
  QLineEdit *cap_input = nullptr;
  QLineEdit *extra_rails_input = nullptr;
  QLineEdit *amp_amp_input = nullptr;
  QLineEdit *amp_freq_input = nullptr;
  QLineEdit *amp_gain_input = nullptr;

  std::function<void()> on_add_power;
  std::function<void()> on_add_amp;
  std::function<void()> on_delete_selected;
  std::function<void()> on_auto_layout;
  std::function<void(int, const std::string &)> on_port_clicked;
  std::function<void()> on_scene_selection_changed;

  std::function<void()> on_variant_changed;
  std::function<void()> on_power_input_changed;
  std::function<void()> on_extra_rails_changed;
  std::function<void()> on_amp_power_source_changed;
  std::function<void()> on_amp_waveform_changed;
  std::function<void()> on_amp_amp_changed;
  std::function<void()> on_amp_freq_changed;
  std::function<void()> on_amp_gain_changed;

  std::function<void(QComboBox *, QComboBox *)> refresh_ports_combo;
  std::function<void()> on_add_connection;
  std::function<void()> on_remove_connection;
  std::function<void()> on_export_active;
  std::function<void()> on_export_project;
};

void bind_widget_interactions(const WidgetBindings &bindings);

} // namespace pep::modules::project_design
