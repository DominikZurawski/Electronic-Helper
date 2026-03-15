#include "pep/ltspice_template.hpp"

#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace pep {

namespace {

static bool is_digit(char ch) {
  return ch >= '0' && ch <= '9';
}

std::vector<std::string> split_lines(const std::string& text) {
  std::vector<std::string> lines;
  std::string current;
  current.reserve(128);
  for (char ch : text) {
    if (ch == '\n') {
      lines.push_back(current);
      current.clear();
    } else if (ch != '\r') {
      current.push_back(ch);
    }
  }
  lines.push_back(current);
  return lines;
}

std::string join_lines(const std::vector<std::string>& lines) {
  std::ostringstream out;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    out << lines[i];
    if (i + 1 != lines.size()) out << "\n";
  }
  return out.str();
}

} // namespace

std::string ltspice_detect_value_suffix(const std::string& asc, const std::string& inst_name) {
  const auto lines = split_lines(asc);
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const auto& line = lines[i];
    if (line.rfind("SYMATTR InstName ", 0) != 0) continue;
    const std::string name = line.substr(std::string("SYMATTR InstName ").size());
    if (name != inst_name) continue;

    for (std::size_t j = i + 1; j < lines.size(); ++j) {
      const auto& next = lines[j];
      if (next.rfind("SYMATTR InstName ", 0) == 0) break;
      if (next.rfind("SYMATTR Value ", 0) != 0) continue;
      const std::string raw = next.substr(std::string("SYMATTR Value ").size());
      std::size_t k = 0;
      while (k < raw.size() && (is_digit(raw[k]) || raw[k] == '.' || raw[k] == '+' || raw[k] == '-')) ++k;
      if (k >= raw.size()) return "";
      return raw.substr(k);
    }
    return "";
  }
  return "";
}

LtspiceAscPatchResult ltspice_patch_asc_values(
    const std::string& asc,
    const std::unordered_map<std::string, std::string>& inst_to_value) {
  LtspiceAscPatchResult result;
  result.asc = asc;
  if (asc.empty()) {
    result.warnings.push_back("Pusty szablon .asc — pomijam patchowanie.");
    return result;
  }

  auto lines = split_lines(asc);
  std::string current_inst;
  std::unordered_set<std::string> seen_insts;
  std::unordered_set<std::string> patched_insts;

  for (auto& line : lines) {
    std::istringstream iss(line);
    std::string head;
    iss >> head;
    if (head != "SYMATTR") continue;

    std::string key;
    iss >> key;
    if (key == "InstName") {
      iss >> current_inst;
      if (!current_inst.empty()) seen_insts.insert(current_inst);
      continue;
    }

    if (key == "Value" && !current_inst.empty()) {
      const auto it = inst_to_value.find(current_inst);
      if (it != inst_to_value.end()) {
        line = "SYMATTR Value " + it->second;
        patched_insts.insert(current_inst);
      }
    }
  }

  for (const auto& [inst, _] : inst_to_value) {
    if (patched_insts.find(inst) == patched_insts.end()) {
      if (seen_insts.find(inst) == seen_insts.end()) {
        result.warnings.push_back("Nie znaleziono elementu w .asc: " + inst);
      } else {
        result.warnings.push_back("Nie znaleziono pola Value dla elementu: " + inst);
      }
    }
  }

  result.asc = join_lines(lines);
  return result;
}

} // namespace pep

