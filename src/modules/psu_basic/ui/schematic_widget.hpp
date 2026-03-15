#pragma once

#include <QWidget>

namespace pep::modules::psu_basic {

enum class SchematicStage {
  Transformer,
  Rectifier,
  Capacitor,
  Load
};

class SchematicWidget : public QWidget {
public:
  explicit SchematicWidget(QWidget* parent = nullptr);

  void set_stage(SchematicStage stage);

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  SchematicStage stage_ = SchematicStage::Transformer;
};

} // namespace pep::modules::psu_basic
