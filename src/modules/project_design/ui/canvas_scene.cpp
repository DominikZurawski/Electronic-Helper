#include "canvas_scene.hpp"

#include "../graph/graph.hpp"
#include "canvas_layout.hpp"
#include "enum_ui.hpp"

#include <QBrush>
#include <QColor>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QPen>
#include <QTimer>

#include <algorithm>
#include <optional>

namespace pep::modules::project_design {

namespace {

QPointF to_qpointf(const CanvasPoint &point) {
  return QPointF(point.x, point.y);
}

bool is_power_type(PortType type) {
  return type == PortType::PowerPos || type == PortType::PowerNeg || type == PortType::Ground;
}

std::optional<QPointF> port_center_for(const std::vector<Block> &blocks, int block_id,
                                       const std::string &port_id) {
  const Block *block = find_block(blocks, block_id);
  if (!block) {
    return std::nullopt;
  }

  int pin_index = 0;
  for (const auto &port : ports_for(*block)) {
    if (port.id == port_id) {
      const double radius = 6.0;
      const bool is_left = port.type == PortType::AnalogIn ||
                           (port.type == PortType::Ground && block->kind != BlockKind::Power);
      const double x = is_left ? 6.0 : (240.0 - 12.0);
      const double y = 12.0 + pin_index * 14.0;
      return QPointF(block->canvas_pos.x + x + radius, block->canvas_pos.y + y + radius);
    }
    ++pin_index;
  }

  return std::nullopt;
}

void schedule_signal_lines_update(const CanvasSceneState &state) {
  if (!state.signal_lines || !state.blocks || !state.scene || !state.timer_owner ||
      !state.signal_lines_update_scheduled) {
    return;
  }
  if (*state.signal_lines_update_scheduled) {
    return;
  }

  *state.signal_lines_update_scheduled = true;
  QTimer::singleShot(0, state.timer_owner, [state]() {
    if (!state.signal_lines_update_scheduled || !state.scene || !state.blocks ||
        !state.signal_lines) {
      return;
    }

    *state.signal_lines_update_scheduled = false;
    for (auto &signal_line : *state.signal_lines) {
      if (!signal_line.item || signal_line.item->scene() != state.scene) {
        continue;
      }

      const auto a = port_center_for(*state.blocks, signal_line.a_block_id, signal_line.a_port_id);
      const auto b = port_center_for(*state.blocks, signal_line.b_block_id, signal_line.b_port_id);
      if (!a.has_value() || !b.has_value()) {
        continue;
      }
      signal_line.item->setLine(QLineF(*a, *b));
    }
  });
}

QString power_summary_for_block(const Block &block, const std::vector<Block> &blocks,
                                const std::vector<Connection> &connections) {
  QStringList used_power_supplies;
  for (const auto &connection : connections) {
    const bool a_is_this = (connection.a.block_id == block.id);
    const bool b_is_this = (connection.b.block_id == block.id);
    if (!a_is_this && !b_is_this) {
      continue;
    }

    const Endpoint other = a_is_this ? connection.b : connection.a;
    const Block *other_block = find_block(blocks, other.block_id);
    if (!other_block || other_block->kind != BlockKind::Power) {
      continue;
    }

    const auto block_port =
        find_port(block, a_is_this ? connection.a.port_id : connection.b.port_id);
    if (!block_port.has_value() || !is_power_type(block_port->type)) {
      continue;
    }

    const QString label = QString("#%1").arg(other_block->id);
    if (!used_power_supplies.contains(label)) {
      used_power_supplies.push_back(label);
    }
  }

  if (block.kind != BlockKind::Power) {
    return used_power_supplies.isEmpty() ? "Zasilanie: —"
                                         : ("Zasilanie: " + used_power_supplies.join(", "));
  }

  QStringList outputs;
  for (const auto &port : ports_for(block)) {
    if (port.type != PortType::Ground) {
      outputs.push_back(QString::fromStdString(port.label));
    }
  }
  return "Wyjścia: " + (outputs.isEmpty() ? "—" : outputs.join(", "));
}

QColor port_fill_color(PortType type, bool selected) {
  if (selected) {
    return QColor(255, 245, 180);
  }
  if (type == PortType::PowerPos) {
    return QColor(255, 210, 210);
  }
  if (type == PortType::PowerNeg) {
    return QColor(210, 220, 255);
  }
  if (type == PortType::Ground) {
    return QColor(220, 220, 220);
  }
  if (type == PortType::AnalogIn || type == PortType::AnalogOut) {
    return QColor(210, 245, 210);
  }
  return QColor(230, 230, 230);
}

void draw_block_ports(const Block &block, const std::optional<Endpoint> &pending_connection,
                      BlockItem *item) {
  int pin_index = 0;
  for (const auto &port : ports_for(block)) {
    const double radius = 6.0;
    const bool is_left = port.type == PortType::AnalogIn ||
                         (port.type == PortType::Ground && block.kind != BlockKind::Power);
    const double x = is_left ? 6.0 : (240.0 - 12.0);
    const double y = 12.0 + pin_index * 14.0;
    const bool selected = pending_connection.has_value() &&
                          pending_connection->block_id == block.id &&
                          pending_connection->port_id == port.id;

    auto *pin = new QGraphicsEllipseItem(item);
    pin->setRect(x, y, radius * 2, radius * 2);
    pin->setPen(QPen(QColor(80, 80, 80)));
    pin->setBrush(QBrush(port_fill_color(port.type, selected)));
    pin->setData(1, block.id);
    pin->setData(2, QString::fromStdString(port.id));
    pin->setToolTip(QString::fromStdString(port.label) + " (" + port_type_label(port.type) + ")");
    pin->setZValue(5);
    ++pin_index;
  }
}

void draw_connection_lines(const CanvasSceneState &state) {
  if (!state.scene || !state.blocks || !state.connections || !state.signal_lines) {
    return;
  }

  for (const auto &connection : *state.connections) {
    const Block *from = find_block(*state.blocks, connection.a.block_id);
    const Block *to = find_block(*state.blocks, connection.b.block_id);
    if (!from || !to) {
      continue;
    }

    const auto from_port = find_port(*from, connection.a.port_id);
    const auto to_port = find_port(*to, connection.b.port_id);
    if (!from_port.has_value() || !to_port.has_value()) {
      continue;
    }

    const bool from_power = is_power_type(from_port->type);
    const bool to_power = is_power_type(to_port->type);
    if (from_power && to_power) {
      continue;
    }

    const auto a = port_center_for(*state.blocks, connection.a.block_id, connection.a.port_id);
    const auto b = port_center_for(*state.blocks, connection.b.block_id, connection.b.port_id);
    if (!a.has_value() || !b.has_value()) {
      continue;
    }

    QColor color(0, 140, 70);
    if (!ports_compatible(from_port->type, to_port->type)) {
      color = QColor(200, 20, 20);
    }

    auto *line = state.scene->addLine(QLineF(*a, *b), QPen(color, 2.0));
    line->setZValue(-10);
    line->setAcceptedMouseButtons(Qt::NoButton);
    state.signal_lines->push_back(CanvasSignalLine{connection.a.block_id, connection.a.port_id,
                                                   connection.b.block_id, connection.b.port_id,
                                                   line});
  }
}

void update_scene_rect(QGraphicsScene *scene, const std::vector<Block> &blocks) {
  int max_pins = 0;
  for (const auto &block : blocks) {
    max_pins = std::max(max_pins, static_cast<int>(ports_for(block).size()));
  }

  const double block_height = std::max(140.0, 40.0 + max_pins * 16.0);
  double max_x = 520.0;
  double max_y = 160.0;
  for (const auto &block : blocks) {
    max_x = std::max(max_x, block.canvas_pos.x + 260.0);
    max_y = std::max(max_y, block.canvas_pos.y + block_height + 60.0);
  }
  scene->setSceneRect(0, 0, max_x + 20.0, max_y);
}

} // namespace

void refresh_canvas_scene(const CanvasSceneState &state) {
  if (!state.scene || !state.blocks || !state.connections || !state.signal_lines ||
      !state.refreshing_canvas) {
    return;
  }

  *state.refreshing_canvas = true;
  state.signal_lines->clear();
  state.scene->clear();

  if (state.blocks->empty()) {
    state.scene->setSceneRect(0, 0, 520.0, 160.0);
    *state.refreshing_canvas = false;
    return;
  }

  if (!state.manual_layout) {
    apply_auto_layout(*state.blocks, *state.connections);
  }

  BlockItem *selected_item = nullptr;
  for (const auto &block : *state.blocks) {
    const QString text =
        QString("#%1\n%2\n%3")
            .arg(block.id)
            .arg(QString::fromStdString(block.title))
            .arg(power_summary_for_block(block, *state.blocks, *state.connections));

    auto *item = new BlockItem(block.id, text, [state](int block_id, const QPointF &pos) {
      if (!state.blocks || !state.refreshing_canvas || *state.refreshing_canvas) {
        return;
      }

      Block *block = find_block(*state.blocks, block_id);
      if (!block) {
        return;
      }
      block->canvas_pos = CanvasPoint{pos.x(), pos.y()};
      schedule_signal_lines_update(state);
      if (state.on_block_moved) {
        state.on_block_moved(block_id, pos);
      }
    });
    item->setPos(to_qpointf(block.canvas_pos));
    state.scene->addItem(item);

    if (state.active_block_id.has_value() && block.id == *state.active_block_id) {
      selected_item = item;
    }
    draw_block_ports(block, state.pending_connection, item);
  }

  draw_connection_lines(state);

  if (selected_item) {
    selected_item->setSelected(true);
  }

  update_scene_rect(state.scene, *state.blocks);
  *state.refreshing_canvas = false;
}

} // namespace pep::modules::project_design
