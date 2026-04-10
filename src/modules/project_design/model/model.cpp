#include "model.hpp"

#include <string>
#include <string_view>

namespace pep::modules::project_design {

bool ports_compatible(PortType a, PortType b) {
  if (a == PortType::Unknown || b == PortType::Unknown) {
    return true;
  }
  if (a == PortType::Ground || b == PortType::Ground) {
    return a == b;
  }
  if ((a == PortType::PowerPos || a == PortType::PowerNeg) ||
      (b == PortType::PowerPos || b == PortType::PowerNeg)) {
    return a == b;
  }
  if (a == PortType::AnalogOut) {
    return b == PortType::AnalogIn;
  }
  if (b == PortType::AnalogOut) {
    return a == PortType::AnalogIn;
  }
  return true;
}

const char *block_variant_id(BlockVariant variant) {
  switch (variant) {
  case BlockVariant::PsuSymmetric:
    return "psu_symmetric";
  case BlockVariant::PsuUnregulated:
    return "psu_unregulated";
  case BlockVariant::PsuSwitching:
    return "psu_switching";
  case BlockVariant::AmpModel1b:
    return "model1b";
  default:
    return "unknown";
  }
}

BlockVariant block_variant_from_id(std::string_view variant_id) {
  if (variant_id == "psu_symmetric") {
    return BlockVariant::PsuSymmetric;
  }
  if (variant_id == "psu_unregulated") {
    return BlockVariant::PsuUnregulated;
  }
  if (variant_id == "psu_switching") {
    return BlockVariant::PsuSwitching;
  }
  if (variant_id == "model1b") {
    return BlockVariant::AmpModel1b;
  }
  return BlockVariant::Unknown;
}

const char *signal_waveform_id(SignalWaveform waveform) {
  switch (waveform) {
  case SignalWaveform::Square:
    return "square";
  case SignalWaveform::Triangle:
    return "triangle";
  default:
    return "sine";
  }
}

SignalWaveform signal_waveform_from_id(std::string_view waveform_id) {
  if (waveform_id == "square") {
    return SignalWaveform::Square;
  }
  if (waveform_id == "triangle") {
    return SignalWaveform::Triangle;
  }
  return SignalWaveform::Sine;
}

Block make_power_block(int id, const std::string &title, BlockVariant variant) {
  Block b;
  b.id = id;
  b.kind = BlockKind::Power;
  b.variant = variant;
  b.title = title;
  b.canvas_pos = CanvasPoint{0.0, 0.0};
  b.vin_ac_rms = 12.0;
  b.mains_hz = 50.0;
  b.signal_waveform = SignalWaveform::Sine;
  b.transformer_primary_v = 230.0;
  b.transformer_secondary_v = 12.0;
  b.transformer_turns_ratio = 230.0 / 12.0;
  b.transformer_solve_mode = TransformerSolveMode::SecondaryFromRatio;
  b.transformer_voltage_quantity = VoltageQuantity::Rms;
  b.transformer_waveform = SignalWaveform::Sine;
  return b;
}

Block make_amp_model1b_block(int id, const std::string &title) {
  Block b;
  b.id = id;
  b.kind = BlockKind::Amplifier;
  b.variant = BlockVariant::AmpModel1b;
  b.title = title;
  b.canvas_pos = CanvasPoint{0.0, 0.0};
  b.mains_hz = 50.0;
  b.signal_waveform = SignalWaveform::Sine;
  b.signal_amp_v = 1.0;
  b.signal_hz = 1000.0;
  b.gain = 10.0;
  return b;
}

std::vector<PortDef> ports_for(const Block &b) {
  if (b.kind == BlockKind::Power) {
    if (b.variant == BlockVariant::PsuSymmetric) {
      // Port flags based on provided template (assets/ltspice/power.asc).
      return {
          {std::string("vcc"), "Vcc", PortType::PowerPos, {FlagRef{672, -352}}},
          {std::string("vee"), "Vee", PortType::PowerNeg, {FlagRef{672, -64}}},
          {std::string("gnd"), "0", PortType::Ground, {}},
      };
    }

    // Unregulated and switching PSUs currently have no exported template, but still participate
    // in typed connections.
    return {
        {std::string("vcc"), "Vcc", PortType::PowerPos, {}},
        {std::string("vee"), "Vee", PortType::PowerNeg, {}},
        {std::string("gnd"), "0", PortType::Ground, {}},
    };
  }

  // Amplifier model 1(B) (template: assets/ltspice/opamp_model1b.asc).
  return {
      {std::string("vcc"), "Vcc", PortType::PowerPos, {FlagRef{1136, -512}, FlagRef{944, -208}}},
      {std::string("vee"), "Vee", PortType::PowerNeg, {FlagRef{1136, -32}, FlagRef{944, -400}}},
      {std::string("gnd"), "0", PortType::Ground, {}},
      {std::string("in"), "IN", PortType::AnalogIn, {FlagRef{784, -320}}},
      {std::string("out"), "OUT", PortType::AnalogOut, {FlagRef{1360, -304}}},
  };
}

std::optional<PortDef> find_port(const Block &b, const std::string &port_id) {
  for (const auto &p : ports_for(b)) {
    if (p.id == port_id) {
      return p;
    }
  }
  return std::nullopt;
}

Block *find_block(std::vector<Block> &blocks, int id) {
  for (auto &b : blocks) {
    if (b.id == id) {
      return &b;
    }
  }
  return nullptr;
}

const Block *find_block(const std::vector<Block> &blocks, int id) {
  for (const auto &b : blocks) {
    if (b.id == id) {
      return &b;
    }
  }
  return nullptr;
}

} // namespace pep::modules::project_design
