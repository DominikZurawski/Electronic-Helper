#include "pep/module.hpp"
#include "pep/module_registry.hpp"

namespace pep::modules::antenna_basic {

namespace {
class AntennaModule : public Module {
public:
  ModuleInfo info() const override {
    return {"antenna-basic", "Anteny", "Podstawowe wzory i checklisty dla anten"};
  }

  int run(const std::vector<std::string>&) override {
    return 0;
  }
};
}  // namespace

void register_module(pep::ModuleRegistry& registry) {
  registry.register_module("antenna-basic", [] { return std::make_unique<AntennaModule>(); });
}

} // namespace pep::modules::antenna_basic
