#pragma once

#include "../../psu_basic/ui/waveform_widget.hpp"

#include <vector>

class QLabel;
class QTabWidget;

namespace pep::modules::project_design {

struct Block;
struct Connection;

struct ActiveBlockWidgets {
  QLabel *active_title = nullptr;
  QLabel *compute_result = nullptr;
  QLabel *validation = nullptr;
  QLabel *ports_label = nullptr;
  pep::modules::psu_basic::WaveformWidget *waveform_in = nullptr;
  pep::modules::psu_basic::WaveformWidget *waveform_out = nullptr;
  QTabWidget *bottom_tabs = nullptr;
  int waveform_tab_index = -1;
};

void recompute_and_validate(const Block *active, const std::vector<Block> &blocks,
                            const std::vector<Connection> &connections,
                            const ActiveBlockWidgets &widgets);

} // namespace pep::modules::project_design
