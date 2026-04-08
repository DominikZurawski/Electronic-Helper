#include "model.hpp"

namespace pep::modules::psu_basic {

namespace {
struct CalcResult {
  double vrect = 0.0;
  double ripple_hz = 0.0;
  double ripple_vpp = 0.0;
  double vdc_loaded = 0.0;
  double vmin = 0.0;
};

CalcResult compute_case(double vin_ac_rms, double diode_drop, double load_current,
                        double capacitor_uF, double mains_hz, RectifierType rectifier) {
  CalcResult out;
  const int diode_count = (rectifier == RectifierType::FullWaveBridge) ? 2 : 1;
  const double ripple_factor = (rectifier == RectifierType::FullWaveBridge) ? 2.0 : 1.0;

  const double vpeak = vin_ac_rms * 1.414;
  out.vrect = vpeak - diode_count * diode_drop;
  out.ripple_hz = mains_hz * ripple_factor;

  if (capacitor_uF > 0.0 && mains_hz > 0.0) {
    const double c = capacitor_uF * 1e-6;
    out.ripple_vpp = load_current / (c * out.ripple_hz);
  }

  out.vdc_loaded = out.vrect - out.ripple_vpp / 2.0;
  out.vmin = out.vrect - out.ripple_vpp;
  return out;
}

static double pct_down(double value, double pct) {
  return value * (1.0 - pct / 100.0);
}

static double pct_up(double value, double pct) {
  return value * (1.0 + pct / 100.0);
}

} // namespace

Output compute(const Input &input) {
  Output out;

  const int diode_count = (input.rectifier == RectifierType::FullWaveBridge) ? 2 : 1;
  const double ripple_factor = (input.rectifier == RectifierType::FullWaveBridge) ? 2.0 : 1.0;

  out.vpeak = input.vin_ac_rms * 1.414;
  out.vrect = out.vpeak - diode_count * input.diode_drop;
  out.ripple_hz = input.mains_hz * ripple_factor;
  out.vdc_no_load = out.vrect;

  if (input.capacitor_uF > 0.0 && input.mains_hz > 0.0) {
    const double c = input.capacitor_uF * 1e-6;
    out.ripple_vpp = input.load_current / (c * out.ripple_hz);
  }

  out.vdc_loaded = out.vrect - out.ripple_vpp / 2.0;
  out.vmin = out.vrect - out.ripple_vpp;

  // Worst-case ranges with tolerances
  const double vin_min = (input.vin_min > 0.0) ? input.vin_min : pct_down(input.vin_ac_rms, 5.0);
  const double vin_max = (input.vin_max > 0.0) ? input.vin_max : pct_up(input.vin_ac_rms, 5.0);
  const double diode_min = pct_down(input.diode_drop, input.diode_tol_pct);
  const double diode_max = pct_up(input.diode_drop, input.diode_tol_pct);
  const double current_min = pct_down(input.load_current, input.current_tol_pct);
  const double current_max = pct_up(input.load_current, input.current_tol_pct);
  const double cap_min = pct_down(input.capacitor_uF, input.cap_tol_pct);
  const double cap_max = pct_up(input.capacitor_uF, input.cap_tol_pct);
  const double mains_min = pct_down(input.mains_hz, input.mains_tol_pct);
  const double mains_max = pct_up(input.mains_hz, input.mains_tol_pct);

  const auto low =
      compute_case(vin_min, diode_max, current_max, cap_min, mains_min, input.rectifier);
  const auto high =
      compute_case(vin_max, diode_min, current_min, cap_max, mains_max, input.rectifier);

  out.vdc_loaded_min = low.vdc_loaded;
  out.vdc_loaded_max = high.vdc_loaded;
  out.ripple_vpp_min = high.ripple_vpp;
  out.ripple_vpp_max = low.ripple_vpp;
  out.vmin_min = low.vmin;
  out.vmin_max = high.vmin;

  return out;
}

} // namespace pep::modules::psu_basic
