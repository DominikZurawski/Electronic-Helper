#include "pep/modules/antenna_basic/ui/widget.hpp"

#include "src/ui/calculation/calculation_view.hpp"

#include <QApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>

#include <cassert>
#include <cstdlib>
#include <vector>

namespace {

pep::ui::calculation::CalculationView *find_calculation_view(QWidget &widget) {
  const auto children = widget.findChildren<QWidget *>();
  for (auto *child : children) {
    if (auto *view = dynamic_cast<pep::ui::calculation::CalculationView *>(child)) {
      return view;
    }
  }
  return nullptr;
}

void test_antenna_widget_uses_shared_calculation_view() {
  int argc = 0;
  char **argv = nullptr;
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  pep::modules::antenna_basic::Widget widget;
  widget.show();
  app.processEvents();

  auto *input = widget.findChild<QLineEdit *>();
  auto *button = widget.findChild<QPushButton *>();
  auto *view = find_calculation_view(widget);

  assert(input != nullptr);
  assert(button != nullptr);
  assert(view != nullptr);

  input->setText("145");
  button->click();
  app.processEvents();

  auto *fallback = view->findChild<QTextBrowser *>();
  if (fallback != nullptr) {
    const QString text = fallback->toPlainText();
    assert(text.contains("Kalkulator anten"));
    assert(text.contains("Dlugosc fali"));
    assert(text.contains("Pol fali"));
    assert(text.contains("Cwierc fali"));
  }
}

} // namespace

int main() {
  test_antenna_widget_uses_shared_calculation_view();
  return 0;
}
