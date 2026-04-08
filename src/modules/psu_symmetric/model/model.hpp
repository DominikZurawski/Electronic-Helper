#pragma once

namespace pep::modules::psu_symmetric {

struct Input {
  double vin_ac_rms = 0.0;
  double diode_drop = 0.9;
  double load_current = 0.0;
  double capacitor_uF = 0.0;
  double mains_hz = 50.0;
};

struct Output {
  double vdc_plus = 0.0;
  double vdc_minus = 0.0;
  double ripple_vpp = 0.0;
};

Output compute(const Input &input);

} // namespace pep::modules::psu_symmetric
