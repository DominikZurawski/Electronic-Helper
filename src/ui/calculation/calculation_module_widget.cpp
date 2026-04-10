#include "calculation_module_widget.hpp"

#include "calculation_view.hpp"

#include <QFormLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace pep::ui::calculation {

CalculationModuleWidget::CalculationModuleWidget(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto *description_label = new QLabel(this);
  description_label->setWordWrap(true);
  description_label->setVisible(false);
  description_label_ = description_label;
  layout->addWidget(description_label);

  auto *form_layout = new QFormLayout();
  form_layout->setContentsMargins(0, 0, 0, 0);
  form_layout->setRowWrapPolicy(QFormLayout::DontWrapRows);
  form_layout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  form_layout->setHorizontalSpacing(18);
  form_layout->setVerticalSpacing(10);
  form_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  form_layout_ = form_layout;
  layout->addLayout(form_layout);

  auto *result_view = new CalculationView(this);
  result_view->setMinimumHeight(220);
  result_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  result_view_ = result_view;
  layout->addWidget(result_view, 1);
}

QFormLayout *CalculationModuleWidget::form_layout() const {
  return form_layout_;
}

CalculationView *CalculationModuleWidget::result_view() const {
  return result_view_;
}

void CalculationModuleWidget::set_description(const QString &description) {
  if (!description_label_) {
    return;
  }
  description_label_->setText(description);
  description_label_->setVisible(!description.isEmpty());
}

} // namespace pep::ui::calculation
