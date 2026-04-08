#pragma once

#include <optional>
#include <string>
#include <vector>

class QWidget;

namespace pep::modules::project_design {

struct Block;
struct Connection;

struct AscAssembly {
  std::string asc;
  std::vector<std::string> warnings;
};

std::optional<std::string> prompt_tran_directive(QWidget *parent, bool has_amplifier);

AscAssembly export_project_asc(const std::vector<Block> &blocks,
                               const std::vector<Connection> &connections,
                               const std::optional<std::string> &tran_directive);

} // namespace pep::modules::project_design
