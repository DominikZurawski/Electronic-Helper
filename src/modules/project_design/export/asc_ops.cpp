#include "asc_ops.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pep::modules::project_design {

void AscBounds::add(int x, int y) {
  min_x = std::min(min_x, x);
  max_x = std::max(max_x, x);
  min_y = std::min(min_y, y);
  max_y = std::max(max_y, y);
}

bool AscBounds::valid() const {
  return min_x <= max_x && min_y <= max_y;
}

std::string strip_header(const std::string &asc) {
  std::ostringstream out;
  std::istringstream in(asc);
  std::string line;
  bool first = true;
  while (std::getline(in, line)) {
    if (line.rfind("Version", 0) == 0 || line.rfind("SHEET", 0) == 0) {
      continue;
    }
    if (!first) {
      out << "\n";
    }
    out << line;
    first = false;
  }
  return out.str();
}

AscBounds asc_bounds(const std::string &asc, bool include_text) {
  AscBounds bounds{std::numeric_limits<int>::max(), std::numeric_limits<int>::min(),
                   std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};
  std::istringstream in(asc);
  std::string line;
  while (std::getline(in, line)) {
    std::istringstream iss(line);
    std::string head;
    iss >> head;

    if (head == "WIRE") {
      int x1 = 0;
      int y1 = 0;
      int x2 = 0;
      int y2 = 0;
      if (iss >> x1 >> y1 >> x2 >> y2) {
        bounds.add(x1, y1);
        bounds.add(x2, y2);
      }
      continue;
    }

    if (head == "FLAG") {
      int x = 0;
      int y = 0;
      std::string net;
      if (iss >> x >> y >> net) {
        bounds.add(x, y);
      }
      continue;
    }

    if (head == "SYMBOL") {
      std::string sym;
      int x = 0;
      int y = 0;
      if (iss >> sym >> x >> y) {
        bounds.add(x, y);
      }
      continue;
    }

    if (include_text && head == "TEXT") {
      int x = 0;
      int y = 0;
      if (iss >> x >> y) {
        bounds.add(x, y);
      }
    }
  }
  return bounds;
}

std::string shift_coords(const std::string &asc, int dx, int dy) {
  std::istringstream in(asc);
  std::ostringstream out;
  std::string line;
  bool first = true;

  auto append_line = [&](const std::string &value) {
    if (!first) {
      out << "\n";
    }
    out << value;
    first = false;
  };

  while (std::getline(in, line)) {
    std::istringstream iss(line);
    std::string head;
    iss >> head;

    if (head == "WIRE") {
      int x1 = 0;
      int y1 = 0;
      int x2 = 0;
      int y2 = 0;
      if (iss >> x1 >> y1 >> x2 >> y2) {
        append_line("WIRE " + std::to_string(x1 + dx) + " " + std::to_string(y1 + dy) + " " +
                    std::to_string(x2 + dx) + " " + std::to_string(y2 + dy));
        continue;
      }
    } else if (head == "FLAG") {
      int x = 0;
      int y = 0;
      std::string net;
      if (iss >> x >> y >> net) {
        append_line("FLAG " + std::to_string(x + dx) + " " + std::to_string(y + dy) + " " + net);
        continue;
      }
    } else if (head == "SYMBOL") {
      std::string symbol;
      int x = 0;
      int y = 0;
      std::string rest;
      if (iss >> symbol >> x >> y) {
        std::getline(iss, rest);
        append_line("SYMBOL " + symbol + " " + std::to_string(x + dx) + " " +
                    std::to_string(y + dy) + rest);
        continue;
      }
    } else if (head == "TEXT") {
      int x = 0;
      int y = 0;
      std::string rest;
      if (iss >> x >> y) {
        std::getline(iss, rest);
        append_line("TEXT " + std::to_string(x + dx) + " " + std::to_string(y + dy) + rest);
        continue;
      }
    }

    append_line(line);
  }

  return out.str();
}

std::string uniquify_instnames(const std::string &asc, const std::string &suffix) {
  if (suffix.empty()) {
    return asc;
  }

  auto apply_suffix = [&](const std::string &name) {
    if (name.size() >= suffix.size() && name.rfind(suffix) == name.size() - suffix.size()) {
      return name;
    }
    return name + suffix;
  };

  std::istringstream in(asc);
  std::string line;
  std::vector<std::string> lines;
  while (std::getline(in, line)) {
    lines.push_back(line);
  }

  std::unordered_map<std::string, std::string> inst_map;
  for (const auto &current : lines) {
    if (current.rfind("SYMATTR InstName ", 0) == 0) {
      const std::string name = current.substr(std::string("SYMATTR InstName ").size());
      inst_map.emplace(name, apply_suffix(name));
    }
  }

  std::ostringstream out;
  bool first = true;
  for (auto &current : lines) {
    if (current.rfind("SYMATTR InstName ", 0) == 0) {
      const std::string name = current.substr(std::string("SYMATTR InstName ").size());
      current = "SYMATTR InstName " + apply_suffix(name);
    } else if (current.rfind("TEXT ", 0) == 0) {
      const auto bang = current.find('!');
      if (bang != std::string::npos && bang + 1 < current.size()) {
        const std::string directive = current.substr(bang + 1);
        if (!directive.empty() && (directive[0] == 'K' || directive[0] == 'k')) {
          std::istringstream iss(directive);
          std::vector<std::string> tokens;
          std::string token;
          while (iss >> token) {
            tokens.push_back(token);
          }
          if (tokens.size() >= 4) {
            tokens[0] = apply_suffix(tokens[0]);
            for (std::size_t i = 1; i + 1 < tokens.size(); ++i) {
              const auto it = inst_map.find(tokens[i]);
              if (it != inst_map.end()) {
                tokens[i] = it->second;
              }
            }
            std::ostringstream rebuilt;
            for (std::size_t i = 0; i < tokens.size(); ++i) {
              if (i) {
                rebuilt << " ";
              }
              rebuilt << tokens[i];
            }
            current = current.substr(0, bang + 1) + rebuilt.str();
          }
        }
      }
    }

    if (!first) {
      out << "\n";
    }
    out << current;
    first = false;
  }

  return out.str();
}

std::string uniquify_flag_nets(const std::string &asc, const std::string &suffix,
                               const std::unordered_set<std::string> &reserved_nets) {
  if (suffix.empty()) {
    return asc;
  }

  auto apply_suffix = [&](const std::string &name) {
    if (name.size() >= suffix.size() && name.rfind(suffix) == name.size() - suffix.size()) {
      return name;
    }
    return name + suffix;
  };

  std::istringstream in(asc);
  std::ostringstream out;
  std::string line;
  bool first = true;
  while (std::getline(in, line)) {
    if (line.rfind("FLAG ", 0) == 0) {
      std::istringstream iss(line);
      std::string head;
      int x = 0;
      int y = 0;
      std::string net;
      if (iss >> head >> x >> y >> net) {
        if (reserved_nets.find(net) == reserved_nets.end() && net != "0") {
          line = "FLAG " + std::to_string(x) + " " + std::to_string(y) + " " + apply_suffix(net);
        }
      }
    }

    if (!first) {
      out << "\n";
    }
    out << line;
    first = false;
  }
  return out.str();
}

std::string remove_component_by_instname(const std::string &asc, const std::string &instname) {
  if (instname.empty()) {
    return asc;
  }

  std::istringstream in(asc);
  std::ostringstream out;
  std::string line;
  bool first = true;

  std::vector<std::string> block_lines;
  bool in_symbol = false;
  bool drop_block = false;

  auto flush_block = [&]() {
    if (!drop_block) {
      for (const auto &bline : block_lines) {
        if (!first) {
          out << "\n";
        }
        out << bline;
        first = false;
      }
    }
    block_lines.clear();
    drop_block = false;
    in_symbol = false;
  };

  while (std::getline(in, line)) {
    if (line.rfind("SYMBOL ", 0) == 0) {
      if (in_symbol) {
        flush_block();
      }
      in_symbol = true;
      drop_block = false;
      block_lines.clear();
      block_lines.push_back(line);
      continue;
    }

    if (in_symbol) {
      block_lines.push_back(line);
      if (line.rfind("SYMATTR InstName ", 0) == 0) {
        const std::string name = line.substr(std::string("SYMATTR InstName ").size());
        if (name == instname) {
          drop_block = true;
        }
      }
      continue;
    }

    if (!first) {
      out << "\n";
    }
    out << line;
    first = false;
  }

  if (in_symbol) {
    flush_block();
  }

  return out.str();
}

std::string append_directives_below(const std::string &asc,
                                    const std::vector<std::string> &directives) {
  if (directives.empty()) {
    return asc;
  }

  const auto bounds = asc_bounds(asc, true);
  int x = 40;
  int y = 40;
  if (bounds.valid()) {
    x = bounds.min_x + 40;
    y = bounds.max_y + 40;
  }

  std::ostringstream out;
  out << asc;
  for (const auto &directive : directives) {
    if (directive.empty()) {
      continue;
    }
    out << "\nTEXT " << x << " " << y << " Left 2 !" << directive;
    y += 16;
  }
  return out.str();
}

std::string remove_wire_segment(const std::string &asc, int x1, int y1, int x2, int y2) {
  std::istringstream in(asc);
  std::ostringstream out;
  std::string line;
  bool first = true;

  const auto matches = [&](int ax, int ay, int bx, int by) {
    return (ax == x1 && ay == y1 && bx == x2 && by == y2) ||
           (ax == x2 && ay == y2 && bx == x1 && by == y1);
  };

  while (std::getline(in, line)) {
    std::istringstream iss(line);
    std::string head;
    iss >> head;
    if (head == "WIRE") {
      int ax = 0;
      int ay = 0;
      int bx = 0;
      int by = 0;
      if (iss >> ax >> ay >> bx >> by) {
        if (matches(ax, ay, bx, by)) {
          continue;
        }
      }
    }

    if (!first) {
      out << "\n";
    }
    out << line;
    first = false;
  }

  return out.str();
}

pep::LtspiceAscPatchResult patch_flags(const std::string &asc,
                                       const std::unordered_map<long long, std::string> &xy_to_net,
                                       std::vector<std::string> &warnings) {
  pep::LtspiceAscPatchResult result;
  result.asc = asc;

  std::vector<std::string> lines;
  {
    std::istringstream in(asc);
    std::string line;
    while (std::getline(in, line)) {
      lines.push_back(line);
    }
  }

  std::set<long long> patched;
  for (auto &line : lines) {
    std::istringstream iss(line);
    std::string head;
    iss >> head;
    if (head != "FLAG") {
      continue;
    }

    int x = 0;
    int y = 0;
    std::string net;
    if (!(iss >> x >> y >> net)) {
      continue;
    }

    const long long key = (static_cast<long long>(x) << 32) ^ (static_cast<unsigned int>(y));
    const auto it = xy_to_net.find(key);
    if (it == xy_to_net.end()) {
      continue;
    }

    line = "FLAG " + std::to_string(x) + " " + std::to_string(y) + " " + it->second;
    patched.insert(key);
  }

  for (const auto &[key, _] : xy_to_net) {
    if (patched.find(key) == patched.end()) {
      warnings.push_back("Nie znaleziono FLAG do podmiany netu (x,y).");
    }
  }

  std::ostringstream out;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    out << lines[i];
    if (i + 1 != lines.size()) {
      out << "\n";
    }
  }
  result.asc = out.str();
  return result;
}

} // namespace pep::modules::project_design
