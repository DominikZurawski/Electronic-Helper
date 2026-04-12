#include "template_loader.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
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

  QString relative_asset_path = resource_path;
  if (relative_asset_path.startsWith(":/")) {
    relative_asset_path.remove(0, 2);
  }

  const QStringList fallback_paths = {
      QDir::current().filePath(relative_asset_path),
      QDir::current().filePath("assets/" + relative_asset_path),
      QDir::current().filePath("../" + relative_asset_path),
      QDir::current().filePath("../assets/" + relative_asset_path),
      QDir::current().filePath("../../" + relative_asset_path),
      QDir::current().filePath("../../assets/" + relative_asset_path),
  };

  for (const auto &fallback_path : fallback_paths) {
    QFileInfo info(fallback_path);
    if (!info.exists() || !info.isFile()) {
      continue;
    }

    QFile fallback_file(fallback_path);
    if (fallback_file.open(QIODevice::ReadOnly)) {
      label_out = fallback_path;
      return fallback_file.readAll().toStdString();
    }
  }

  label_out = "brak";
  return "";
}

} // namespace

std::string load_psu_symmetric_template(QString &label_out) {
  return load_template_with_fallback("ltspice/templates/psu_symmetric_asc", "ltspice/template_asc",
                                     ":/ltspice/power.asc", label_out);
}

std::string load_psu_unregulated_template(QString &label_out) {
  return load_template_with_fallback("ltspice/templates/psu_unregulated_asc", "",
                                     ":/ltspice/power_single.asc", label_out);
}

std::string load_psu_unregulated_no_load_template(QString &label_out) {
  return load_template_with_fallback("ltspice/templates/psu_unregulated_noload_asc", "",
                                     ":/ltspice/power_single_noload.asc", label_out);
}

std::string load_amp_model1b_template(QString &label_out) {
  return load_template_with_fallback("ltspice/templates/amp_model1b_asc", "",
                                     ":/ltspice/opamp_model1b.asc", label_out);
}

} // namespace pep::modules::project_design
