#pragma once

namespace pep::modules::antenna_basic {

struct Input {
  double frequency_mhz = 0.0;
};

struct Output {
  double wavelength_m = 0.0;
  double quarter_wave_m = 0.0;
  double half_wave_m = 0.0;
};

Output compute(const Input &input);

} // namespace pep::modules::antenna_basic
