#include "export_value_builders.hpp"

#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <unordered_map>

namespace pep::modules::project_design {

namespace {

std::string format_double(double value, int precision) {
  std::ostringstream out;
  out.setf(std::ios::fixed);
  out << std::setprecision(precision) << value;
  std::string text = out.str();
  while (text.size() > 1 && text.find('.') != std::string::npos && text.back() == '0') {
    text.pop_back();
  }
  if (!text.empty() && text.back() == '.') {
    text.pop_back();
  }
  return text;
}

} // namespace

std::unordered_map<std::string, std::string>
build_amplifier_instance_values(const Block &block, std::vector<std::string> &warnings) {
  std::unordered_map<std::string, std::string> inst_values;
  const double amp = block.signal_amp_v;
  const double hz = block.signal_hz;
  if (amp <= 0.0 || hz <= 0.0) {
    warnings.push_back("Brak parametrów wejścia wzmacniacza — nie nadpisuję V2.");
    return inst_values;
  }

  if (block.signal_waveform == SignalWaveform::Square) {
    const double period = 1.0 / hz;
    const double ton = 0.5 * period;
    inst_values.emplace("V2", "PULSE(" + format_double(-amp, 6) + " " + format_double(amp, 6) +
                                  " 0 1u 1u " + format_double(ton, 9) + " " +
                                  format_double(period, 9) + ")");
    return inst_values;
  }

  if (block.signal_waveform == SignalWaveform::Triangle) {
    const double period = 1.0 / hz;
    inst_values.emplace("V2", "PWL(0 0 " + format_double(0.25 * period, 9) + " " +
                                  format_double(amp, 6) + " " + format_double(0.75 * period, 9) +
                                  " " + format_double(-amp, 6) + " " + format_double(period, 9) +
                                  " 0)");
    return inst_values;
  }

  inst_values.emplace("V2", "SINE(0 " + format_double(amp, 6) + " " + format_double(hz, 3) + ")");
  return inst_values;
}

} // namespace pep::modules::project_design
