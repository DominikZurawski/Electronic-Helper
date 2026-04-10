#include <QApplication>
#include <QDialog>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

#include "pep/ltspice_import.hpp"

#include "pep/modules/antenna_basic_ui.hpp"
#include "pep/modules/project_design.hpp"

namespace {

void update_theme_action_label(QAction *action, bool dark) {
  if (!action) {
    return;
  }
  action->setText(dark ? "Tryb jasny" : "Tryb ciemny");
  action->setToolTip(dark ? "Przełącz na tryb jasny" : "Przełącz na tryb ciemny");
}

QString build_app_theme_stylesheet() {
  return QStringLiteral(
      "QLineEdit, QComboBox, QAbstractSpinBox, QTextEdit, QPlainTextEdit, QListWidget, "
      "QTableWidget {"
      "  background-color: palette(base);"
      "  color: palette(text);"
      "  border: 1px solid palette(mid);"
      "  border-radius: 6px;"
      "  padding: 4px 8px;"
      "  selection-background-color: palette(highlight);"
      "  selection-color: palette(highlighted-text);"
      "}"
      "QComboBox {"
      "  padding-right: 30px;"
      "}"
      "QLineEdit[readOnly=\"true\"], QTextEdit[readOnly=\"true\"], QPlainTextEdit[readOnly=\"true\"] {"
      "  background-color: palette(alternate-base);"
      "}"
      "QComboBox::drop-down {"
      "  subcontrol-origin: padding;"
      "  subcontrol-position: top right;"
      "  border: 0;"
      "  width: 24px;"
      "  background: transparent;"
      "}"
      "QComboBox::down-arrow {"
      "  image: url(:/icons/chevron-down.svg);"
      "  width: 10px;"
      "  height: 6px;"
      "}"
      "QComboBox QAbstractItemView {"
      "  background-color: palette(base);"
      "  color: palette(text);"
      "  border: 1px solid palette(mid);"
      "  selection-background-color: palette(highlight);"
      "  selection-color: palette(highlighted-text);"
      "}"
      "QToolTip {"
      "  background-color: palette(base);"
      "  color: palette(text);"
      "  border: 1px solid palette(mid);"
      "}"
      "QScrollBar:vertical {"
      "  background: transparent;"
      "  width: 14px;"
      "  margin: 4px 2px 4px 2px;"
      "}"
      "QScrollBar::handle:vertical {"
      "  background: palette(mid);"
      "  min-height: 28px;"
      "  border-radius: 7px;"
      "}"
      "QScrollBar::handle:vertical:hover {"
      "  background: palette(highlight);"
      "}"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
      "  height: 0px;"
      "  background: transparent;"
      "  border: none;"
      "}"
      "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
      "  background: transparent;"
      "}"
      "QScrollBar:horizontal {"
      "  background: transparent;"
      "  height: 14px;"
      "  margin: 2px 4px 2px 4px;"
      "}"
      "QScrollBar::handle:horizontal {"
      "  background: palette(mid);"
      "  min-width: 28px;"
      "  border-radius: 7px;"
      "}"
      "QScrollBar::handle:horizontal:hover {"
      "  background: palette(highlight);"
      "}"
      "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
      "  width: 0px;"
      "  background: transparent;"
      "  border: none;"
      "}"
      "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
      "  background: transparent;"
      "}");
}

void apply_app_theme_palette(QApplication &app, const QPalette &default_palette, bool dark) {
  app.setProperty("ppe.dark_mode", dark);

  QPalette palette = default_palette;
  if (dark) {
    palette.setColor(QPalette::Window, QColor(27, 27, 30));
    palette.setColor(QPalette::WindowText, QColor(235, 239, 245));
    palette.setColor(QPalette::Base, QColor(38, 40, 45));
    palette.setColor(QPalette::AlternateBase, QColor(31, 33, 37));
    palette.setColor(QPalette::ToolTipBase, QColor(38, 40, 45));
    palette.setColor(QPalette::ToolTipText, QColor(235, 239, 245));
    palette.setColor(QPalette::Text, QColor(235, 239, 245));
    palette.setColor(QPalette::Button, QColor(42, 44, 49));
    palette.setColor(QPalette::ButtonText, QColor(235, 239, 245));
    palette.setColor(QPalette::BrightText, QColor(255, 255, 255));
    palette.setColor(QPalette::PlaceholderText, QColor(145, 151, 160));
    palette.setColor(QPalette::Light, QColor(69, 72, 79));
    palette.setColor(QPalette::Midlight, QColor(58, 61, 67));
    palette.setColor(QPalette::Mid, QColor(84, 88, 96));
    palette.setColor(QPalette::Dark, QColor(22, 23, 26));
    palette.setColor(QPalette::Shadow, QColor(12, 13, 15));
    palette.setColor(QPalette::Highlight, QColor(72, 126, 214));
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    palette.setColor(QPalette::Link, QColor(118, 171, 255));
    palette.setColor(QPalette::LinkVisited, QColor(154, 130, 224));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(130, 136, 145));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(130, 136, 145));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(130, 136, 145));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(66, 72, 84));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(180, 186, 196));
  } else {
    palette.setColor(QPalette::Window, QColor(248, 249, 251));
    palette.setColor(QPalette::WindowText, QColor(24, 33, 43));
    palette.setColor(QPalette::Base, QColor(255, 255, 255));
    palette.setColor(QPalette::AlternateBase, QColor(243, 246, 250));
    palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    palette.setColor(QPalette::ToolTipText, QColor(24, 33, 43));
    palette.setColor(QPalette::Text, QColor(24, 33, 43));
    palette.setColor(QPalette::Button, QColor(246, 248, 251));
    palette.setColor(QPalette::ButtonText, QColor(24, 33, 43));
    palette.setColor(QPalette::PlaceholderText, QColor(111, 124, 140));
    palette.setColor(QPalette::Light, QColor(240, 243, 247));
    palette.setColor(QPalette::Midlight, QColor(220, 226, 234));
    palette.setColor(QPalette::Mid, QColor(173, 184, 198));
    palette.setColor(QPalette::Dark, QColor(131, 142, 156));
    palette.setColor(QPalette::Shadow, QColor(96, 106, 118));
    palette.setColor(QPalette::Highlight, QColor(72, 126, 214));
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    palette.setColor(QPalette::Link, QColor(57, 107, 186));
    palette.setColor(QPalette::LinkVisited, QColor(101, 83, 165));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(142, 150, 160));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(142, 150, 160));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(142, 150, 160));
  }

  app.setPalette(palette);
  app.setStyleSheet(build_app_theme_stylesheet());
}

} // namespace

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setStyle(QStyleFactory::create("Fusion"));

  QSettings settings("PomocnikProjektantaElektronika", "PPE");
  const bool dark_mode_enabled = settings.value("ui/dark_mode", true).toBool();
  const QPalette default_palette = app.palette();
  apply_app_theme_palette(app, default_palette, dark_mode_enabled);

  QMainWindow window;
  window.setWindowTitle("Pomocnik Projektanta Elektronika");
  window.resize(1400, 900);

  auto *root = new QWidget();
  auto *root_layout = new QHBoxLayout(root);
  root_layout->setContentsMargins(12, 12, 12, 12);
  root_layout->setSpacing(12);

  auto *module_list = new QListWidget();
  module_list->addItem("Projektowanie układu");
  module_list->addItem("Anteny");
  module_list->setCurrentRow(0);
  module_list->setMinimumWidth(180);

  auto *stack = new QStackedWidget();
  stack->addWidget(new pep::modules::project_design::Widget());
  stack->addWidget(new pep::modules::antenna_basic::Widget());

  QObject::connect(module_list, &QListWidget::currentRowChanged, stack,
                   &QStackedWidget::setCurrentIndex);

  auto *main_splitter = new QSplitter(Qt::Horizontal, root);
  main_splitter->setChildrenCollapsible(false);
  main_splitter->setHandleWidth(8);
  main_splitter->addWidget(module_list);
  main_splitter->addWidget(stack);
  main_splitter->setStretchFactor(0, 0);
  main_splitter->setStretchFactor(1, 1);
  main_splitter->setSizes({220, 1180});
  root_layout->addWidget(main_splitter, 1);

  window.setCentralWidget(root);

  window.menuBar()->setNativeMenuBar(false);
  auto *file_menu = window.menuBar()->addMenu("Plik");
  auto *import_ltspice = file_menu->addAction("Import LTspice...");
  auto *ltspice_settings = file_menu->addAction("Ustawienia LTspice...");

  QObject::connect(import_ltspice, &QAction::triggered, [&]() {
    QSettings settings("PomocnikProjektantaElektronika", "PPE");
    const QString default_dir = settings.value("ltspice/default_dir", "").toString();
    const QString path =
        QFileDialog::getOpenFileName(&window, "Wybierz plik LTspice", default_dir,
                                     "LTspice (*.asc *.cir *.net);;Wszystkie pliki (*.*)");
    if (path.isEmpty()) {
      return;
    }

    pep::LtspiceImporter importer;
    auto result = importer.import_file(path.toStdString());

    QDialog dialog(&window);
    dialog.setWindowTitle("LTspice - podgląd elementów");
    dialog.resize(640, 360);

    auto *layout = new QVBoxLayout(&dialog);
    auto *info = new QLabel(QString("Plik: %1\nFormat: %2\nLiczba linii: %3")
                                .arg(QString::fromStdString(result.source_path))
                                .arg(QString::fromStdString(result.format))
                                .arg(result.line_count));
    layout->addWidget(info);

    auto *table = new QTableWidget(&dialog);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({"Typ", "Nazwa", "Wartość"});
    table->setRowCount(static_cast<int>(result.components.size()));

    for (int i = 0; i < static_cast<int>(result.components.size()); ++i) {
      const auto &c = result.components[i];
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

    for (const auto &c : result.components) {
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

    auto *checklist = new QLabel(QString("Checklist (heurystyka):\n"
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

    auto *layout = new QVBoxLayout(&dialog);
    auto *info = new QLabel(
        "Podaj opcjonalnie ścieżkę do folderu LTspice oraz domyślny folder ze schematami.\n"
        "Możesz też podać nazwy symboli (jeśli masz własne biblioteki).");
    info->setWordWrap(true);
    layout->addWidget(info);

    auto *form = new QFormLayout();
    auto *ltspice_dir = new QLineEdit(settings.value("ltspice/install_dir", "").toString());
    auto *default_dir = new QLineEdit(settings.value("ltspice/default_dir", "").toString());
    auto *symbol_transformer =
        new QLineEdit(settings.value("ltspice/symbol_transformer", "").toString());
    auto *symbol_bridge = new QLineEdit(settings.value("ltspice/symbol_bridge", "").toString());
    auto *psu_symmetric_template_asc =
        new QLineEdit(settings.value("ltspice/templates/psu_symmetric_asc", "").toString());
    auto *amp_model1b_template_asc =
        new QLineEdit(settings.value("ltspice/templates/amp_model1b_asc", "").toString());

    auto *browse1 = new QPushButton("Wybierz...");
    auto *browse2 = new QPushButton("Wybierz...");
    auto *browse3 = new QPushButton("Wybierz...");
    auto *browse4 = new QPushButton("Wybierz...");

    auto *row1 = new QWidget();
    auto *row1l = new QHBoxLayout(row1);
    row1l->setContentsMargins(0, 0, 0, 0);
    row1l->addWidget(ltspice_dir, 1);
    row1l->addWidget(browse1);

    auto *row2 = new QWidget();
    auto *row2l = new QHBoxLayout(row2);
    row2l->setContentsMargins(0, 0, 0, 0);
    row2l->addWidget(default_dir, 1);
    row2l->addWidget(browse2);

    form->addRow("Folder LTspice", row1);
    form->addRow("Domyślny folder schematów", row2);
    form->addRow("Symbol transformatora (opcjonalnie)", symbol_transformer);
    form->addRow("Symbol mostka (opcjonalnie)", symbol_bridge);

    auto *row3 = new QWidget();
    auto *row3l = new QHBoxLayout(row3);
    row3l->setContentsMargins(0, 0, 0, 0);
    row3l->addWidget(psu_symmetric_template_asc, 1);
    row3l->addWidget(browse3);
    form->addRow("Szablon zasilania symetrycznego (.asc) (opcjonalnie)", row3);

    auto *row4 = new QWidget();
    auto *row4l = new QHBoxLayout(row4);
    row4l->setContentsMargins(0, 0, 0, 0);
    row4l->addWidget(amp_model1b_template_asc, 1);
    row4l->addWidget(browse4);
    form->addRow("Szablon wzmacniacza model 1(B) (.asc) (opcjonalnie)", row4);
    layout->addLayout(form);

    QObject::connect(browse1, &QPushButton::clicked, [&]() {
      const QString dir = QFileDialog::getExistingDirectory(&dialog, "Wybierz folder LTspice");
      if (!dir.isEmpty()) {
        ltspice_dir->setText(dir);
      }
    });
    QObject::connect(browse2, &QPushButton::clicked, [&]() {
      const QString dir = QFileDialog::getExistingDirectory(&dialog, "Wybierz folder schematów");
      if (!dir.isEmpty()) {
        default_dir->setText(dir);
      }
    });
    QObject::connect(browse3, &QPushButton::clicked, [&]() {
      const QString path =
          QFileDialog::getOpenFileName(&dialog, "Wybierz szablon zasilania symetrycznego (.asc)",
                                       QString(), "LTspice (*.asc);;Wszystkie pliki (*.*)");
      if (!path.isEmpty()) {
        psu_symmetric_template_asc->setText(path);
      }
    });
    QObject::connect(browse4, &QPushButton::clicked, [&]() {
      const QString path =
          QFileDialog::getOpenFileName(&dialog, "Wybierz szablon wzmacniacza model 1(B) (.asc)",
                                       QString(), "LTspice (*.asc);;Wszystkie pliki (*.*)");
      if (!path.isEmpty()) {
        amp_model1b_template_asc->setText(path);
      }
    });

    auto *buttons = new QHBoxLayout();
    auto *save = new QPushButton("Zapisz");
    auto *cancel = new QPushButton("Anuluj");
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

  auto *view_menu = window.menuBar()->addMenu("Widok");
  auto *toggle_theme = view_menu->addAction("Tryb ciemny");
  toggle_theme->setCheckable(true);

  QObject::connect(toggle_theme, &QAction::toggled, [&](bool dark) {
    settings.setValue("ui/dark_mode", dark);
    update_theme_action_label(toggle_theme, dark);
    apply_app_theme_palette(app, default_palette, dark);
  });
  {
    toggle_theme->setChecked(dark_mode_enabled);
    update_theme_action_label(toggle_theme, toggle_theme->isChecked());
  }

  auto *toolbar = new QToolBar("Widok");
  toolbar->addAction(toggle_theme);
  window.addToolBar(toolbar);

  window.show();

  return app.exec();
}
