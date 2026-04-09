#pragma once

#include "../model/model.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace pep::modules::project_design {

std::string endpoint_export_key(const Endpoint &endpoint);

struct ExportTopology {
  std::unordered_map<std::string, std::string> endpoint_to_net;
  std::vector<std::vector<int>> ordered_components;
  std::vector<std::string> warnings;

  [[nodiscard]] std::string net_for_endpoint(const Endpoint &endpoint) const;
};

ExportTopology build_export_topology(const std::vector<Block> &blocks,
                                     const std::vector<Connection> &connections);

} // namespace pep::modules::project_design
