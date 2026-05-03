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

QString describe_block(const Block &block) {
  const QString kind = block.kind == BlockKind::Power      ? "power"
                       : block.kind == BlockKind::Regulator ? "regulator"
                       : block.kind == BlockKind::Amplifier ? "amp"
                                                            : "unknown";
  return QString("#%1 %2 (%3)")
      .arg(block.id)
      .arg(QString::fromStdString(block_variant_id(block.variant)))
      .arg(kind);
}

QString build_export_message(const QString &out_path, const std::vector<Block> &blocks,
                             const AscAssembly &assembly) {
  QString message = "Zapisano:\n" + out_path;

  if (!blocks.empty()) {
    QStringList block_descriptions;
    for (const auto &block : blocks) {
      block_descriptions.push_back(describe_block(block));
    }
    message += "\n\nWyeksportowane bloki:\n- " + block_descriptions.join("\n- ");
  }

  if (!assembly.warnings.empty()) {
    message += "\n\nUwagi:\n";
    for (const auto &warning : assembly.warnings) {
      message += "- " + QString::fromStdString(warning) + "\n";
    }
  }
  return message;
}

QByteArray to_ltspice_file_bytes(const std::string &asc) {
  QByteArray out;
  out.reserve(static_cast<int>(asc.size() + 64));

  for (std::size_t i = 0; i < asc.size(); ++i) {
    const char ch = asc[i];
    if (ch == '\r') {
      out.append('\r');
      if (i + 1 < asc.size() && asc[i + 1] == '\n') {
        out.append('\n');
        ++i;
      } else {
        out.append('\n');
      }
      continue;
    }

    if (ch == '\n') {
      out.append('\r');
      out.append('\n');
      continue;
    }

    out.append(ch);
  }

  return out;
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
  const QByteArray file_bytes = to_ltspice_file_bytes(assembly.asc);
  if (out.write(file_bytes) != file_bytes.size()) {
    QMessageBox::warning(parent, "Eksport LTspice",
                         "Nie udało się zapisać całej zawartości pliku:\n" + out_path);
    return;
  }
  out.flush();
  out.close();

  QMessageBox::information(parent, "Eksport LTspice",
                           build_export_message(out_path, blocks, assembly));
}

} // namespace pep::modules::project_design
