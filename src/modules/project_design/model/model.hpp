#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pep::modules::project_design {

enum class PortType {
  PowerPos,
  PowerNeg,
  PowerInPos,
  PowerInNeg,
  PowerOutPos,
  PowerOutNeg,
  Ground,
  AnalogIn,
  AnalogOut,
  Unknown
};
enum class BlockKind { Power, Regulator, Amplifier, Unknown };
enum class BlockVariant {
  PsuSymmetric,
  PsuUnregulated,
  PsuSwitching,
  RegZener,
  RegZenerBjt,
  RegIntegrated,
  AmpModel1b,
  Unknown
};
enum class SignalWaveform { Sine, Square, Triangle };
enum class TransformerSolveMode { SecondaryFromRatio, RatioFromSecondary };
enum class VoltageQuantity { Rms, Peak };
enum class PowerDesignMode { SupplyForLoad, LoadForSupply, AllFields };
enum class AmpDesignMode { SupplyForAmp, AmpForSupply, AllFields };
enum class SupplyRail { Vcc, Vee };

struct FlagRef {
  int x = 0;
  int y = 0;
};

struct CanvasPoint {
  double x = 0.0;
  double y = 0.0;
};

struct PortDef {
  std::string id;
  std::string label;
  PortType type = PortType::Unknown;
  std::vector<FlagRef> flags;
};

struct Block {
  int id = 0;
  BlockKind kind = BlockKind::Unknown;
  BlockVariant variant = BlockVariant::Unknown;
  std::string title;
  CanvasPoint canvas_pos{0.0, 0.0};

  // Parameters (only meaningful for some kinds/variants).
  double vin_ac_rms = 0.0;
  double mains_hz = 50.0;
  double load_current = 0.0;
  double capacitor_uF = 0.0;
  double capacitor_tol_pct = 20.0;
  double max_ripple_vpp = 0.0;
  double diode_drop = 0.7;
  double transformer_primary_v = 230.0;
  double transformer_primary_tol_pct = 10.0;
  double transformer_secondary_v = 12.0;
  double transformer_turns_ratio = 230.0 / 12.0;
  TransformerSolveMode transformer_solve_mode = TransformerSolveMode::SecondaryFromRatio;
  VoltageQuantity transformer_voltage_quantity = VoltageQuantity::Rms;
  SignalWaveform transformer_waveform = SignalWaveform::Sine;
  PowerDesignMode power_design_mode = PowerDesignMode::SupplyForLoad;

  // Amplifier/signal parameters.
  double signal_amp_v = 1.0;
  double signal_hz = 1000.0;
  double gain = 10.0;
  SignalWaveform signal_waveform = SignalWaveform::Sine;
  double load_resistance_ohm = 8.0;
  double target_power_w = 0.0;
  double supply_headroom_v = 4.0;
  double psrr_db = 80.0;
  double supply_disturbance_rejection_db = 100.0;
  double supply_disturbance_freq_hz = 100.0;
  double capacitor_esr_ohm = 0.08;
  double transformer_secondary_res_ohm = 0.30;
  AmpDesignMode amp_design_mode = AmpDesignMode::AmpForSupply;
  int amp_power_pos_source_id = 0;
  int amp_power_neg_source_id = 0;

  // Voltage regulator parameters.
  int regulator_input_source_id = 0;
  SupplyRail regulator_supply_rail = SupplyRail::Vcc;
  double regulator_input_min_v = 6.0;
  double regulator_output_v = 5.0;
  double regulator_output_current_a = 0.02;
  double regulator_zener_current_a = 0.005;
  double regulator_dropout_v = 2.0;
  double regulator_series_res_ohm = 50.0;
};

struct Endpoint {
  int block_id = 0;
  std::string port_id;
};

struct Connection {
  Endpoint a;
  Endpoint b;
};

bool ports_compatible(PortType a, PortType b);
const char *block_variant_id(BlockVariant variant);
BlockVariant block_variant_from_id(std::string_view variant_id);
const char *signal_waveform_id(SignalWaveform waveform);
SignalWaveform signal_waveform_from_id(std::string_view waveform_id);

Block make_power_block(int id, const std::string &title, BlockVariant variant);
Block make_regulator_block(int id, const std::string &title, BlockVariant variant);
Block make_amp_model1b_block(int id, const std::string &title);

std::vector<PortDef> ports_for(const Block &b);
std::optional<PortDef> find_port(const Block &b, const std::string &port_id);
Block *find_block(std::vector<Block> &blocks, int id);
const Block *find_block(const std::vector<Block> &blocks, int id);

} // namespace pep::modules::project_design
