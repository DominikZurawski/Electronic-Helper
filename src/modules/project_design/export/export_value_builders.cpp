#include "export_value_builders.hpp"

#include <cmath>
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

std::unordered_map<std::string, std::string>
build_regulator_instance_values(const Block &block, bool project_export,
                                std::vector<std::string> &warnings) {
  std::unordered_map<std::string, std::string> inst_values;
  if (block.variant != BlockVariant::RegZener && block.variant != BlockVariant::RegZenerBjt) {
    warnings.push_back("Brak buildera wartości LTspice dla wybranego wariantu stabilizatora.");
    return inst_values;
  }

  const double vin_min = std::abs(block.regulator_input_min_v);
  const double vout = std::abs(block.regulator_output_v);
  const double iout = block.regulator_output_current_a;
  const double iz = block.regulator_zener_current_a;
  const double total_current = iout + iz;
  if (vin_min <= vout || total_current <= 0.0) {
    warnings.push_back(
        "Nieprawidłowe parametry stabilizatora Zenera — potrzebne Vin > Vout oraz dodatni prąd.");
    return inst_values;
  }

  const double series_res = (vin_min - vout) / total_current;
  const double source_value = block.regulator_supply_rail == SupplyRail::Vee ? -vin_min : vin_min;
  const double current_value = block.regulator_supply_rail == SupplyRail::Vee ? -iout : iout;
  inst_values.emplace("R1", format_double(series_res, 6));
  inst_values.emplace("V1", format_double(source_value, 6));
  inst_values.emplace("I1", format_double(current_value, 6));
  if (project_export) {
    inst_values.emplace("D1", "PPE_SELECT_ZENER");
    if (block.variant == BlockVariant::RegZenerBjt) {
      inst_values.emplace("Q1", "PPE_SELECT_NPN");
      warnings.push_back("Stabilizator Zenera z tranzystorem: po eksporcie całego układu wybierz "
                         "model diody Zenera dla około " +
                         format_double(vout, 3) +
                         " V oraz odpowiedni model tranzystora NPN dla wymaganego prądu.");
    } else {
      warnings.push_back("Stabilizator Zenera: po eksporcie całego układu wybierz model diody dla "
                         "około " +
                         format_double(vout, 3) + " V i wymaganego prądu.");
    }
  }
  return inst_values;
}

} // namespace pep::modules::project_design
