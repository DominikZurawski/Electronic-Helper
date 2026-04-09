#include "widget_bindings.hpp"

#include "../graph/graph.hpp"

#include <QComboBox>
#include <QGraphicsScene>
#include <QLineEdit>
#include <QPushButton>

namespace pep::modules::project_design {

void bind_widget_interactions(const WidgetBindings &bindings) {
  if (!bindings.owner) {
    return;
  }

  if (bindings.view) {
    bindings.view->on_add_power = bindings.on_add_power;
    bindings.view->on_add_amp_model1b = bindings.on_add_amp;
    bindings.view->on_delete_selected = bindings.on_delete_selected;
    bindings.view->on_port_clicked = bindings.on_port_clicked;
  }

  if (bindings.scene && bindings.on_scene_selection_changed) {
    QObject::connect(bindings.scene, &QGraphicsScene::selectionChanged, bindings.owner,
                     [on_scene_selection_changed = bindings.on_scene_selection_changed]() {
                       on_scene_selection_changed();
                     });
  }

  if (bindings.btn_add_power && bindings.on_add_power) {
    QObject::connect(bindings.btn_add_power, &QPushButton::clicked, bindings.owner,
                     [on_add_power = bindings.on_add_power]() { on_add_power(); });
  }
  if (bindings.btn_add_amp && bindings.on_add_amp) {
    QObject::connect(bindings.btn_add_amp, &QPushButton::clicked, bindings.owner,
                     [on_add_amp = bindings.on_add_amp]() { on_add_amp(); });
  }
  if (bindings.btn_auto_layout && bindings.on_auto_layout) {
    QObject::connect(bindings.btn_auto_layout, &QPushButton::clicked, bindings.owner,
                     [on_auto_layout = bindings.on_auto_layout]() { on_auto_layout(); });
  }

  if (bindings.variant && bindings.on_variant_changed) {
    QObject::connect(
        bindings.variant, &QComboBox::currentIndexChanged, bindings.owner,
        [on_variant_changed = bindings.on_variant_changed]() { on_variant_changed(); });
  }

  const auto connect_line_edit = [&bindings](QLineEdit *line_edit,
                                             const std::function<void()> &handler) {
    if (!line_edit || !handler) {
      return;
    }
    QObject::connect(line_edit, &QLineEdit::textChanged, bindings.owner,
                     [handler]() { handler(); });
  };
  connect_line_edit(bindings.vin_input, bindings.on_power_input_changed);
  connect_line_edit(bindings.freq_input, bindings.on_power_input_changed);
  connect_line_edit(bindings.current_input, bindings.on_power_input_changed);
  connect_line_edit(bindings.cap_input, bindings.on_power_input_changed);
  connect_line_edit(bindings.extra_rails_input, bindings.on_extra_rails_changed);
  connect_line_edit(bindings.amp_amp_input, bindings.on_amp_amp_changed);
  connect_line_edit(bindings.amp_freq_input, bindings.on_amp_freq_changed);
  connect_line_edit(bindings.amp_gain_input, bindings.on_amp_gain_changed);

  if (bindings.amp_power_source && bindings.on_amp_power_source_changed) {
    QObject::connect(bindings.amp_power_source, &QComboBox::currentIndexChanged, bindings.owner,
                     [on_amp_power_source_changed = bindings.on_amp_power_source_changed]() {
                       on_amp_power_source_changed();
                     });
  }
  if (bindings.amp_waveform && bindings.on_amp_waveform_changed) {
    QObject::connect(bindings.amp_waveform, &QComboBox::currentIndexChanged, bindings.owner,
                     [on_amp_waveform_changed = bindings.on_amp_waveform_changed]() {
                       on_amp_waveform_changed();
                     });
  }

  if (bindings.conn_from_block && bindings.conn_from_port && bindings.refresh_ports_combo) {
    QObject::connect(
        bindings.conn_from_block, &QComboBox::currentIndexChanged, bindings.owner, [&bindings]() {
          bindings.refresh_ports_combo(bindings.conn_from_block, bindings.conn_from_port);
        });
  }
  if (bindings.conn_to_block && bindings.conn_to_port && bindings.refresh_ports_combo) {
    QObject::connect(bindings.conn_to_block, &QComboBox::currentIndexChanged, bindings.owner,
                     [&bindings]() {
                       bindings.refresh_ports_combo(bindings.conn_to_block, bindings.conn_to_port);
                     });
  }

  if (bindings.add_conn && bindings.on_add_connection) {
    QObject::connect(bindings.add_conn, &QPushButton::clicked, bindings.owner,
                     [on_add_connection = bindings.on_add_connection]() { on_add_connection(); });
  }
  if (bindings.remove_conn && bindings.on_remove_connection) {
    QObject::connect(
        bindings.remove_conn, &QPushButton::clicked, bindings.owner,
        [on_remove_connection = bindings.on_remove_connection]() { on_remove_connection(); });
  }

  if (bindings.export_active && bindings.on_export_active) {
    QObject::connect(bindings.export_active, &QPushButton::clicked, bindings.owner,
                     [on_export_active = bindings.on_export_active]() { on_export_active(); });
  }
  if (bindings.export_project && bindings.on_export_project) {
    QObject::connect(bindings.export_project, &QPushButton::clicked, bindings.owner,
                     [on_export_project = bindings.on_export_project]() { on_export_project(); });
  }
}

} // namespace pep::modules::project_design
