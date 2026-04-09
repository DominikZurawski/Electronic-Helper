#include "export_actions.hpp"

#include "../export/export.hpp"
#include "../model/model.hpp"
#include "export_dialog.hpp"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QWidget>

namespace pep::modules::project_design {

namespace {

bool has_amplifier_block(const std::vector<Block> &blocks) {
  for (const auto &block : blocks) {
    if (block.kind == BlockKind::Amplifier) {
      return true;
    }
  }
  return false;
}

QString build_export_message(const QString &out_path, const AscAssembly &assembly) {
  QString message = "Zapisano:\n" + out_path;
  if (!assembly.warnings.empty()) {
    message += "\n\nUwagi:\n";
    for (const auto &warning : assembly.warnings) {
      message += "- " + QString::fromStdString(warning) + "\n";
    }
  }
  return message;
}

} // namespace

void export_ltspice_asc(QWidget *parent, const std::vector<Block> &blocks,
                        const std::vector<Connection> &connections) {
  const auto tran = prompt_tran_directive(parent, has_amplifier_block(blocks));
  const auto assembly = export_project_asc(blocks, connections, tran);

  QString out_path = QFileDialog::getSaveFileName(parent, "Zapisz schemat LTspice", QString(),
                                                  "Schematic (*.asc)");
  if (out_path.isEmpty()) {
    return;
  }
  if (!out_path.endsWith(".asc", Qt::CaseInsensitive)) {
    out_path += ".asc";
  }

  QFile out(out_path);
  if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    QMessageBox::warning(parent, "Eksport LTspice", "Nie udało się zapisać:\n" + out_path);
    return;
  }
  out.write(assembly.asc.c_str(), static_cast<int>(assembly.asc.size()));

  QMessageBox::information(parent, "Eksport LTspice", build_export_message(out_path, assembly));
}

} // namespace pep::modules::project_design
