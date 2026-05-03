#include "widget_actions.hpp"

#include "../graph/graph.hpp"
#include "../model/connection_ops.hpp"
#include "../model/model.hpp"
#include "export_actions.hpp"

#include <QComboBox>
#include <QGraphicsScene>
#include <QLabel>

namespace pep::modules::project_design {

void refresh_supply_source_combo(const std::vector<Block> &blocks, QComboBox *combo,
                                 bool include_regulators, std::optional<SupplyRail> regulator_rail) {
  if (!combo) {
    return;
  }

  const QVariant current = combo->currentData();
  combo->blockSignals(true);
  combo->clear();
  combo->addItem("— (brak)", 0);
  for (const auto &block : blocks) {
    const bool include_regulator =
        include_regulators && block.kind == BlockKind::Regulator &&
        (!regulator_rail.has_value() || block.regulator_supply_rail == *regulator_rail);
    if (block.kind == BlockKind::Power || include_regulator) {
      combo->addItem(QString("#%1 %2").arg(block.id).arg(QString::fromStdString(block.title)),
                     block.id);
    }
  }
  const int index = combo->findData(current);
  combo->setCurrentIndex(index >= 0 ? index : 0);
  combo->blockSignals(false);
}

bool delete_selected_block(QGraphicsScene *scene, std::vector<Block> &blocks,
                           std::vector<Connection> &connections,
                           std::optional<int> &active_block_id,
                           const std::function<void()> &refresh_connections_ui,
                           const std::function<void()> &refresh_supply_source_combos,
                           const std::function<void()> &refresh_connection_controls,
                           const std::function<void(int)> &set_active,
                           const std::function<void()> &refresh_canvas,
                           const std::function<void()> &recompute_and_validate) {
  if (!scene) {
    return false;
  }

  const auto selection = scene->selectedItems();
  if (selection.empty()) {
    return false;
  }

  auto *item = dynamic_cast<BlockItem *>(selection.front());
  if (!item) {
    return false;
  }

  remove_block(blocks, connections, item->block_id());
  refresh_connections_ui();
  refresh_supply_source_combos();
  refresh_connection_controls();

  if (blocks.empty()) {
    active_block_id.reset();
    refresh_canvas();
    recompute_and_validate();
    return true;
  }

  set_active(blocks.front().id);
  refresh_canvas();
  return true;
}

void handle_port_click(std::optional<Endpoint> &pending_connection, QLabel *validation,
                       std::vector<Connection> &connections, int block_id,
                       const std::string &port_id,
                       const std::function<void()> &refresh_connections_ui,
                       const std::function<void()> &recompute_and_validate,
                       const std::function<void()> &refresh_canvas) {
  const Endpoint endpoint{block_id, port_id};
  if (!pending_connection.has_value()) {
    pending_connection = endpoint;
    if (validation) {
      validation->setText(
          "Połączenia: wybierz drugi port (albo dodaj nowy element), aby połączyć.");
    }
    refresh_canvas();
    return;
  }

  const Endpoint start = *pending_connection;
  pending_connection.reset();
  if (start.block_id == endpoint.block_id && start.port_id == endpoint.port_id) {
    return;
  }

  try_add_connection(connections, start, endpoint);
  refresh_connections_ui();
  recompute_and_validate();
  refresh_canvas();
}

void export_active_block(QWidget *parent, std::optional<int> active_block_id,
                         const std::vector<Block> &blocks,
                         const std::vector<Connection> &connections) {
  if (!active_block_id.has_value()) {
    return;
  }

  const Block *block = find_block(blocks, *active_block_id);
  if (!block) {
    return;
  }

  std::vector<Block> single_blocks = {*block};
  std::vector<Connection> single_connections;
  for (const auto &connection : connections) {
    if (connection.a.block_id == *active_block_id && connection.b.block_id == *active_block_id) {
      single_connections.push_back(connection);
    }
  }

  export_ltspice_asc(parent, single_blocks, single_connections);
}

} // namespace pep::modules::project_design
