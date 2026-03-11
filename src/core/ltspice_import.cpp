#include "pep/ltspice_import.hpp"

#include <fstream>

namespace pep {

LtspiceImportResult LtspiceImporter::import_file(const std::string& path) const {
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
  }

  return result;
}

} // namespace pep
