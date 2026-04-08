#include "graph.hpp"

#include <QAction>
#include <QContextMenuEvent>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QMenu>
#include <QMouseEvent>

namespace pep::modules::project_design {

BlockItem::BlockItem(int block_id, const QString &text,
                     std::function<void(int, const QPointF &)> on_moved, QGraphicsItem *parent)
    : QGraphicsRectItem(parent), block_id_(block_id), on_moved_(std::move(on_moved)) {
  setRect(0, 0, 240, 70);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setFlag(QGraphicsItem::ItemIsMovable, true);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
  setBrush(QColor(245, 245, 245));
  setPen(QPen(QColor(180, 180, 180)));

  label_ = new QGraphicsTextItem(text, this);
  label_->setDefaultTextColor(QColor(20, 20, 20));
  label_->setPos(10, 8);
}

int BlockItem::block_id() const {
  return block_id_;
}

QVariant BlockItem::itemChange(GraphicsItemChange change, const QVariant &value) {
  const QVariant v = QGraphicsRectItem::itemChange(change, value);
  if (change == QGraphicsItem::ItemPositionHasChanged) {
    if (on_moved_) {
      on_moved_(block_id_, pos());
    }
  }
  return v;
}

CanvasView::CanvasView(QGraphicsScene *scene, QWidget *parent) : QGraphicsView(scene, parent) {}

void CanvasView::wheelEvent(QWheelEvent *event) {
  if (event->modifiers() & Qt::ControlModifier) {
    const double factor = (event->angleDelta().y() > 0) ? 1.15 : (1.0 / 1.15);
    scale(factor, factor);
    event->accept();
    return;
  }
  QGraphicsView::wheelEvent(event);
}

void CanvasView::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    if (auto *scene = this->scene()) {
      const QPointF p = mapToScene(event->pos());
      QGraphicsItem *item = scene->itemAt(p, transform());
      if (item) {
        // Port item is identified by data() markers.
        const QVariant bid = item->data(1);
        const QVariant pid = item->data(2);
        if (bid.isValid() && pid.isValid()) {
          if (on_port_clicked) {
            on_port_clicked(bid.toInt(), pid.toString().toStdString());
          }
          event->accept();
          return;
        }
      }
    }
  }
  QGraphicsView::mousePressEvent(event);
}

void CanvasView::contextMenuEvent(QContextMenuEvent *event) {
  QMenu menu(this);
  auto *add_power = menu.addAction("Dodaj element: Zasilanie");
  auto *add_amp = menu.addAction("Dodaj element: Wzmacniacz model 1(B)");
  menu.addSeparator();
  auto *del = menu.addAction("Usuń zaznaczony element");

  QObject::connect(add_power, &QAction::triggered, this, [&]() {
    if (on_add_power) {
      on_add_power();
    }
  });
  QObject::connect(add_amp, &QAction::triggered, this, [&]() {
    if (on_add_amp_model1b) {
      on_add_amp_model1b();
    }
  });
  QObject::connect(del, &QAction::triggered, this, [&]() {
    if (on_delete_selected) {
      on_delete_selected();
    }
  });

  menu.exec(event->globalPos());
}

void CanvasView::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
    if (on_delete_selected) {
      on_delete_selected();
    }
    event->accept();
    return;
  }
  if (event->modifiers() & Qt::ControlModifier) {
    if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
      scale(1.15, 1.15);
      event->accept();
      return;
    }
    if (event->key() == Qt::Key_Minus) {
      scale(1.0 / 1.15, 1.0 / 1.15);
      event->accept();
      return;
    }
    if (event->key() == Qt::Key_0) {
      resetTransform();
      event->accept();
      return;
    }
  }
  QGraphicsView::keyPressEvent(event);
}

} // namespace pep::modules::project_design
