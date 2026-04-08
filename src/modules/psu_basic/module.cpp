#include "pep/module.hpp"
#include "pep/module_registry.hpp"

namespace pep::modules::psu_basic {

namespace {
class PsuBasicModule : public Module {
public:
  ModuleInfo info() const override {
    return {"psu-basic", "Zasilacz niestabilizowany",
            "Prosty kalkulator zasilacza niestabilizowanego"};
  }

  int run(const std::vector<std::string> &) override { return 0; }
};
} // namespace

void register_module(pep::ModuleRegistry &registry) {
  registry.register_module("psu-basic", [] { return std::make_unique<PsuBasicModule>(); });
}

} // namespace pep::modules::psu_basic
