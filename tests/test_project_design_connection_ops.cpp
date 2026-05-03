#include "src/modules/project_design/model/connection_ops.hpp"
#include "src/modules/project_design/model/model.hpp"

#include <cassert>
#include <cmath>
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

void test_nonsymmetric_power_supply_exposes_positive_and_ground_only() {
  auto block = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuUnregulated);

  const auto ports = pd::ports_for(block);

  assert(ports.size() == 3);
  assert(ports[0].id == "vcc");
  assert(ports[1].id == "vee");
  assert(ports[2].id == "gnd");
}

void test_connect_new_block_with_nonsymmetric_supply_maps_negative_rail_to_ground() {
  std::vector<pd::Block> blocks;
  blocks.push_back(pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuUnregulated));
  blocks.push_back(pd::make_amp_model1b_block(2, "Amp #2"));
  std::vector<pd::Connection> connections;

  pd::connect_new_block(connections, blocks, std::nullopt, 1, 2);

  assert(pd::connection_exists(connections, endpoint(1, "vcc"), endpoint(2, "vcc")));
  assert(pd::connection_exists(connections, endpoint(1, "vee"), endpoint(2, "vee")));
  assert(pd::connection_exists(connections, endpoint(1, "gnd"), endpoint(2, "gnd")));
}

void test_remap_block_power_connections_maps_negative_rail_to_ground_for_nonsymmetric_supply() {
  std::vector<pd::Block> blocks;
  blocks.push_back(pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuUnregulated));
  blocks.push_back(pd::make_amp_model1b_block(2, "Amp #2"));

  std::vector<pd::Connection> connections;
  pd::remap_amp_power_connections(blocks, connections, 2, 1, 1);

  assert(pd::connection_exists(connections, endpoint(1, "vcc"), endpoint(2, "vcc")));
  assert(pd::connection_exists(connections, endpoint(1, "vee"), endpoint(2, "vee")));
  assert(pd::connection_exists(connections, endpoint(1, "gnd"), endpoint(2, "gnd")));
}

void test_apply_amp_requirements_updates_connected_power_block() {
  std::vector<pd::Block> blocks;
  blocks.push_back(pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuSymmetric));
  blocks.push_back(pd::make_amp_model1b_block(2, "Amp #2"));
  std::vector<pd::Connection> connections;

  pd::remap_block_power_connections(blocks, connections, 2, 1);

  pd::Block *amp = pd::find_block(blocks, 2);
  pd::Block *psu = pd::find_block(blocks, 1);
  assert(amp != nullptr);
  assert(psu != nullptr);

  amp->amp_design_mode = pd::AmpDesignMode::SupplyForAmp;
  amp->amp_power_pos_source_id = 1;
  amp->load_resistance_ohm = 8.0;
  amp->target_power_w = 20.0;
  amp->supply_headroom_v = 4.0;
  amp->max_ripple_vpp = 1.0;

  pd::apply_amp_requirements_to_power(blocks, connections, 2);

  assert(psu->transformer_solve_mode == pd::TransformerSolveMode::RatioFromSecondary);
  assert(psu->transformer_voltage_quantity == pd::VoltageQuantity::Rms);
  assert(psu->transformer_secondary_v > 17.0 && psu->transformer_secondary_v < 17.3);
  assert(psu->vin_ac_rms > 17.0 && psu->vin_ac_rms < 17.3);
  assert(psu->load_current > 1.57 && psu->load_current < 1.59);
  assert(psu->max_ripple_vpp > 0.99 && psu->max_ripple_vpp < 1.01);
  assert(psu->capacitor_uF > 15800.0 && psu->capacitor_uF < 15850.0);
  assert(std::abs(psu->transformer_turns_ratio -
                  (psu->transformer_primary_v / psu->transformer_secondary_v)) < 1e-9);
}

void test_connect_new_block_auto_connects_power_to_regulator_then_amp() {
  std::vector<pd::Block> blocks;
  blocks.push_back(pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuUnregulated));
  blocks.push_back(pd::make_regulator_block(2, "Reg #2", pd::BlockVariant::RegZener));
  std::vector<pd::Connection> connections;

  pd::connect_new_block(connections, blocks, std::nullopt, 1, 2);

  assert(pd::connection_exists(connections, endpoint(1, "vcc"), endpoint(2, "vin")));
  assert(pd::connection_exists(connections, endpoint(1, "gnd"), endpoint(2, "gnd")));

  blocks.push_back(pd::make_amp_model1b_block(3, "Amp #3"));
  pd::connect_new_block(connections, blocks, std::nullopt, 2, 3);

  assert(pd::connection_exists(connections, endpoint(2, "vout"), endpoint(3, "vcc")));
  assert(pd::connection_exists(connections, endpoint(2, "gnd"), endpoint(3, "gnd")));
}

void test_connect_new_block_inserts_regulator_between_supply_and_existing_load() {
  std::vector<pd::Block> blocks;
  blocks.push_back(pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuUnregulated));
  blocks.push_back(pd::make_amp_model1b_block(2, "Amp #2"));
  std::vector<pd::Connection> connections;

  pd::connect_new_block(connections, blocks, std::nullopt, 1, 2);

  blocks.push_back(pd::make_regulator_block(3, "Reg #3", pd::BlockVariant::RegZener));
  pd::connect_new_block(connections, blocks, std::nullopt, 2, 3);

  assert(pd::connection_exists(connections, endpoint(1, "vcc"), endpoint(3, "vin")));
  assert(pd::connection_exists(connections, endpoint(1, "gnd"), endpoint(3, "gnd")));
  assert(pd::connection_exists(connections, endpoint(3, "vout"), endpoint(2, "vcc")));
  assert(pd::connection_exists(connections, endpoint(3, "gnd"), endpoint(2, "vee")));
  assert(pd::connection_exists(connections, endpoint(3, "gnd"), endpoint(2, "gnd")));
  assert(!pd::connection_exists(connections, endpoint(1, "vcc"), endpoint(2, "vcc")));
  assert(!pd::connection_exists(connections, endpoint(1, "vee"), endpoint(2, "vee")));
  assert(!pd::connection_exists(connections, endpoint(1, "gnd"), endpoint(2, "gnd")));
}

void test_connected_power_block_resolves_upstream_power_through_regulator() {
  std::vector<pd::Block> blocks;
  blocks.push_back(pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuUnregulated));
  blocks.push_back(pd::make_regulator_block(2, "Reg #2", pd::BlockVariant::RegZener));
  blocks.push_back(pd::make_amp_model1b_block(3, "Amp #3"));
  blocks[1].regulator_input_source_id = 1;

  std::vector<pd::Connection> connections;
  pd::remap_block_power_connections(blocks, connections, 2, 1);
  pd::remap_block_power_connections(blocks, connections, 3, 2);

  assert(pd::connected_direct_supply_block_id(blocks, connections, 3) == 2);
  assert(pd::resolve_power_block_id(blocks, connections, 2) == 1);
  assert(pd::connected_power_block_id(blocks, connections, 3) == 1);
}

void test_negative_rail_regulator_connects_to_symmetric_supply_and_amp_vee() {
  std::vector<pd::Block> blocks;
  blocks.push_back(pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuSymmetric));
  blocks.push_back(pd::make_regulator_block(2, "Reg #2", pd::BlockVariant::RegZener));
  blocks.push_back(pd::make_amp_model1b_block(3, "Amp #3"));
  blocks[1].regulator_supply_rail = pd::SupplyRail::Vee;
  blocks[1].regulator_input_source_id = 1;

  std::vector<pd::Connection> connections;
  pd::remap_block_power_connections(blocks, connections, 2, 1);
  pd::remap_block_power_connections(blocks, connections, 3, 2);

  assert(pd::connection_exists(connections, endpoint(1, "vee"), endpoint(2, "vin")));
  assert(pd::connection_exists(connections, endpoint(1, "gnd"), endpoint(2, "gnd")));
  assert(pd::connection_exists(connections, endpoint(2, "vout"), endpoint(3, "vee")));
  assert(pd::connection_exists(connections, endpoint(2, "gnd"), endpoint(3, "gnd")));
}

void test_amp_can_take_vcc_and_vee_from_two_separate_regulators() {
  std::vector<pd::Block> blocks;
  blocks.push_back(pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuSymmetric));
  blocks.push_back(pd::make_regulator_block(2, "Reg +", pd::BlockVariant::RegZener));
  blocks.push_back(pd::make_regulator_block(3, "Reg -", pd::BlockVariant::RegZener));
  blocks.push_back(pd::make_amp_model1b_block(4, "Amp #4"));
  blocks[2].regulator_supply_rail = pd::SupplyRail::Vee;

  std::vector<pd::Connection> connections;
  pd::remap_block_power_connections(blocks, connections, 2, 1);
  pd::remap_block_power_connections(blocks, connections, 3, 1);
  pd::remap_amp_power_connections(blocks, connections, 4, 2, 3);

  assert(pd::connection_exists(connections, endpoint(2, "vout"), endpoint(4, "vcc")));
  assert(pd::connection_exists(connections, endpoint(3, "vout"), endpoint(4, "vee")));
  assert(pd::connection_exists(connections, endpoint(2, "gnd"), endpoint(4, "gnd")) ||
         pd::connection_exists(connections, endpoint(3, "gnd"), endpoint(4, "gnd")));
  assert(!pd::connection_exists(connections, endpoint(2, "gnd"), endpoint(4, "vee")));
}

void test_amp_uses_ground_as_negative_rail_for_nonsymmetric_supply_when_vee_unset() {
  std::vector<pd::Block> blocks;
  blocks.push_back(pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuUnregulated));
  blocks.push_back(pd::make_amp_model1b_block(2, "Amp #2"));

  std::vector<pd::Connection> connections;
  pd::remap_amp_power_connections(blocks, connections, 2, 1, 0);

  assert(pd::connection_exists(connections, endpoint(1, "vcc"), endpoint(2, "vcc")));
  assert(pd::connection_exists(connections, endpoint(1, "gnd"), endpoint(2, "vee")));
  assert(pd::connection_exists(connections, endpoint(1, "gnd"), endpoint(2, "gnd")));
}

} // namespace

int main() {
  test_connection_exists_and_try_add();
  test_connect_new_block_from_pending_connection_prefers_compatible_port();
  test_connect_new_block_auto_connects_power_rails();
  test_remove_block_removes_related_connections();
  test_remap_block_power_connections_replaces_existing_power_mapping();
  test_symmetric_power_supply_exposes_only_standard_power_ports();
  test_nonsymmetric_power_supply_exposes_positive_and_ground_only();
  test_connect_new_block_with_nonsymmetric_supply_maps_negative_rail_to_ground();
  test_remap_block_power_connections_maps_negative_rail_to_ground_for_nonsymmetric_supply();
  test_apply_amp_requirements_updates_connected_power_block();
  test_connect_new_block_auto_connects_power_to_regulator_then_amp();
  test_connect_new_block_inserts_regulator_between_supply_and_existing_load();
  test_connected_power_block_resolves_upstream_power_through_regulator();
  test_negative_rail_regulator_connects_to_symmetric_supply_and_amp_vee();
  test_amp_can_take_vcc_and_vee_from_two_separate_regulators();
  test_amp_uses_ground_as_negative_rail_for_nonsymmetric_supply_when_vee_unset();
  return 0;
}
