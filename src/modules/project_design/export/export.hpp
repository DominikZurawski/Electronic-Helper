#pragma once

#include <optional>
#include <string>
#include <vector>

namespace pep::modules::project_design {

struct Block;
struct Connection;

struct AscAssembly {
  std::string asc;
  std::vector<std::string> warnings;
};

AscAssembly export_project_asc(const std::vector<Block> &blocks,
                               const std::vector<Connection> &connections,
                               const std::optional<std::string> &tran_directive);

} // namespace pep::modules::project_design
