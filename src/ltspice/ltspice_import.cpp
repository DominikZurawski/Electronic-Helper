#include "pep/ltspice_import.hpp"

#include <fstream>
#include <sstream>

namespace pep {

static void parse_asc_line(const std::string &line, LtspiceImportResult &result) {
  std::istringstream iss(line);
  std::string head;
  iss >> head;
  if (head == "SYMBOL") {
    std::string kind;
    iss >> kind;
    if (!kind.empty()) {
      result.components.push_back({kind, "", ""});
    }
  } else if (head == "SYMATTR") {
    std::string key;
    iss >> key;
    if (key == "InstName") {
      std::string name;
      iss >> name;
      if (!result.components.empty()) {
        result.components.back().name = name;
      }
    } else if (key == "Value") {
      std::string value;
      std::getline(iss, value);
      if (!value.empty() && value.front() == ' ') {
        value.erase(0, 1);
      }
      if (!result.components.empty()) {
        result.components.back().value = value;
      }
    }
  }
}

LtspiceImportResult LtspiceImporter::import_file(const std::string &path) const {
  LtspiceImportResult result;
  result.source_path = path;

  if (path.size() >= 4 && path.substr(path.size() - 4) == ".asc") {
    result.format = "LTspice schematic (.asc)";
  } else if (path.size() >= 4 && path.substr(path.size() - 4) == ".cir") {
    result.format = "SPICE netlist (.cir)";
  } else if (path.size() >= 4 && path.substr(path.size() - 4) == ".net") {
    result.format = "SPICE netlist (.net)";
  } else {
    result.format = "unknown";
  }

  std::ifstream file(path);
  std::string line;
  while (std::getline(file, line)) {
    ++result.line_count;
    if (result.format == "LTspice schematic (.asc)") {
      parse_asc_line(line, result);
    }
  }

  return result;
}

} // namespace pep
