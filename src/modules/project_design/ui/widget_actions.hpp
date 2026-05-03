#pragma once

#include "../model/model.hpp"

#include <functional>
#include <optional>
#include <vector>

class QComboBox;
class QGraphicsScene;
class QLabel;
class QWidget;

enum class SupplyRail;

namespace pep::modules::project_design {

void refresh_supply_source_combo(const std::vector<Block> &blocks, QComboBox *combo,
                                 bool include_regulators, std::optional<SupplyRail> regulator_rail = std::nullopt);

bool delete_selected_block(QGraphicsScene *scene, std::vector<Block> &blocks,
                           std::vector<Connection> &connections,
                           std::optional<int> &active_block_id,
                           const std::function<void()> &refresh_connections_ui,
                           const std::function<void()> &refresh_supply_source_combos,
                           const std::function<void()> &refresh_connection_controls,
                           const std::function<void(int)> &set_active,
                           const std::function<void()> &refresh_canvas,
                           const std::function<void()> &recompute_and_validate);

void handle_port_click(std::optional<Endpoint> &pending_connection, QLabel *validation,
                       std::vector<Connection> &connections, int block_id,
                       const std::string &port_id,
                       const std::function<void()> &refresh_connections_ui,
                       const std::function<void()> &recompute_and_validate,
                       const std::function<void()> &refresh_canvas);

void export_active_block(QWidget *parent, std::optional<int> active_block_id,
                         const std::vector<Block> &blocks,
                         const std::vector<Connection> &connections);

} // namespace pep::modules::project_design
