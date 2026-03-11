#include "pep/module_registry.hpp"

#include <cassert>
#include <memory>

int main() {
  pep::ModuleRegistry registry;
  registry.register_module("demo", [] {
    struct Demo : pep::Module {
      pep::ModuleInfo info() const override { return {"demo", "Demo", "Test"}; }
      int run(const std::vector<std::string>&) override { return 0; }
    };
    return std::make_unique<Demo>();
  });

  auto modules = registry.list();
  assert(!modules.empty());
  assert(modules[0].id == "demo");
  auto module = registry.create("demo");
  assert(module != nullptr);
  return 0;
}
