#include "export_topology.hpp"

#include "export_component_order.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pep::modules::project_design {

namespace {

struct Dsu {
  std::vector<int> parent;
  std::vector<int> rank;

  explicit Dsu(int n) : parent(n), rank(n, 0) {
    for (int i = 0; i < n; ++i) {
      parent[i] = i;
    }
  }

  int find(int x) {
    if (parent[x] == x) {
      return x;
    }
    parent[x] = find(parent[x]);
    return parent[x];
  }

  void unite(int a, int b) {
    a = find(a);
    b = find(b);
    if (a == b) {
      return;
    }
    if (rank[a] < rank[b]) {
      std::swap(a, b);
    }
    parent[b] = a;
    if (rank[a] == rank[b]) {
      ++rank[a];
    }
  }
};

struct EpInfo {
  int idx = -1;
  Endpoint endpoint;
  PortType type = PortType::Unknown;
};

std::string base_name_for_group(const std::vector<EpInfo> &endpoints,
                                std::vector<std::string> &warnings) {
  bool has_vcc = false;
  bool has_vee = false;
  bool has_in = false;
  bool has_out = false;
  for (const auto &endpoint : endpoints) {
    if (endpoint.type == PortType::PowerPos) {
      has_vcc = true;
    }
    if (endpoint.type == PortType::PowerNeg) {
      has_vee = true;
    }
    if (endpoint.type == PortType::AnalogIn) {
      has_in = true;
    }
    if (endpoint.type == PortType::AnalogOut) {
      has_out = true;
    }
  }

  if (has_vcc && has_vee) {
    warnings.push_back("Sprzeczne typy w jednej sieci (Vcc i Vee) — nadaję nazwę automatyczną.");
    return "NET";
  }
  if (has_vcc) {
    return "Vcc";
  }
  if (has_vee) {
    return "Vee";
  }
  if (has_in && has_out) {
    return "SIG";
  }
  if (has_in) {
    return "IN";
  }
  if (has_out) {
    return "OUT";
  }
  return "NET";
}

std::string reserve_name(std::unordered_map<std::string, int> &base_counters,
                         const std::string &base) {
  int &counter = base_counters[base];
  if (counter == 0) {
    counter = 1;
    return base;
  }
  ++counter;
  return base + std::to_string(counter);
}

} // namespace

std::string endpoint_export_key(const Endpoint &endpoint) {
  return std::to_string(endpoint.block_id) + ":" + endpoint.port_id;
}

std::string ExportTopology::net_for_endpoint(const Endpoint &endpoint) const {
  const auto it = endpoint_to_net.find(endpoint_export_key(endpoint));
  if (it == endpoint_to_net.end()) {
    return "";
  }
  return it->second;
}

ExportTopology build_export_topology(const std::vector<Block> &blocks,
                                     const std::vector<Connection> &connections) {
  ExportTopology topology;

  std::vector<EpInfo> endpoints;
  endpoints.reserve(32);
  std::unordered_map<std::string, int> endpoint_to_index;

  auto add_endpoint = [&](const Block &block, const PortDef &port) {
    Endpoint endpoint{block.id, port.id};
    const std::string key = endpoint_export_key(endpoint);
    if (endpoint_to_index.find(key) != endpoint_to_index.end()) {
      return;
    }
    const int idx = static_cast<int>(endpoints.size());
    endpoints.push_back({idx, endpoint, port.type});
    endpoint_to_index.emplace(key, idx);
  };

  for (const auto &block : blocks) {
    for (const auto &port : ports_for(block)) {
      if (port.type == PortType::Ground) {
        continue;
      }
      add_endpoint(block, port);
    }
  }

  Dsu dsu(static_cast<int>(endpoints.size()));
  for (const auto &connection : connections) {
    const auto ita = endpoint_to_index.find(endpoint_export_key(connection.a));
    const auto itb = endpoint_to_index.find(endpoint_export_key(connection.b));
    if (ita == endpoint_to_index.end() || itb == endpoint_to_index.end()) {
      continue;
    }
    dsu.unite(ita->second, itb->second);
  }

  std::unordered_map<int, std::vector<EpInfo>> group_to_endpoints;
  group_to_endpoints.reserve(endpoints.size());
  for (const auto &endpoint : endpoints) {
    group_to_endpoints[dsu.find(endpoint.idx)].push_back(endpoint);
  }

  std::unordered_map<int, std::string> group_to_net;
  std::unordered_map<std::string, int> base_counters;
  for (const auto &[root, group_endpoints] : group_to_endpoints) {
    const std::string base = base_name_for_group(group_endpoints, topology.warnings);
    if (group_endpoints.size() == 1) {
      const int block_id = group_endpoints.front().endpoint.block_id;
      group_to_net.emplace(root,
                           reserve_name(base_counters, base + "_B" + std::to_string(block_id)));
      continue;
    }
    group_to_net.emplace(root, reserve_name(base_counters, base));
  }

  topology.endpoint_to_net.reserve(endpoints.size());
  for (const auto &endpoint : endpoints) {
    const int root = dsu.find(endpoint.idx);
    topology.endpoint_to_net.emplace(endpoint_export_key(endpoint.endpoint), group_to_net[root]);
  }

  topology.ordered_components = ordered_components_from_blocks(blocks, connections);
  return topology;
}

} // namespace pep::modules::project_design
