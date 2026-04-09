#pragma once

#include "../model/model.hpp"

#include <vector>

namespace pep::modules::project_design {

std::vector<std::vector<int>>
ordered_components_from_blocks(const std::vector<Block> &blocks,
                               const std::vector<Connection> &connections);

} // namespace pep::modules::project_design
