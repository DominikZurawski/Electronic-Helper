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
  QComboBox *variant = nullptr;
  QLineEdit *vin_input = nullptr;
  QLineEdit *freq_input = nullptr;
  QLineEdit *current_input = nullptr;
  QLineEdit *cap_input = nullptr;
  QLineEdit *extra_rails_input = nullptr;
  QComboBox *amp_waveform = nullptr;
  QComboBox *amp_power_source = nullptr;
  QLineEdit *amp_amp_input = nullptr;
  QLineEdit *amp_freq_input = nullptr;
  QLineEdit *amp_gain_input = nullptr;
};

void sync_active_to_form(const Block *active, const std::vector<Block> &blocks,
                         const std::vector<Connection> &connections, const FormWidgets &widgets,
                         bool *sync_flag, const std::function<void()> &refresh_power_source_combo);

void sync_form_to_active(Block *active, const FormWidgets &widgets);

} // namespace pep::modules::project_design
