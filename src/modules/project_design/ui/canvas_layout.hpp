#pragma once

#include <vector>

namespace pep::modules::project_design {

struct Block;
struct Connection;

void apply_auto_layout(std::vector<Block> &blocks, const std::vector<Connection> &connections);

} // namespace pep::modules::project_design
