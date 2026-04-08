#include "pep/ltspice_export.hpp"

#include "ltspice_export_render.hpp"

#include <sstream>

namespace pep {

std::string LtspiceExporter::export_psu_basic_netlist(const LtspiceExportRequest &req) const {
  std::ostringstream out;
  append_psu_basic_netlist(out, req);
  return out.str();
}

std::string LtspiceExporter::export_psu_basic_schematic(const LtspiceExportRequest &req,
                                                        const std::string &cir_filename,
                                                        const std::string &symbol_transformer,
                                                        const std::string &symbol_bridge) const {
  std::ostringstream out;
  (void)cir_filename;
  append_psu_basic_schematic_header(out);
  append_psu_basic_source(out, req, symbol_transformer);
  append_psu_basic_rectifier(out, req, symbol_bridge);
  append_psu_basic_filter_and_load(out, req);

  return out.str();
}

} // namespace pep
