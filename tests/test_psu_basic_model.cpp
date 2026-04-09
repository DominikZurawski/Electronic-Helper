#include "src/modules/psu_basic/model/model.hpp"

#include <cassert>
#include <cmath>

namespace psu = pep::modules::psu_basic;

namespace {

bool approx(double actual, double expected, double eps = 1e-6) {
  return std::fabs(actual - expected) <= eps;
}

void test_full_wave_bridge_uses_two_diodes_and_double_ripple_frequency() {
  psu::Input input;
  input.vin_ac_rms = 12.0;
  input.diode_drop = 0.8;
  input.load_current = 0.1;
  input.capacitor_uF = 2200.0;
  input.mains_hz = 50.0;
  input.rectifier = psu::RectifierType::FullWaveBridge;

  const auto out = psu::compute(input);

  const double expected_vpeak = 12.0 * 1.414;
  const double expected_vrect = expected_vpeak - 2.0 * 0.8;
  const double expected_ripple_hz = 100.0;
  const double expected_ripple_vpp = 0.1 / ((2200.0 * 1e-6) * expected_ripple_hz);

  assert(approx(out.vpeak, expected_vpeak, 1e-9));
  assert(approx(out.vrect, expected_vrect, 1e-9));
  assert(approx(out.ripple_hz, expected_ripple_hz, 1e-9));
  assert(approx(out.ripple_vpp, expected_ripple_vpp, 1e-9));
  assert(approx(out.vdc_no_load, expected_vrect, 1e-9));
  assert(approx(out.vdc_loaded, expected_vrect - expected_ripple_vpp / 2.0, 1e-9));
  assert(approx(out.vmin, expected_vrect - expected_ripple_vpp, 1e-9));
}

void test_half_wave_uses_one_diode_and_single_ripple_frequency() {
  psu::Input input;
  input.vin_ac_rms = 9.0;
  input.diode_drop = 0.7;
  input.load_current = 0.05;
  input.capacitor_uF = 1000.0;
  input.mains_hz = 60.0;
  input.rectifier = psu::RectifierType::HalfWave;

  const auto out = psu::compute(input);

  const double expected_vpeak = 9.0 * 1.414;
  const double expected_vrect = expected_vpeak - 0.7;
  const double expected_ripple_hz = 60.0;

  assert(approx(out.vrect, expected_vrect, 1e-9));
  assert(approx(out.ripple_hz, expected_ripple_hz, 1e-9));
}

void test_zero_capacitor_keeps_ripple_at_zero() {
  psu::Input input;
  input.vin_ac_rms = 15.0;
  input.diode_drop = 0.9;
  input.load_current = 0.2;
  input.capacitor_uF = 0.0;
  input.mains_hz = 50.0;

  const auto out = psu::compute(input);

  assert(approx(out.ripple_vpp, 0.0));
  assert(approx(out.vdc_loaded, out.vrect));
  assert(approx(out.vmin, out.vrect));
}

void test_tolerance_ranges_are_ordered_consistently() {
  psu::Input input;
  input.vin_ac_rms = 12.0;
  input.diode_drop = 0.9;
  input.diode_tol_pct = 10.0;
  input.load_current = 0.15;
  input.current_tol_pct = 15.0;
  input.capacitor_uF = 2200.0;
  input.cap_tol_pct = 20.0;
  input.mains_hz = 50.0;
  input.mains_tol_pct = 2.0;

  const auto out = psu::compute(input);

  assert(out.vdc_loaded_min <= out.vdc_loaded_max);
  assert(out.vmin_min <= out.vmin_max);
  assert(out.ripple_vpp_min <= out.ripple_vpp_max);
  assert(out.vdc_loaded_min <= out.vdc_loaded);
  assert(out.vdc_loaded <= out.vdc_loaded_max);
}

} // namespace

int main() {
  test_full_wave_bridge_uses_two_diodes_and_double_ripple_frequency();
  test_half_wave_uses_one_diode_and_single_ripple_frequency();
  test_zero_capacitor_keeps_ripple_at_zero();
  test_tolerance_ranges_are_ordered_consistently();
  return 0;
}
