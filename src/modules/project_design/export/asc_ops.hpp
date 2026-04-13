#pragma once

#include "pep/ltspice_template.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pep::modules::project_design {

struct AscBounds {
  int min_x = 0;
  int max_x = -1;
  int min_y = 0;
  int max_y = -1;

  void add(int x, int y);
  bool valid() const;
};

std::string strip_header(const std::string &asc);
AscBounds asc_bounds(const std::string &asc, bool include_text);
std::string shift_coords(const std::string &asc, int dx, int dy);
std::string uniquify_instnames(const std::string &asc, const std::string &suffix);
std::string uniquify_flag_nets(const std::string &asc, const std::string &suffix,
                               const std::unordered_set<std::string> &reserved_nets);
std::string remove_component_by_instname(const std::string &asc, const std::string &instname);
std::string append_directives_below(const std::string &asc,
                                    const std::vector<std::string> &directives);
std::string remove_wire_segment(const std::string &asc, int x1, int y1, int x2, int y2);
pep::LtspiceAscPatchResult patch_flags(const std::string &asc,
                                       const std::unordered_map<long long, std::string> &xy_to_net,
                                       std::vector<std::string> &warnings);

} // namespace pep::modules::project_design
