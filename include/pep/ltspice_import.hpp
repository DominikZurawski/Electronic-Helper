#pragma once

#include <string>

namespace pep {

struct LtspiceImportResult {
  std::string source_path;
  std::string format;
  std::size_t line_count = 0;
};

class LtspiceImporter {
public:
  LtspiceImportResult import_file(const std::string& path) const;
};

} // namespace pep
