#include "canvas_layout.hpp"

#include "../model/model.hpp"

#include <unordered_map>
#include <vector>

namespace pep::modules::project_design {

namespace {

bool is_power_type(PortType type) {
  return type == PortType::PowerPos || type == PortType::PowerNeg ||
         type == PortType::PowerInPos || type == PortType::PowerOutPos ||
         type == PortType::Ground;
}

void add_edge(const std::unordered_map<int, int> &id_to_idx, std::vector<std::vector<int>> &adj,
              int a_id, int b_id) {
  if (a_id == b_id) {
    return;
  }

  const auto ia = id_to_idx.find(a_id);
  const auto ib = id_to_idx.find(b_id);
  if (ia == id_to_idx.end() || ib == id_to_idx.end()) {
    return;
  }

  adj[ia->second].push_back(ib->second);
  adj[ib->second].push_back(ia->second);
}

} // namespace

void apply_auto_layout(std::vector<Block> &blocks, const std::vector<Connection> &connections) {
  const int n = static_cast<int>(blocks.size());
  if (n == 0) {
    return;
  }

  std::unordered_map<int, int> id_to_idx;
  id_to_idx.reserve(blocks.size());
  for (int i = 0; i < n; ++i) {
    id_to_idx.emplace(blocks[i].id, i);
  }

  std::vector<std::vector<int>> adj(n);
  for (const auto &connection : connections) {
    const Block *from = find_block(blocks, connection.a.block_id);
    const Block *to = find_block(blocks, connection.b.block_id);
    if (!from || !to) {
      continue;
    }

    const auto from_port = find_port(*from, connection.a.port_id);
    const auto to_port = find_port(*to, connection.b.port_id);
    if (!from_port.has_value() || !to_port.has_value()) {
      continue;
    }

    if (is_power_type(from_port->type) && is_power_type(to_port->type)) {
      add_edge(id_to_idx, adj, from->id, to->id);
    } else if (from_port->type == PortType::AnalogIn || from_port->type == PortType::AnalogOut ||
               to_port->type == PortType::AnalogIn || to_port->type == PortType::AnalogOut) {
      add_edge(id_to_idx, adj, from->id, to->id);
    }
  }

  std::vector<int> comp(n, -1);
  int comp_count = 0;
  for (int i = 0; i < n; ++i) {
    if (comp[i] != -1) {
      continue;
    }

    std::vector<int> stack = {i};
    comp[i] = comp_count;
    while (!stack.empty()) {
      const int v = stack.back();
      stack.pop_back();
      for (int u : adj[v]) {
        if (comp[u] == -1) {
          comp[u] = comp_count;
          stack.push_back(u);
        }
      }
    }
    ++comp_count;
  }

  std::vector<std::vector<int>> comps(comp_count);
  for (int i = 0; i < n; ++i) {
    comps[comp[i]].push_back(i);
  }

  const int cols = 4;
  const double margin_x = 30.0;
  const double margin_y = 30.0;
  const double spacing_x = 260.0;
  const double spacing_y = 130.0;
  const double comp_gap_y = 30.0;

  double y_cursor = margin_y;
  for (const auto &indices : comps) {
    int row = 0;
    int col = 0;
    for (int idx : indices) {
      blocks[idx].canvas_pos = CanvasPoint{margin_x + col * spacing_x, y_cursor + row * spacing_y};
      ++col;
      if (col >= cols) {
        col = 0;
        ++row;
      }
    }
    const int rows_used = (static_cast<int>(indices.size()) + cols - 1) / cols;
    y_cursor += rows_used * spacing_y + comp_gap_y;
  }
}

} // namespace pep::modules::project_design
