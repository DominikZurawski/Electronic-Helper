#include "connection_ops.hpp"

#include "../../psu_basic/model/model.hpp"

#include <algorithm>
#include <cmath>

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

void connect_power_ports(std::vector<Connection> &connections, int source_block_id,
                         const std::string &source_port_id, int target_block_id,
                         const std::string &target_port_id) {
  if (source_port_id.empty() || target_port_id.empty()) {
    return;
  }
  try_add_connection(connections, Endpoint{source_block_id, source_port_id},
                     Endpoint{target_block_id, target_port_id});
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

  const auto from_vcc = unique_port(from_ports, PortType::PowerPos);
  const auto from_vee = unique_port(from_ports, PortType::PowerNeg);
  const auto from_gnd = unique_port(from_ports, PortType::Ground);
  const auto to_vcc = unique_port(to_ports, PortType::PowerPos);
  const auto to_vee = unique_port(to_ports, PortType::PowerNeg);
  const auto to_gnd = unique_port(to_ports, PortType::Ground);

  if (from_is_psu) {
    if (from_vcc && to_vcc) {
      connect_power_ports(connections, from_id, from_vcc->id, to_id, to_vcc->id);
    }
    if (from_vee && to_vee) {
      connect_power_ports(connections, from_id, from_vee->id, to_id, to_vee->id);
    } else if (from_gnd && to_vee) {
      connect_power_ports(connections, from_id, from_gnd->id, to_id, to_vee->id);
    }
    if (from_gnd && to_gnd) {
      connect_power_ports(connections, from_id, from_gnd->id, to_id, to_gnd->id);
    }
    return;
  }

  if (to_vcc && from_vcc) {
    connect_power_ports(connections, to_id, to_vcc->id, from_id, from_vcc->id);
  }
  if (to_vee && from_vee) {
    connect_power_ports(connections, to_id, to_vee->id, from_id, from_vee->id);
  } else if (to_gnd && from_vee) {
    connect_power_ports(connections, to_id, to_gnd->id, from_id, from_vee->id);
  }
  if (to_gnd && from_gnd) {
    connect_power_ports(connections, to_id, to_gnd->id, from_id, from_gnd->id);
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
  } else if (!active_vee.empty() && !psu_gnd.empty()) {
    try_add_connection(connections, Endpoint{psu_id, psu_gnd}, Endpoint{block_id, active_vee});
  }
  if (!active_gnd.empty() && !psu_gnd.empty()) {
    try_add_connection(connections, Endpoint{psu_id, psu_gnd}, Endpoint{block_id, active_gnd});
  }
}

int connected_power_block_id(const std::vector<Block> &blocks,
                             const std::vector<Connection> &connections, int block_id) {
  const Block *active = find_block(blocks, block_id);
  if (!active) {
    return 0;
  }

  for (const auto &connection : connections) {
    const Endpoint a = connection.a;
    const Endpoint b = connection.b;
    const bool a_is_active = (a.block_id == block_id);
    const bool b_is_active = (b.block_id == block_id);
    if (!a_is_active && !b_is_active) {
      continue;
    }

    const Endpoint other = a_is_active ? b : a;
    const Block *other_block = find_block(blocks, other.block_id);
    if (!other_block || other_block->kind != BlockKind::Power) {
      continue;
    }

    const auto active_port = find_port(*active, a_is_active ? a.port_id : b.port_id);
    if (!active_port.has_value() || !is_power_type(active_port->type)) {
      continue;
    }

    return other_block->id;
  }

  return 0;
}

void apply_amp_requirements_to_power(std::vector<Block> &blocks,
                                     const std::vector<Connection> &connections, int amp_id,
                                     int preferred_psu_id) {
  Block *amp = find_block(blocks, amp_id);
  if (!amp || amp->kind != BlockKind::Amplifier) {
    return;
  }

  int psu_id = preferred_psu_id;
  if (psu_id == 0) {
    psu_id = amp->amp_power_source_id;
  }
  if (psu_id == 0) {
    psu_id = connected_power_block_id(blocks, connections, amp_id);
  }
  if (psu_id == 0) {
    return;
  }

  Block *psu = find_block(blocks, psu_id);
  if (!psu || psu->kind != BlockKind::Power) {
    return;
  }

  const auto waveform_shape_for = [](SignalWaveform waveform) {
    if (waveform == SignalWaveform::Square) {
      return pep::modules::psu_basic::WaveformShape::Square;
    }
    if (waveform == SignalWaveform::Triangle) {
      return pep::modules::psu_basic::WaveformShape::Triangle;
    }
    return pep::modules::psu_basic::WaveformShape::Sine;
  };

  if (amp->load_resistance_ohm > 0.0) {
    psu->load_resistance_ohm = amp->load_resistance_ohm;
  }

  const bool has_target = amp->load_resistance_ohm > 0.0 && amp->target_power_w > 0.0;
  const bool has_waveform_power =
      amp->load_resistance_ohm > 0.0 && amp->signal_amp_v > 0.0 && amp->gain > 0.0;
  if ((has_target || has_waveform_power) && amp->amp_design_mode == AmpDesignMode::SupplyForAmp) {
    const double vout_peak = amp->signal_amp_v * amp->gain;
    const double v_rms_need =
        has_target ? std::sqrt(amp->target_power_w * amp->load_resistance_ohm)
                   : pep::modules::psu_basic::peak_to_rms(
                         vout_peak, waveform_shape_for(amp->signal_waveform));
    psu->load_current = v_rms_need / amp->load_resistance_ohm;

    const double v_peak_need =
        has_target ? pep::modules::psu_basic::rms_to_peak(
                         v_rms_need, waveform_shape_for(amp->signal_waveform))
                   : vout_peak;
    double v_rect_req = v_peak_need + amp->supply_headroom_v;
    if (amp->max_ripple_vpp > 0.0) {
      v_rect_req += amp->max_ripple_vpp;
    }
    const double v_peak_secondary = v_rect_req + 2.0 * psu->diode_drop;
    psu->transformer_secondary_v = v_peak_secondary / std::sqrt(2.0);
    psu->vin_ac_rms = psu->transformer_secondary_v;
    psu->transformer_solve_mode = TransformerSolveMode::RatioFromSecondary;
    psu->transformer_voltage_quantity = VoltageQuantity::Rms;
    if (psu->transformer_primary_v > 0.0 && psu->transformer_secondary_v > 0.0) {
      psu->transformer_turns_ratio = psu->transformer_primary_v / psu->transformer_secondary_v;
    }
  }

  if (amp->max_ripple_vpp > 0.0) {
    psu->max_ripple_vpp = amp->max_ripple_vpp;
    const double ripple_hz = psu->mains_hz > 0.0 ? psu->mains_hz * 2.0 : 0.0;
    if (psu->load_current > 0.0 && ripple_hz > 0.0) {
      psu->capacitor_uF = pep::modules::psu_basic::required_capacitance_uF(
          psu->load_current, psu->max_ripple_vpp, ripple_hz);
    }
  }
}

} // namespace pep::modules::project_design
