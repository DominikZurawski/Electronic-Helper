#include "pep/app.hpp"
#include "pep/command_router.hpp"
#include "pep/module_registry.hpp"
#include "pep/modules/antenna_basic.hpp"
#include "pep/modules/psu_basic.hpp"
#include "pep/modules/psu_symmetric.hpp"

#include <string>
#include <vector>

namespace pep {

namespace {
ModuleRegistry build_registry() {
  ModuleRegistry registry;
  pep::modules::psu_basic::register_module(registry);
  pep::modules::psu_symmetric::register_module(registry);
  pep::modules::antenna_basic::register_module(registry);
  return registry;
}

} // namespace

int App::run(const std::vector<std::string> &args) {
  ModuleRegistry registry = build_registry();
  CommandRouter router;
  return router.route(args, registry);
}

} // namespace pep
