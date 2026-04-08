#pragma once

#include <string>
#include <vector>

namespace pep {

struct LtspiceComponent {
  std::string kind;
  std::string name;
  std::string value;
};

struct LtspiceImportResult {
  std::string source_path;
  std::string format;
  std::size_t line_count = 0;
  std::vector<LtspiceComponent> components;
};

class LtspiceImporter {
public:
  LtspiceImportResult import_file(const std::string &path) const;
};

} // namespace pep
