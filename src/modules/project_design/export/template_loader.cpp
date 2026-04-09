#include "template_loader.hpp"

#include <QFile>
#include <QSettings>

#include <string>

namespace pep::modules::project_design {

namespace {

std::string load_template_with_fallback(const QString &settings_key, const QString &legacy_key,
                                        const QString &resource_path, QString &label_out) {
  QSettings settings("PomocnikProjektantaElektronika", "PPE");
  QString template_path = settings.value(settings_key, "").toString();
  if (template_path.isEmpty() && !legacy_key.isEmpty()) {
    template_path = settings.value(legacy_key, "").toString();
  }

  if (!template_path.isEmpty()) {
    QFile file(template_path);
    if (file.open(QIODevice::ReadOnly)) {
      label_out = template_path;
      return file.readAll().toStdString();
    }
  }

  QFile resource_file(resource_path);
  if (resource_file.open(QIODevice::ReadOnly)) {
    label_out = "wbudowany (resource)";
    return resource_file.readAll().toStdString();
  }

  label_out = "brak";
  return "";
}

} // namespace

std::string load_psu_symmetric_template(QString &label_out) {
  return load_template_with_fallback("ltspice/templates/psu_symmetric_asc", "ltspice/template_asc",
                                     ":/ltspice/power.asc", label_out);
}

std::string load_amp_model1b_template(QString &label_out) {
  return load_template_with_fallback("ltspice/templates/amp_model1b_asc", "",
                                     ":/ltspice/opamp_model1b.asc", label_out);
}

} // namespace pep::modules::project_design
