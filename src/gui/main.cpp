#include <QApplication>
#include <QDialog>
#include <QFileDialog>
#include <QFrame>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QSettings>

#include <algorithm>

#include "pep/ltspice_import.hpp"

#include "pep/modules/project_design.hpp"
#include "pep/modules/antenna_basic_ui.hpp"

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  app.setStyle(QStyleFactory::create("Fusion"));

  QMainWindow window;
  window.setWindowTitle("Pomocnik Projektanta Elektronika");

  auto* root = new QWidget();
  auto* root_layout = new QHBoxLayout(root);
  root_layout->setContentsMargins(12, 12, 12, 12);
  root_layout->setSpacing(12);

  auto* module_list = new QListWidget();
  module_list->addItem("Projektowanie układu");
  module_list->addItem("Anteny");
  module_list->setCurrentRow(0);
  module_list->setMaximumWidth(260);

  auto* divider = new QFrame();
  divider->setFrameShape(QFrame::VLine);
  divider->setFrameShadow(QFrame::Sunken);

  auto* stack = new QStackedWidget();
  stack->addWidget(new pep::modules::project_design::Widget());
  stack->addWidget(new pep::modules::antenna_basic::Widget());

  QObject::connect(module_list, &QListWidget::currentRowChanged, stack, &QStackedWidget::setCurrentIndex);

  root_layout->addWidget(module_list);
  root_layout->addWidget(divider);
  root_layout->addWidget(stack, 1);

  window.setCentralWidget(root);

  window.menuBar()->setNativeMenuBar(false);
  auto* file_menu = window.menuBar()->addMenu("Plik");
  auto* import_ltspice = file_menu->addAction("Import LTspice...");
  auto* ltspice_settings = file_menu->addAction("Ustawienia LTspice...");

  QObject::connect(import_ltspice, &QAction::triggered, [&]() {
    QSettings settings("PomocnikProjektantaElektronika", "PPE");
    const QString default_dir = settings.value("ltspice/default_dir", "").toString();
    const QString path = QFileDialog::getOpenFileName(
        &window,
        "Wybierz plik LTspice",
        default_dir,
        "LTspice (*.asc *.cir *.net);;Wszystkie pliki (*.*)");
    if (path.isEmpty()) {
      return;
    }

    pep::LtspiceImporter importer;
    auto result = importer.import_file(path.toStdString());

    QDialog dialog(&window);
    dialog.setWindowTitle("LTspice - podgląd elementów");
    dialog.resize(640, 360);

    auto* layout = new QVBoxLayout(&dialog);
    auto* info = new QLabel(QString("Plik: %1\nFormat: %2\nLiczba linii: %3")
                                .arg(QString::fromStdString(result.source_path))
                                .arg(QString::fromStdString(result.format))
                                .arg(result.line_count));
    layout->addWidget(info);

    auto* table = new QTableWidget(&dialog);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({"Typ", "Nazwa", "Wartość"});
    table->setRowCount(static_cast<int>(result.components.size()));

    for (int i = 0; i < static_cast<int>(result.components.size()); ++i) {
      const auto& c = result.components[i];
      table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(c.kind)));
      table->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(c.name)));
      table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(c.value)));
    }
    table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(table);

    bool has_transformer = false;
    bool has_rectifier = false;
    bool has_capacitor = false;
    bool has_load = false;

    for (const auto& c : result.components) {
      std::string kind = c.kind;
      std::transform(kind.begin(), kind.end(), kind.begin(), ::tolower);
      if (kind.find("tran") != std::string::npos) {
        has_transformer = true;
      }
      if (kind.find("diode") != std::string::npos || kind.find("bridge") != std::string::npos) {
        has_rectifier = true;
      }
      if (kind.find("cap") != std::string::npos) {
        has_capacitor = true;
      }
      if (kind.find("res") != std::string::npos || kind.find("load") != std::string::npos) {
        has_load = true;
      }
    }

    auto* checklist = new QLabel(QString(
        "Checklist (heurystyka):\n"
        "- Transformator: %1\n"
        "- Prostownik/diody: %2\n"
        "- Kondensator filtrujący: %3\n"
        "- Obciążenie: %4\n")
        .arg(has_transformer ? "OK" : "Brak")
        .arg(has_rectifier ? "OK" : "Brak")
        .arg(has_capacitor ? "OK" : "Brak")
        .arg(has_load ? "OK" : "Brak"));
    checklist->setWordWrap(true);
    layout->addWidget(checklist);

    dialog.exec();
  });

  QObject::connect(ltspice_settings, &QAction::triggered, [&]() {
    QSettings settings("PomocnikProjektantaElektronika", "PPE");
    QDialog dialog(&window);
    dialog.setWindowTitle("Ustawienia LTspice");
    dialog.resize(560, 300);

    auto* layout = new QVBoxLayout(&dialog);
    auto* info = new QLabel(
        "Podaj opcjonalnie ścieżkę do folderu LTspice oraz domyślny folder ze schematami.\n"
        "Możesz też podać nazwy symboli (jeśli masz własne biblioteki).");
    info->setWordWrap(true);
    layout->addWidget(info);

    auto* form = new QFormLayout();
    auto* ltspice_dir = new QLineEdit(settings.value("ltspice/install_dir", "").toString());
    auto* default_dir = new QLineEdit(settings.value("ltspice/default_dir", "").toString());
    auto* symbol_transformer = new QLineEdit(settings.value("ltspice/symbol_transformer", "").toString());
    auto* symbol_bridge = new QLineEdit(settings.value("ltspice/symbol_bridge", "").toString());
    auto* psu_symmetric_template_asc = new QLineEdit(settings.value("ltspice/templates/psu_symmetric_asc", "").toString());
    auto* amp_model1b_template_asc = new QLineEdit(settings.value("ltspice/templates/amp_model1b_asc", "").toString());

    auto* browse1 = new QPushButton("Wybierz...");
    auto* browse2 = new QPushButton("Wybierz...");
    auto* browse3 = new QPushButton("Wybierz...");
    auto* browse4 = new QPushButton("Wybierz...");

    auto* row1 = new QWidget();
    auto* row1l = new QHBoxLayout(row1);
    row1l->setContentsMargins(0, 0, 0, 0);
    row1l->addWidget(ltspice_dir, 1);
    row1l->addWidget(browse1);

    auto* row2 = new QWidget();
    auto* row2l = new QHBoxLayout(row2);
    row2l->setContentsMargins(0, 0, 0, 0);
    row2l->addWidget(default_dir, 1);
    row2l->addWidget(browse2);

    form->addRow("Folder LTspice", row1);
    form->addRow("Domyślny folder schematów", row2);
    form->addRow("Symbol transformatora (opcjonalnie)", symbol_transformer);
    form->addRow("Symbol mostka (opcjonalnie)", symbol_bridge);

    auto* row3 = new QWidget();
    auto* row3l = new QHBoxLayout(row3);
    row3l->setContentsMargins(0, 0, 0, 0);
    row3l->addWidget(psu_symmetric_template_asc, 1);
    row3l->addWidget(browse3);
    form->addRow("Szablon zasilania symetrycznego (.asc) (opcjonalnie)", row3);

    auto* row4 = new QWidget();
    auto* row4l = new QHBoxLayout(row4);
    row4l->setContentsMargins(0, 0, 0, 0);
    row4l->addWidget(amp_model1b_template_asc, 1);
    row4l->addWidget(browse4);
    form->addRow("Szablon wzmacniacza model 1(B) (.asc) (opcjonalnie)", row4);
    layout->addLayout(form);

    QObject::connect(browse1, &QPushButton::clicked, [&]() {
      const QString dir = QFileDialog::getExistingDirectory(&dialog, "Wybierz folder LTspice");
      if (!dir.isEmpty()) ltspice_dir->setText(dir);
    });
    QObject::connect(browse2, &QPushButton::clicked, [&]() {
      const QString dir = QFileDialog::getExistingDirectory(&dialog, "Wybierz folder schematów");
      if (!dir.isEmpty()) default_dir->setText(dir);
    });
    QObject::connect(browse3, &QPushButton::clicked, [&]() {
      const QString path = QFileDialog::getOpenFileName(&dialog,
                                                        "Wybierz szablon zasilania symetrycznego (.asc)",
                                                        QString(),
                                                        "LTspice (*.asc);;Wszystkie pliki (*.*)");
      if (!path.isEmpty()) psu_symmetric_template_asc->setText(path);
    });
    QObject::connect(browse4, &QPushButton::clicked, [&]() {
      const QString path = QFileDialog::getOpenFileName(&dialog,
                                                        "Wybierz szablon wzmacniacza model 1(B) (.asc)",
                                                        QString(),
                                                        "LTspice (*.asc);;Wszystkie pliki (*.*)");
      if (!path.isEmpty()) amp_model1b_template_asc->setText(path);
    });

    auto* buttons = new QHBoxLayout();
    auto* save = new QPushButton("Zapisz");
    auto* cancel = new QPushButton("Anuluj");
    buttons->addStretch(1);
    buttons->addWidget(cancel);
    buttons->addWidget(save);
    layout->addLayout(buttons);

    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(save, &QPushButton::clicked, [&]() {
      settings.setValue("ltspice/install_dir", ltspice_dir->text());
      settings.setValue("ltspice/default_dir", default_dir->text());
      settings.setValue("ltspice/symbol_transformer", symbol_transformer->text());
      settings.setValue("ltspice/symbol_bridge", symbol_bridge->text());
      settings.setValue("ltspice/templates/psu_symmetric_asc", psu_symmetric_template_asc->text());
      settings.setValue("ltspice/templates/amp_model1b_asc", amp_model1b_template_asc->text());
      dialog.accept();
    });

    dialog.exec();
  });

  auto* view_menu = window.menuBar()->addMenu("Widok");
  auto* toggle_theme = view_menu->addAction("Tryb ciemny");
  toggle_theme->setCheckable(true);

  const QPalette default_palette = app.palette();
  QObject::connect(toggle_theme, &QAction::toggled, [&](bool dark) {
    QPalette palette = default_palette;
    if (dark) {
      palette.setColor(QPalette::Window, QColor(30, 30, 30));
      palette.setColor(QPalette::WindowText, Qt::white);
      palette.setColor(QPalette::Base, QColor(45, 45, 45));
      palette.setColor(QPalette::AlternateBase, QColor(35, 35, 35));
      palette.setColor(QPalette::Text, Qt::white);
      palette.setColor(QPalette::Button, QColor(45, 45, 45));
      palette.setColor(QPalette::ButtonText, Qt::white);
      palette.setColor(QPalette::Highlight, QColor(0, 120, 215));
      palette.setColor(QPalette::HighlightedText, Qt::white);
    }
    app.setPalette(palette);
  });

  auto* toolbar = new QToolBar("Widok");
  toolbar->addAction(toggle_theme);
  window.addToolBar(toolbar);

  window.showMaximized();

  return app.exec();
}
