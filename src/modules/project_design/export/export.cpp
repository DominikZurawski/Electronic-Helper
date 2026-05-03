#include "export.hpp"

#include "asc_ops.hpp"
#include "export_render.hpp"
#include "export_topology.hpp"
#include "export_value_builders.hpp"
#include "template_loader.hpp"

#include "../model/model.hpp"

#include "../../psu_basic/export/template_export.hpp"

#include "pep/ltspice_template.hpp"

#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace pep::modules::project_design {

namespace {

struct RegulatorTemplateConfig {
  std::string template_asc;
  std::string missing_template_warning;
  int vin_source_wire_top_x = 0;
  int vin_source_wire_top_y1 = 0;
  int vin_source_wire_top_y2 = 0;
  int vin_source_wire_bottom_x = 0;
  int vin_source_wire_bottom_y1 = 0;
  int vin_source_wire_bottom_y2 = 0;
  int vin_source_flag_x = 0;
  int vin_source_flag_y = 0;
  int load_wire_top_x = 0;
  int load_wire_top_y1 = 0;
  int load_wire_top_y2 = 0;
  int load_wire_bottom_x = 0;
  int load_wire_bottom_y1 = 0;
  int load_wire_bottom_y2 = 0;
  int load_link_x1 = 0;
  int load_link_y1 = 0;
  int load_link_x2 = 0;
  int load_link_y2 = 0;
  int load_flag_x = 0;
  int load_flag_y = 0;
};

RegulatorTemplateConfig regulator_template_config(const Block &block, QString &template_label) {
  if (block.variant == BlockVariant::RegZenerBjt) {
    return {load_regulator_zener_bjt_template(template_label),
            "Nie udało się wczytać szablonu stabilizatora Zenera z tranzystorem.",
            -32,
            176,
            96,
            -32,
            320,
            256,
            -32,
            320,
            448,
            176,
            96,
            448,
            320,
            256,
            448,
            96,
            416,
            96,
            448,
            320};
  }

  return {load_regulator_zener_template(template_label),
          "Nie udało się wczytać szablonu stabilizatora Zenera.",
          -32,
          176,
          96,
          -32,
          320,
          256,
          -32,
          320,
          400,
          176,
          96,
          400,
          320,
          256,
          400,
          96,
          256,
          96,
          400,
          320};
}

bool power_block_has_non_power_load(const Block &block, const std::vector<Block> &blocks,
                                    const std::vector<Connection> &connections) {
  for (const auto &connection : connections) {
    const bool a_is_block = connection.a.block_id == block.id;
    const bool b_is_block = connection.b.block_id == block.id;
    if (!a_is_block && !b_is_block) {
      continue;
    }

    const int other_id = a_is_block ? connection.b.block_id : connection.a.block_id;
    const Block *other = find_block(blocks, other_id);
    if (other && other->kind != BlockKind::Power) {
      return true;
    }
  }
  return false;
}

bool regulator_has_external_input_source(const Block &block, const std::vector<Block> &blocks,
                                         const std::vector<Connection> &connections) {
  for (const auto &connection : connections) {
    const Endpoint other = connection.a.block_id == block.id ? connection.b
                           : connection.b.block_id == block.id ? connection.a
                                                               : Endpoint{};
    const std::string own_port = connection.a.block_id == block.id ? connection.a.port_id
                                : connection.b.block_id == block.id ? connection.b.port_id
                                                                    : "";
    if (own_port != "vin") {
      continue;
    }
    const Block *other_block = find_block(blocks, other.block_id);
    if (other_block) {
      return true;
    }
  }
  return false;
}

bool regulator_has_external_output_load(const Block &block, const std::vector<Block> &blocks,
                                        const std::vector<Connection> &connections) {
  for (const auto &connection : connections) {
    const Endpoint other = connection.a.block_id == block.id ? connection.b
                           : connection.b.block_id == block.id ? connection.a
                                                               : Endpoint{};
    const std::string own_port = connection.a.block_id == block.id ? connection.a.port_id
                                : connection.b.block_id == block.id ? connection.b.port_id
                                                                    : "";
    if (own_port != "vout") {
      continue;
    }
    const Block *other_block = find_block(blocks, other.block_id);
    if (other_block) {
      return true;
    }
  }
  return false;
}

bool template_has_load_resistor(const std::string &asc_template) {
  return asc_template.find("InstName Rload") != std::string::npos ||
         asc_template.find("InstName RL") != std::string::npos;
}

bool template_has_inst(const std::string &asc_template, const std::string &name) {
  return asc_template.find("InstName " + name) != std::string::npos;
}

void erase_matching_warning(std::vector<std::string> &warnings, const std::string &pattern) {
  warnings.erase(std::remove_if(warnings.begin(), warnings.end(),
                                [&](const std::string &warning) {
                                  return warning.find(pattern) != std::string::npos;
                                }),
                 warnings.end());
}

std::string format_number(double v, int precision = 6) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out.setf(std::ios::fixed);
  out << std::setprecision(precision) << v;
  std::string s = out.str();
  while (s.size() > 1 && s.find('.') != std::string::npos && s.back() == '0') {
    s.pop_back();
  }
  if (!s.empty() && s.back() == '.') {
    s.pop_back();
  }
  return s;
}

} // namespace

AscAssembly export_project_asc(const std::vector<Block> &blocks,
                               const std::vector<Connection> &connections,
                               const std::optional<std::string> &tran_directive) {
  AscAssembly out;
  const auto topology = build_export_topology(blocks, connections);
  out.warnings.insert(out.warnings.end(), topology.warnings.begin(), topology.warnings.end());
  ExportSheetState sheet;
  initialize_sheet(sheet);
  const ExportLayoutConfig layout;
  bool step_added = false;
  bool cap_step_added = false;

  int row_y = 0;

  for (const auto &order : topology.ordered_components) {
    int row_cursor_x = 0;
    int row_max_h = 220;

    for (const int bi : order) {
      const auto &b = blocks[bi];
      const int dx = row_cursor_x;
      const int dy = row_y;

      if (b.kind == BlockKind::Power) {
        QString template_label;
        std::string tpl;
        if (b.variant == BlockVariant::PsuSymmetric) {
          tpl = load_psu_symmetric_template(template_label);
          if (power_block_has_non_power_load(b, blocks, connections)) {
            tpl = remove_component_by_instname(tpl, "Rload");
            tpl = remove_component_by_instname(tpl, "RL");
            tpl = remove_wire_segment(tpl, 1152, 208, 1152, 112);
            tpl = remove_wire_segment(tpl, 1152, 400, 1152, 288);
          }
        } else if (b.variant == BlockVariant::PsuUnregulated) {
          const bool has_real_load = power_block_has_non_power_load(b, blocks, connections);
          tpl = load_psu_unregulated_template(template_label);
          if (has_real_load) {
            tpl = remove_component_by_instname(tpl, "Rload");
            tpl = remove_component_by_instname(tpl, "RL");
            tpl = remove_wire_segment(tpl, 1248, 176, 1248, 112);
            tpl = remove_wire_segment(tpl, 1248, 400, 1248, 256);
            tpl = remove_wire_segment(tpl, 1152, 400, 1248, 400);
          }
        } else {
          append_missing_element_note(
              out, sheet, row_cursor_x, row_y, row_cursor_x, row_max_h, layout,
              "brak schematu dla wariantu \"" + std::string(block_variant_id(b.variant)) + "\"",
              "Brak szablonu LTspice dla wariantu: " + std::string(block_variant_id(b.variant)));
          continue;
        }

        if (tpl.empty()) {
          out.warnings.push_back("Nie udało się wczytać szablonu zasilania.");
          row_cursor_x += (700 + layout.gap_x);
          row_max_h = std::max(row_max_h, 220);
          continue;
        }

        std::string step_param;
        if (!step_added && b.transformer_primary_tol_pct > 0.0) {
          step_param = "KM_B" + std::to_string(b.id);
          step_added = true;
        }
        std::string cap_step_param;
        if (!cap_step_added && b.capacitor_tol_pct > 0.0) {
          cap_step_param = "KC_B" + std::to_string(b.id);
          cap_step_added = true;
        }
        const double mains_rms = (b.transformer_primary_v > 0.0 ? b.transformer_primary_v
                                                               : b.vin_ac_rms);
        auto patched_values = pep::modules::psu_basic::export_schematic_from_asc_template(
            mains_rms, b.mains_hz, b.load_current, b.capacitor_uF,
            b.transformer_primary_tol_pct, step_param,
            b.capacitor_tol_pct, cap_step_param, tpl);
        if (!template_has_load_resistor(tpl)) {
          erase_matching_warning(patched_values.warnings, "Nie znaleziono elementu w .asc: Rload");
          erase_matching_warning(patched_values.warnings, "Nie znaleziono elementu w .asc: RL");
          erase_matching_warning(patched_values.warnings,
                                 "Nie znaleziono pola Value dla elementu: Rload");
          erase_matching_warning(patched_values.warnings,
                                 "Nie znaleziono pola Value dla elementu: RL");
        } else if (b.variant == BlockVariant::PsuUnregulated &&
                   power_block_has_non_power_load(b, blocks, connections)) {
          erase_matching_warning(patched_values.warnings, "Nie znaleziono elementu w .asc: Rload");
          erase_matching_warning(patched_values.warnings, "Nie znaleziono elementu w .asc: RL");
          erase_matching_warning(patched_values.warnings,
                                 "Nie znaleziono pola Value dla elementu: Rload");
          erase_matching_warning(patched_values.warnings,
                                 "Nie znaleziono pola Value dla elementu: RL");
        }
        out.warnings.insert(out.warnings.end(), patched_values.warnings.begin(),
                            patched_values.warnings.end());
        std::string asc_with_directives = patched_values.asc;
        std::vector<std::string> local_directives = patched_values.directives;
        const std::string k1_token = "K1";
        if (asc_with_directives.find(k1_token) == std::string::npos &&
            asc_with_directives.find("k1") == std::string::npos) {
          const std::string suffix = "_B" + std::to_string(b.id);
          const bool has_l3 = template_has_inst(tpl, "L3");
          if (has_l3) {
            local_directives.push_back("K1" + suffix + " L1" + suffix + " L2" + suffix + " L3" +
                                       suffix + " 1");
          } else {
            local_directives.push_back("K1" + suffix + " L1" + suffix + " L2" + suffix + " 1");
          }
        }
        if (!local_directives.empty()) {
          asc_with_directives = append_directives_below(asc_with_directives, local_directives);
        }

        append_export_fragment(out, sheet, dx, dy, row_cursor_x, row_max_h, layout,
                               asc_with_directives, build_block_flag_nets(b, topology),
                               "_B" + std::to_string(b.id));
        continue;
      }

      if (b.kind == BlockKind::Regulator) {
        QString template_label;
        if (b.variant != BlockVariant::RegZener && b.variant != BlockVariant::RegZenerBjt) {
          append_missing_element_note(
              out, sheet, row_cursor_x, row_y, row_cursor_x, row_max_h, layout,
              "brak schematu dla wariantu \"" + std::string(block_variant_id(b.variant)) + "\"",
              "Brak szablonu LTspice dla wariantu: " + std::string(block_variant_id(b.variant)));
          continue;
        }

        const auto config = regulator_template_config(b, template_label);
        std::string tpl = config.template_asc;
        if (tpl.empty()) {
          out.warnings.push_back(config.missing_template_warning);
          row_cursor_x += (700 + layout.gap_x);
          row_max_h = std::max(row_max_h, 220);
          continue;
        }

        const bool standalone_demo = blocks.size() == 1 && connections.empty();
        const bool has_input_source = regulator_has_external_input_source(b, blocks, connections);
        const bool has_output_load = regulator_has_external_output_load(b, blocks, connections);
        if (!standalone_demo) {
          tpl = remove_directives_with_prefix(tpl, ".dc");
          tpl = remove_directives_with_prefix(tpl, ".step");
        }
        if (has_input_source) {
          tpl = remove_component_by_instname(tpl, "V1");
          tpl = remove_wire_segment(tpl, config.vin_source_wire_top_x, config.vin_source_wire_top_y1,
                                    config.vin_source_wire_top_x, config.vin_source_wire_top_y2);
          tpl = remove_wire_segment(tpl, config.vin_source_wire_bottom_x,
                                    config.vin_source_wire_bottom_y1, config.vin_source_wire_bottom_x,
                                    config.vin_source_wire_bottom_y2);
          tpl = remove_flag(tpl, config.vin_source_flag_x, config.vin_source_flag_y);
        }
        if (has_output_load) {
          tpl = remove_component_by_instname(tpl, "I1");
          tpl = remove_wire_segment(tpl, config.load_wire_top_x, config.load_wire_top_y1,
                                    config.load_wire_top_x, config.load_wire_top_y2);
          tpl = remove_wire_segment(tpl, config.load_wire_bottom_x, config.load_wire_bottom_y1,
                                    config.load_wire_bottom_x, config.load_wire_bottom_y2);
          tpl = remove_wire_segment(tpl, config.load_link_x1, config.load_link_y1,
                                    config.load_link_x2, config.load_link_y2);
          tpl = remove_flag(tpl, config.load_flag_x, config.load_flag_y);
        }

        auto inst_values = build_regulator_instance_values(b, !standalone_demo, out.warnings);
        auto patched_values = pep::ltspice_patch_asc_values(tpl, inst_values);
        if (has_input_source) {
          erase_matching_warning(patched_values.warnings, "Nie znaleziono elementu w .asc: V1");
          erase_matching_warning(patched_values.warnings,
                                 "Nie znaleziono pola Value dla elementu: V1");
        }
        if (has_output_load) {
          erase_matching_warning(patched_values.warnings, "Nie znaleziono elementu w .asc: I1");
          erase_matching_warning(patched_values.warnings,
                                 "Nie znaleziono pola Value dla elementu: I1");
        }
        out.warnings.insert(out.warnings.end(), patched_values.warnings.begin(),
                            patched_values.warnings.end());

        append_export_fragment(out, sheet, dx, dy, row_cursor_x, row_max_h, layout,
                               patched_values.asc, build_block_flag_nets(b, topology),
                               "_B" + std::to_string(b.id));
        continue;
      }

      if (b.kind == BlockKind::Amplifier) {
        QString template_label;
        std::string tpl = load_amp_model1b_template(template_label);
        if (tpl.empty()) {
          append_missing_element_note(out, sheet, row_cursor_x, row_y, row_cursor_x, row_max_h,
                                      layout, "brak schematu dla elementu \"" + b.title + "\"",
                                      "Brak szablonu LTspice dla elementu: " + b.title);
          continue;
        }

        auto inst_values = build_amplifier_instance_values(b, out.warnings);
        auto patched_values = pep::ltspice_patch_asc_values(tpl, inst_values);
        out.warnings.insert(out.warnings.end(), patched_values.warnings.begin(),
                            patched_values.warnings.end());

        std::string amp_with_directives = patched_values.asc;
        if (b.gain > 0.0) {
          std::vector<std::string> amp_directives;
          const std::string suffix = "_B" + std::to_string(b.id);
          amp_directives.push_back(".param AV" + suffix + "=" + format_number(b.gain, 6));
          amp_directives.push_back("BGAIN" + suffix + " OUT" + suffix +
                                   " 0 V=V(IN" + suffix + ")*AV" + suffix);
          amp_directives.push_back(".options reltol=0.01 gmin=1e-9");
          amp_with_directives = append_directives_below(amp_with_directives, amp_directives);
        }

        append_export_fragment(out, sheet, dx, dy, row_cursor_x, row_max_h, layout,
                               amp_with_directives, build_block_flag_nets(b, topology),
                               "_B" + std::to_string(b.id));
        continue;
      }

      append_missing_element_note(out, sheet, row_cursor_x, row_y, row_cursor_x, row_max_h, layout,
                                  "brak schematu dla elementu \"" + b.title + "\"",
                                  "Brak szablonu LTspice dla elementu: " + b.title);
    }

    row_y += row_max_h + layout.gap_y;
  }

  finalize_sheet(sheet, tran_directive, out.directives);

  std::ostringstream result;
  for (std::size_t i = 0; i < sheet.lines.size(); ++i) {
    result << sheet.lines[i];
    if (i + 1 != sheet.lines.size()) {
      result << "\n";
    }
  }
  out.asc = result.str();
  return out;
}

} // namespace pep::modules::project_design
