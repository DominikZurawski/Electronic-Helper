#include "pep/app.hpp"
#include "pep/command_router.hpp"
#include "pep/module_registry.hpp"

#include <string>
#include <vector>

namespace pep {

namespace {
ModuleRegistry build_registry() {
  ModuleRegistry registry;

  registry.register_module("psu-basic", [] {
    struct BasicPsuModule : Module {
      ModuleInfo info() const override {
        return {"psu-basic", "Zasilacz niestabilizowany", "Prosty kalkulator zasilacza niestabilizowanego"};
      }
      int run(const std::vector<std::string>&) override { return 0; }
    };
    return std::make_unique<BasicPsuModule>();
  });

  registry.register_module("psu-symmetric", [] {
    struct SymmetricPsuModule : Module {
      ModuleInfo info() const override {
        return {"psu-symmetric", "Zasilacz symetryczny", "Kalkulator i checklista dla zasilaczy symetrycznych"};
      }
      int run(const std::vector<std::string>&) override { return 0; }
    };
    return std::make_unique<SymmetricPsuModule>();
  });

  registry.register_module("antenna-basic", [] {
    struct AntennaModule : Module {
      ModuleInfo info() const override {
        return {"antenna-basic", "Anteny", "Podstawowe wzory i checklisty dla anten"};
      }
      int run(const std::vector<std::string>&) override { return 0; }
    };
    return std::make_unique<AntennaModule>();
  });

  return registry;
}

}  // namespace

int App::run(const std::vector<std::string>& args) {
  ModuleRegistry registry = build_registry();
  CommandRouter router;
  return router.route(args, registry);
}

} // namespace pep
