#include "model.hpp"

namespace pep::modules::psu_symmetric {

Output compute(const Input &input) {
  Output out;
  const double vpeak = input.vin_ac_rms * 1.414;
  const double vrect = vpeak - 2.0 * input.diode_drop;

  if (input.capacitor_uF > 0.0 && input.mains_hz > 0.0) {
    const double c = input.capacitor_uF * 1e-6;
    const double freq = input.mains_hz * 2.0;
    out.ripple_vpp = input.load_current / (c * freq);
  }

  const double vloaded = vrect - out.ripple_vpp / 2.0;
  out.vdc_plus = vloaded;
  out.vdc_minus = -vloaded;
  return out;
}

} // namespace pep::modules::psu_symmetric
