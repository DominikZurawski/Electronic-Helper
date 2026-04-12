#pragma once

#include <functional>
#include <vector>

class QComboBox;
class QLineEdit;
class QStackedWidget;

namespace pep::modules::project_design {

struct Block;
struct Connection;

struct FormWidgets {
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
  QLineEdit *cap_tol_input = nullptr;
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
};

void sync_active_to_form(const Block *active, const std::vector<Block> &blocks,
                         const std::vector<Connection> &connections, const FormWidgets &widgets,
                         bool *sync_flag, const std::function<void()> &refresh_power_source_combo);

void sync_form_to_active(Block *active, const FormWidgets &widgets);

} // namespace pep::modules::project_design
