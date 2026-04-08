#include "export.hpp"

#include "../model/model.hpp"

#include "../../psu_basic/export/template_export.hpp"

#include "pep/ltspice_template.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pep::modules::project_design {

namespace {

struct Dsu {
  std::vector<int> parent;
  std::vector<int> rank;

  explicit Dsu(int n) : parent(n), rank(n, 0) {
    for (int i = 0; i < n; ++i) {
      parent[i] = i;
    }
  }

  int find(int x) {
    if (parent[x] == x) {
      return x;
    }
    parent[x] = find(parent[x]);
    return parent[x];
  }

  void unite(int a, int b) {
    a = find(a);
    b = find(b);
    if (a == b) {
      return;
    }
    if (rank[a] < rank[b]) {
      std::swap(a, b);
    }
    parent[b] = a;
    if (rank[a] == rank[b]) {
      ++rank[a];
    }
  }
};

std::string load_psu_symmetric_template(QString &label_out) {
  QSettings settings("PomocnikProjektantaElektronika", "PPE");
  QString template_path = settings.value("ltspice/templates/psu_symmetric_asc", "").toString();
  if (template_path.isEmpty()) {
    template_path = settings.value("ltspice/template_asc", "").toString();
  }

  if (!template_path.isEmpty()) {
    QFile f(template_path);
    if (f.open(QIODevice::ReadOnly)) {
      label_out = template_path;
      return f.readAll().toStdString();
    }
  }

  QFile resource_file(":/ltspice/power.asc");
  if (resource_file.open(QIODevice::ReadOnly)) {
    label_out = "wbudowany (resource)";
    return resource_file.readAll().toStdString();
  }

  label_out = "brak";
  return "";
}

std::string load_amp_model1b_template(QString &label_out) {
  QSettings settings("PomocnikProjektantaElektronika", "PPE");
  QString template_path = settings.value("ltspice/templates/amp_model1b_asc", "").toString();

  if (!template_path.isEmpty()) {
    QFile f(template_path);
    if (f.open(QIODevice::ReadOnly)) {
      label_out = template_path;
      return f.readAll().toStdString();
    }
  }

  QFile resource_file(":/ltspice/opamp_model1b.asc");
  if (resource_file.open(QIODevice::ReadOnly)) {
    label_out = "wbudowany (resource)";
    return resource_file.readAll().toStdString();
  }

  label_out = "brak";
  return "";
}

std::string strip_header(const std::string &asc) {
  std::ostringstream out;
  std::istringstream in(asc);
  std::string line;
  bool first = true;
  while (std::getline(in, line)) {
    if (line.rfind("Version", 0) == 0) {
      continue;
    }
    if (line.rfind("SHEET", 0) == 0) {
      continue;
    }
    if (!first) {
      out << "\n";
    }
    out << line;
    first = false;
  }
  return out.str();
}

struct AscBounds {
  int min_x = std::numeric_limits<int>::max();
  int max_x = std::numeric_limits<int>::min();
  int min_y = std::numeric_limits<int>::max();
  int max_y = std::numeric_limits<int>::min();

  void add(int x, int y) {
    min_x = std::min(min_x, x);
    max_x = std::max(max_x, x);
    min_y = std::min(min_y, y);
    max_y = std::max(max_y, y);
  }

  bool valid() const { return min_x <= max_x && min_y <= max_y; }
};

AscBounds asc_bounds(const std::string &asc, bool include_text) {
  AscBounds b;
  std::istringstream in(asc);
  std::string line;
  while (std::getline(in, line)) {
    std::istringstream iss(line);
    std::string head;
    iss >> head;

    if (head == "WIRE") {
      int x1, y1, x2, y2;
      if (iss >> x1 >> y1 >> x2 >> y2) {
        b.add(x1, y1);
        b.add(x2, y2);
      }
      continue;
    }

    if (head == "FLAG") {
      int x, y;
      std::string net;
      if (iss >> x >> y >> net) {
        b.add(x, y);
      }
      continue;
    }

    if (head == "SYMBOL") {
      std::string sym;
      int x, y;
      if (iss >> sym >> x >> y) {
        b.add(x, y);
      }
      continue;
    }

    if (include_text && head == "TEXT") {
      int x, y;
      if (iss >> x >> y) {
        b.add(x, y);
      }
      continue;
    }
  }
  return b;
}

std::string shift_coords(const std::string &asc, int dx, int dy) {
  std::istringstream in(asc);
  std::ostringstream out;
  std::string line;
  bool first = true;

  auto append_line = [&](const std::string &l) {
    if (!first) {
      out << "\n";
    }
    out << l;
    first = false;
  };

  while (std::getline(in, line)) {
    std::istringstream iss(line);
    std::string head;
    iss >> head;

    if (head == "WIRE") {
      int x1, y1, x2, y2;
      if (iss >> x1 >> y1 >> x2 >> y2) {
        x1 += dx;
        y1 += dy;
        x2 += dx;
        y2 += dy;
        append_line("WIRE " + std::to_string(x1) + " " + std::to_string(y1) + " " +
                    std::to_string(x2) + " " + std::to_string(y2));
        continue;
      }
    } else if (head == "FLAG") {
      int x, y;
      std::string net;
      if (iss >> x >> y >> net) {
        x += dx;
        y += dy;
        append_line("FLAG " + std::to_string(x) + " " + std::to_string(y) + " " + net);
        continue;
      }
    } else if (head == "SYMBOL") {
      std::string sym;
      int x, y;
      std::string rest;
      if (iss >> sym >> x >> y) {
        std::getline(iss, rest);
        x += dx;
        y += dy;
        std::string rebuilt =
            "SYMBOL " + sym + " " + std::to_string(x) + " " + std::to_string(y) + rest;
        append_line(rebuilt);
        continue;
      }
    } else if (head == "TEXT") {
      int x, y;
      std::string rest;
      if (iss >> x >> y) {
        std::getline(iss, rest);
        x += dx;
        y += dy;
        append_line("TEXT " + std::to_string(x) + " " + std::to_string(y) + rest);
        continue;
      }
    }

    append_line(line);
  }

  return out.str();
}

std::string uniquify_instnames(const std::string &asc, const std::string &suffix) {
  if (suffix.empty()) {
    return asc;
  }
  std::istringstream in(asc);
  std::ostringstream out;
  std::string line;
  bool first = true;
  std::unordered_map<std::string, std::string> inst_map;

  auto apply_suffix = [&](const std::string &name) -> std::string {
    if (name.size() >= suffix.size() && name.rfind(suffix) == name.size() - suffix.size()) {
      return name;
    }
    return name + suffix;
  };

  std::vector<std::string> lines;
  while (std::getline(in, line)) {
    lines.push_back(line);
  }

  for (const auto &l : lines) {
    if (l.rfind("SYMATTR InstName ", 0) == 0) {
      const std::string name = l.substr(std::string("SYMATTR InstName ").size());
      inst_map.emplace(name, apply_suffix(name));
    }
  }

  for (auto &l : lines) {
    if (l.rfind("SYMATTR InstName ", 0) == 0) {
      const std::string name = l.substr(std::string("SYMATTR InstName ").size());
      l = "SYMATTR InstName " + apply_suffix(name);
    } else if (l.rfind("TEXT ", 0) == 0) {
      const auto bang = l.find('!');
      if (bang != std::string::npos && bang + 1 < l.size()) {
        const std::string directive = l.substr(bang + 1);
        if (!directive.empty() && (directive[0] == 'K' || directive[0] == 'k')) {
          std::istringstream iss(directive);
          std::vector<std::string> toks;
          std::string tok;
          while (iss >> tok) {
            toks.push_back(tok);
          }
          if (toks.size() >= 4) {
            toks[0] = apply_suffix(toks[0]);
            for (std::size_t i = 1; i + 1 < toks.size(); ++i) {
              const auto it = inst_map.find(toks[i]);
              if (it != inst_map.end()) {
                toks[i] = it->second;
              }
            }
            std::ostringstream rebuilt;
            for (std::size_t i = 0; i < toks.size(); ++i) {
              if (i) {
                rebuilt << " ";
              }
              rebuilt << toks[i];
            }
            l = l.substr(0, bang + 1) + rebuilt.str();
          }
        }
      }
    }
    if (!first) {
      out << "\n";
    }
    out << l;
    first = false;
  }
  return out.str();
}

std::string uniquify_flag_nets(const std::string &asc, const std::string &suffix,
                               const std::unordered_set<std::string> &reserved_nets) {
  if (suffix.empty()) {
    return asc;
  }
  std::istringstream in(asc);
  std::ostringstream out;
  std::string line;
  bool first = true;

  auto apply_suffix = [&](const std::string &name) -> std::string {
    if (name.size() >= suffix.size() && name.rfind(suffix) == name.size() - suffix.size()) {
      return name;
    }
    return name + suffix;
  };

  while (std::getline(in, line)) {
    if (line.rfind("FLAG ", 0) == 0) {
      std::istringstream iss(line);
      std::string head;
      int x, y;
      std::string net;
      if ((iss >> head >> x >> y >> net)) {
        if (reserved_nets.find(net) == reserved_nets.end() && net != "0") {
          net = apply_suffix(net);
          line = "FLAG " + std::to_string(x) + " " + std::to_string(y) + " " + net;
        }
      }
    }
    if (!first) {
      out << "\n";
    }
    out << line;
    first = false;
  }
  return out.str();
}

pep::LtspiceAscPatchResult patch_flags(const std::string &asc,
                                       const std::unordered_map<long long, std::string> &xy_to_net,
                                       std::vector<std::string> &warnings) {
  // key = (x<<32) ^ (y & 0xffffffff)
  pep::LtspiceAscPatchResult result;
  result.asc = asc;
  auto lines = [&]() {
    std::vector<std::string> out;
    std::istringstream in(asc);
    std::string line;
    while (std::getline(in, line)) {
      out.push_back(line);
    }
    return out;
  }();

  std::set<long long> patched;

  for (auto &line : lines) {
    std::istringstream iss(line);
    std::string head;
    iss >> head;
    if (head != "FLAG") {
      continue;
    }
    int x, y;
    std::string net;
    if (!(iss >> x >> y >> net)) {
      continue;
    }
    const long long key = (static_cast<long long>(x) << 32) ^ (static_cast<unsigned int>(y));
    const auto it = xy_to_net.find(key);
    if (it == xy_to_net.end()) {
      continue;
    }
    line = "FLAG " + std::to_string(x) + " " + std::to_string(y) + " " + it->second;
    patched.insert(key);
  }

  for (const auto &[key, _] : xy_to_net) {
    if (patched.find(key) == patched.end()) {
      warnings.push_back("Nie znaleziono FLAG do podmiany netu (x,y).");
    }
  }

  std::ostringstream out;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    out << lines[i];
    if (i + 1 != lines.size()) {
      out << "\n";
    }
  }
  result.asc = out.str();
  return result;
}

} // namespace

std::optional<std::string> prompt_tran_directive(QWidget *parent, bool has_amplifier) {
  QDialog dialog(parent);
  dialog.setWindowTitle("Eksport LTspice — symulacja (.tran)");
  dialog.setMinimumSize(760, 520);
  dialog.resize(760, 520);

  auto *layout = new QVBoxLayout(&dialog);

  auto *expl = new QLabel(&dialog);
  expl->setWordWrap(true);
  expl->setTextFormat(Qt::RichText);
  expl->setText(
      "<div style='font-size:14px; font-weight:600; margin:0 0 4px 0;'>Do czego służy "
      "<code>.tran</code>?</div>"
      "<div style='margin:0 0 10px 0;'>"
      "Analiza czasowa (transient) — pokazuje przebiegi w funkcji czasu "
      "(np. rozruch zasilania, tętnienia, odpowiedź wzmacniacza)."
      "</div>"
      "<div style='font-size:14px; font-weight:600; margin:0 0 4px 0;'>Składnia</div>"
      "<div style='margin:0 0 8px 0; padding:8px; background:#f6f6f6; border:1px solid #dedede;'>"
      "<code>.tran &lt;Tstep&gt; &lt;Tstop&gt; [Tstart [dTmax]]</code>"
      "</div>"
      "<div style='margin:0 0 6px 0;'>Znaczenie parametrów (w kolejności jak w składni):</div>"
      "<ul style='margin:0 0 10px 18px;'>"
      "<li><code>Tstep</code> — krok próbkowania dla wykresu (często 0 lub pomijany)</li>"
      "<li><code>Tstop</code> — czas końcowy symulacji (wymagany)</li>"
      "<li><code>Tstart</code> — od kiedy zapisywać dane (pomija początkowy rozruch)</li>"
      "<li><code>dTmax</code> — maksymalny krok całkowania (ogranicza „skakanie” solvera)</li>"
      "</ul>"
      "<div style='margin:0;'>Możesz pominąć <code>.tran</code> w eksporcie i dopisać ją później w "
      "LTspice.</div>");
  layout->addWidget(expl);

  auto *include = new QCheckBox("Dodaj dyrektywę .tran do eksportu", &dialog);
  include->setChecked(true);
  layout->addWidget(include);

  auto *form = new QFormLayout();
  auto *preset = new QComboBox(&dialog);
  preset->addItem("Automatycznie (dobór domyślny)", "auto");
  preset->addItem("Zasilacz (tętnienia / stan ustalony)", "psu");
  preset->addItem("Audio / wzmacniacz (krótka symulacja)", "audio");
  preset->addItem("Własne (ręcznie)", "custom");

  auto *custom = new QLineEdit(&dialog);
  custom->setPlaceholderText(".tran 0 10m 0 1u  (przykład)");

  auto *preview = new QLabel(&dialog);
  preview->setWordWrap(true);
  preview->setStyleSheet("color: #333; padding:6px; background:#fafafa; border:1px solid #e3e3e3;");
  preview->setTextFormat(Qt::RichText);

  auto *warn = new QLabel(&dialog);
  warn->setWordWrap(true);
  warn->setStyleSheet("color: #b00020;");

  form->addRow("Propozycja", preset);
  form->addRow("Dyrektywa", custom);
  form->addRow("Podgląd", preview);
  form->addRow("", warn);
  layout->addLayout(form);

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  buttons->button(QDialogButtonBox::Ok)->setText("Eksportuj");
  buttons->button(QDialogButtonBox::Cancel)->setText("Anuluj");
  layout->addWidget(buttons);

  auto compute = [&]() -> QString {
    const QString p = preset->currentData().toString();
    if (p == "psu") {
      return ".tran 0 2500m 2460m 10u";
    }
    if (p == "audio") {
      return ".tran 0 20m 0 1u";
    }
    if (p == "auto") {
      return has_amplifier ? ".tran 0 20m 0 1u" : ".tran 0 2500m 2460m 10u";
    }
    return custom->text().trimmed();
  };

  auto refresh = [&]() {
    const bool on = include->isChecked();
    preset->setEnabled(on);
    const bool is_custom = preset->currentData().toString() == "custom";
    custom->setEnabled(on && is_custom);
    if (!is_custom && on) {
      custom->setText(compute());
    }

    if (!on) {
      preview->setText("Brak .tran (eksport bez dyrektywy).");
      warn->setText("");
      buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
      return;
    }

    const QString dir = compute();
    preview->setText("<code>" + dir.toHtmlEscaped() + "</code>");
    if (dir.isEmpty()) {
      warn->setText("Podaj dyrektywę .tran albo odznacz dodawanie .tran.");
      buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
      return;
    }
    if (!dir.startsWith(".tran", Qt::CaseInsensitive)) {
      warn->setText("Dyrektywa musi zaczynać się od .tran");
      buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
      return;
    }
    warn->setText("");
    buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
  };

  QObject::connect(include, &QCheckBox::toggled, &dialog, refresh);
  QObject::connect(preset, &QComboBox::currentIndexChanged, &dialog, refresh);
  QObject::connect(custom, &QLineEdit::textChanged, &dialog, refresh);
  QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  preset->setCurrentIndex(has_amplifier ? preset->findData("audio") : preset->findData("psu"));
  refresh();

  if (dialog.exec() != QDialog::Accepted) {
    return std::nullopt;
  }
  if (!include->isChecked()) {
    return std::nullopt;
  }
  return compute().toStdString();
}

AscAssembly export_project_asc(const std::vector<Block> &blocks,
                               const std::vector<Connection> &connections,
                               const std::optional<std::string> &tran_directive) {
  AscAssembly out;

  // Build endpoint index (only for exported ports that have FLAG coordinates).
  struct EpInfo {
    int idx = -1;
    Endpoint ep;
    PortType type = PortType::Unknown;
  };

  std::vector<EpInfo> endpoints;
  endpoints.reserve(32);

  auto endpoint_key = [](const Endpoint &e) {
    return QString("%1:%2").arg(e.block_id).arg(e.port_id);
  };

  std::unordered_map<std::string, int> ep_to_index;

  auto add_endpoint = [&](const Block &b, const PortDef &p) {
    Endpoint ep{b.id, p.id};
    const std::string key = endpoint_key(ep).toStdString();
    if (ep_to_index.find(key) != ep_to_index.end()) {
      return;
    }
    const int idx = static_cast<int>(endpoints.size());
    endpoints.push_back({idx, ep, p.type});
    ep_to_index.emplace(key, idx);
  };

  for (const auto &b : blocks) {
    for (const auto &p : ports_for(b)) {
      // Only endpoints that can participate in net-unification for export.
      if (p.type == PortType::Ground) {
        continue;
      }
      add_endpoint(b, p);
    }
  }

  Dsu dsu(static_cast<int>(endpoints.size()));

  for (const auto &c : connections) {
    const auto ka = endpoint_key(c.a).toStdString();
    const auto kb = endpoint_key(c.b).toStdString();
    const auto ita = ep_to_index.find(ka);
    const auto itb = ep_to_index.find(kb);
    if (ita == ep_to_index.end() || itb == ep_to_index.end()) {
      continue;
    }
    dsu.unite(ita->second, itb->second);
  }

  // Assign readable net names per DSU group (must stay unique to avoid accidental shorts).
  std::unordered_map<int, std::vector<EpInfo>> group_to_eps;
  group_to_eps.reserve(endpoints.size());
  for (const auto &ep : endpoints) {
    group_to_eps[dsu.find(ep.idx)].push_back(ep);
  }

  std::unordered_map<int, std::string> group_to_net;
  std::unordered_map<std::string, int> base_counters;

  auto base_for_group = [&](const std::vector<EpInfo> &eps) -> std::string {
    bool has_vcc = false;
    bool has_vee = false;
    bool has_in = false;
    bool has_out = false;
    for (const auto &e : eps) {
      if (e.type == PortType::PowerPos) {
        has_vcc = true;
      }
      if (e.type == PortType::PowerNeg) {
        has_vee = true;
      }
      if (e.type == PortType::AnalogIn) {
        has_in = true;
      }
      if (e.type == PortType::AnalogOut) {
        has_out = true;
      }
    }
    if (has_vcc && has_vee) {
      out.warnings.push_back(
          "Sprzeczne typy w jednej sieci (Vcc i Vee) — nadaję nazwę automatyczną.");
      return "NET";
    }
    if (has_vcc) {
      return "Vcc";
    }
    if (has_vee) {
      return "Vee";
    }
    if (has_in && has_out) {
      return "SIG";
    }
    if (has_in) {
      return "IN";
    }
    if (has_out) {
      return "OUT";
    }
    return "NET";
  };

  auto reserve_name = [&](const std::string &base) -> std::string {
    int &k = base_counters[base];
    if (k == 0) {
      k = 1;
      return base;
    }
    ++k;
    return base + std::to_string(k);
  };

  for (const auto &[root, eps] : group_to_eps) {
    const std::string base = base_for_group(eps);
    if (eps.size() == 1) {
      const int bid = eps.front().ep.block_id;
      group_to_net.emplace(root, reserve_name(base + "_B" + std::to_string(bid)));
    } else {
      group_to_net.emplace(root, reserve_name(base));
    }
  }

  auto net_for_endpoint = [&](const Endpoint &e) -> std::string {
    const auto it = ep_to_index.find(endpoint_key(e).toStdString());
    if (it == ep_to_index.end()) {
      return "";
    }
    const int root = dsu.find(it->second);
    return group_to_net[root];
  };

  // Start output.
  std::vector<std::string> lines;
  lines.push_back("Version 4.1");
  lines.push_back("SHEET 1 3200 1800"); // patched at the end based on content

  std::set<std::string> seen_directives;

  const int gap_x = 180;
  const int gap_y = 160;
  const int pad_x = 60;
  const int pad_y = 60;
  int global_max_x = 0;
  int global_max_y = 0;

  // We insert a single .tran later (or none), so strip all .tran directives from fragments.

  // Group blocks into connected components (even power-only ties count) and place each component on
  // its own row.
  std::unordered_map<int, int> id_to_idx;
  id_to_idx.reserve(blocks.size());
  for (int i = 0; i < static_cast<int>(blocks.size()); ++i) {
    id_to_idx.emplace(blocks[i].id, i);
  }

  std::vector<std::vector<int>> comp_adj(blocks.size());
  for (const auto &c : connections) {
    const auto ia = id_to_idx.find(c.a.block_id);
    const auto ib = id_to_idx.find(c.b.block_id);
    if (ia == id_to_idx.end() || ib == id_to_idx.end()) {
      continue;
    }
    comp_adj[ia->second].push_back(ib->second);
    comp_adj[ib->second].push_back(ia->second);
  }

  std::vector<int> comp(blocks.size(), -1);
  int comp_count = 0;
  for (int i = 0; i < static_cast<int>(blocks.size()); ++i) {
    if (comp[i] != -1) {
      continue;
    }
    std::vector<int> stack = {i};
    comp[i] = comp_count;
    while (!stack.empty()) {
      const int v = stack.back();
      stack.pop_back();
      for (const int u : comp_adj[v]) {
        if (comp[u] == -1) {
          comp[u] = comp_count;
          stack.push_back(u);
        }
      }
    }
    ++comp_count;
  }

  std::vector<std::vector<int>> comps(comp_count);
  for (int i = 0; i < static_cast<int>(blocks.size()); ++i) {
    comps[comp[i]].push_back(i);
  }

  auto port_type_of = [&](int block_idx, const QString &port_id) -> PortType {
    const auto p = find_port(blocks[block_idx], port_id);
    return p.has_value() ? p->type : PortType::Unknown;
  };

  auto is_power_type = [](PortType t) {
    return t == PortType::PowerPos || t == PortType::PowerNeg || t == PortType::Ground;
  };

  // Prepare traversal edges for "logical" ordering within a component.
  std::vector<std::vector<int>> fwd(blocks.size());
  auto add_fwd = [&](int a, int b) { fwd[a].push_back(b); };
  for (const auto &c : connections) {
    const auto ia = id_to_idx.find(c.a.block_id);
    const auto ib = id_to_idx.find(c.b.block_id);
    if (ia == id_to_idx.end() || ib == id_to_idx.end()) {
      continue;
    }
    const int a = ia->second;
    const int b = ib->second;
    const PortType ta = port_type_of(a, c.a.port_id);
    const PortType tb = port_type_of(b, c.b.port_id);

    if (is_power_type(ta) && is_power_type(tb)) {
      // Power: direction PSU -> consumer if possible, otherwise undirected.
      const bool a_psu = blocks[a].kind == "power";
      const bool b_psu = blocks[b].kind == "power";
      if (a_psu && !b_psu) {
        add_fwd(a, b);
      } else if (b_psu && !a_psu) {
        add_fwd(b, a);
      } else {
        add_fwd(a, b);
        add_fwd(b, a);
      }
      continue;
    }

    // Signal: AnalogOut -> AnalogIn if detected.
    if (ta == PortType::AnalogOut && tb == PortType::AnalogIn) {
      add_fwd(a, b);
      continue;
    }
    if (tb == PortType::AnalogOut && ta == PortType::AnalogIn) {
      add_fwd(b, a);
      continue;
    }

    add_fwd(a, b);
    add_fwd(b, a);
  }

  // Stable neighbor ordering.
  for (auto &v : fwd) {
    std::sort(v.begin(), v.end(),
              [&](int lhs, int rhs) { return blocks[lhs].id < blocks[rhs].id; });
    v.erase(std::unique(v.begin(), v.end()), v.end());
  }

  auto component_order = [&](const std::vector<int> &indices) -> std::vector<int> {
    std::vector<int> order;
    order.reserve(indices.size());
    std::unordered_map<int, bool> in_comp;
    in_comp.reserve(indices.size());
    for (int idx : indices) {
      in_comp.emplace(idx, true);
    }

    std::unordered_map<int, bool> vis;
    vis.reserve(indices.size());
    auto visit_from = [&](int start) {
      std::vector<int> st = {start};
      while (!st.empty()) {
        const int v = st.back();
        st.pop_back();
        if (vis[v]) {
          continue;
        }
        vis[v] = true;
        order.push_back(v);
        for (const int u : fwd[v]) {
          if (!in_comp[u]) {
            continue;
          }
          if (!vis[u]) {
            st.push_back(u);
          }
        }
      }
    };

    std::vector<int> starts;
    for (int idx : indices) {
      if (blocks[idx].kind == "power") {
        starts.push_back(idx);
      }
    }
    std::sort(starts.begin(), starts.end(),
              [&](int lhs, int rhs) { return blocks[lhs].id < blocks[rhs].id; });
    for (int s : starts) {
      visit_from(s);
    }

    std::vector<int> rest = indices;
    std::sort(rest.begin(), rest.end(),
              [&](int lhs, int rhs) { return blocks[lhs].id < blocks[rhs].id; });
    for (int s : rest) {
      visit_from(s);
    }

    return order;
  };

  int row_y = 0;

  auto export_fragment = [&](int dx, int dy, int &row_cursor_x, int &row_max_h,
                             const std::string &asc_with_header,
                             const std::unordered_map<long long, std::string> &xy_to_net,
                             const std::string &inst_suffix) {
    std::vector<std::string> flag_warnings;
    auto patched_flags = patch_flags(asc_with_header, xy_to_net, flag_warnings);
    out.warnings.insert(out.warnings.end(), flag_warnings.begin(), flag_warnings.end());

    std::unordered_set<std::string> reserved_nets;
    reserved_nets.insert("0");
    for (const auto &kv : xy_to_net) {
      reserved_nets.insert(kv.second);
    }
    const std::string flags_unique =
        uniquify_flag_nets(patched_flags.asc, inst_suffix, reserved_nets);

    // LTspice requires unique instance names within one schematic.
    const std::string with_unique_inst = uniquify_instnames(flags_unique, inst_suffix);

    const auto bounds = asc_bounds(with_unique_inst, /*include_text=*/false);
    int local_dx = dx;
    int local_dy = dy;
    int w = 700;
    int h = 600;
    if (bounds.valid()) {
      local_dx = dx - bounds.min_x + pad_x;
      local_dy = dy - bounds.min_y + pad_y;
      w = (bounds.max_x - bounds.min_x) + 2 * pad_x;
      h = (bounds.max_y - bounds.min_y) + 2 * pad_y;
    }
    std::string fragment = strip_header(with_unique_inst);
    fragment = shift_coords(fragment, local_dx, local_dy);

    row_cursor_x += (w + gap_x);
    row_max_h = std::max(row_max_h, h);
    const auto shifted_bounds = asc_bounds(fragment, /*include_text=*/true);
    if (shifted_bounds.valid()) {
      global_max_x = std::max(global_max_x, shifted_bounds.max_x);
      global_max_y = std::max(global_max_y, shifted_bounds.max_y);
    } else {
      global_max_x = std::max(global_max_x, local_dx + pad_x + w);
      global_max_y = std::max(global_max_y, local_dy + pad_y + h);
    }

    std::istringstream in(fragment);
    std::string line;
    while (std::getline(in, line)) {
      if (line.rfind("TEXT ", 0) == 0) {
        const auto bang = line.find('!');
        if (bang != std::string::npos && bang + 1 < line.size()) {
          const std::string directive = line.substr(bang + 1);
          if (!directive.empty() && directive.front() == '.') {
            if (directive.rfind(".tran", 0) == 0) {
              continue;
            }
            if (seen_directives.find(directive) != seen_directives.end()) {
              continue;
            }
            seen_directives.insert(directive);
          }
        }
      }
      lines.push_back(line);
    }
  };

  for (const auto &indices : comps) {
    const auto order = component_order(indices);
    int row_cursor_x = 0;
    int row_max_h = 220;

    for (const int bi : order) {
      const auto &b = blocks[bi];
      const int dx = row_cursor_x;
      const int dy = row_y;

      if (b.kind == "power") {
        if (b.variant != "psu_symmetric") {
          const int nx = row_cursor_x + pad_x;
          const int ny = row_y + pad_y;
          std::ostringstream note;
          note << "TEXT " << nx << " " << ny << " Left 2 !PPE: brak schematu dla wariantu \""
               << b.variant.toStdString() << "\"";
          lines.push_back(note.str());
          out.warnings.push_back("Brak szablonu LTspice dla wariantu: " + b.variant.toStdString());
          row_cursor_x += (700 + gap_x);
          row_max_h = std::max(row_max_h, 220);
          global_max_x = std::max(global_max_x, nx + 700);
          global_max_y = std::max(global_max_y, ny + 200);
          continue;
        }

        QString template_label;
        std::string tpl = load_psu_symmetric_template(template_label);
        if (tpl.empty()) {
          out.warnings.push_back("Nie udało się wczytać szablonu zasilania symetrycznego.");
          row_cursor_x += (700 + gap_x);
          row_max_h = std::max(row_max_h, 220);
          continue;
        }

        auto patched_values = pep::modules::psu_basic::export_schematic_from_asc_template(
            b.vin_ac_rms, b.mains_hz, b.load_current, b.capacitor_uF, tpl);
        out.warnings.insert(out.warnings.end(), patched_values.warnings.begin(),
                            patched_values.warnings.end());

        std::unordered_map<long long, std::string> xy_to_net;
        for (const auto &p : ports_for(b)) {
          if (p.type == PortType::Ground) {
            continue;
          }
          Endpoint ep{b.id, p.id};
          const std::string net = net_for_endpoint(ep);
          if (net.empty()) {
            continue;
          }
          for (const auto &flag : p.flags) {
            const long long key =
                (static_cast<long long>(flag.x) << 32) ^ (static_cast<unsigned int>(flag.y));
            xy_to_net.emplace(key, net);
          }
        }

        export_fragment(dx, dy, row_cursor_x, row_max_h, patched_values.asc, xy_to_net,
                        "_B" + std::to_string(b.id));
        continue;
      }

      if (b.kind == "amplifier") {
        QString template_label;
        std::string tpl = load_amp_model1b_template(template_label);
        if (tpl.empty()) {
          const int nx = row_cursor_x + pad_x;
          const int ny = row_y + pad_y;
          std::ostringstream note;
          note << "TEXT " << nx << " " << ny << " Left 2 !PPE: brak schematu dla elementu \""
               << b.title.toStdString() << "\"";
          lines.push_back(note.str());
          out.warnings.push_back("Brak szablonu LTspice dla elementu: " + b.title.toStdString());
          row_cursor_x += (700 + gap_x);
          row_max_h = std::max(row_max_h, 220);
          global_max_x = std::max(global_max_x, nx + 700);
          global_max_y = std::max(global_max_y, ny + 200);
          continue;
        }

        auto fmt = [](double v, int precision) {
          std::ostringstream out;
          out.setf(std::ios::fixed);
          out << std::setprecision(precision) << v;
          std::string s = out.str();
          while (s.size() > 1 && s.find('.') != std::string::npos && s.back() == '0') {
            s.pop_back();
          }
          if (!s.empty() && s.back() == '.') {
            s.pop_back();
          }
          return s;
        };

        std::unordered_map<std::string, std::string> inst_values;
        const double amp = b.signal_amp_v;
        const double hz = b.signal_hz;
        if (amp > 0.0 && hz > 0.0) {
          if (b.signal_waveform == "square") {
            const double t = 1.0 / hz;
            const double ton = 0.5 * t;
            inst_values.emplace("V2", "PULSE(" + fmt(-amp, 6) + " " + fmt(amp, 6) + " 0 1u 1u " +
                                          fmt(ton, 9) + " " + fmt(t, 9) + ")");
          } else if (b.signal_waveform == "triangle") {
            const double t = 1.0 / hz;
            inst_values.emplace("V2", "PWL(0 0 " + fmt(0.25 * t, 9) + " " + fmt(amp, 6) + " " +
                                          fmt(0.75 * t, 9) + " " + fmt(-amp, 6) + " " + fmt(t, 9) +
                                          " 0)");
          } else {
            inst_values.emplace("V2", "SINE(0 " + fmt(amp, 6) + " " + fmt(hz, 3) + ")");
          }
        } else {
          out.warnings.push_back("Brak parametrów wejścia wzmacniacza — nie nadpisuję V2.");
        }

        auto patched_values = pep::ltspice_patch_asc_values(tpl, inst_values);
        out.warnings.insert(out.warnings.end(), patched_values.warnings.begin(),
                            patched_values.warnings.end());

        std::unordered_map<long long, std::string> xy_to_net;
        for (const auto &p : ports_for(b)) {
          if (p.type == PortType::Ground) {
            continue;
          }
          Endpoint ep{b.id, p.id};
          const std::string net = net_for_endpoint(ep);
          if (net.empty()) {
            continue;
          }
          for (const auto &flag : p.flags) {
            const long long key =
                (static_cast<long long>(flag.x) << 32) ^ (static_cast<unsigned int>(flag.y));
            xy_to_net.emplace(key, net);
          }
        }

        export_fragment(dx, dy, row_cursor_x, row_max_h, patched_values.asc, xy_to_net,
                        "_B" + std::to_string(b.id));
        continue;
      }

      const int nx = row_cursor_x + pad_x;
      const int ny = row_y + pad_y;
      std::ostringstream note;
      note << "TEXT " << nx << " " << ny << " Left 2 !PPE: brak schematu dla elementu \""
           << b.title.toStdString() << "\"";
      lines.push_back(note.str());
      out.warnings.push_back("Brak szablonu LTspice dla elementu: " + b.title.toStdString());
      row_cursor_x += (700 + gap_x);
      row_max_h = std::max(row_max_h, 220);
      global_max_x = std::max(global_max_x, nx + 700);
      global_max_y = std::max(global_max_y, ny + 200);
    }

    row_y += row_max_h + gap_y;
  }

  // Patch sheet size based on content.
  const int sheet_w = std::max(1416, global_max_x + 400);
  const int sheet_h = std::max(1140, global_max_y + 400);
  lines[1] = "SHEET 1 " + std::to_string(sheet_w) + " " + std::to_string(sheet_h);

  if (tran_directive.has_value() && !tran_directive->empty()) {
    // Place the directive near the top-left, outside typical fragments.
    lines.push_back("TEXT 40 40 Left 2 !" + *tran_directive);
  }

  std::ostringstream result;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    result << lines[i];
    if (i + 1 != lines.size()) {
      result << "\n";
    }
  }
  out.asc = result.str();
  return out;
}

} // namespace pep::modules::project_design
