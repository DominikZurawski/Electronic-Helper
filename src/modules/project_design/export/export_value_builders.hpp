#pragma once

#include "../model/model.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace pep::modules::project_design {

std::unordered_map<std::string, std::string>
build_amplifier_instance_values(const Block &block, std::vector<std::string> &warnings);

} // namespace pep::modules::project_design
