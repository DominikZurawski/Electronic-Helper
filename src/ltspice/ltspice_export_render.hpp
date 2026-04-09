#pragma once

#include "pep/ltspice_export.hpp"

#include <ostream>
#include <string>
#include <string_view>

namespace pep {

[[nodiscard]] std::string default_export_title(std::string_view requested_title,
                                               std::string_view fallback_title);
[[nodiscard]] double exported_rload(const LtspiceExportRequest &req);

void append_psu_basic_netlist(std::ostream &out, const LtspiceExportRequest &req);
void append_psu_basic_schematic_header(std::ostream &out);
void append_psu_basic_source(std::ostream &out, const LtspiceExportRequest &req,
                             const std::string &symbol_transformer);
void append_psu_basic_rectifier(std::ostream &out, const LtspiceExportRequest &req,
                                const std::string &symbol_bridge);
void append_psu_basic_filter_and_load(std::ostream &out, const LtspiceExportRequest &req);

} // namespace pep
