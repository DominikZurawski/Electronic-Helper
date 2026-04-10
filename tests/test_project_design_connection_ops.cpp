#include "src/modules/project_design/model/connection_ops.hpp"
#include "src/modules/project_design/model/model.hpp"

#include <cassert>
#include <vector>

namespace pd = pep::modules::project_design;

namespace {

pd::Endpoint endpoint(int block_id, const char *port_id) {
  return pd::Endpoint{block_id, std::string(port_id)};
}

void test_connection_exists_and_try_add() {
  std::vector<pd::Connection> connections;

  const pd::Endpoint a = endpoint(1, "out");
  const pd::Endpoint b = endpoint(2, "in");

  assert(!pd::connection_exists(connections, a, b));
  assert(pd::try_add_connection(connections, a, b));
  assert(pd::connection_exists(connections, a, b));
  assert(pd::connection_exists(connections, b, a));
  assert(!pd::try_add_connection(connections, a, b));
  assert(!pd::try_add_connection(connections, b, a));
  assert(!pd::try_add_connection(connections, a, a));
  assert(connections.size() == 1);
}

void test_connect_new_block_from_pending_connection_prefers_compatible_port() {
  std::vector<pd::Block> blocks;
  blocks.push_back(pd::make_amp_model1b_block(1, "Amp #1"));
  blocks.push_back(pd::make_amp_model1b_block(2, "Amp #2"));
  std::vector<pd::Connection> connections;

  pd::connect_new_block(connections, blocks, endpoint(1, "out"), std::nullopt, 2);

  assert(connections.size() == 1);
  assert(connections.front().a.block_id == 1);
  assert(connections.front().a.port_id == "out");
  assert(connections.front().b.block_id == 2);
  assert(connections.front().b.port_id == "in");
}

void test_connect_new_block_auto_connects_power_rails() {
  std::vector<pd::Block> blocks;
  blocks.push_back(pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuSymmetric));
  blocks.push_back(pd::make_amp_model1b_block(2, "Amp #2"));
  std::vector<pd::Connection> connections;

  pd::connect_new_block(connections, blocks, std::nullopt, 1, 2);

  assert(connections.size() == 3);
  assert(pd::connection_exists(connections, endpoint(1, "vcc"), endpoint(2, "vcc")));
  assert(pd::connection_exists(connections, endpoint(1, "vee"), endpoint(2, "vee")));
  assert(pd::connection_exists(connections, endpoint(1, "gnd"), endpoint(2, "gnd")));
}

void test_remove_block_removes_related_connections() {
  std::vector<pd::Block> blocks;
  blocks.push_back(pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuSymmetric));
  blocks.push_back(pd::make_amp_model1b_block(2, "Amp #2"));
  blocks.push_back(pd::make_amp_model1b_block(3, "Amp #3"));

  std::vector<pd::Connection> connections = {
      {endpoint(1, "vcc"), endpoint(2, "vcc")},
      {endpoint(2, "out"), endpoint(3, "in")},
      {endpoint(1, "gnd"), endpoint(3, "gnd")},
  };

  pd::remove_block(blocks, connections, 2);

  assert(blocks.size() == 2);
  assert(connections.size() == 1);
  assert(connections.front().a.block_id == 1);
  assert(connections.front().b.block_id == 3);
}

void test_remap_block_power_connections_replaces_existing_power_mapping() {
  std::vector<pd::Block> blocks;
  blocks.push_back(pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuSymmetric));
  blocks.push_back(pd::make_power_block(2, "PSU #2", pd::BlockVariant::PsuSymmetric));
  blocks.push_back(pd::make_amp_model1b_block(3, "Amp #3"));
  blocks.push_back(pd::make_amp_model1b_block(4, "Amp #4"));

  std::vector<pd::Connection> connections = {
      {endpoint(1, "vcc"), endpoint(3, "vcc")},
      {endpoint(1, "vee"), endpoint(3, "vee")},
      {endpoint(1, "gnd"), endpoint(3, "gnd")},
      {endpoint(3, "out"), endpoint(4, "in")},
  };

  pd::remap_block_power_connections(blocks, connections, 3, 2);

  assert(!pd::connection_exists(connections, endpoint(1, "vcc"), endpoint(3, "vcc")));
  assert(!pd::connection_exists(connections, endpoint(1, "vee"), endpoint(3, "vee")));
  assert(!pd::connection_exists(connections, endpoint(1, "gnd"), endpoint(3, "gnd")));
  assert(pd::connection_exists(connections, endpoint(2, "vcc"), endpoint(3, "vcc")));
  assert(pd::connection_exists(connections, endpoint(2, "vee"), endpoint(3, "vee")));
  assert(pd::connection_exists(connections, endpoint(2, "gnd"), endpoint(3, "gnd")));
  assert(pd::connection_exists(connections, endpoint(3, "out"), endpoint(4, "in")));
}

void test_symmetric_power_supply_exposes_only_standard_power_ports() {
  auto block = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuSymmetric);

  const auto ports = pd::ports_for(block);

  assert(ports.size() == 3);
  assert(ports[0].id == "vcc");
  assert(ports[1].id == "vee");
  assert(ports[2].id == "gnd");
}

} // namespace

int main() {
  test_connection_exists_and_try_add();
  test_connect_new_block_from_pending_connection_prefers_compatible_port();
  test_connect_new_block_auto_connects_power_rails();
  test_remove_block_removes_related_connections();
  test_remap_block_power_connections_replaces_existing_power_mapping();
  test_symmetric_power_supply_exposes_only_standard_power_ports();
  return 0;
}
