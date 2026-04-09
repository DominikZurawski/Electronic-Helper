#pragma once

#include <optional>
#include <string>

class QWidget;

namespace pep::modules::project_design {

std::optional<std::string> prompt_tran_directive(QWidget *parent, bool has_amplifier);

} // namespace pep::modules::project_design
