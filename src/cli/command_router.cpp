#include "pep/command_router.hpp"
#include "pep/ltspice_import.hpp"

#include <iostream>

namespace pep {

int CommandRouter::route(const std::vector<std::string> &args,
                         const ModuleRegistry &registry) const {
  if (args.size() <= 1) {
    print_usage();
    return 1;
  }

  const std::string command = args[1];
  if (command == "list") {
    return run_list(registry);
  }
  if (command == "ltspice-import") {
    return run_ltspice_import(args);
  }
  if (command == "run") {
    if (args.size() < 3) {
      std::cout << "Brak ID modułu.\n";
      return 1;
    }
    auto module = registry.create(args[2]);
    if (!module) {
      std::cout << "Nieznany moduł: " << args[2] << "\n";
      return 1;
    }
    std::vector<std::string> module_args(args.begin() + 3, args.end());
    return module->run(module_args);
  }

  print_usage();
  return 1;
}

int CommandRouter::run_list(const ModuleRegistry &registry) const {
  auto modules = registry.list();
  std::cout << "Dostępne moduły:" << "\n";
  for (const auto &module : modules) {
    std::cout << "- " << module.id << " | " << module.name << " | " << module.description << "\n";
  }
  return 0;
}

int CommandRouter::run_ltspice_import(const std::vector<std::string> &args) const {
  if (args.size() < 3) {
    std::cout << "Podaj ścieżkę do pliku .asc/.cir/.net\n";
    return 1;
  }

  LtspiceImporter importer;
  auto result = importer.import_file(args[2]);
  std::cout << "Zaimportowano: " << result.source_path << "\n";
  std::cout << "Format: " << result.format << "\n";
  std::cout << "Liczba linii: " << result.line_count << "\n";
  return 0;
}

void CommandRouter::print_usage() const {
  std::cout << "Pomocnik Projektanta Elektronika\n";
  std::cout << "Użycie:\n";
  std::cout << "  ppe list\n";
  std::cout << "  ppe run <module-id> [args...]\n";
  std::cout << "  ppe ltspice-import <path>\n";
}

} // namespace pep
