#include "export.hpp"

#include "asc_ops.hpp"
#include "export_render.hpp"
#include "export_topology.hpp"
#include "export_value_builders.hpp"
#include "template_loader.hpp"

#include "../model/model.hpp"

#include "../../psu_basic/export/template_export.hpp"

#include "pep/ltspice_template.hpp"

#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace pep::modules::project_design {

namespace {

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

void erase_matching_warning(std::vector<std::string> &warnings, const std::string &pattern) {
  warnings.erase(std::remove_if(warnings.begin(), warnings.end(),
                                [&](const std::string &warning) {
                                  return warning.find(pattern) != std::string::npos;
                                }),
                 warnings.end());
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
        } else if (b.variant == BlockVariant::PsuUnregulated) {
          const bool has_real_load = power_block_has_non_power_load(b, blocks, connections);
          tpl = has_real_load ? load_psu_unregulated_no_load_template(template_label)
                              : load_psu_unregulated_template(template_label);
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
        auto patched_values = pep::modules::psu_basic::export_schematic_from_asc_template(
            b.vin_ac_rms, b.mains_hz, b.load_current, b.capacitor_uF,
            b.transformer_primary_tol_pct, step_param, tpl);
        if (b.variant == BlockVariant::PsuUnregulated &&
            power_block_has_non_power_load(b, blocks, connections)) {
          erase_matching_warning(patched_values.warnings, "Nie znaleziono elementu w .asc: Rload");
          erase_matching_warning(patched_values.warnings, "Nie znaleziono elementu w .asc: RL");
          erase_matching_warning(patched_values.warnings, "Nie znaleziono pola Value dla elementu: Rload");
          erase_matching_warning(patched_values.warnings, "Nie znaleziono pola Value dla elementu: RL");
        }
        out.warnings.insert(out.warnings.end(), patched_values.warnings.begin(),
                            patched_values.warnings.end());
        if (!patched_values.directives.empty()) {
          out.directives.insert(out.directives.end(), patched_values.directives.begin(),
                                patched_values.directives.end());
        }

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

        append_export_fragment(out, sheet, dx, dy, row_cursor_x, row_max_h, layout,
                               patched_values.asc, build_block_flag_nets(b, topology),
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
