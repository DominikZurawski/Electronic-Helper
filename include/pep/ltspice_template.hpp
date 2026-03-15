#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace pep {

struct LtspiceAscPatchResult {
  std::string asc;
  std::vector<std::string> warnings;
};

// Detects unit suffix from an existing "SYMATTR Value ..." line of a given instance name.
// Example: "3500u" -> "u", "3500µ" -> "µ", "10k" -> "k". Returns "" if not found.
std::string ltspice_detect_value_suffix(const std::string& asc, const std::string& inst_name);

// Applies a simple patch to an LTspice schematic (.asc) by replacing "SYMATTR Value ..." lines
// for instances listed in inst_to_value, matched by preceding "SYMATTR InstName ...".
LtspiceAscPatchResult ltspice_patch_asc_values(
    const std::string& asc,
    const std::unordered_map<std::string, std::string>& inst_to_value);

} // namespace pep

