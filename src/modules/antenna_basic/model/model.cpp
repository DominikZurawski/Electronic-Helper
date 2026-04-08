#include "model.hpp"

namespace pep::modules::antenna_basic {

Output compute(const Input &input) {
  Output out;
  if (input.frequency_mhz > 0.0) {
    out.wavelength_m = 300.0 / input.frequency_mhz;
    out.quarter_wave_m = out.wavelength_m / 4.0;
    out.half_wave_m = out.wavelength_m / 2.0;
  }
  return out;
}

} // namespace pep::modules::antenna_basic
