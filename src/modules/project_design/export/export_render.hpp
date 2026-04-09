#pragma once

#include "export.hpp"
#include "export_topology.hpp"

#include "../model/model.hpp"

#include <optional>
#include <set>
#include <string>
#include <unordered_map>

namespace pep::modules::project_design {

struct ExportLayoutConfig {
  int gap_x = 180;
  int gap_y = 160;
  int pad_x = 60;
  int pad_y = 60;
};

struct ExportSheetState {
  std::vector<std::string> lines;
  std::set<std::string> seen_directives;
  int global_max_x = 0;
  int global_max_y = 0;
};

void initialize_sheet(ExportSheetState &sheet);
void finalize_sheet(ExportSheetState &sheet, const std::optional<std::string> &tran_directive);

void append_missing_element_note(AscAssembly &out, ExportSheetState &sheet, int row_cursor_x,
                                 int row_y, int &row_cursor_x_out, int &row_max_h,
                                 const ExportLayoutConfig &layout, const std::string &note_message,
                                 const std::string &warning_message);

void append_export_fragment(AscAssembly &out, ExportSheetState &sheet, int dx, int dy,
                            int &row_cursor_x, int &row_max_h, const ExportLayoutConfig &layout,
                            const std::string &asc_with_header,
                            const std::unordered_map<long long, std::string> &xy_to_net,
                            const std::string &inst_suffix);

std::unordered_map<long long, std::string> build_block_flag_nets(const Block &block,
                                                                 const ExportTopology &topology);

} // namespace pep::modules::project_design
