#pragma once

#include <QString>

#include <string>

namespace pep::modules::project_design {

std::string load_psu_symmetric_template(QString &label_out);
std::string load_psu_unregulated_template(QString &label_out);
std::string load_psu_unregulated_no_load_template(QString &label_out);
std::string load_amp_model1b_template(QString &label_out);

} // namespace pep::modules::project_design
