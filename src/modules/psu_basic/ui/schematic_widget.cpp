#include "schematic_widget.hpp"

#include <QPainter>

namespace pep::modules::psu_basic {

SchematicWidget::SchematicWidget(QWidget* parent) : QWidget(parent) {
  setMinimumHeight(160);
}

void SchematicWidget::set_stage(SchematicStage stage) {
  stage_ = stage;
  update();
}

void SchematicWidget::paintEvent(QPaintEvent*) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.fillRect(rect(), palette().base());

  const int h = height();

  const int box_w = 120;
  const int box_h = 50;
  const int y = h / 2 - box_h / 2;
  const int x1 = 20;
  const int x2 = x1 + 150;
  const int x3 = x2 + 150;
  const int x4 = x3 + 150;

  auto draw_box = [&](int x, const QString& label, SchematicStage stage) {
    QColor fill = palette().button().color();
    if (stage_ >= stage) {
      fill = palette().highlight().color();
    }
    painter.setBrush(fill);
    painter.setPen(palette().text().color());
    painter.drawRoundedRect(QRect(x, y, box_w, box_h), 6, 6);
    painter.drawText(QRect(x, y, box_w, box_h), Qt::AlignCenter, label);
  };

  draw_box(x1, "Transformator", SchematicStage::Transformer);
  draw_box(x2, "Prostownik", SchematicStage::Rectifier);
  draw_box(x3, "Kondensator", SchematicStage::Capacitor);
  draw_box(x4, "Obciążenie", SchematicStage::Load);

  painter.setPen(QPen(palette().text().color(), 2));
  painter.drawLine(x1 + box_w, y + box_h / 2, x2, y + box_h / 2);
  painter.drawLine(x2 + box_w, y + box_h / 2, x3, y + box_h / 2);
  painter.drawLine(x3 + box_w, y + box_h / 2, x4, y + box_h / 2);
}

} // namespace pep::modules::psu_basic
