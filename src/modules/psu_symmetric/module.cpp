#include "pep/module.hpp"
#include "pep/module_registry.hpp"

namespace pep::modules::psu_symmetric {

namespace {
class PsuSymmetricModule : public Module {
public:
  ModuleInfo info() const override {
    return {"psu-symmetric", "Zasilacz symetryczny", "Kalkulator i checklista dla zasilaczy symetrycznych"};
  }

  int run(const std::vector<std::string>&) override {
    return 0;
  }
};
}  // namespace

void register_module(pep::ModuleRegistry& registry) {
  registry.register_module("psu-symmetric", [] { return std::make_unique<PsuSymmetricModule>(); });
}

} // namespace pep::modules::psu_symmetric
