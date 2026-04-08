#pragma once

#include <string>
#include <vector>

namespace pep {

struct ModuleInfo {
  std::string id;
  std::string name;
  std::string description;
};

class Module {
public:
  virtual ~Module() = default;
  virtual ModuleInfo info() const = 0;
  virtual int run(const std::vector<std::string> &args) = 0;
};

} // namespace pep
