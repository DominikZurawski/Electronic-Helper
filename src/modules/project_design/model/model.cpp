#include "model.hpp"

#include <string>
#include <string_view>

namespace pep::modules::project_design {

bool ports_compatible(PortType a, PortType b) {
  if (a == PortType::Unknown || b == PortType::Unknown) {
    return true;
  }
  const auto positive = [](PortType type) {
    return type == PortType::PowerPos || type == PortType::PowerInPos ||
           type == PortType::PowerOutPos;
  };
  const auto negative = [](PortType type) {
    return type == PortType::PowerNeg || type == PortType::PowerInNeg ||
           type == PortType::PowerOutNeg;
  };
  if (a == PortType::Ground || b == PortType::Ground) {
    return a == b;
  }
  if (positive(a) || positive(b)) {
    return positive(a) && positive(b);
  }
  if (negative(a) || negative(b)) {
    return negative(a) && negative(b);
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
  case BlockVariant::RegZener:
    return "reg_zener";
  case BlockVariant::RegZenerBjt:
    return "reg_zener_bjt";
  case BlockVariant::RegIntegrated:
    return "reg_integrated";
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
  if (variant_id == "reg_zener") {
    return BlockVariant::RegZener;
  }
  if (variant_id == "reg_zener_bjt") {
    return BlockVariant::RegZenerBjt;
  }
  if (variant_id == "reg_integrated") {
    return BlockVariant::RegIntegrated;
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
  b.diode_drop = 0.7;
  b.signal_waveform = SignalWaveform::Sine;
  b.transformer_primary_v = 230.0;
  b.transformer_primary_tol_pct = 10.0;
  b.transformer_secondary_v = 12.0;
  b.transformer_turns_ratio = 230.0 / 12.0;
  b.transformer_solve_mode = TransformerSolveMode::SecondaryFromRatio;
  b.transformer_voltage_quantity = VoltageQuantity::Rms;
  b.transformer_waveform = SignalWaveform::Sine;
  b.power_design_mode = PowerDesignMode::SupplyForLoad;
  b.capacitor_tol_pct = 20.0;
  return b;
}

Block make_regulator_block(int id, const std::string &title, BlockVariant variant) {
  Block b;
  b.id = id;
  b.kind = BlockKind::Regulator;
  b.variant = variant;
  b.title = title;
  b.canvas_pos = CanvasPoint{0.0, 0.0};
  b.regulator_supply_rail = SupplyRail::Vcc;
  b.regulator_input_min_v = 6.0;
  b.regulator_output_v = 5.0;
  b.regulator_output_current_a = 0.02;
  b.regulator_zener_current_a = 0.005;
  b.regulator_dropout_v = 2.0;
  b.regulator_series_res_ohm = 50.0;
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
  b.load_resistance_ohm = 8.0;
  b.target_power_w = 0.0;
  b.supply_headroom_v = 4.0;
  b.psrr_db = 80.0;
  b.supply_disturbance_rejection_db = 100.0;
  b.supply_disturbance_freq_hz = 100.0;
  b.capacitor_esr_ohm = 0.08;
  b.transformer_secondary_res_ohm = 0.30;
  b.max_ripple_vpp = 2.0;
  b.amp_design_mode = AmpDesignMode::AmpForSupply;
  b.amp_power_pos_source_id = 0;
  b.amp_power_neg_source_id = 0;
  return b;
}

std::vector<PortDef> ports_for(const Block &b) {
  if (b.kind == BlockKind::Power) {
    if (b.variant == BlockVariant::PsuSymmetric) {
      // Port flags based on provided template (assets/ltspice/power.asc).
      return {
          {std::string("vcc"), "Vcc", PortType::PowerPos, {FlagRef{1200, 112}}},
          {std::string("vee"), "Vee", PortType::PowerNeg, {FlagRef{1200, 400}}},
          {std::string("gnd"), "0", PortType::Ground, {}},
      };
    }

    if (b.variant == BlockVariant::PsuUnregulated) {
      return {
          {std::string("vcc"), "Vcc", PortType::PowerPos, {FlagRef{1152, 112}}},
          {std::string("vee"), "Vee", PortType::PowerNeg, {FlagRef{1152, 400}}},
          {std::string("gnd"), "0", PortType::Ground, {}},
      };
    }

    // Switching PSU currently has no exported template, but still participates in typed
    // connections.
    return {
        {std::string("vcc"), "Vcc", PortType::PowerPos, {}},
        {std::string("vee"), "Vee", PortType::PowerNeg, {}},
        {std::string("gnd"), "0", PortType::Ground, {}},
    };
  }

  if (b.kind == BlockKind::Regulator) {
    const bool negative_rail = b.regulator_supply_rail == SupplyRail::Vee;
    const std::vector<FlagRef> vin_flags =
        b.variant == BlockVariant::RegZenerBjt ? std::vector<FlagRef>{{0, 96}}
                                               : std::vector<FlagRef>{{-32, 96}};
    const std::vector<FlagRef> vout_flags =
        b.variant == BlockVariant::RegZenerBjt ? std::vector<FlagRef>{{416, 96}}
                                               : std::vector<FlagRef>{{256, 96}};
    return {
        {std::string("vin"), negative_rail ? "VIN-" : "VIN",
         negative_rail ? PortType::PowerInNeg : PortType::PowerInPos, vin_flags},
        {std::string("vout"), negative_rail ? "VOUT-" : "VOUT",
         negative_rail ? PortType::PowerOutNeg : PortType::PowerOutPos, vout_flags},
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
