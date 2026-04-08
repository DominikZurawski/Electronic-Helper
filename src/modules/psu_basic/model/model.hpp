#pragma once

namespace pep::modules::psu_basic {

enum class RectifierType { HalfWave, FullWaveBridge };

struct Input {
  double vin_ac_rms = 0.0;
  double vin_min = 0.0;
  double vin_max = 0.0;
  double diode_drop = 0.9;
  double diode_tol_pct = 10.0;
  double load_current = 0.0;
  double current_tol_pct = 10.0;
  double capacitor_uF = 0.0;
  double cap_tol_pct = 20.0;
  double mains_hz = 50.0;
  double mains_tol_pct = 2.0;
  RectifierType rectifier = RectifierType::FullWaveBridge;
};

struct Output {
  double vpeak = 0.0;
  double vrect = 0.0;
  double ripple_hz = 0.0;
  double vdc_no_load = 0.0;
  double vdc_loaded = 0.0;
  double ripple_vpp = 0.0;
  double vmin = 0.0;

  double vdc_loaded_min = 0.0;
  double vdc_loaded_max = 0.0;
  double ripple_vpp_min = 0.0;
  double ripple_vpp_max = 0.0;
  double vmin_min = 0.0;
  double vmin_max = 0.0;
};

Output compute(const Input &input);

} // namespace pep::modules::psu_basic
