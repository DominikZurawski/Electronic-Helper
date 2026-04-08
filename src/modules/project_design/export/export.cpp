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

namespace {} // namespace

AscAssembly export_project_asc(const std::vector<Block> &blocks,
                               const std::vector<Connection> &connections,
                               const std::optional<std::string> &tran_directive) {
  AscAssembly out;
  const auto topology = build_export_topology(blocks, connections);
  out.warnings.insert(out.warnings.end(), topology.warnings.begin(), topology.warnings.end());
  ExportSheetState sheet;
  initialize_sheet(sheet);
  const ExportLayoutConfig layout;

  int row_y = 0;

  for (const auto &order : topology.ordered_components) {
    int row_cursor_x = 0;
    int row_max_h = 220;

    for (const int bi : order) {
      const auto &b = blocks[bi];
      const int dx = row_cursor_x;
      const int dy = row_y;

      if (b.kind == BlockKind::Power) {
        if (b.variant != BlockVariant::PsuSymmetric) {
          append_missing_element_note(
              out, sheet, row_cursor_x, row_y, row_cursor_x, row_max_h, layout,
              "brak schematu dla wariantu \"" + std::string(block_variant_id(b.variant)) + "\"",
              "Brak szablonu LTspice dla wariantu: " + std::string(block_variant_id(b.variant)));
          continue;
        }

        QString template_label;
        std::string tpl = load_psu_symmetric_template(template_label);
        if (tpl.empty()) {
          out.warnings.push_back("Nie udało się wczytać szablonu zasilania symetrycznego.");
          row_cursor_x += (700 + layout.gap_x);
          row_max_h = std::max(row_max_h, 220);
          continue;
        }

        auto patched_values = pep::modules::psu_basic::export_schematic_from_asc_template(
            b.vin_ac_rms, b.mains_hz, b.load_current, b.capacitor_uF, tpl);
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

  finalize_sheet(sheet, tran_directive);

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
