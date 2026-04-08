#include "pep/module_registry.hpp"

#include <algorithm>

namespace pep {

void ModuleRegistry::register_module(const std::string &id, Factory factory) {
  factories_[id] = std::move(factory);
}

std::unique_ptr<Module> ModuleRegistry::create(const std::string &id) const {
  auto it = factories_.find(id);
  if (it == factories_.end()) {
    return nullptr;
  }
  return it->second();
}

std::vector<ModuleInfo> ModuleRegistry::list() const {
  std::vector<ModuleInfo> items;
  items.reserve(factories_.size());
  for (const auto &[id, factory] : factories_) {
    auto module = factory();
    items.push_back(module->info());
  }

  std::sort(items.begin(), items.end(),
            [](const ModuleInfo &a, const ModuleInfo &b) { return a.id < b.id; });

  return items;
}

} // namespace pep
