#include "connection_ui.hpp"

#include "../model/model.hpp"
#include "enum_ui.hpp"

#include <QComboBox>
#include <QListWidget>
#include <QString>

namespace pep::modules::project_design {

namespace {

QString endpoint_label(const Block &block, const PortDef &port) {
  return QString("#%1 %2: %3")
      .arg(block.id)
      .arg(QString::fromStdString(block.title))
      .arg(QString::fromStdString(port.label));
}

} // namespace

void refresh_connection_form(const std::vector<Block> &blocks, QComboBox *conn_from_block,
                             QComboBox *conn_to_block) {
  if (!conn_from_block || !conn_to_block) {
    return;
  }

  conn_from_block->clear();
  conn_to_block->clear();
  for (const auto &block : blocks) {
    conn_from_block->addItem(
        QString("#%1 %2").arg(block.id).arg(QString::fromStdString(block.title)), block.id);
    conn_to_block->addItem(QString("#%1 %2").arg(block.id).arg(QString::fromStdString(block.title)),
                           block.id);
  }

  if (conn_from_block->count() > 0 && conn_from_block->currentIndex() < 0) {
    conn_from_block->setCurrentIndex(0);
  }
  if (conn_to_block->count() > 0 && conn_to_block->currentIndex() < 0) {
    conn_to_block->setCurrentIndex(0);
  }
}

void refresh_ports_combo(const std::vector<Block> &blocks, QComboBox *block_combo,
                         QComboBox *port_combo) {
  if (!block_combo || !port_combo) {
    return;
  }

  port_combo->clear();
  const int id = block_combo->currentData().toInt();
  const Block *block = find_block(const_cast<std::vector<Block> &>(blocks), id);
  if (!block) {
    return;
  }

  for (const auto &port : ports_for(*block)) {
    port_combo->addItem(QString::fromStdString(port.label) + " (" + port_type_label(port.type) +
                            ")",
                        QString::fromStdString(port.id));
  }
}

void refresh_connections_ui(const std::vector<Block> &blocks,
                            const std::vector<Connection> &connections, QListWidget *list) {
  if (!list) {
    return;
  }

  list->clear();
  auto &mutable_blocks = const_cast<std::vector<Block> &>(blocks);
  for (const auto &connection : connections) {
    const Block *from = find_block(mutable_blocks, connection.a.block_id);
    const Block *to = find_block(mutable_blocks, connection.b.block_id);
    if (!from || !to) {
      continue;
    }

    const auto from_port = find_port(*from, connection.a.port_id);
    const auto to_port = find_port(*to, connection.b.port_id);
    if (!from_port.has_value() || !to_port.has_value()) {
      continue;
    }

    list->addItem(endpoint_label(*from, *from_port) + "  ↔  " + endpoint_label(*to, *to_port));
  }
}

bool add_connection_from_form(const std::vector<Block> &blocks,
                              std::vector<Connection> &connections, QComboBox *conn_from_block,
                              QComboBox *conn_from_port, QComboBox *conn_to_block,
                              QComboBox *conn_to_port) {
  if (!conn_from_block || !conn_from_port || !conn_to_block || !conn_to_port) {
    return false;
  }
  if (conn_from_block->count() == 0 || conn_to_block->count() == 0) {
    return false;
  }

  const int from_id = conn_from_block->currentData().toInt();
  const int to_id = conn_to_block->currentData().toInt();
  const std::string from_port_id = conn_from_port->currentData().toString().toStdString();
  const std::string to_port_id = conn_to_port->currentData().toString().toStdString();
  if (from_id == to_id && from_port_id == to_port_id) {
    return false;
  }

  auto &mutable_blocks = const_cast<std::vector<Block> &>(blocks);
  const Block *from = find_block(mutable_blocks, from_id);
  const Block *to = find_block(mutable_blocks, to_id);
  if (!from || !to) {
    return false;
  }

  const auto from_port = find_port(*from, from_port_id);
  const auto to_port = find_port(*to, to_port_id);
  if (!from_port.has_value() || !to_port.has_value()) {
    return false;
  }

  connections.push_back(Connection{Endpoint{from_id, from_port_id}, Endpoint{to_id, to_port_id}});
  return true;
}

bool remove_selected_connection(std::vector<Connection> &connections, QListWidget *list) {
  if (!list) {
    return false;
  }

  const int row = list->currentRow();
  if (row < 0 || row >= static_cast<int>(connections.size())) {
    return false;
  }

  connections.erase(connections.begin() + row);
  return true;
}

} // namespace pep::modules::project_design
