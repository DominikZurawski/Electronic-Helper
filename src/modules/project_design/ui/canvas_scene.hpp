#pragma once

#include "../model/model.hpp"

#include <QPointF>

#include <functional>
#include <optional>
#include <vector>

class QGraphicsLineItem;
class QGraphicsScene;
class QObject;

namespace pep::modules::project_design {

struct CanvasSignalLine {
  int a_block_id = 0;
  std::string a_port_id;
  int b_block_id = 0;
  std::string b_port_id;
  QGraphicsLineItem *item = nullptr;
};

struct CanvasSceneState {
  QGraphicsScene *scene = nullptr;
  std::vector<CanvasSignalLine> *signal_lines = nullptr;
  std::vector<Block> *blocks = nullptr;
  const std::vector<Connection> *connections = nullptr;
  std::optional<int> active_block_id;
  std::optional<Endpoint> pending_connection;
  bool manual_layout = false;
  bool *refreshing_canvas = nullptr;
  bool *signal_lines_update_scheduled = nullptr;
  QObject *timer_owner = nullptr;
  std::function<void(int, const QPointF &)> on_block_moved;
};

void refresh_canvas_scene(const CanvasSceneState &state);

} // namespace pep::modules::project_design
