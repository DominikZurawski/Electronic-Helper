#pragma once

#include <QGraphicsRectItem>
#include <QGraphicsView>

#include <functional>

namespace pep::modules::project_design {

class BlockItem : public QGraphicsRectItem {
public:
  BlockItem(int block_id,
            const QString& text,
            std::function<void(int, const QPointF&)> on_moved,
            QGraphicsItem* parent = nullptr);

  int block_id() const;

protected:
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
  int block_id_ = 0;
  QGraphicsTextItem* label_ = nullptr;
  std::function<void(int, const QPointF&)> on_moved_;
};

class CanvasView : public QGraphicsView {
public:
  explicit CanvasView(QGraphicsScene* scene, QWidget* parent = nullptr);

  std::function<void()> on_add_power;
  std::function<void()> on_add_amp_model1b;
  std::function<void()> on_delete_selected;
  std::function<void(int block_id, const QString& port_id)> on_port_clicked;

protected:
  void wheelEvent(QWheelEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
};

} // namespace pep::modules::project_design
