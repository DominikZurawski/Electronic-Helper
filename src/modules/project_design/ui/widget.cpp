#include "pep/modules/project_design/ui/widget.hpp"

#include "../export/export.hpp"
#include "../graph/graph.hpp"
#include "../model/connection_ops.hpp"
#include "../model/model.hpp"
#include "active_block_view.hpp"
#include "canvas_scene.hpp"
#include "connection_ui.hpp"
#include "export_actions.hpp"
#include "form_sync.hpp"
#include "widget_actions.hpp"
#include "widget_bindings.hpp"

#include "../../psu_basic/ui/waveform_widget.hpp"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
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

namespace {} // namespace

struct Widget::Impl {
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
  std::vector<CanvasSignalLine> signal_lines;
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
      make_power_block(impl_->next_block_id++, "Zasilanie #1", BlockVariant::PsuSymmetric));
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
  variant->addItem("Zasilacz symetryczny", static_cast<int>(BlockVariant::PsuSymmetric));
  variant->addItem("Zasilacz niestabilizowany (brak schematu)",
                   static_cast<int>(BlockVariant::PsuUnregulated));

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
  amp_waveform->addItem("Sinus", static_cast<int>(SignalWaveform::Sine));
  amp_waveform->addItem("Prostokąt", static_cast<int>(SignalWaveform::Square));
  amp_waveform->addItem("Trójkąt", static_cast<int>(SignalWaveform::Triangle));
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
    pep::modules::project_design::refresh_connection_form(impl_->blocks, impl_->conn_from_block,
                                                          impl_->conn_to_block);
  };

  auto refresh_ports_combo = [this](QComboBox *block_combo, QComboBox *port_combo) {
    pep::modules::project_design::refresh_ports_combo(impl_->blocks, block_combo, port_combo);
  };

  auto refresh_power_source_combo = [this]() {
    pep::modules::project_design::refresh_power_source_combo(impl_->blocks,
                                                             impl_->amp_power_source);
  };

  auto refresh_connections_ui = [this]() {
    pep::modules::project_design::refresh_connections_ui(impl_->blocks, impl_->connections,
                                                         impl_->connections_list);
  };

  auto refresh_canvas = [this]() {
    pep::modules::project_design::refresh_canvas_scene(
        CanvasSceneState{impl_->scene, &impl_->signal_lines, &impl_->blocks, &impl_->connections,
                         impl_->active_block_id, impl_->pending_connection, impl_->manual_layout,
                         &impl_->refreshing_canvas, &impl_->signal_lines_update_scheduled, this,
                         [this](int, const QPointF &) { impl_->manual_layout = true; }});
  };

  auto sync_active_to_form = [this, active_block, refresh_power_source_combo]() {
    const FormWidgets widgets{
        impl_->props_stack,       impl_->variant,        impl_->vin_input,
        impl_->freq_input,        impl_->current_input,  impl_->cap_input,
        impl_->extra_rails_input, impl_->amp_waveform,   impl_->amp_power_source,
        impl_->amp_amp_input,     impl_->amp_freq_input, impl_->amp_gain_input};
    pep::modules::project_design::sync_active_to_form(
        active_block(), impl_->blocks, impl_->connections, widgets, &impl_->syncing_active_to_form,
        refresh_power_source_combo);
  };

  auto sync_form_to_active = [this, active_block]() {
    const FormWidgets widgets{
        impl_->props_stack,       impl_->variant,        impl_->vin_input,
        impl_->freq_input,        impl_->current_input,  impl_->cap_input,
        impl_->extra_rails_input, impl_->amp_waveform,   impl_->amp_power_source,
        impl_->amp_amp_input,     impl_->amp_freq_input, impl_->amp_gain_input};
    pep::modules::project_design::sync_form_to_active(active_block(), widgets);
  };

  auto recompute_and_validate = [this, active_block]() {
    const ActiveBlockWidgets widgets{
        impl_->active_title, impl_->compute_result, impl_->validation,  impl_->ports_label,
        impl_->waveform_in,  impl_->waveform_out,   impl_->bottom_tabs, impl_->waveform_tab_index};
    pep::modules::project_design::recompute_and_validate(active_block(), impl_->blocks,
                                                         impl_->connections, widgets);
  };

  auto set_active = [this, sync_active_to_form, recompute_and_validate](int id) {
    impl_->active_block_id = id;
    sync_active_to_form();
    recompute_and_validate();
  };

  auto add_block = [this, refresh_canvas, refresh_connection_form, refresh_ports_combo, set_active,
                    recompute_and_validate, refresh_connections_ui,
                    refresh_power_source_combo](Block block) {
    const int id = block.id;
    const std::optional<int> prev_active = impl_->active_block_id;
    impl_->blocks.push_back(std::move(block));
    pep::modules::project_design::connect_new_block(impl_->connections, impl_->blocks,
                                                    impl_->pending_connection, prev_active, id);
    impl_->pending_connection.reset();

    refresh_connection_form();
    refresh_power_source_combo();
    refresh_ports_combo(impl_->conn_from_block, impl_->conn_from_port);
    refresh_ports_combo(impl_->conn_to_block, impl_->conn_to_port);
    set_active(id);
    refresh_connections_ui();
    recompute_and_validate();
    refresh_canvas();
  };

  auto refresh_connection_controls = [refresh_connection_form, refresh_ports_combo, conn_from_block,
                                      conn_from_port, conn_to_block, conn_to_port]() {
    refresh_connection_form();
    refresh_ports_combo(conn_from_block, conn_from_port);
    refresh_ports_combo(conn_to_block, conn_to_port);
  };

  auto add_power_block = [this, add_block]() {
    const int id = impl_->next_block_id++;
    add_block(make_power_block(id, QString("Zasilanie #%1").arg(id).toStdString(),
                               BlockVariant::PsuSymmetric));
  };
  auto add_amp_block = [this, add_block]() {
    const int id = impl_->next_block_id++;
    add_block(make_amp_model1b_block(id, QString("Wzmacniacz 1(B) #%1").arg(id).toStdString()));
  };

  auto delete_selected_block = [this, refresh_connections_ui, refresh_connection_controls,
                                set_active, refresh_canvas, recompute_and_validate,
                                refresh_power_source_combo]() {
    pep::modules::project_design::delete_selected_block(
        impl_->scene, impl_->blocks, impl_->connections, impl_->active_block_id,
        refresh_connections_ui, refresh_power_source_combo, refresh_connection_controls, set_active,
        refresh_canvas, recompute_and_validate);
  };

  auto handle_scene_selection = [this, set_active]() {
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
  };

  auto handle_auto_layout = [this, refresh_canvas]() {
    impl_->manual_layout = false;
    refresh_canvas();
  };

  auto handle_port_clicked = [this, refresh_connections_ui, recompute_and_validate,
                              refresh_canvas](int block_id, const std::string &port_id) {
    pep::modules::project_design::handle_port_click(
        impl_->pending_connection, impl_->validation, impl_->connections, block_id, port_id,
        refresh_connections_ui, recompute_and_validate, refresh_canvas);
  };

  auto handle_variant_changed = [=]() {
    sync_form_to_active();
    refresh_connection_controls();
    refresh_connections_ui();
    recompute_and_validate();
  };

  auto handle_power_input_changed = [=]() {
    sync_form_to_active();
    recompute_and_validate();
  };

  auto handle_extra_rails_changed = [=, this]() {
    sync_form_to_active();
    refresh_connection_controls();
    refresh_connections_ui();
    recompute_and_validate();
    refresh_canvas();
  };

  auto handle_amp_power_source_changed = [=, this]() {
    if (impl_->refreshing_canvas) {
      return;
    }
    if (impl_->syncing_active_to_form) {
      return;
    }
    Block *active = active_block();
    if (!active || active->kind == BlockKind::Power) {
      return;
    }

    const int psu_id = impl_->amp_power_source->currentData().toInt();
    pep::modules::project_design::remap_block_power_connections(impl_->blocks, impl_->connections,
                                                                active->id, psu_id);

    refresh_connections_ui();
    refresh_connection_controls();
    recompute_and_validate();
    refresh_canvas();
  };

  auto handle_amp_waveform_changed = [=, this]() {
    sync_form_to_active();
    recompute_and_validate();
  };
  auto handle_amp_amp_changed = [=, this]() {
    sync_form_to_active();
    recompute_and_validate();
  };
  auto handle_amp_freq_changed = [=, this]() {
    sync_form_to_active();
    recompute_and_validate();
  };
  auto handle_amp_gain_changed = [=, this]() {
    sync_form_to_active();
    recompute_and_validate();
  };

  auto handle_add_connection = [=, this]() {
    if (!pep::modules::project_design::add_connection_from_form(impl_->blocks, impl_->connections,
                                                                conn_from_block, conn_from_port,
                                                                conn_to_block, conn_to_port)) {
      return;
    }
    refresh_connections_ui();
    recompute_and_validate();
  };

  auto handle_remove_connection = [=, this]() {
    if (!pep::modules::project_design::remove_selected_connection(impl_->connections,
                                                                  impl_->connections_list)) {
      return;
    }
    refresh_connections_ui();
    recompute_and_validate();
  };

  auto handle_export_active = [=, this]() {
    pep::modules::project_design::export_active_block(this, impl_->active_block_id, impl_->blocks,
                                                      impl_->connections);
  };

  auto handle_export_project = [=, this]() {
    export_ltspice_asc(this, impl_->blocks, impl_->connections);
  };

  bind_widget_interactions(WidgetBindings{this,
                                          scene,
                                          view,
                                          btn_add_power,
                                          btn_add_amp,
                                          btn_auto_layout,
                                          add_conn,
                                          remove_conn,
                                          export_active,
                                          export_project,
                                          variant,
                                          impl_->amp_waveform,
                                          impl_->amp_power_source,
                                          conn_from_block,
                                          conn_from_port,
                                          conn_to_block,
                                          conn_to_port,
                                          vin_input,
                                          freq_input,
                                          current_input,
                                          cap_input,
                                          impl_->extra_rails_input,
                                          impl_->amp_amp_input,
                                          impl_->amp_freq_input,
                                          impl_->amp_gain_input,
                                          add_power_block,
                                          add_amp_block,
                                          delete_selected_block,
                                          handle_auto_layout,
                                          handle_port_clicked,
                                          handle_scene_selection,
                                          handle_variant_changed,
                                          handle_power_input_changed,
                                          handle_extra_rails_changed,
                                          handle_amp_power_source_changed,
                                          handle_amp_waveform_changed,
                                          handle_amp_amp_changed,
                                          handle_amp_freq_changed,
                                          handle_amp_gain_changed,
                                          refresh_ports_combo,
                                          handle_add_connection,
                                          handle_remove_connection,
                                          handle_export_active,
                                          handle_export_project});

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
