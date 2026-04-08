#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pep::modules::project_design {

enum class PortType { PowerPos, PowerNeg, Ground, AnalogIn, AnalogOut, Unknown };
enum class BlockKind { Power, Amplifier, Unknown };
enum class BlockVariant { PsuSymmetric, PsuUnregulated, AmpModel1b, Unknown };
enum class SignalWaveform { Sine, Square, Triangle };

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
  double diode_drop = 0.9;

  // Amplifier/signal parameters.
  double signal_amp_v = 1.0;
  double signal_hz = 1000.0;
  double gain = 10.0;
  SignalWaveform signal_waveform = SignalWaveform::Sine;

  // Additional power rails (dynamic, based on user/calculator fields).
  std::string extra_pos_rails_csv; // e.g. "+5V,+12V"
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
Block make_amp_model1b_block(int id, const std::string &title);

std::vector<PortDef> ports_for(const Block &b);
std::optional<PortDef> find_port(const Block &b, const std::string &port_id);
Block *find_block(std::vector<Block> &blocks, int id);
const Block *find_block(const std::vector<Block> &blocks, int id);

} // namespace pep::modules::project_design
