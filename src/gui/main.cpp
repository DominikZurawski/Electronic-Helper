#include <QApplication>
#include <QLabel>
#include <QMainWindow>

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  QMainWindow window;
  window.setWindowTitle("Pomocnik Projektanta Elektronika");

  auto* label = new QLabel(&window);
  label->setText("GUI startowe — wybierz moduł w przyszłości");
  label->setAlignment(Qt::AlignCenter);

  window.setCentralWidget(label);
  window.resize(640, 360);
  window.show();

  return app.exec();
}
