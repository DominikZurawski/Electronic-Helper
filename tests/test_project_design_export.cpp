#include "src/modules/project_design/export/asc_ops.hpp"
#include "src/modules/project_design/export/export.hpp"
#include "src/modules/project_design/export/export_topology.hpp"
#include "src/modules/project_design/export/export_value_builders.hpp"
#include "src/modules/project_design/model/model.hpp"

#include <cassert>
#include <optional>
#include <string>
#include <vector>

namespace pd = pep::modules::project_design;

namespace {

bool contains(const std::string &text, const std::string &needle) {
  return text.find(needle) != std::string::npos;
}

bool contains_warning(const std::vector<std::string> &warnings, const std::string &needle) {
  for (const auto &warning : warnings) {
    if (contains(warning, needle)) {
      return true;
    }
  }
  return false;
}

pd::Endpoint endpoint(int block_id, const char *port_id) {
  return pd::Endpoint{block_id, std::string(port_id)};
}

void test_export_project_adds_header_and_tran_directive() {
  const auto result = pd::export_project_asc({}, {}, std::optional<std::string>{".tran 0 10m"});

  assert(contains(result.asc, "Version 4.1"));
  assert(contains(result.asc, "SHEET 1 "));
  assert(contains(result.asc, "TEXT 40 40 Left 2 !.tran 0 10m"));
}

void test_export_project_warns_for_unsupported_power_variant() {
  auto block = pd::make_power_block(1, "Zasilanie #1", pd::BlockVariant::PsuUnregulated);
  const auto result = pd::export_project_asc({block}, {}, std::nullopt);

  assert(!contains_warning(result.warnings, "Brak szablonu LTspice dla wariantu: psu_unregulated"));
  assert(!contains(result.asc, "PPE: brak schematu dla wariantu \"psu_unregulated\""));
  assert(contains(result.asc, "SYMATTR InstName V1_B1"));
  assert(contains(result.asc, "SYMATTR InstName C1_B1"));
  assert(contains(result.asc, "SYMATTR InstName Rload_B1"));
  assert(contains(result.asc, "WIRE "));
  assert(contains(result.asc, "Vcc_B1"));
  assert(contains(result.asc, "Vee_B1"));
}

void test_export_project_omits_dummy_load_when_real_consumer_is_connected() {
  const auto power = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuUnregulated);
  const auto amp = pd::make_amp_model1b_block(2, "Amp #2");
  const std::vector<pd::Connection> connections = {
      {endpoint(1, "vcc"), endpoint(2, "vcc")},
      {endpoint(1, "gnd"), endpoint(2, "gnd")},
  };

  const auto result = pd::export_project_asc({power, amp}, connections, std::nullopt);

  assert(!contains(result.asc, "Rload_B1"));
  assert(contains(result.asc, "Vcc"));
  assert(!contains_warning(result.warnings, "Nie znaleziono elementu w .asc: Rload"));
}

void test_export_project_keeps_disconnected_nonsymmetric_supply_when_other_supply_drives_amp() {
  const auto power_a = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuUnregulated);
  const auto power_b = pd::make_power_block(2, "PSU #2", pd::BlockVariant::PsuUnregulated);
  const auto amp = pd::make_amp_model1b_block(3, "Amp #3");
  const std::vector<pd::Connection> connections = {
      {endpoint(2, "vcc"), endpoint(3, "vcc")},
      {endpoint(2, "gnd"), endpoint(3, "gnd")},
  };

  const auto result = pd::export_project_asc({power_a, power_b, amp}, connections, std::nullopt);

  assert(contains(result.asc, "SYMATTR InstName V1_B1"));
  assert(contains(result.asc, "SYMATTR InstName C1_B1"));
  assert(contains(result.asc, "SYMATTR InstName Rload_B1"));
  assert(contains(result.asc, "SYMATTR InstName V1_B2"));
  assert(contains(result.asc, "SYMATTR InstName C1_B2"));
  assert(!contains(result.asc, "Rload_B2"));
  assert(contains(result.asc, "SYMATTR InstName U1_B3"));
}

void test_export_topology_ignores_dangling_connections_to_non_exported_blocks() {
  const auto power = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuUnregulated);
  const std::vector<pd::Connection> dangling = {
      {endpoint(1, "vcc"), endpoint(2, "vcc")},
      {endpoint(1, "gnd"), endpoint(2, "gnd")},
  };

  const auto topology = pd::build_export_topology({power}, dangling);

  assert(topology.net_for_endpoint(endpoint(1, "vcc")) == "Vcc_B1");
  assert(topology.net_for_endpoint(endpoint(1, "gnd")) == "0");
}

void test_export_project_warns_for_conflicting_power_net_types() {
  const auto power = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuSymmetric);
  const auto amp = pd::make_amp_model1b_block(2, "Amp #2");
  const std::vector<pd::Connection> connections = {
      {endpoint(1, "vcc"), endpoint(2, "vee")},
  };

  const auto result = pd::export_project_asc({power, amp}, connections, std::nullopt);

  assert(contains_warning(
      result.warnings, "Sprzeczne typy w jednej sieci (Vcc i Vee) — nadaję nazwę automatyczną."));
}

void test_export_topology_maps_negative_rail_to_ground_when_connected_to_nonsymmetric_supply() {
  const auto power = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuUnregulated);
  const auto amp = pd::make_amp_model1b_block(2, "Amp #2");
  const std::vector<pd::Connection> connections = {
      {endpoint(1, "vcc"), endpoint(2, "vcc")},
      {endpoint(1, "vee"), endpoint(2, "vee")},
      {endpoint(1, "gnd"), endpoint(2, "gnd")},
  };

  const auto topology = pd::build_export_topology({power, amp}, connections);

  assert(topology.net_for_endpoint(endpoint(2, "vee")) == "Vee");
}

void test_export_topology_orders_power_before_amplifier_in_component() {
  const auto power = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuSymmetric);
  const auto amp = pd::make_amp_model1b_block(2, "Amp #2");
  const std::vector<pd::Connection> connections = {
      {endpoint(1, "vcc"), endpoint(2, "vcc")},
      {endpoint(1, "vee"), endpoint(2, "vee")},
  };

  const auto topology = pd::build_export_topology({power, amp}, connections);

  assert(topology.ordered_components.size() == 1);
  assert(topology.ordered_components.front().size() == 2);
  assert(topology.ordered_components.front().front() == 0);
  assert(topology.net_for_endpoint(endpoint(1, "vcc")) == "Vcc");
  assert(topology.net_for_endpoint(endpoint(2, "vee")) == "Vee");
}

void test_export_topology_assigns_unique_names_to_disconnected_power_nets() {
  const auto power_a = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuSymmetric);
  const auto power_b = pd::make_power_block(2, "PSU #2", pd::BlockVariant::PsuSymmetric);

  const auto topology = pd::build_export_topology({power_a, power_b}, {});

  const auto net_a = topology.net_for_endpoint(endpoint(1, "vcc"));
  const auto net_b = topology.net_for_endpoint(endpoint(2, "vcc"));

  assert(!net_a.empty());
  assert(!net_b.empty());
  assert(net_a != net_b);
  assert(contains(net_a, "Vcc_B1"));
  assert(contains(net_b, "Vcc_B2"));
}

void test_export_render_builds_square_wave_values() {
  auto amp = pd::make_amp_model1b_block(2, "Amp #2");
  amp.signal_waveform = pd::SignalWaveform::Square;
  amp.signal_amp_v = 2.5;
  amp.signal_hz = 1000.0;

  std::vector<std::string> warnings;
  const auto values = pd::build_amplifier_instance_values(amp, warnings);

  assert(warnings.empty());
  assert(values.contains("V2"));
  assert(contains(values.at("V2"), "PULSE("));
}

void test_export_render_builds_triangle_wave_values() {
  auto amp = pd::make_amp_model1b_block(2, "Amp #2");
  amp.signal_waveform = pd::SignalWaveform::Triangle;
  amp.signal_amp_v = 1.5;
  amp.signal_hz = 500.0;

  std::vector<std::string> warnings;
  const auto values = pd::build_amplifier_instance_values(amp, warnings);

  assert(warnings.empty());
  assert(values.contains("V2"));
  assert(contains(values.at("V2"), "PWL("));
}

void test_export_render_builds_sine_wave_values() {
  auto amp = pd::make_amp_model1b_block(2, "Amp #2");
  amp.signal_waveform = pd::SignalWaveform::Sine;
  amp.signal_amp_v = 0.75;
  amp.signal_hz = 1500.0;

  std::vector<std::string> warnings;
  const auto values = pd::build_amplifier_instance_values(amp, warnings);

  assert(warnings.empty());
  assert(values.contains("V2"));
  assert(contains(values.at("V2"), "SINE("));
}

void test_export_render_warns_for_missing_amplifier_input_values() {
  auto amp = pd::make_amp_model1b_block(2, "Amp #2");
  amp.signal_amp_v = 0.0;

  std::vector<std::string> warnings;
  const auto values = pd::build_amplifier_instance_values(amp, warnings);

  assert(values.empty());
  assert(contains_warning(warnings, "Brak parametrów wejścia wzmacniacza"));
}

void test_export_project_warns_for_unknown_element_kind() {
  pd::Block block;
  block.id = 7;
  block.kind = pd::BlockKind::Unknown;
  block.variant = pd::BlockVariant::Unknown;
  block.title = "Nieznany blok";

  const auto result = pd::export_project_asc({block}, {}, std::nullopt);

  assert(contains_warning(result.warnings, "Brak szablonu LTspice dla elementu: Nieznany blok"));
  assert(contains(result.asc, "PPE: brak schematu dla elementu \"Nieznany blok\""));
}

void test_export_project_adds_k1_for_symmetric_supply() {
  const auto power = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuSymmetric);
  const auto result = pd::export_project_asc({power}, {}, std::nullopt);

  assert(contains(result.asc, "K1_B1 L1_B1 L2_B1 L3_B1 1"));
}

void test_export_project_adds_k1_for_nonsymmetric_supply() {
  const auto power = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuUnregulated);
  const auto result = pd::export_project_asc({power}, {}, std::nullopt);

  assert(contains(result.asc, "K1_B1 L1_B1 L2_B1 1"));
  assert(!contains_warning(result.warnings, "Nie znaleziono elementu w .asc: C2"));
}

void test_export_project_adds_amplifier_gain_directive() {
  auto amp = pd::make_amp_model1b_block(1, "Amp #1");
  amp.gain = 10.0;
  const auto result = pd::export_project_asc({amp}, {}, std::nullopt);

  assert(contains(result.asc, ".param AV_B1=10"));
  assert(contains(result.asc, "BGAIN_B1 OUT_B1 0 V=V(IN_B1)*AV_B1"));
}

void test_export_project_renders_zener_regulator_standalone() {
  const auto regulator = pd::make_regulator_block(1, "Stabilizator #1", pd::BlockVariant::RegZener);
  const auto result = pd::export_project_asc({regulator}, {}, std::nullopt);

  assert(contains(result.asc, "SYMATTR InstName R1_B1"));
  assert(contains(result.asc, "SYMATTR InstName D1_B1"));
  assert(contains(result.asc, "SYMATTR InstName V1_B1"));
  assert(contains(result.asc, "SYMATTR InstName I1_B1"));
}

void test_export_project_removes_local_source_and_load_for_connected_zener_regulator() {
  const auto psu = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuUnregulated);
  const auto regulator =
      pd::make_regulator_block(2, "Stabilizator #2", pd::BlockVariant::RegZener);
  const auto amp = pd::make_amp_model1b_block(3, "Amp #3");
  const std::vector<pd::Connection> connections = {
      {endpoint(1, "vcc"), endpoint(2, "vin")},
      {endpoint(1, "gnd"), endpoint(2, "gnd")},
      {endpoint(2, "vout"), endpoint(3, "vcc")},
      {endpoint(2, "gnd"), endpoint(3, "gnd")},
  };

  const auto result = pd::export_project_asc({psu, regulator, amp}, connections, std::nullopt);

  assert(!contains(result.asc, "SYMATTR InstName V1_B2"));
  assert(!contains(result.asc, "SYMATTR InstName I1_B2"));
  assert(contains(result.asc, "SYMATTR Value PPE_SELECT_ZENER"));
  assert(contains_warning(result.warnings, "wybierz model diody"));
}

void test_export_project_supports_negative_rail_zener_regulator() {
  auto regulator = pd::make_regulator_block(1, "Stabilizator #1", pd::BlockVariant::RegZener);
  regulator.regulator_supply_rail = pd::SupplyRail::Vee;

  const auto result = pd::export_project_asc({regulator}, {}, std::nullopt);

  assert(contains(result.asc, "SYMATTR InstName V1_B1"));
  assert(contains(result.asc, "SYMATTR Value -6"));
}

void test_export_project_renders_zener_bjt_regulator_standalone() {
  const auto regulator =
      pd::make_regulator_block(1, "Stabilizator #1", pd::BlockVariant::RegZenerBjt);
  const auto result = pd::export_project_asc({regulator}, {}, std::nullopt);

  assert(contains(result.asc, "SYMATTR InstName R1_B1"));
  assert(contains(result.asc, "SYMATTR InstName D1_B1"));
  assert(contains(result.asc, "SYMATTR InstName Q1_B1"));
  assert(contains(result.asc, "SYMATTR InstName V1_B1"));
  assert(contains(result.asc, "SYMATTR InstName I1_B1"));
}

void test_export_project_removes_local_source_and_load_for_connected_zener_bjt_regulator() {
  const auto psu = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuUnregulated);
  const auto regulator =
      pd::make_regulator_block(2, "Stabilizator #2", pd::BlockVariant::RegZenerBjt);
  const auto amp = pd::make_amp_model1b_block(3, "Amp #3");
  const std::vector<pd::Connection> connections = {
      {endpoint(1, "vcc"), endpoint(2, "vin")},
      {endpoint(1, "gnd"), endpoint(2, "gnd")},
      {endpoint(2, "vout"), endpoint(3, "vcc")},
      {endpoint(2, "gnd"), endpoint(3, "gnd")},
  };

  const auto result = pd::export_project_asc({psu, regulator, amp}, connections, std::nullopt);

  assert(!contains(result.asc, "SYMATTR InstName V1_B2"));
  assert(!contains(result.asc, "SYMATTR InstName I1_B2"));
  assert(contains(result.asc, "SYMATTR Value PPE_SELECT_ZENER"));
  assert(contains(result.asc, "SYMATTR Value PPE_SELECT_NPN"));
}

void test_export_project_renders_two_zener_regulators_for_symmetric_supply() {
  const auto psu = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuSymmetric);
  auto reg_pos = pd::make_regulator_block(2, "Reg +", pd::BlockVariant::RegZener);
  auto reg_neg = pd::make_regulator_block(3, "Reg -", pd::BlockVariant::RegZener);
  reg_neg.regulator_supply_rail = pd::SupplyRail::Vee;
  const auto amp = pd::make_amp_model1b_block(4, "Amp #4");

  const std::vector<pd::Connection> connections = {
      {endpoint(1, "vcc"), endpoint(2, "vin")},
      {endpoint(1, "gnd"), endpoint(2, "gnd")},
      {endpoint(2, "vout"), endpoint(4, "vcc")},
      {endpoint(1, "vee"), endpoint(3, "vin")},
      {endpoint(1, "gnd"), endpoint(3, "gnd")},
      {endpoint(3, "vout"), endpoint(4, "vee")},
      {endpoint(2, "gnd"), endpoint(4, "gnd")},
      {endpoint(3, "gnd"), endpoint(4, "gnd")},
  };

  const auto result = pd::export_project_asc({psu, reg_pos, reg_neg, amp}, connections, std::nullopt);

  assert(contains(result.asc, "SYMATTR InstName D1_B2"));
  assert(contains(result.asc, "SYMATTR InstName D1_B3"));
  assert(!contains(result.asc, "SYMATTR InstName V1_B2"));
  assert(!contains(result.asc, "SYMATTR InstName I1_B2"));
  assert(!contains(result.asc, "SYMATTR InstName V1_B3"));
  assert(!contains(result.asc, "SYMATTR InstName I1_B3"));
}

void test_export_project_renders_two_zener_bjt_regulators_for_symmetric_supply() {
  const auto psu = pd::make_power_block(1, "PSU #1", pd::BlockVariant::PsuSymmetric);
  auto reg_pos = pd::make_regulator_block(2, "Reg +", pd::BlockVariant::RegZenerBjt);
  auto reg_neg = pd::make_regulator_block(3, "Reg -", pd::BlockVariant::RegZenerBjt);
  reg_neg.regulator_supply_rail = pd::SupplyRail::Vee;
  const auto amp = pd::make_amp_model1b_block(4, "Amp #4");

  const std::vector<pd::Connection> connections = {
      {endpoint(1, "vcc"), endpoint(2, "vin")},
      {endpoint(1, "gnd"), endpoint(2, "gnd")},
      {endpoint(2, "vout"), endpoint(4, "vcc")},
      {endpoint(1, "vee"), endpoint(3, "vin")},
      {endpoint(1, "gnd"), endpoint(3, "gnd")},
      {endpoint(3, "vout"), endpoint(4, "vee")},
      {endpoint(2, "gnd"), endpoint(4, "gnd")},
      {endpoint(3, "gnd"), endpoint(4, "gnd")},
  };

  const auto result = pd::export_project_asc({psu, reg_pos, reg_neg, amp}, connections, std::nullopt);

  assert(contains(result.asc, "SYMATTR InstName Q1_B2"));
  assert(contains(result.asc, "SYMATTR InstName Q1_B3"));
  assert(!contains(result.asc, "SYMATTR InstName V1_B2"));
  assert(!contains(result.asc, "SYMATTR InstName I1_B2"));
  assert(!contains(result.asc, "SYMATTR InstName V1_B3"));
  assert(!contains(result.asc, "SYMATTR InstName I1_B3"));
}

void test_append_missing_flags_adds_regulator_input_and_output_flags() {
  const std::string asc = "WIRE 80 96 -32 96\n"
                          "WIRE 256 96 160 96\n"
                          "FLAG -32 320 0\n"
                          "FLAG 256 320 0";
  const std::unordered_map<long long, std::string> xy_to_net = {
      {(static_cast<long long>(-32) << 32) ^ static_cast<unsigned int>(96), "Vcc"},
      {(static_cast<long long>(256) << 32) ^ static_cast<unsigned int>(96), "VREG"},
  };

  const auto patched = pd::append_missing_flags(asc, xy_to_net);

  assert(contains(patched, "FLAG -32 96 Vcc"));
  assert(contains(patched, "FLAG 256 96 VREG"));
}

void test_asc_ops_strip_header_and_shift_coords() {
  const std::string asc = "Version 4.1\n"
                          "SHEET 1 880 680\n"
                          "WIRE 10 20 30 40\n"
                          "TEXT 50 60 Left 2 !.ac";

  const auto stripped = pd::strip_header(asc);
  assert(!contains(stripped, "Version 4.1"));
  assert(!contains(stripped, "SHEET 1 880 680"));
  assert(contains(stripped, "WIRE 10 20 30 40"));

  const auto shifted = pd::shift_coords(stripped, 5, -10);
  assert(contains(shifted, "WIRE 15 10 35 30"));
  assert(contains(shifted, "TEXT 55 50 Left 2 !.ac"));
}

void test_asc_ops_uniquify_instnames_and_flag_nets() {
  const std::string asc = "SYMATTR InstName V1\n"
                          "TEXT 0 0 Left 2 !K1 V1 V2 0.9\n"
                          "FLAG 10 20 NET_A\n"
                          "FLAG 30 40 Vcc";

  const auto with_unique_inst = pd::uniquify_instnames(asc, "_B7");
  assert(contains(with_unique_inst, "SYMATTR InstName V1_B7"));
  assert(contains(with_unique_inst, "!K1_B7 V1_B7 V2 0.9"));

  const std::unordered_set<std::string> reserved_nets = {"Vcc"};
  const auto with_unique_nets = pd::uniquify_flag_nets(with_unique_inst, "_B7", reserved_nets);
  assert(contains(with_unique_nets, "FLAG 10 20 NET_A_B7"));
  assert(contains(with_unique_nets, "FLAG 30 40 Vcc"));
}

void test_asc_ops_patch_flags_updates_matching_coordinates_and_warns_for_missing() {
  const std::string asc = "FLAG 10 20 NET_A\n"
                          "FLAG 30 40 NET_B";

  std::vector<std::string> warnings;
  const std::unordered_map<long long, std::string> xy_to_net = {
      {(static_cast<long long>(10) << 32) ^ static_cast<unsigned int>(20), "Vcc"},
      {(static_cast<long long>(99) << 32) ^ static_cast<unsigned int>(100), "SIG"},
  };

  const auto patched = pd::patch_flags(asc, xy_to_net, warnings);

  assert(contains(patched.asc, "FLAG 10 20 Vcc"));
  assert(contains(patched.asc, "FLAG 30 40 NET_B"));
  assert(contains_warning(warnings, "Nie znaleziono FLAG do podmiany netu"));
}

} // namespace

int main() {
  test_export_project_adds_header_and_tran_directive();
  test_export_project_warns_for_unsupported_power_variant();
  test_export_project_omits_dummy_load_when_real_consumer_is_connected();
  test_export_project_keeps_disconnected_nonsymmetric_supply_when_other_supply_drives_amp();
  test_export_topology_ignores_dangling_connections_to_non_exported_blocks();
  test_export_project_warns_for_conflicting_power_net_types();
  test_export_topology_maps_negative_rail_to_ground_when_connected_to_nonsymmetric_supply();
  test_export_topology_orders_power_before_amplifier_in_component();
  test_export_topology_assigns_unique_names_to_disconnected_power_nets();
  test_export_render_builds_square_wave_values();
  test_export_render_builds_triangle_wave_values();
  test_export_render_builds_sine_wave_values();
  test_export_render_warns_for_missing_amplifier_input_values();
  test_export_project_warns_for_unknown_element_kind();
  test_export_project_adds_k1_for_symmetric_supply();
  test_export_project_adds_k1_for_nonsymmetric_supply();
  test_export_project_adds_amplifier_gain_directive();
  test_export_project_renders_zener_regulator_standalone();
  test_export_project_removes_local_source_and_load_for_connected_zener_regulator();
  test_export_project_supports_negative_rail_zener_regulator();
  test_export_project_renders_zener_bjt_regulator_standalone();
  test_export_project_removes_local_source_and_load_for_connected_zener_bjt_regulator();
  test_export_project_renders_two_zener_regulators_for_symmetric_supply();
  test_export_project_renders_two_zener_bjt_regulators_for_symmetric_supply();
  test_append_missing_flags_adds_regulator_input_and_output_flags();
  test_asc_ops_strip_header_and_shift_coords();
  test_asc_ops_uniquify_instnames_and_flag_nets();
  test_asc_ops_patch_flags_updates_matching_coordinates_and_warns_for_missing();
  return 0;
}
