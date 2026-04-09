#include "connection_ops.hpp"

#include <algorithm>

namespace pep::modules::project_design {

namespace {

std::optional<PortDef> unique_port(const std::vector<PortDef> &ports, PortType type) {
  std::optional<PortDef> found;
  for (const auto &port : ports) {
    if (port.type != type) {
      continue;
    }
    if (found.has_value()) {
      return std::nullopt;
    }
    found = port;
  }
  return found;
}

bool is_power_type(PortType type) {
  return type == PortType::PowerPos || type == PortType::PowerNeg || type == PortType::Ground;
}

bool is_power_endpoint(const std::vector<Block> &blocks, const Endpoint &endpoint) {
  const Block *block = find_block(blocks, endpoint.block_id);
  if (!block) {
    return false;
  }

  const auto port = find_port(*block, endpoint.port_id);
  return port.has_value() && is_power_type(port->type);
}

void auto_connect_blocks(std::vector<Connection> &connections, const std::vector<Block> &blocks,
                         int from_id, int to_id) {
  const Block *from = find_block(blocks, from_id);
  const Block *to = find_block(blocks, to_id);
  if (!from || !to) {
    return;
  }

  const bool from_is_psu = (from->kind == BlockKind::Power);
  const bool to_is_psu = (to->kind == BlockKind::Power);
  if (from_is_psu == to_is_psu) {
    return;
  }

  const auto from_ports = ports_for(*from);
  const auto to_ports = ports_for(*to);
  for (const PortType type : {PortType::PowerPos, PortType::PowerNeg, PortType::Ground}) {
    const auto from_port = unique_port(from_ports, type);
    const auto to_port = unique_port(to_ports, type);
    if (from_port.has_value() && to_port.has_value()) {
      try_add_connection(connections, Endpoint{from_id, from_port->id},
                         Endpoint{to_id, to_port->id});
    }
  }
}

std::string first_compatible_port_id(const Block &block, PortType type) {
  for (const auto &port : ports_for(block)) {
    if (ports_compatible(type, port.type)) {
      return port.id;
    }
  }

  const auto ports = ports_for(block);
  if (!ports.empty()) {
    return ports.front().id;
  }
  return "";
}

std::string first_port_id(const std::vector<PortDef> &ports, PortType type) {
  for (const auto &port : ports) {
    if (port.type == type) {
      return port.id;
    }
  }
  return "";
}

} // namespace

bool connection_exists(const std::vector<Connection> &connections, const Endpoint &a,
                       const Endpoint &b) {
  for (const auto &connection : connections) {
    const bool same_order =
        connection.a.block_id == a.block_id && connection.a.port_id == a.port_id &&
        connection.b.block_id == b.block_id && connection.b.port_id == b.port_id;
    const bool reversed_order =
        connection.a.block_id == b.block_id && connection.a.port_id == b.port_id &&
        connection.b.block_id == a.block_id && connection.b.port_id == a.port_id;
    if (same_order || reversed_order) {
      return true;
    }
  }
  return false;
}

bool try_add_connection(std::vector<Connection> &connections, const Endpoint &a,
                        const Endpoint &b) {
  if (a.block_id == b.block_id && a.port_id == b.port_id) {
    return false;
  }
  if (connection_exists(connections, a, b)) {
    return false;
  }

  connections.push_back(Connection{a, b});
  return true;
}

void connect_new_block(std::vector<Connection> &connections, const std::vector<Block> &blocks,
                       std::optional<Endpoint> pending_connection,
                       std::optional<int> previous_active_id, int new_block_id) {
  if (pending_connection.has_value()) {
    const Block *start_block = find_block(blocks, pending_connection->block_id);
    const Block *new_block = find_block(blocks, new_block_id);
    if (!start_block || !new_block) {
      return;
    }

    const auto start_port = find_port(*start_block, pending_connection->port_id);
    const PortType start_type = start_port.has_value() ? start_port->type : PortType::Unknown;
    const std::string best_port_id = first_compatible_port_id(*new_block, start_type);
    if (!best_port_id.empty()) {
      try_add_connection(connections, *pending_connection, Endpoint{new_block_id, best_port_id});
    }
    return;
  }

  if (previous_active_id.has_value()) {
    auto_connect_blocks(connections, blocks, *previous_active_id, new_block_id);
  }
}

void remove_block(std::vector<Block> &blocks, std::vector<Connection> &connections, int block_id) {
  blocks.erase(std::remove_if(blocks.begin(), blocks.end(),
                              [&](const Block &block) { return block.id == block_id; }),
               blocks.end());
  connections.erase(std::remove_if(connections.begin(), connections.end(),
                                   [&](const Connection &connection) {
                                     return connection.a.block_id == block_id ||
                                            connection.b.block_id == block_id;
                                   }),
                    connections.end());
}

void remap_block_power_connections(const std::vector<Block> &blocks,
                                   std::vector<Connection> &connections, int block_id, int psu_id) {
  const Block *active = find_block(blocks, block_id);
  if (!active || active->kind == BlockKind::Power) {
    return;
  }

  connections.erase(std::remove_if(connections.begin(), connections.end(),
                                   [&](const Connection &connection) {
                                     if (connection.a.block_id != block_id &&
                                         connection.b.block_id != block_id) {
                                       return false;
                                     }
                                     return is_power_endpoint(blocks, connection.a) ||
                                            is_power_endpoint(blocks, connection.b);
                                   }),
                    connections.end());

  if (psu_id == 0) {
    return;
  }

  const Block *psu = find_block(blocks, psu_id);
  if (!psu) {
    return;
  }

  const auto active_ports = ports_for(*active);
  const auto psu_ports = ports_for(*psu);
  const std::string active_vcc = first_port_id(active_ports, PortType::PowerPos);
  const std::string active_vee = first_port_id(active_ports, PortType::PowerNeg);
  const std::string active_gnd = first_port_id(active_ports, PortType::Ground);
  const std::string psu_vcc = first_port_id(psu_ports, PortType::PowerPos);
  const std::string psu_vee = first_port_id(psu_ports, PortType::PowerNeg);
  const std::string psu_gnd = first_port_id(psu_ports, PortType::Ground);

  if (!active_vcc.empty() && !psu_vcc.empty()) {
    try_add_connection(connections, Endpoint{psu_id, psu_vcc}, Endpoint{block_id, active_vcc});
  }
  if (!active_vee.empty() && !psu_vee.empty()) {
    try_add_connection(connections, Endpoint{psu_id, psu_vee}, Endpoint{block_id, active_vee});
  }
  if (!active_gnd.empty() && !psu_gnd.empty()) {
    try_add_connection(connections, Endpoint{psu_id, psu_gnd}, Endpoint{block_id, active_gnd});
  }
}

} // namespace pep::modules::project_design
