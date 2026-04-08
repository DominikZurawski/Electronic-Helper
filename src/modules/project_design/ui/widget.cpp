#include "pep/modules/project_design/ui/widget.hpp"

#include "../export/export.hpp"
#include "../graph/graph.hpp"
#include "../model/model.hpp"

#include "../../psu_basic/model/model.hpp"
#include "../../psu_basic/ui/waveform_widget.hpp"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHash>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pep::modules::project_design {

namespace {

double read_or(QLineEdit *edit, double fallback) {
  return edit->text().isEmpty() ? fallback : edit->text().toDouble();
}

QString endpoint_label(const Block &b, const PortDef &p) {
  return QString("#%1 %2: %3").arg(b.id).arg(b.title).arg(p.label);
}

} // namespace

struct Widget::Impl {
  struct SignalLine {
    int a_block_id = 0;
    QString a_port_id;
    int b_block_id = 0;
    QString b_port_id;
    QGraphicsLineItem *item = nullptr;
  };

  std::vector<Block> blocks;
  std::vector<Connection> connections;
  int next_block_id = 1;
  std::optional<int> active_block_id;
  bool refreshing_canvas = false;
  std::optional<Endpoint> pending_connection;
  bool manual_layout = false;
  bool signal_lines_update_scheduled = false;
  bool syncing_active_to_form = false;

  // UI pointers (owned by QWidget parent tree).
  QGraphicsScene *scene = nullptr;
  std::vector<SignalLine> signal_lines;
  QListWidget *connections_list = nullptr;
  QComboBox *conn_from_block = nullptr;
  QComboBox *conn_from_port = nullptr;
  QComboBox *conn_to_block = nullptr;
  QComboBox *conn_to_port = nullptr;
  QTabWidget *bottom_tabs = nullptr;
  int waveform_tab_index = -1;

  QLabel *active_title = nullptr;
  QLabel *ports_label = nullptr;
  QStackedWidget *props_stack = nullptr;
  QComboBox *variant = nullptr;
  QLineEdit *vin_input = nullptr;
  QLineEdit *freq_input = nullptr;
  QLineEdit *current_input = nullptr;
  QLineEdit *cap_input = nullptr;
  QLineEdit *extra_rails_input = nullptr;

  QComboBox *amp_waveform = nullptr;
  QComboBox *amp_power_source = nullptr;
  QLineEdit *amp_amp_input = nullptr;
  QLineEdit *amp_freq_input = nullptr;
  QLineEdit *amp_gain_input = nullptr;
  QLabel *compute_result = nullptr;
  QLabel *validation = nullptr;

  pep::modules::psu_basic::WaveformWidget *waveform_in = nullptr;
  pep::modules::psu_basic::WaveformWidget *waveform_out = nullptr;

  // Helpers (stable, stored here to avoid capturing references to locals).
  std::function<void()> refresh_canvas;
  std::function<void()> refresh_connections_ui;
  std::function<void()> refresh_connection_form;
  std::function<void(QComboBox *, QComboBox *)> refresh_ports_combo;
  std::function<void()> sync_active_to_form;
  std::function<void()> sync_form_to_active;
  std::function<void()> recompute_and_validate;
  std::function<void(int)> set_active;
};

Widget::Widget(QWidget *parent) : QWidget(parent) {
  impl_ = std::make_unique<Impl>();

  auto *root = new QHBoxLayout(this);
  root->setContentsMargins(12, 12, 12, 12);
  root->setSpacing(12);

  // Seed with one power block.
  impl_->blocks.push_back(
      make_power_block(impl_->next_block_id++, "Zasilanie #1", "psu_symmetric"));
  impl_->active_block_id = impl_->blocks.front().id;

  auto *left = new QWidget(this);  // calculators
  auto *right = new QWidget(this); // waveforms + graph
  left->setMinimumWidth(320);
  right->setMinimumWidth(420);
  auto *left_layout = new QVBoxLayout(left);
  auto *right_layout = new QVBoxLayout(right);

  auto *scene = new QGraphicsScene(right);
  impl_->scene = scene;
  auto *view = new CanvasView(scene, right);
  view->setMinimumHeight(280);
  view->setRenderHint(QPainter::Antialiasing, true);
  view->setFocusPolicy(Qt::StrongFocus);

  auto *canvas_toolbar = new QWidget(right);
  auto *canvas_toolbar_l = new QHBoxLayout(canvas_toolbar);
  canvas_toolbar_l->setContentsMargins(0, 0, 0, 0);
  auto *btn_add_power = new QPushButton("Dodaj zasilanie", canvas_toolbar);
  auto *btn_add_amp = new QPushButton("Dodaj wzmacniacz 1(B)", canvas_toolbar);
  auto *btn_auto_layout = new QPushButton("Auto-rozmieszczenie", canvas_toolbar);
  canvas_toolbar_l->addWidget(btn_add_power);
  canvas_toolbar_l->addWidget(btn_add_amp);
  canvas_toolbar_l->addWidget(btn_auto_layout);
  canvas_toolbar_l->addStretch(1);

  auto *waveforms_box = new QGroupBox("Wykresy", right);
  auto *waveforms_layout = new QVBoxLayout(waveforms_box);
  auto *w_in_label = new QLabel("Sygnał wejściowy", waveforms_box);
  auto *w_out_label = new QLabel("Sygnał wyjściowy", waveforms_box);
  auto *waveform_in = new pep::modules::psu_basic::WaveformWidget(waveforms_box);
  auto *waveform_out = new pep::modules::psu_basic::WaveformWidget(waveforms_box);
  impl_->waveform_in = waveform_in;
  impl_->waveform_out = waveform_out;
  waveform_in->setMinimumHeight(160);
  waveform_out->setMinimumHeight(160);
  waveforms_layout->addWidget(w_in_label);
  waveforms_layout->addWidget(waveform_in);
  waveforms_layout->addWidget(w_out_label);
  waveforms_layout->addWidget(waveform_out);

  auto *connections_tab = new QWidget(right);
  auto *connections_layout = new QVBoxLayout(connections_tab);
  auto *connections_list = new QListWidget(connections_tab);
  connections_list->setSelectionMode(QAbstractItemView::SingleSelection);
  impl_->connections_list = connections_list;
  connections_layout->addWidget(connections_list);

  auto *conn_form = new QFormLayout();
  auto *conn_from_block = new QComboBox(connections_tab);
  auto *conn_from_port = new QComboBox(connections_tab);
  auto *conn_to_block = new QComboBox(connections_tab);
  auto *conn_to_port = new QComboBox(connections_tab);
  impl_->conn_from_block = conn_from_block;
  impl_->conn_from_port = conn_from_port;
  impl_->conn_to_block = conn_to_block;
  impl_->conn_to_port = conn_to_port;
  conn_form->addRow("Z", conn_from_block);
  conn_form->addRow("Port", conn_from_port);
  conn_form->addRow("Do", conn_to_block);
  conn_form->addRow("Port", conn_to_port);
  auto *add_conn = new QPushButton("Dodaj połączenie", connections_tab);
  auto *remove_conn = new QPushButton("Usuń zaznaczone połączenie", connections_tab);

  connections_layout->addLayout(conn_form);
  connections_layout->addWidget(add_conn);
  connections_layout->addWidget(remove_conn);

  auto *active_title = new QLabel("Aktywny element: —", right);
  active_title->setStyleSheet("font-weight: 600; font-size: 14px;");
  impl_->active_title = active_title;

  auto *ports_label = new QLabel(right);
  ports_label->setWordWrap(true);
  impl_->ports_label = ports_label;

  auto *props_stack = new QStackedWidget(left);
  impl_->props_stack = props_stack;
  auto *power_props = new QWidget(props_stack);
  auto *power_form = new QFormLayout(power_props);
  auto *variant = new QComboBox(power_props);
  impl_->variant = variant;
  variant->addItem("Zasilacz symetryczny", "psu_symmetric");
  variant->addItem("Zasilacz niestabilizowany (brak schematu)", "psu_unregulated");

  auto *vin_input = new QLineEdit(power_props);
  auto *freq_input = new QLineEdit(power_props);
  auto *current_input = new QLineEdit(power_props);
  auto *cap_input = new QLineEdit(power_props);
  auto *extra_rails_input = new QLineEdit(power_props);
  impl_->vin_input = vin_input;
  impl_->freq_input = freq_input;
  impl_->current_input = current_input;
  impl_->cap_input = cap_input;
  impl_->extra_rails_input = extra_rails_input;
  vin_input->setPlaceholderText("VAC RMS");
  freq_input->setPlaceholderText("50 / 60");
  current_input->setPlaceholderText("A");
  cap_input->setPlaceholderText("uF");
  extra_rails_input->setPlaceholderText("+5V, +12V (opcjonalnie)");
  power_form->addRow("Typ zasilacza", variant);
  power_form->addRow("Napięcie wejściowe (VAC RMS)", vin_input);
  power_form->addRow("Częstotliwość sieci (Hz)", freq_input);
  power_form->addRow("Prąd obciążenia (A)", current_input);
  power_form->addRow("Pojemność kondensatora (uF)", cap_input);
  power_form->addRow("Dodatkowe szyny dodatnie (+)", extra_rails_input);

  auto *amp_props = new QWidget(props_stack);
  auto *amp_form = new QFormLayout(amp_props);
  auto *amp_model = new QLabel("Model 1(B)", amp_props);
  auto *amp_power_source = new QComboBox(amp_props);
  auto *amp_waveform = new QComboBox(amp_props);
  amp_waveform->addItem("Sinus", "sine");
  amp_waveform->addItem("Prostokąt", "square");
  amp_waveform->addItem("Trójkąt", "triangle");
  auto *amp_amp_input = new QLineEdit(amp_props);
  auto *amp_freq_input = new QLineEdit(amp_props);
  auto *amp_gain_input = new QLineEdit(amp_props);
  amp_amp_input->setPlaceholderText("V (amplituda)");
  amp_freq_input->setPlaceholderText("Hz");
  amp_gain_input->setPlaceholderText("np. 10");
  impl_->amp_waveform = amp_waveform;
  impl_->amp_power_source = amp_power_source;
  impl_->amp_amp_input = amp_amp_input;
  impl_->amp_freq_input = amp_freq_input;
  impl_->amp_gain_input = amp_gain_input;
  amp_form->addRow("Model wzmacniacza", amp_model);
  amp_form->addRow("Źródło zasilania", amp_power_source);
  amp_form->addRow("Rodzaj przebiegu wejściowego", amp_waveform);
  amp_form->addRow("Amplituda wejściowa (V)", amp_amp_input);
  amp_form->addRow("Częstotliwość sygnału (Hz)", amp_freq_input);
  amp_form->addRow("Wzmocnienie napięciowe (V/V)", amp_gain_input);

  props_stack->addWidget(power_props); // index 0
  props_stack->addWidget(amp_props);   // index 1

  auto *compute_result = new QLabel("Wyniki pojawią się tutaj", left);
  compute_result->setWordWrap(true);
  impl_->compute_result = compute_result;

  auto *validation = new QLabel(left);
  validation->setWordWrap(true);
  impl_->validation = validation;

  auto *export_active = new QPushButton("Eksport aktywnego elementu do LtSpice (.asc)", right);
  auto *export_project = new QPushButton("Eksport całego układu do LtSpice (.asc)", right);

  auto *left_title = new QLabel("Kalkulatory", left);
  left_title->setStyleSheet("font-weight: 600; font-size: 14px;");
  left_layout->addWidget(left_title);
  left_layout->addWidget(props_stack);
  left_layout->addWidget(compute_result);
  // validation moved to Export tab
  left_layout->addStretch(1);

  auto *info_tab = new QWidget(right);
  auto *info_layout = new QVBoxLayout(info_tab);
  info_layout->addWidget(active_title);
  info_layout->addWidget(ports_label);
  info_layout->addStretch(1);

  auto *export_tab = new QWidget(right);
  auto *export_layout = new QVBoxLayout(export_tab);
  export_layout->addWidget(validation);
  export_layout->addWidget(export_active);
  export_layout->addWidget(export_project);
  export_layout->addStretch(1);

  auto *waveforms_tab = new QWidget(right);
  auto *waveforms_tab_layout = new QVBoxLayout(waveforms_tab);
  waveforms_tab_layout->addWidget(waveforms_box);

  auto *bottom_tabs = new QTabWidget(right);
  impl_->bottom_tabs = bottom_tabs;
  impl_->waveform_tab_index = bottom_tabs->addTab(waveforms_tab, "Wykresy");
  bottom_tabs->addTab(connections_tab, "Połączenia portów");
  bottom_tabs->addTab(info_tab, "Aktywny element");
  bottom_tabs->addTab(export_tab, "Eksport");

  auto *graph_help =
      new QLabel("Schemat blokowy (aby połączyć porty, kliknij najpierw pierwszy port, a potem "
                 "drugi; Ctrl+scroll = zoom; Ctrl+0 reset). "
                 "Blok zasilania jest widoczny na schemacie, ale nie jest łączony liniami.",
                 right);
  graph_help->setWordWrap(true);
  right_layout->addWidget(graph_help);

  auto *graph_legend = new QLabel("Legenda portów: czerwony = dodatnie zasilanie, niebieski = "
                                  "ujemne zasilanie, szary = masa, zielony = sygnał, "
                                  "żółty = wybrany port do połączenia. Linie zielone oznaczają "
                                  "poprawne połączenie, a czerwone niezgodne typy portów.",
                                  right);
  graph_legend->setWordWrap(true);
  right_layout->addWidget(graph_legend);
  right_layout->addWidget(canvas_toolbar);
  right_layout->addWidget(view);
  right_layout->addWidget(bottom_tabs);

  auto *splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setChildrenCollapsible(false);
  splitter->setHandleWidth(8);
  splitter->addWidget(left);
  splitter->addWidget(right);
  splitter->setStretchFactor(0, 2);
  splitter->setStretchFactor(1, 4);
  splitter->setSizes({420, 840});
  root->addWidget(splitter, 1);

  auto active_block = [this]() -> Block * {
    if (!impl_->active_block_id.has_value()) {
      return nullptr;
    }
    return find_block(impl_->blocks, *impl_->active_block_id);
  };

  auto refresh_connection_form = [this]() {
    auto *conn_from_block = impl_->conn_from_block;
    auto *conn_to_block = impl_->conn_to_block;
    if (!conn_from_block || !conn_to_block) {
      return;
    }
    conn_from_block->clear();
    conn_to_block->clear();
    for (const auto &b : impl_->blocks) {
      conn_from_block->addItem(QString("#%1 %2").arg(b.id).arg(b.title), b.id);
      conn_to_block->addItem(QString("#%1 %2").arg(b.id).arg(b.title), b.id);
    }
    if (conn_from_block->count() > 0 && conn_from_block->currentIndex() < 0) {
      conn_from_block->setCurrentIndex(0);
    }
    if (conn_to_block->count() > 0 && conn_to_block->currentIndex() < 0) {
      conn_to_block->setCurrentIndex(0);
    }
  };

  auto refresh_ports_combo = [this](QComboBox *block_combo, QComboBox *port_combo) {
    port_combo->clear();
    const int id = block_combo->currentData().toInt();
    const Block *b = find_block(impl_->blocks, id);
    if (!b) {
      return;
    }
    for (const auto &p : ports_for(*b)) {
      port_combo->addItem(p.label + " (" + port_type_label(p.type) + ")", p.id);
    }
  };

  auto refresh_power_source_combo = [this]() {
    auto *combo = impl_->amp_power_source;
    if (!combo) {
      return;
    }
    const QVariant current = combo->currentData();
    combo->blockSignals(true);
    combo->clear();
    combo->addItem("— (brak)", 0);
    for (const auto &b : impl_->blocks) {
      if (b.kind != "power") {
        continue;
      }
      combo->addItem(QString("#%1 %2").arg(b.id).arg(b.title), b.id);
    }
    const int idx = combo->findData(current);
    combo->setCurrentIndex(idx >= 0 ? idx : 0);
    combo->blockSignals(false);
  };

  auto refresh_connections_ui = [this]() {
    auto *list = impl_->connections_list;
    if (!list) {
      return;
    }
    list->clear();
    for (const auto &c : impl_->connections) {
      const Block *ba = find_block(impl_->blocks, c.a.block_id);
      const Block *bb = find_block(impl_->blocks, c.b.block_id);
      if (!ba || !bb) {
        continue;
      }
      const auto pa = find_port(*ba, c.a.port_id);
      const auto pb = find_port(*bb, c.b.port_id);
      if (!pa.has_value() || !pb.has_value()) {
        continue;
      }
      list->addItem(endpoint_label(*ba, *pa) + "  ↔  " + endpoint_label(*bb, *pb));
    }
  };

  auto refresh_canvas = [this]() {
    auto *scene = impl_->scene;
    if (!scene) {
      return;
    }
    impl_->refreshing_canvas = true;
    impl_->signal_lines.clear();
    scene->clear();

    // Layout: connected components (signals + power association) go into rows.
    const int n = static_cast<int>(impl_->blocks.size());
    if (n == 0) {
      scene->setSceneRect(0, 0, 520.0, 160.0);
      impl_->refreshing_canvas = false;
      return;
    }

    auto is_power_type = [](PortType t) {
      return t == PortType::PowerPos || t == PortType::PowerNeg || t == PortType::Ground;
    };

    auto port_center_for = [this, is_power_type](int block_id,
                                                 const QString &port_id) -> std::optional<QPointF> {
      const Block *b = find_block(impl_->blocks, block_id);
      if (!b) {
        return std::nullopt;
      }
      const auto ports = ports_for(*b);
      int pin_index = 0;
      for (const auto &p : ports) {
        if (p.id == port_id) {
          const double r = 6.0;
          const bool is_left =
              (p.type == PortType::AnalogIn) ||
              (p.type == PortType::Ground && b->kind != "power" /* consumer-side */);
          const double px = (is_left ? 6.0 : (240.0 - 12.0));
          const double py = 12.0 + pin_index * 14.0;
          return b->canvas_pos + QPointF(px + r, py + r);
        }
        ++pin_index;
      }
      return std::nullopt;
    };

    auto schedule_signal_lines_update = [this]() {
      if (impl_->signal_lines_update_scheduled) {
        return;
      }
      impl_->signal_lines_update_scheduled = true;
      QTimer::singleShot(0, this, [this]() {
        impl_->signal_lines_update_scheduled = false;
        if (!impl_->scene) {
          return;
        }

        auto is_power_type = [](PortType t) {
          return t == PortType::PowerPos || t == PortType::PowerNeg || t == PortType::Ground;
        };
        auto port_center_for =
            [this, is_power_type](int block_id, const QString &port_id) -> std::optional<QPointF> {
          const Block *b = find_block(impl_->blocks, block_id);
          if (!b) {
            return std::nullopt;
          }
          const auto ports = ports_for(*b);
          int pin_index = 0;
          for (const auto &p : ports) {
            if (p.id == port_id) {
              const double r = 6.0;
              const bool is_left = (p.type == PortType::AnalogIn) ||
                                   (p.type == PortType::Ground && b->kind != "power");
              const double px = (is_left ? 6.0 : (240.0 - 12.0));
              const double py = 12.0 + pin_index * 14.0;
              return b->canvas_pos + QPointF(px + r, py + r);
            }
            ++pin_index;
          }
          return std::nullopt;
        };

        for (auto &sl : impl_->signal_lines) {
          if (!sl.item) {
            continue;
          }
          if (sl.item->scene() != impl_->scene) {
            continue;
          }
          const auto a = port_center_for(sl.a_block_id, sl.a_port_id);
          const auto b = port_center_for(sl.b_block_id, sl.b_port_id);
          if (!a.has_value() || !b.has_value()) {
            continue;
          }
          sl.item->setLine(QLineF(*a, *b));
        }
      });
    };

    if (!impl_->manual_layout) {
      std::unordered_map<int, int> id_to_idx;
      id_to_idx.reserve(impl_->blocks.size());
      for (int i = 0; i < n; ++i) {
        id_to_idx.emplace(impl_->blocks[i].id, i);
      }

      std::vector<std::vector<int>> adj(n);
      auto add_edge = [&](int a_id, int b_id) {
        if (a_id == b_id) {
          return;
        }
        const auto ia = id_to_idx.find(a_id);
        const auto ib = id_to_idx.find(b_id);
        if (ia == id_to_idx.end() || ib == id_to_idx.end()) {
          return;
        }
        adj[ia->second].push_back(ib->second);
        adj[ib->second].push_back(ia->second);
      };

      // Edges from explicit connections.
      for (const auto &c : impl_->connections) {
        const Block *ba = find_block(impl_->blocks, c.a.block_id);
        const Block *bb = find_block(impl_->blocks, c.b.block_id);
        if (!ba || !bb) {
          continue;
        }
        const auto pa = find_port(*ba, c.a.port_id);
        const auto pb = find_port(*bb, c.b.port_id);
        if (!pa.has_value() || !pb.has_value()) {
          continue;
        }
        if (is_power_type(pa->type) && is_power_type(pb->type)) {
          // Power association groups blocks, but we don't draw wires.
          add_edge(ba->id, bb->id);
        } else if (pa->type == PortType::AnalogIn || pa->type == PortType::AnalogOut ||
                   pb->type == PortType::AnalogIn || pb->type == PortType::AnalogOut) {
          add_edge(ba->id, bb->id);
        }
      }

      std::vector<int> comp(n, -1);
      int comp_count = 0;
      for (int i = 0; i < n; ++i) {
        if (comp[i] != -1) {
          continue;
        }
        std::vector<int> stack = {i};
        comp[i] = comp_count;
        while (!stack.empty()) {
          int v = stack.back();
          stack.pop_back();
          for (int u : adj[v]) {
            if (comp[u] == -1) {
              comp[u] = comp_count;
              stack.push_back(u);
            }
          }
        }
        ++comp_count;
      }

      std::vector<std::vector<int>> comps(comp_count);
      for (int i = 0; i < n; ++i) {
        comps[comp[i]].push_back(i);
      }

      // Keep insertion order within each component.
      const int cols = 4;
      const double margin_x = 30.0;
      const double margin_y = 30.0;
      const double spacing_x = 260.0;
      const double spacing_y = 130.0;
      const double comp_gap_y = 30.0;

      double y_cursor = margin_y;
      for (const auto &indices : comps) {
        int row = 0;
        int col = 0;
        for (int idx : indices) {
          impl_->blocks[idx].canvas_pos =
              QPointF(margin_x + col * spacing_x, y_cursor + row * spacing_y);
          ++col;
          if (col >= cols) {
            col = 0;
            ++row;
          }
        }
        const int rows_used = (static_cast<int>(indices.size()) + cols - 1) / cols;
        y_cursor += rows_used * spacing_y + comp_gap_y;
      }
    }

    BlockItem *to_select = nullptr;
    for (const auto &b : impl_->blocks) {
      // Power usage summary inside the block (instead of dashed lines).
      QStringList used_psu;
      for (const auto &c : impl_->connections) {
        const Endpoint a = c.a;
        const Endpoint d = c.b;
        const bool a_is_this = (a.block_id == b.id);
        const bool b_is_this = (d.block_id == b.id);
        if (!a_is_this && !b_is_this) {
          continue;
        }
        const Endpoint other = a_is_this ? d : a;
        const Block *ob = find_block(impl_->blocks, other.block_id);
        if (!ob || ob->kind != "power") {
          continue;
        }

        const auto bp = find_port(b, a_is_this ? a.port_id : d.port_id);
        if (!bp.has_value()) {
          continue;
        }
        if (!is_power_type(bp->type)) {
          continue;
        }

        const QString label = QString("#%1").arg(ob->id);
        if (!used_psu.contains(label)) {
          used_psu.push_back(label);
        }
      }

      QString pwr_line;
      if (b.kind != "power") {
        pwr_line = used_psu.isEmpty() ? "Zasilanie: —" : ("Zasilanie: " + used_psu.join(", "));
      } else {
        QStringList outs;
        for (const auto &p : ports_for(b)) {
          if (p.type == PortType::Ground) {
            continue;
          }
          outs.push_back(p.label);
        }
        pwr_line = "Wyjścia: " + (outs.isEmpty() ? "—" : outs.join(", "));
      }

      const QString text = QString("#%1\n%2\n%3").arg(b.id).arg(b.title).arg(pwr_line);
      auto *item = new BlockItem(b.id, text,
                                 [this, schedule_signal_lines_update](int id, const QPointF &pos) {
                                   if (impl_->refreshing_canvas) {
                                     return;
                                   }
                                   Block *b = find_block(impl_->blocks, id);
                                   if (!b) {
                                     return;
                                   }
                                   b->canvas_pos = pos;
                                   impl_->manual_layout = true;
                                   schedule_signal_lines_update();
                                 });
      item->setPos(b.canvas_pos);
      scene->addItem(item);
      if (impl_->active_block_id.has_value() && b.id == *impl_->active_block_id) {
        to_select = item;
      }

      // Port "pins" to support click-to-connect.
      const auto ports = ports_for(b);
      int pin_index = 0;
      for (const auto &p : ports) {
        const double r = 6.0;
        const bool is_left = (p.type == PortType::AnalogIn) ||
                             (p.type == PortType::Ground && b.kind != "power" /* consumer-side */);
        const double px = is_left ? 6.0 : (240.0 - 12.0);
        const double py = 12.0 + pin_index * 14.0;
        QColor fill(230, 230, 230);
        if (p.type == PortType::PowerPos) {
          fill = QColor(255, 210, 210);
        }
        if (p.type == PortType::PowerNeg) {
          fill = QColor(210, 220, 255);
        }
        if (p.type == PortType::Ground) {
          fill = QColor(220, 220, 220);
        }
        if (p.type == PortType::AnalogIn || p.type == PortType::AnalogOut) {
          fill = QColor(210, 245, 210);
        }
        if (impl_->pending_connection.has_value() && impl_->pending_connection->block_id == b.id &&
            impl_->pending_connection->port_id == p.id) {
          fill = QColor(255, 245, 180);
        }

        auto *pin = new QGraphicsEllipseItem(item);
        pin->setRect(px, py, r * 2, r * 2);
        pin->setPen(QPen(QColor(80, 80, 80)));
        pin->setBrush(QBrush(fill));
        pin->setData(1, b.id);
        pin->setData(2, p.id);
        pin->setToolTip(p.label + " (" + port_type_label(p.type) + ")");
        pin->setZValue(5);
        ++pin_index;
      }
    }

    // Draw connection lines between pins (signals only).
    for (const auto &c : impl_->connections) {
      const Block *ba = find_block(impl_->blocks, c.a.block_id);
      const Block *bb = find_block(impl_->blocks, c.b.block_id);
      if (!ba || !bb) {
        continue;
      }
      const auto pa = find_port(*ba, c.a.port_id);
      const auto pb = find_port(*bb, c.b.port_id);
      if (!pa.has_value() || !pb.has_value()) {
        continue;
      }

      const bool a_power = (pa->type == PortType::PowerPos || pa->type == PortType::PowerNeg ||
                            pa->type == PortType::Ground);
      const bool b_power = (pb->type == PortType::PowerPos || pb->type == PortType::PowerNeg ||
                            pb->type == PortType::Ground);
      if (a_power && b_power) {
        continue; // don't draw power wires on the graph
      }

      const auto a = port_center_for(c.a.block_id, c.a.port_id);
      const auto bpt = port_center_for(c.b.block_id, c.b.port_id);
      if (!a.has_value() || !bpt.has_value()) {
        continue;
      }

      QColor color(0, 140, 70);
      if (!ports_compatible(pa->type, pb->type)) {
        color = QColor(200, 20, 20);
      }

      QPen pen(color, 2.0);
      auto *line = scene->addLine(QLineF(*a, *bpt), pen);
      line->setZValue(-10);
      line->setAcceptedMouseButtons(Qt::NoButton);
      impl_->signal_lines.push_back(
          Impl::SignalLine{c.a.block_id, c.a.port_id, c.b.block_id, c.b.port_id, line});
    }

    if (to_select) {
      to_select->setSelected(true);
    }
    int max_pins = 0;
    for (const auto &b : impl_->blocks) {
      max_pins = std::max<int>(max_pins, static_cast<int>(ports_for(b).size()));
    }
    const double h = std::max(140.0, 40.0 + max_pins * 16.0);
    // Scene rect based on placed blocks.
    double max_x = 520.0;
    double max_y = 160.0;
    for (const auto &b : impl_->blocks) {
      max_x = std::max(max_x, b.canvas_pos.x() + 260.0);
      max_y = std::max(max_y, b.canvas_pos.y() + h + 60.0);
    }
    scene->setSceneRect(0, 0, max_x + 20.0, max_y);
    impl_->refreshing_canvas = false;
  };

  auto sync_active_to_form = [this, active_block, refresh_power_source_combo]() {
    auto *props_stack = impl_->props_stack;
    auto *variant = impl_->variant;
    auto *vin_input = impl_->vin_input;
    auto *freq_input = impl_->freq_input;
    auto *current_input = impl_->current_input;
    auto *cap_input = impl_->cap_input;
    auto *extra_rails_input = impl_->extra_rails_input;
    auto *amp_power_source = impl_->amp_power_source;
    auto *amp_waveform = impl_->amp_waveform;
    auto *amp_amp_input = impl_->amp_amp_input;
    auto *amp_freq_input = impl_->amp_freq_input;
    auto *amp_gain_input = impl_->amp_gain_input;
    if (!props_stack || !variant || !vin_input || !freq_input || !current_input || !cap_input ||
        !amp_power_source || !extra_rails_input || !amp_waveform || !amp_amp_input ||
        !amp_freq_input || !amp_gain_input) {
      return;
    }

    const Block *active = active_block();
    if (!active) {
      return;
    }

    struct SyncGuard {
      bool *flag = nullptr;
      explicit SyncGuard(bool *f) : flag(f) {
        if (flag) {
          *flag = true;
        }
      }
      ~SyncGuard() {
        if (flag) {
          *flag = false;
        }
      }
    } guard(&impl_->syncing_active_to_form);

    if (active->kind == "power") {
      props_stack->setCurrentIndex(0);
      const int idx = variant->findData(active->variant);
      if (idx >= 0) {
        variant->setCurrentIndex(idx);
      }
      vin_input->setText(active->vin_ac_rms == 0.0 ? "" : QString::number(active->vin_ac_rms));
      freq_input->setText(active->mains_hz == 0.0 ? "" : QString::number(active->mains_hz));
      current_input->setText(active->load_current == 0.0 ? ""
                                                         : QString::number(active->load_current));
      cap_input->setText(active->capacitor_uF == 0.0 ? "" : QString::number(active->capacitor_uF));
      extra_rails_input->setText(active->extra_pos_rails_csv);
    } else {
      props_stack->setCurrentIndex(1);
      refresh_power_source_combo();

      // Guess current PSU selection from existing power-rail connections (if any).
      int selected_psu = 0;
      for (const auto &c : impl_->connections) {
        const Endpoint a = c.a;
        const Endpoint b = c.b;
        const bool a_is_active = (a.block_id == active->id);
        const bool b_is_active = (b.block_id == active->id);
        if (!a_is_active && !b_is_active) {
          continue;
        }
        const Endpoint other = a_is_active ? b : a;
        const Block *ob = find_block(impl_->blocks, other.block_id);
        if (!ob || ob->kind != "power") {
          continue;
        }
        const Block *ab = find_block(impl_->blocks, active->id);
        if (!ab) {
          continue;
        }
        const auto ap = find_port(*ab, a_is_active ? a.port_id : b.port_id);
        if (!ap.has_value()) {
          continue;
        }
        if (ap->type != PortType::PowerPos && ap->type != PortType::PowerNeg &&
            ap->type != PortType::Ground) {
          continue;
        }
        selected_psu = ob->id;
        break;
      }
      const int pidx = amp_power_source->findData(selected_psu);
      {
        const QSignalBlocker blocker(amp_power_source);
        amp_power_source->setCurrentIndex(pidx >= 0 ? pidx : 0);
      }

      const int widx = amp_waveform->findData(active->signal_waveform);
      if (widx >= 0) {
        amp_waveform->setCurrentIndex(widx);
      }
      amp_amp_input->setText(active->signal_amp_v == 0.0 ? ""
                                                         : QString::number(active->signal_amp_v));
      amp_freq_input->setText(active->signal_hz == 0.0 ? "" : QString::number(active->signal_hz));
      amp_gain_input->setText(active->gain == 0.0 ? "" : QString::number(active->gain));
    }
  };

  auto sync_form_to_active = [this, active_block]() {
    auto *variant = impl_->variant;
    auto *vin_input = impl_->vin_input;
    auto *freq_input = impl_->freq_input;
    auto *current_input = impl_->current_input;
    auto *cap_input = impl_->cap_input;
    auto *extra_rails_input = impl_->extra_rails_input;
    auto *amp_waveform = impl_->amp_waveform;
    auto *amp_amp_input = impl_->amp_amp_input;
    auto *amp_freq_input = impl_->amp_freq_input;
    auto *amp_gain_input = impl_->amp_gain_input;
    if (!variant || !vin_input || !freq_input || !current_input || !cap_input || !amp_waveform ||
        !amp_amp_input || !amp_freq_input || !amp_gain_input || !extra_rails_input) {
      return;
    }

    Block *active = active_block();
    if (!active) {
      return;
    }
    if (active->kind == "power") {
      active->variant = variant->currentData().toString();
      active->vin_ac_rms = vin_input->text().toDouble();
      active->mains_hz = read_or(freq_input, 50.0);
      active->load_current = read_or(current_input, 0.0);
      active->capacitor_uF = read_or(cap_input, 0.0);
      active->extra_pos_rails_csv = extra_rails_input->text();
      return;
    }

    active->variant = "model1b";
    active->signal_waveform = amp_waveform->currentData().toString();
    active->signal_amp_v = amp_amp_input->text().toDouble();
    active->signal_hz = read_or(amp_freq_input, 1000.0);
    active->gain = read_or(amp_gain_input, 10.0);
  };

  auto recompute_and_validate = [this, active_block]() {
    auto *active_title = impl_->active_title;
    auto *compute_result = impl_->compute_result;
    auto *validation = impl_->validation;
    auto *ports_label = impl_->ports_label;
    auto *w_in = impl_->waveform_in;
    auto *w_out = impl_->waveform_out;
    auto *bottom_tabs = impl_->bottom_tabs;
    if (!active_title || !compute_result || !validation || !ports_label || !w_in || !w_out) {
      return;
    }

    Block *active = active_block();
    if (!active) {
      active_title->setText("Aktywny element: —");
      compute_result->setText("Wyniki pojawią się tutaj");
      validation->setText("Walidacja: —");
      ports_label->setText("");
      return;
    }

    active_title->setText(QString("Aktywny element: #%1 %2").arg(active->id).arg(active->title));

    QString ports_text = "Porty:\n";
    for (const auto &p : ports_for(*active)) {
      ports_text += "- " + p.label + " (" + port_type_label(p.type) + ")\n";
    }
    ports_label->setText(ports_text.trimmed());

    if (active->kind == "power") {
      pep::modules::psu_basic::Input input;
      input.vin_ac_rms = active->vin_ac_rms;
      input.mains_hz = active->mains_hz;
      input.load_current = active->load_current;
      input.capacitor_uF = active->capacitor_uF;
      input.diode_drop = 0.9;
      input.rectifier = pep::modules::psu_basic::RectifierType::FullWaveBridge;

      const auto outp = pep::modules::psu_basic::compute(input);
      compute_result->setText(QString("Podgląd zasilania:\n"
                                      "Szczytowe napięcie po transformatorze (Vpeak) = %1 V\n"
                                      "Napięcie po prostowaniu (Vrect) = %2 V\n"
                                      "Średnie napięcie DC pod obciążeniem (Vdc) = %3 V\n"
                                      "Tętnienia szczyt‑szczyt (Ripple Vpp) = %4 V\n\n"
                                      "Wzory (uproszczone):\n"
                                      "Vpeak ≈ Vin_rms · √2\n"
                                      "Vrect ≈ Vpeak − 2·Vd\n"
                                      "Ripple ≈ Iload / (f · C)\n")
                                  .arg(outp.vpeak, 0, 'f', 2)
                                  .arg(outp.vrect, 0, 'f', 2)
                                  .arg(outp.vdc_loaded, 0, 'f', 2)
                                  .arg(outp.ripple_vpp, 0, 'f', 3));

      pep::modules::psu_basic::WaveformParams in;
      in.vpeak = outp.vpeak;
      in.vrect = outp.vrect;
      in.ripple_vpp = outp.ripple_vpp;
      in.mains_hz = input.mains_hz;
      in.type = pep::modules::psu_basic::WaveformType::Sinus;
      w_in->set_params(in);

      pep::modules::psu_basic::WaveformParams out = in;
      out.type = pep::modules::psu_basic::WaveformType::Filtered;
      w_out->set_params(out);
    } else {
      const double amp = (active->signal_amp_v > 0.0 ? active->signal_amp_v : 1.0);
      const double hz = (active->signal_hz > 0.0 ? active->signal_hz : 1000.0);
      const double gain = (active->gain > 0.0 ? active->gain : 1.0);

      compute_result->setText(QString("Wzmacniacz model 1(B):\n"
                                      "Amplituda wejściowa = %1 V\n"
                                      "Częstotliwość = %2 Hz\n"
                                      "Wzmocnienie napięciowe = %3 V/V\n"
                                      "Amplituda wyjściowa ≈ Vin · Av = %4 V\n")
                                  .arg(amp, 0, 'f', 3)
                                  .arg(hz, 0, 'f', 1)
                                  .arg(gain, 0, 'f', 2)
                                  .arg(amp * gain, 0, 'f', 3));

      pep::modules::psu_basic::WaveformParams in;
      in.vpeak = amp;
      in.vrect = amp;
      in.ripple_vpp = 0.0;
      in.mains_hz = hz;
      if (active->signal_waveform == "square") {
        in.type = pep::modules::psu_basic::WaveformType::Square;
      } else if (active->signal_waveform == "triangle") {
        in.type = pep::modules::psu_basic::WaveformType::Triangle;
      } else {
        in.type = pep::modules::psu_basic::WaveformType::Sinus;
      }
      w_in->set_params(in);

      pep::modules::psu_basic::WaveformParams out = in;
      out.vpeak = amp * gain;
      out.vrect = amp * gain;
      w_out->set_params(out);
    }

    std::vector<QString> warnings;
    for (const auto &c : impl_->connections) {
      const Block *ba = find_block(impl_->blocks, c.a.block_id);
      const Block *bb = find_block(impl_->blocks, c.b.block_id);
      if (!ba || !bb) {
        continue;
      }
      const auto pa = find_port(*ba, c.a.port_id);
      const auto pb = find_port(*bb, c.b.port_id);
      if (!pa.has_value() || !pb.has_value()) {
        continue;
      }
      if (!ports_compatible(pa->type, pb->type)) {
        warnings.push_back("Niekompatybilne typy portów: " + endpoint_label(*ba, *pa) + " ↔ " +
                           endpoint_label(*bb, *pb));
      }
    }
    if (warnings.empty()) {
      validation->setText("Walidacja: OK (miękka walidacja – nie blokuje eksportu).");
    } else {
      QString text = "Walidacja (ostrzeżenia):\n";
      for (const auto &w : warnings) {
        text += "- " + w + "\n";
      }
      validation->setText(text.trimmed());
    }

    if (bottom_tabs && impl_->waveform_tab_index >= 0) {
      bottom_tabs->setCurrentIndex(impl_->waveform_tab_index);
    }
  };

  auto set_active = [this, sync_active_to_form, recompute_and_validate](int id) {
    impl_->active_block_id = id;
    sync_active_to_form();
    recompute_and_validate();
  };

  auto connection_exists = [this](const Endpoint &a, const Endpoint &b) -> bool {
    for (const auto &c : impl_->connections) {
      const bool ab = (c.a.block_id == a.block_id && c.a.port_id == a.port_id &&
                       c.b.block_id == b.block_id && c.b.port_id == b.port_id);
      const bool ba = (c.a.block_id == b.block_id && c.a.port_id == b.port_id &&
                       c.b.block_id == a.block_id && c.b.port_id == a.port_id);
      if (ab || ba) {
        return true;
      }
    }
    return false;
  };

  auto try_add_connection = [this, connection_exists](const Endpoint &a,
                                                      const Endpoint &b) -> bool {
    if (a.block_id == b.block_id && a.port_id == b.port_id) {
      return false;
    }
    if (connection_exists(a, b)) {
      return false;
    }
    impl_->connections.push_back(Connection{a, b});
    return true;
  };

  auto unique_port = [](const std::vector<PortDef> &ports,
                        PortType type) -> std::optional<PortDef> {
    std::optional<PortDef> found;
    for (const auto &p : ports) {
      if (p.type != type) {
        continue;
      }
      if (found.has_value()) {
        return std::nullopt;
      }
      found = p;
    }
    return found;
  };

  auto auto_connect_blocks = [this, unique_port, try_add_connection](int from_id, int to_id) {
    const Block *from = find_block(impl_->blocks, from_id);
    const Block *to = find_block(impl_->blocks, to_id);
    if (!from || !to) {
      return;
    }

    const auto from_ports = ports_for(*from);
    const auto to_ports = ports_for(*to);

    // Auto-map only power rails between PSU <-> consumer blocks.
    // Never auto-connect PSU <-> PSU (would short supplies) and never auto-connect signal between
    // consumers.
    const bool from_is_psu = (from->kind == "power");
    const bool to_is_psu = (to->kind == "power");
    if (from_is_psu == to_is_psu) {
      return;
    }

    for (const PortType t : {PortType::PowerPos, PortType::PowerNeg, PortType::Ground}) {
      const auto a = unique_port(from_ports, t);
      const auto b = unique_port(to_ports, t);
      if (a.has_value() && b.has_value()) {
        try_add_connection(Endpoint{from_id, a->id}, Endpoint{to_id, b->id});
      }
    }
  };

  // Canvas actions: add/remove/select from schematic.
  view->on_add_power = [this, refresh_canvas, refresh_connection_form, refresh_ports_combo,
                        set_active, auto_connect_blocks, recompute_and_validate,
                        refresh_connections_ui, refresh_power_source_combo]() {
    const int id = impl_->next_block_id++;
    const std::optional<int> prev_active = impl_->active_block_id;
    impl_->blocks.push_back(
        make_power_block(id, QString("Zasilanie #%1").arg(id), "psu_symmetric"));

    // If user started a connection from a pin, attach the new block automatically.
    if (impl_->pending_connection.has_value()) {
      const Endpoint start = *impl_->pending_connection;
      impl_->pending_connection.reset();
      const Block *start_block = find_block(impl_->blocks, start.block_id);
      const Block *new_block = find_block(impl_->blocks, id);
      if (start_block && new_block) {
        const auto sp = find_port(*start_block, start.port_id);
        const PortType st = sp.has_value() ? sp->type : PortType::Unknown;
        QString best_port;
        const auto ports = ports_for(*new_block);
        for (const auto &p : ports) {
          if (ports_compatible(st, p.type)) {
            best_port = p.id;
            break;
          }
        }
        if (best_port.isEmpty() && !ports.empty()) {
          best_port = ports.front().id;
        }
        if (!best_port.isEmpty()) {
          impl_->connections.push_back(Connection{start, Endpoint{id, best_port}});
        }
      }
    } else if (prev_active.has_value()) {
      auto_connect_blocks(*prev_active, id);
    }

    refresh_connection_form();
    refresh_power_source_combo();
    refresh_ports_combo(impl_->conn_from_block, impl_->conn_from_port);
    refresh_ports_combo(impl_->conn_to_block, impl_->conn_to_port);
    set_active(id);
    refresh_connections_ui();
    recompute_and_validate();
    refresh_canvas();
  };
  view->on_add_amp_model1b = [this, refresh_canvas, refresh_connection_form, refresh_ports_combo,
                              set_active, auto_connect_blocks, recompute_and_validate,
                              refresh_connections_ui, refresh_power_source_combo]() {
    const int id = impl_->next_block_id++;
    const std::optional<int> prev_active = impl_->active_block_id;
    impl_->blocks.push_back(make_amp_model1b_block(id, QString("Wzmacniacz 1(B) #%1").arg(id)));

    if (impl_->pending_connection.has_value()) {
      const Endpoint start = *impl_->pending_connection;
      impl_->pending_connection.reset();
      const Block *start_block = find_block(impl_->blocks, start.block_id);
      const Block *new_block = find_block(impl_->blocks, id);
      if (start_block && new_block) {
        const auto sp = find_port(*start_block, start.port_id);
        const PortType st = sp.has_value() ? sp->type : PortType::Unknown;
        QString best_port;
        const auto ports = ports_for(*new_block);
        for (const auto &p : ports) {
          if (ports_compatible(st, p.type)) {
            best_port = p.id;
            break;
          }
        }
        if (best_port.isEmpty() && !ports.empty()) {
          best_port = ports.front().id;
        }
        if (!best_port.isEmpty()) {
          impl_->connections.push_back(Connection{start, Endpoint{id, best_port}});
        }
      }
    } else if (prev_active.has_value()) {
      auto_connect_blocks(*prev_active, id);
    }

    refresh_connection_form();
    refresh_power_source_combo();
    refresh_ports_combo(impl_->conn_from_block, impl_->conn_from_port);
    refresh_ports_combo(impl_->conn_to_block, impl_->conn_to_port);
    set_active(id);
    refresh_connections_ui();
    recompute_and_validate();
    refresh_canvas();
  };
  view->on_delete_selected = [this, refresh_connections_ui, refresh_connection_form,
                              refresh_ports_combo, set_active, refresh_canvas,
                              recompute_and_validate, refresh_power_source_combo]() {
    auto *scene = impl_->scene;
    if (!scene) {
      return;
    }
    const auto sel = scene->selectedItems();
    if (sel.empty()) {
      return;
    }
    auto *item = dynamic_cast<BlockItem *>(sel.front());
    if (!item) {
      return;
    }
    const int id = item->block_id();

    impl_->blocks.erase(std::remove_if(impl_->blocks.begin(), impl_->blocks.end(),
                                       [&](const Block &b) { return b.id == id; }),
                        impl_->blocks.end());
    impl_->connections.erase(std::remove_if(impl_->connections.begin(), impl_->connections.end(),
                                            [&](const Connection &c) {
                                              return c.a.block_id == id || c.b.block_id == id;
                                            }),
                             impl_->connections.end());

    refresh_connections_ui();
    refresh_connection_form();
    refresh_power_source_combo();
    refresh_ports_combo(impl_->conn_from_block, impl_->conn_from_port);
    refresh_ports_combo(impl_->conn_to_block, impl_->conn_to_port);

    if (impl_->blocks.empty()) {
      impl_->active_block_id.reset();
      refresh_canvas();
      recompute_and_validate();
      return;
    }
    const int new_active = impl_->blocks.front().id;
    set_active(new_active);
    refresh_canvas();
  };

  QObject::connect(scene, &QGraphicsScene::selectionChanged, this, [this, set_active]() {
    if (impl_->refreshing_canvas) {
      return;
    }
    const auto sel = impl_->scene->selectedItems();
    if (sel.empty()) {
      return;
    }
    auto *item = dynamic_cast<BlockItem *>(sel.front());
    if (!item) {
      return;
    }
    set_active(item->block_id());
  });

  // Toolbar buttons (not only RMB).
  QObject::connect(btn_add_power, &QPushButton::clicked, this, [view]() {
    if (view->on_add_power) {
      view->on_add_power();
    }
  });
  QObject::connect(btn_add_amp, &QPushButton::clicked, this, [view]() {
    if (view->on_add_amp_model1b) {
      view->on_add_amp_model1b();
    }
  });
  QObject::connect(btn_auto_layout, &QPushButton::clicked, this, [this, refresh_canvas]() {
    impl_->manual_layout = false;
    refresh_canvas();
  });

  // Click-to-connect via port pins.
  view->on_port_clicked = [this, refresh_connections_ui, recompute_and_validate, refresh_canvas,
                           try_add_connection](int block_id, const QString &port_id) {
    Endpoint ep{block_id, port_id};
    if (!impl_->pending_connection.has_value()) {
      impl_->pending_connection = ep;
      if (impl_->validation) {
        impl_->validation->setText(
            "Połączenia: wybierz drugi port (albo dodaj nowy element), aby połączyć.");
      }
      refresh_canvas();
      return;
    }

    const Endpoint start = *impl_->pending_connection;
    impl_->pending_connection.reset();
    if (start.block_id == ep.block_id && start.port_id == ep.port_id) {
      return;
    }
    try_add_connection(start, ep);
    refresh_connections_ui();
    recompute_and_validate();
    refresh_canvas();
  };

  QObject::connect(variant, &QComboBox::currentIndexChanged, this, [=]() {
    sync_form_to_active();
    refresh_connection_form();
    refresh_ports_combo(conn_from_block, conn_from_port);
    refresh_ports_combo(conn_to_block, conn_to_port);
    refresh_connections_ui();
    recompute_and_validate();
  });
  QObject::connect(vin_input, &QLineEdit::textChanged, this, [=]() {
    sync_form_to_active();
    recompute_and_validate();
  });
  QObject::connect(freq_input, &QLineEdit::textChanged, this, [=]() {
    sync_form_to_active();
    recompute_and_validate();
  });
  QObject::connect(current_input, &QLineEdit::textChanged, this, [=]() {
    sync_form_to_active();
    recompute_and_validate();
  });
  QObject::connect(cap_input, &QLineEdit::textChanged, this, [=]() {
    sync_form_to_active();
    recompute_and_validate();
  });
  QObject::connect(impl_->extra_rails_input, &QLineEdit::textChanged, this, [=, this]() {
    sync_form_to_active();
    refresh_connection_form();
    refresh_ports_combo(conn_from_block, conn_from_port);
    refresh_ports_combo(conn_to_block, conn_to_port);
    refresh_connections_ui();
    recompute_and_validate();
    refresh_canvas();
  });
  QObject::connect(impl_->amp_power_source, &QComboBox::currentIndexChanged, this, [=, this]() {
    if (impl_->refreshing_canvas) {
      return;
    }
    if (impl_->syncing_active_to_form) {
      return;
    }
    Block *active = active_block();
    if (!active || active->kind == "power") {
      return;
    }

    const int psu_id = impl_->amp_power_source->currentData().toInt();

    auto is_power_endpoint = [this](const Endpoint &e) -> bool {
      const Block *b = find_block(impl_->blocks, e.block_id);
      if (!b) {
        return false;
      }
      const auto p = find_port(*b, e.port_id);
      if (!p.has_value()) {
        return false;
      }
      return (p->type == PortType::PowerPos || p->type == PortType::PowerNeg ||
              p->type == PortType::Ground);
    };

    // Remove existing power-rail connections from this block.
    impl_->connections.erase(
        std::remove_if(impl_->connections.begin(), impl_->connections.end(),
                       [&](const Connection &c) {
                         if (c.a.block_id != active->id && c.b.block_id != active->id) {
                           return false;
                         }
                         return is_power_endpoint(c.a) || is_power_endpoint(c.b);
                       }),
        impl_->connections.end());

    if (psu_id != 0) {
      const Block *psu = find_block(impl_->blocks, psu_id);
      if (psu) {
        // Re-create Vcc/Vee/0 mapping in one click.
        const auto ap = ports_for(*active);
        const auto pp = ports_for(*psu);
        auto pick = [](const std::vector<PortDef> &ports, PortType t) -> QString {
          for (const auto &p : ports) {
            if (p.type == t) {
              return p.id;
            }
          }
          return "";
        };
        const QString avcc = pick(ap, PortType::PowerPos);
        const QString avee = pick(ap, PortType::PowerNeg);
        const QString agnd = pick(ap, PortType::Ground);
        const QString pvcc = pick(pp, PortType::PowerPos);
        const QString pvee = pick(pp, PortType::PowerNeg);
        const QString pgnd = pick(pp, PortType::Ground);

        if (!avcc.isEmpty() && !pvcc.isEmpty()) {
          try_add_connection(Endpoint{psu_id, pvcc}, Endpoint{active->id, avcc});
        }
        if (!avee.isEmpty() && !pvee.isEmpty()) {
          try_add_connection(Endpoint{psu_id, pvee}, Endpoint{active->id, avee});
        }
        if (!agnd.isEmpty() && !pgnd.isEmpty()) {
          try_add_connection(Endpoint{psu_id, pgnd}, Endpoint{active->id, agnd});
        }
      }
    }

    refresh_connections_ui();
    refresh_connection_form();
    refresh_ports_combo(conn_from_block, conn_from_port);
    refresh_ports_combo(conn_to_block, conn_to_port);
    recompute_and_validate();
    refresh_canvas();
  });
  QObject::connect(impl_->amp_waveform, &QComboBox::currentIndexChanged, this, [=, this]() {
    sync_form_to_active();
    recompute_and_validate();
  });
  QObject::connect(impl_->amp_amp_input, &QLineEdit::textChanged, this, [=, this]() {
    sync_form_to_active();
    recompute_and_validate();
  });
  QObject::connect(impl_->amp_freq_input, &QLineEdit::textChanged, this, [=, this]() {
    sync_form_to_active();
    recompute_and_validate();
  });
  QObject::connect(impl_->amp_gain_input, &QLineEdit::textChanged, this, [=, this]() {
    sync_form_to_active();
    recompute_and_validate();
  });

  QObject::connect(conn_from_block, &QComboBox::currentIndexChanged, this,
                   [=]() { refresh_ports_combo(conn_from_block, conn_from_port); });
  QObject::connect(conn_to_block, &QComboBox::currentIndexChanged, this,
                   [=]() { refresh_ports_combo(conn_to_block, conn_to_port); });

  QObject::connect(add_conn, &QPushButton::clicked, this, [=, this]() {
    if (conn_from_block->count() == 0 || conn_to_block->count() == 0) {
      return;
    }
    const int a_id = conn_from_block->currentData().toInt();
    const int b_id = conn_to_block->currentData().toInt();
    const QString a_port = conn_from_port->currentData().toString();
    const QString b_port = conn_to_port->currentData().toString();
    if (a_id == b_id && a_port == b_port) {
      return;
    }

    const Block *ba = find_block(impl_->blocks, a_id);
    const Block *bb = find_block(impl_->blocks, b_id);
    if (!ba || !bb) {
      return;
    }
    const auto pa = find_port(*ba, a_port);
    const auto pb = find_port(*bb, b_port);
    if (!pa.has_value() || !pb.has_value()) {
      return;
    }

    impl_->connections.push_back(Connection{Endpoint{a_id, a_port}, Endpoint{b_id, b_port}});
    refresh_connections_ui();
    recompute_and_validate();
  });

  QObject::connect(remove_conn, &QPushButton::clicked, this, [=, this]() {
    auto *list = impl_->connections_list;
    if (!list) {
      return;
    }
    const int row = list->currentRow();
    if (row < 0 || row >= static_cast<int>(impl_->connections.size())) {
      return;
    }
    impl_->connections.erase(impl_->connections.begin() + row);
    refresh_connections_ui();
    recompute_and_validate();
  });

  QObject::connect(export_active, &QPushButton::clicked, this, [=, this]() {
    if (!impl_->active_block_id.has_value()) {
      return;
    }
    const int id = *impl_->active_block_id;
    const Block *b = find_block(impl_->blocks, id);
    if (!b) {
      return;
    }

    std::vector<Block> single_blocks = {*b};
    std::vector<Connection> single_conns;
    for (const auto &c : impl_->connections) {
      if (c.a.block_id == id || c.b.block_id == id) {
        single_conns.push_back(c);
      }
    }
    const bool has_amp = (b->kind == "amplifier");
    const auto tran = prompt_tran_directive(this, has_amp);
    const auto exp = export_project_asc(single_blocks, single_conns, tran);

    QString out_path = QFileDialog::getSaveFileName(this, "Zapisz schemat LTspice", QString(),
                                                    "Schematic (*.asc)");
    if (out_path.isEmpty()) {
      return;
    }
    if (!out_path.endsWith(".asc", Qt::CaseInsensitive)) {
      out_path += ".asc";
    }

    QFile out(out_path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      QMessageBox::warning(this, "Eksport LTspice", "Nie udało się zapisać:\n" + out_path);
      return;
    }
    out.write(exp.asc.c_str(), static_cast<int>(exp.asc.size()));

    QString msg = "Zapisano:\n" + out_path;
    if (!exp.warnings.empty()) {
      msg += "\n\nUwagi:\n";
      for (const auto &w : exp.warnings) {
        msg += "- " + QString::fromStdString(w) + "\n";
      }
    }
    QMessageBox::information(this, "Eksport LTspice", msg);
  });

  QObject::connect(export_project, &QPushButton::clicked, this, [=, this]() {
    bool has_amp = false;
    for (const auto &b : impl_->blocks) {
      if (b.kind == "amplifier") {
        has_amp = true;
        break;
      }
    }
    const auto tran = prompt_tran_directive(this, has_amp);
    const auto exp = export_project_asc(impl_->blocks, impl_->connections, tran);

    QString out_path = QFileDialog::getSaveFileName(this, "Zapisz schemat LTspice", QString(),
                                                    "Schematic (*.asc)");
    if (out_path.isEmpty()) {
      return;
    }
    if (!out_path.endsWith(".asc", Qt::CaseInsensitive)) {
      out_path += ".asc";
    }

    QFile out(out_path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      QMessageBox::warning(this, "Eksport LTspice", "Nie udało się zapisać:\n" + out_path);
      return;
    }
    out.write(exp.asc.c_str(), static_cast<int>(exp.asc.size()));

    QString msg = "Zapisano:\n" + out_path;
    if (!exp.warnings.empty()) {
      msg += "\n\nUwagi:\n";
      for (const auto &w : exp.warnings) {
        msg += "- " + QString::fromStdString(w) + "\n";
      }
    }
    QMessageBox::information(this, "Eksport LTspice", msg);
  });

  refresh_connection_form();
  refresh_ports_combo(conn_from_block, conn_from_port);
  refresh_ports_combo(conn_to_block, conn_to_port);
  refresh_connections_ui();
  refresh_canvas();
  sync_active_to_form();
  recompute_and_validate();
}

Widget::~Widget() = default;

} // namespace pep::modules::project_design
