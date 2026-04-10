#pragma once

#include <QWidget>

class QLabel;
class QFormLayout;

namespace pep::ui::calculation {

class CalculationView;

class CalculationModuleWidget : public QWidget {
public:
  explicit CalculationModuleWidget(QWidget *parent = nullptr);

  QFormLayout *form_layout() const;
  CalculationView *result_view() const;
  void set_description(const QString &description);

private:
  QLabel *description_label_ = nullptr;
  QFormLayout *form_layout_ = nullptr;
  CalculationView *result_view_ = nullptr;
};

} // namespace pep::ui::calculation
