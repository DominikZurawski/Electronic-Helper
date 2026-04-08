#pragma once

#include "pep/module.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace pep {

class ModuleRegistry {
public:
  using Factory = std::function<std::unique_ptr<Module>()>;

  void register_module(const std::string &id, Factory factory);
  std::unique_ptr<Module> create(const std::string &id) const;
  std::vector<ModuleInfo> list() const;

private:
  std::unordered_map<std::string, Factory> factories_;
};

} // namespace pep
