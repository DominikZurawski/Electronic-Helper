#include "export_component_order.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace pep::modules::project_design {

namespace {

PortType port_type_of(const std::vector<Block> &blocks, int block_idx, const std::string &port_id) {
  const auto port = find_port(blocks[block_idx], port_id);
  return port.has_value() ? port->type : PortType::Unknown;
}

bool is_power_type(PortType type) {
  return type == PortType::PowerPos || type == PortType::PowerNeg || type == PortType::Ground;
}

} // namespace

std::vector<std::vector<int>>
ordered_components_from_blocks(const std::vector<Block> &blocks,
                               const std::vector<Connection> &connections) {
  std::unordered_map<int, int> id_to_idx;
  id_to_idx.reserve(blocks.size());
  for (int i = 0; i < static_cast<int>(blocks.size()); ++i) {
    id_to_idx.emplace(blocks[i].id, i);
  }

  std::vector<std::vector<int>> comp_adj(blocks.size());
  for (const auto &connection : connections) {
    const auto ia = id_to_idx.find(connection.a.block_id);
    const auto ib = id_to_idx.find(connection.b.block_id);
    if (ia == id_to_idx.end() || ib == id_to_idx.end()) {
      continue;
    }
    comp_adj[ia->second].push_back(ib->second);
    comp_adj[ib->second].push_back(ia->second);
  }

  std::vector<int> comp(blocks.size(), -1);
  int comp_count = 0;
  for (int i = 0; i < static_cast<int>(blocks.size()); ++i) {
    if (comp[i] != -1) {
      continue;
    }
    std::vector<int> stack = {i};
    comp[i] = comp_count;
    while (!stack.empty()) {
      const int v = stack.back();
      stack.pop_back();
      for (const int u : comp_adj[v]) {
        if (comp[u] == -1) {
          comp[u] = comp_count;
          stack.push_back(u);
        }
      }
    }
    ++comp_count;
  }

  std::vector<std::vector<int>> comps(comp_count);
  for (int i = 0; i < static_cast<int>(blocks.size()); ++i) {
    comps[comp[i]].push_back(i);
  }

  std::vector<std::vector<int>> fwd(blocks.size());
  auto add_fwd = [&](int a, int b) { fwd[a].push_back(b); };
  for (const auto &connection : connections) {
    const auto ia = id_to_idx.find(connection.a.block_id);
    const auto ib = id_to_idx.find(connection.b.block_id);
    if (ia == id_to_idx.end() || ib == id_to_idx.end()) {
      continue;
    }

    const int a = ia->second;
    const int b = ib->second;
    const PortType ta = port_type_of(blocks, a, connection.a.port_id);
    const PortType tb = port_type_of(blocks, b, connection.b.port_id);

    if (is_power_type(ta) && is_power_type(tb)) {
      const bool a_psu = blocks[a].kind == BlockKind::Power;
      const bool b_psu = blocks[b].kind == BlockKind::Power;
      if (a_psu && !b_psu) {
        add_fwd(a, b);
      } else if (b_psu && !a_psu) {
        add_fwd(b, a);
      } else {
        add_fwd(a, b);
        add_fwd(b, a);
      }
      continue;
    }

    if (ta == PortType::AnalogOut && tb == PortType::AnalogIn) {
      add_fwd(a, b);
      continue;
    }
    if (tb == PortType::AnalogOut && ta == PortType::AnalogIn) {
      add_fwd(b, a);
      continue;
    }

    add_fwd(a, b);
    add_fwd(b, a);
  }

  for (auto &neighbors : fwd) {
    std::sort(neighbors.begin(), neighbors.end(),
              [&](int lhs, int rhs) { return blocks[lhs].id < blocks[rhs].id; });
    neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
  }

  std::vector<std::vector<int>> ordered_components;
  ordered_components.reserve(comps.size());
  for (const auto &indices : comps) {
    std::vector<int> order;
    order.reserve(indices.size());

    std::unordered_map<int, bool> in_comp;
    in_comp.reserve(indices.size());
    for (const int idx : indices) {
      in_comp.emplace(idx, true);
    }

    std::unordered_map<int, bool> visited;
    visited.reserve(indices.size());
    auto visit_from = [&](int start) {
      std::vector<int> stack = {start};
      while (!stack.empty()) {
        const int v = stack.back();
        stack.pop_back();
        if (visited[v]) {
          continue;
        }
        visited[v] = true;
        order.push_back(v);
        for (const int u : fwd[v]) {
          if (!in_comp[u] || visited[u]) {
            continue;
          }
          stack.push_back(u);
        }
      }
    };

    std::vector<int> starts;
    for (const int idx : indices) {
      if (blocks[idx].kind == BlockKind::Power) {
        starts.push_back(idx);
      }
    }
    std::sort(starts.begin(), starts.end(),
              [&](int lhs, int rhs) { return blocks[lhs].id < blocks[rhs].id; });
    for (const int start : starts) {
      visit_from(start);
    }

    std::vector<int> rest = indices;
    std::sort(rest.begin(), rest.end(),
              [&](int lhs, int rhs) { return blocks[lhs].id < blocks[rhs].id; });
    for (const int start : rest) {
      visit_from(start);
    }

    ordered_components.push_back(std::move(order));
  }

  return ordered_components;
}

} // namespace pep::modules::project_design
