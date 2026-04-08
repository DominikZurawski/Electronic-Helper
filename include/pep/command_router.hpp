#pragma once

#include "pep/module_registry.hpp"

#include <string>
#include <vector>

namespace pep {

class CommandRouter {
public:
  int route(const std::vector<std::string> &args, const ModuleRegistry &registry) const;

private:
  int run_list(const ModuleRegistry &registry) const;
  int run_ltspice_import(const std::vector<std::string> &args) const;
  void print_usage() const;
};

} // namespace pep
