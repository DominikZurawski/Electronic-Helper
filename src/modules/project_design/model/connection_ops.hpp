#pragma once

#include "model.hpp"

#include <optional>
#include <vector>

namespace pep::modules::project_design {

bool connection_exists(const std::vector<Connection> &connections, const Endpoint &a,
                       const Endpoint &b);

bool try_add_connection(std::vector<Connection> &connections, const Endpoint &a, const Endpoint &b);

void connect_new_block(std::vector<Connection> &connections, const std::vector<Block> &blocks,
                       std::optional<Endpoint> pending_connection,
                       std::optional<int> previous_active_id, int new_block_id);

void remove_block(std::vector<Block> &blocks, std::vector<Connection> &connections, int block_id);

void remap_block_power_connections(const std::vector<Block> &blocks,
                                   std::vector<Connection> &connections, int block_id, int psu_id);

int connected_power_block_id(const std::vector<Block> &blocks,
                             const std::vector<Connection> &connections, int block_id);

void apply_amp_requirements_to_power(std::vector<Block> &blocks,
                                     const std::vector<Connection> &connections, int amp_id,
                                     int preferred_psu_id = 0);

} // namespace pep::modules::project_design
