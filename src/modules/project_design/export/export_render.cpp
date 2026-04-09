#include "export_render.hpp"

#include "asc_ops.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_set>

namespace pep::modules::project_design {

void initialize_sheet(ExportSheetState &sheet) {
  sheet.lines.clear();
  sheet.seen_directives.clear();
  sheet.global_max_x = 0;
  sheet.global_max_y = 0;
  sheet.lines.push_back("Version 4.1");
  sheet.lines.push_back("SHEET 1 3200 1800");
}

void finalize_sheet(ExportSheetState &sheet, const std::optional<std::string> &tran_directive) {
  const int sheet_w = std::max(1416, sheet.global_max_x + 400);
  const int sheet_h = std::max(1140, sheet.global_max_y + 400);
  sheet.lines[1] = "SHEET 1 " + std::to_string(sheet_w) + " " + std::to_string(sheet_h);

  if (tran_directive.has_value() && !tran_directive->empty()) {
    sheet.lines.push_back("TEXT 40 40 Left 2 !" + *tran_directive);
  }
}

void append_missing_element_note(AscAssembly &out, ExportSheetState &sheet, int row_cursor_x,
                                 int row_y, int &row_cursor_x_out, int &row_max_h,
                                 const ExportLayoutConfig &layout, const std::string &note_message,
                                 const std::string &warning_message) {
  const int nx = row_cursor_x + layout.pad_x;
  const int ny = row_y + layout.pad_y;
  std::ostringstream note;
  note << "TEXT " << nx << " " << ny << " Left 2 !PPE: " << note_message;
  sheet.lines.push_back(note.str());
  out.warnings.push_back(warning_message);
  row_cursor_x_out += (700 + layout.gap_x);
  row_max_h = std::max(row_max_h, 220);
  sheet.global_max_x = std::max(sheet.global_max_x, nx + 700);
  sheet.global_max_y = std::max(sheet.global_max_y, ny + 200);
}

void append_export_fragment(AscAssembly &out, ExportSheetState &sheet, int dx, int dy,
                            int &row_cursor_x, int &row_max_h, const ExportLayoutConfig &layout,
                            const std::string &asc_with_header,
                            const std::unordered_map<long long, std::string> &xy_to_net,
                            const std::string &inst_suffix) {
  std::vector<std::string> flag_warnings;
  auto patched_flags = patch_flags(asc_with_header, xy_to_net, flag_warnings);
  out.warnings.insert(out.warnings.end(), flag_warnings.begin(), flag_warnings.end());

  std::unordered_set<std::string> reserved_nets;
  reserved_nets.insert("0");
  for (const auto &kv : xy_to_net) {
    reserved_nets.insert(kv.second);
  }
  const std::string flags_unique =
      uniquify_flag_nets(patched_flags.asc, inst_suffix, reserved_nets);
  const std::string with_unique_inst = uniquify_instnames(flags_unique, inst_suffix);

  const auto bounds = asc_bounds(with_unique_inst, false);
  int local_dx = dx;
  int local_dy = dy;
  int w = 700;
  int h = 600;
  if (bounds.valid()) {
    local_dx = dx - bounds.min_x + layout.pad_x;
    local_dy = dy - bounds.min_y + layout.pad_y;
    w = (bounds.max_x - bounds.min_x) + 2 * layout.pad_x;
    h = (bounds.max_y - bounds.min_y) + 2 * layout.pad_y;
  }

  std::string fragment = strip_header(with_unique_inst);
  fragment = shift_coords(fragment, local_dx, local_dy);

  row_cursor_x += (w + layout.gap_x);
  row_max_h = std::max(row_max_h, h);

  const auto shifted_bounds = asc_bounds(fragment, true);
  if (shifted_bounds.valid()) {
    sheet.global_max_x = std::max(sheet.global_max_x, shifted_bounds.max_x);
    sheet.global_max_y = std::max(sheet.global_max_y, shifted_bounds.max_y);
  } else {
    sheet.global_max_x = std::max(sheet.global_max_x, local_dx + layout.pad_x + w);
    sheet.global_max_y = std::max(sheet.global_max_y, local_dy + layout.pad_y + h);
  }

  std::istringstream in(fragment);
  std::string line;
  while (std::getline(in, line)) {
    if (line.rfind("TEXT ", 0) == 0) {
      const auto bang = line.find('!');
      if (bang != std::string::npos && bang + 1 < line.size()) {
        const std::string directive = line.substr(bang + 1);
        if (!directive.empty() && directive.front() == '.') {
          if (directive.rfind(".tran", 0) == 0) {
            continue;
          }
          if (sheet.seen_directives.find(directive) != sheet.seen_directives.end()) {
            continue;
          }
          sheet.seen_directives.insert(directive);
        }
      }
    }
    sheet.lines.push_back(line);
  }
}

std::unordered_map<long long, std::string> build_block_flag_nets(const Block &block,
                                                                 const ExportTopology &topology) {
  std::unordered_map<long long, std::string> xy_to_net;
  for (const auto &port : ports_for(block)) {
    if (port.type == PortType::Ground) {
      continue;
    }
    const Endpoint endpoint{block.id, port.id};
    const std::string net = topology.net_for_endpoint(endpoint);
    if (net.empty()) {
      continue;
    }
    for (const auto &flag : port.flags) {
      const long long key =
          (static_cast<long long>(flag.x) << 32) ^ (static_cast<unsigned int>(flag.y));
      xy_to_net.emplace(key, net);
    }
  }
  return xy_to_net;
}

} // namespace pep::modules::project_design
