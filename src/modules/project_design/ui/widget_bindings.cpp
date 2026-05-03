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

  if (bindings.power_family && bindings.on_power_family_changed) {
    QObject::connect(
        bindings.power_family, &QComboBox::currentIndexChanged, bindings.owner,
        [on_power_family_changed = bindings.on_power_family_changed]() { on_power_family_changed(); });
  }

  if (bindings.power_linear_variant && bindings.on_power_linear_variant_changed) {
    QObject::connect(bindings.power_linear_variant, &QComboBox::currentIndexChanged, bindings.owner,
                     [on_power_linear_variant_changed = bindings.on_power_linear_variant_changed]() {
                       on_power_linear_variant_changed();
                     });
  }
  if (bindings.power_design_mode && bindings.on_power_design_mode_changed) {
    QObject::connect(bindings.power_design_mode, &QComboBox::currentIndexChanged, bindings.owner,
                     [on_power_design_mode_changed = bindings.on_power_design_mode_changed]() {
                       on_power_design_mode_changed();
                     });
  }

  if (bindings.transformer_mode && bindings.on_transformer_mode_changed) {
    QObject::connect(bindings.transformer_mode, &QComboBox::currentIndexChanged, bindings.owner,
                     [on_transformer_mode_changed = bindings.on_transformer_mode_changed]() {
                       on_transformer_mode_changed();
                     });
  }

  if (bindings.transformer_waveform && bindings.on_transformer_waveform_changed) {
    QObject::connect(bindings.transformer_waveform, &QComboBox::currentIndexChanged, bindings.owner,
                     [on_transformer_waveform_changed = bindings.on_transformer_waveform_changed]() {
                       on_transformer_waveform_changed();
                     });
  }

  if (bindings.transformer_voltage_quantity && bindings.on_transformer_voltage_quantity_changed) {
    QObject::connect(
        bindings.transformer_voltage_quantity, &QComboBox::currentIndexChanged, bindings.owner,
        [on_transformer_voltage_quantity_changed = bindings.on_transformer_voltage_quantity_changed]() {
          on_transformer_voltage_quantity_changed();
        });
  }

  if (bindings.variant && bindings.on_variant_changed) {
    QObject::connect(
        bindings.variant, &QComboBox::currentIndexChanged, bindings.owner,
        [on_variant_changed = bindings.on_variant_changed]() { on_variant_changed(); });
  }
  if (bindings.regulator_variant && bindings.on_regulator_variant_changed) {
    QObject::connect(
        bindings.regulator_variant, &QComboBox::currentIndexChanged, bindings.owner,
        [on_regulator_variant_changed = bindings.on_regulator_variant_changed]() {
          on_regulator_variant_changed();
        });
  }
  if (bindings.regulator_power_source && bindings.on_regulator_power_source_changed) {
    QObject::connect(
        bindings.regulator_power_source, &QComboBox::currentIndexChanged, bindings.owner,
        [on_regulator_power_source_changed = bindings.on_regulator_power_source_changed]() {
          on_regulator_power_source_changed();
        });
  }
  if (bindings.regulator_supply_rail && bindings.on_regulator_supply_rail_changed) {
    QObject::connect(
        bindings.regulator_supply_rail, &QComboBox::currentIndexChanged, bindings.owner,
        [on_regulator_supply_rail_changed = bindings.on_regulator_supply_rail_changed]() {
          on_regulator_supply_rail_changed();
        });
  }

  const auto connect_line_edit = [&bindings](QLineEdit *line_edit,
                                             const std::function<void()> &handler) {
    if (!line_edit || !handler) {
      return;
    }
    QObject::connect(line_edit, &QLineEdit::textChanged, bindings.owner,
                     [handler]() { handler(); });
  };
  connect_line_edit(bindings.transformer_primary_input, bindings.on_power_input_changed);
  connect_line_edit(bindings.transformer_primary_tol_input, bindings.on_power_input_changed);
  connect_line_edit(bindings.transformer_ratio_input, bindings.on_power_input_changed);
  connect_line_edit(bindings.transformer_secondary_input, bindings.on_power_input_changed);
  connect_line_edit(bindings.vin_input, bindings.on_power_input_changed);
  connect_line_edit(bindings.diode_drop_input, bindings.on_power_input_changed);
  connect_line_edit(bindings.freq_input, bindings.on_power_input_changed);
  connect_line_edit(bindings.current_input, bindings.on_power_input_changed);
  connect_line_edit(bindings.cap_input, bindings.on_power_input_changed);
  connect_line_edit(bindings.cap_tol_input, bindings.on_power_input_changed);
  connect_line_edit(bindings.regulator_output_input, bindings.on_regulator_input_changed);
  connect_line_edit(bindings.regulator_current_input, bindings.on_regulator_input_changed);
  connect_line_edit(bindings.regulator_zener_current_input, bindings.on_regulator_input_changed);
  connect_line_edit(bindings.regulator_dropout_input, bindings.on_regulator_input_changed);
  connect_line_edit(bindings.amp_amp_input, bindings.on_amp_amp_changed);
  connect_line_edit(bindings.amp_freq_input, bindings.on_amp_freq_changed);
  connect_line_edit(bindings.amp_gain_input, bindings.on_amp_gain_changed);
  connect_line_edit(bindings.amp_load_input, bindings.on_amp_specs_changed);
  connect_line_edit(bindings.amp_power_input, bindings.on_amp_specs_changed);
  connect_line_edit(bindings.amp_headroom_input, bindings.on_amp_specs_changed);
  connect_line_edit(bindings.amp_psrr_input, bindings.on_amp_specs_changed);
  connect_line_edit(bindings.amp_disturbance_rejection_input, bindings.on_amp_specs_changed);
  connect_line_edit(bindings.amp_disturbance_freq_input, bindings.on_amp_specs_changed);
  connect_line_edit(bindings.amp_cap_esr_input, bindings.on_amp_specs_changed);
  connect_line_edit(bindings.amp_transformer_res_input, bindings.on_amp_specs_changed);
  connect_line_edit(bindings.amp_max_ripple_input, bindings.on_amp_specs_changed);

  if (bindings.amp_power_source_pos && bindings.on_amp_power_source_pos_changed) {
    QObject::connect(bindings.amp_power_source_pos, &QComboBox::currentIndexChanged, bindings.owner,
                     [on_amp_power_source_pos_changed = bindings.on_amp_power_source_pos_changed]() {
                       on_amp_power_source_pos_changed();
                     });
  }
  if (bindings.amp_power_source_neg && bindings.on_amp_power_source_neg_changed) {
    QObject::connect(bindings.amp_power_source_neg, &QComboBox::currentIndexChanged, bindings.owner,
                     [on_amp_power_source_neg_changed = bindings.on_amp_power_source_neg_changed]() {
                       on_amp_power_source_neg_changed();
                     });
  }
  if (bindings.amp_design_mode && bindings.on_amp_design_mode_changed) {
    QObject::connect(bindings.amp_design_mode, &QComboBox::currentIndexChanged, bindings.owner,
                     [on_amp_design_mode_changed = bindings.on_amp_design_mode_changed]() {
                       on_amp_design_mode_changed();
                     });
  }
  if (bindings.amp_waveform && bindings.on_amp_waveform_changed) {
    QObject::connect(bindings.amp_waveform, &QComboBox::currentIndexChanged, bindings.owner,
                     [on_amp_waveform_changed = bindings.on_amp_waveform_changed]() {
                       on_amp_waveform_changed();
                     });
  }

  if (bindings.conn_from_block && bindings.conn_from_port && bindings.refresh_ports_combo) {
    auto *from_block = bindings.conn_from_block;
    auto *from_port = bindings.conn_from_port;
    const auto refresh_ports_combo = bindings.refresh_ports_combo;
    QObject::connect(from_block, &QComboBox::currentIndexChanged, bindings.owner,
                     [from_block, from_port, refresh_ports_combo]() {
                       refresh_ports_combo(from_block, from_port);
                     });
  }
  if (bindings.conn_to_block && bindings.conn_to_port && bindings.refresh_ports_combo) {
    auto *to_block = bindings.conn_to_block;
    auto *to_port = bindings.conn_to_port;
    const auto refresh_ports_combo = bindings.refresh_ports_combo;
    QObject::connect(
        to_block, &QComboBox::currentIndexChanged, bindings.owner,
        [to_block, to_port, refresh_ports_combo]() { refresh_ports_combo(to_block, to_port); });
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
