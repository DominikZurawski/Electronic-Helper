#pragma once

#include <vector>

class QComboBox;
class QListWidget;

namespace pep::modules::project_design {

struct Block;
struct Connection;

void refresh_connection_form(const std::vector<Block> &blocks, QComboBox *conn_from_block,
                             QComboBox *conn_to_block);

void refresh_ports_combo(const std::vector<Block> &blocks, QComboBox *block_combo,
                         QComboBox *port_combo);

void refresh_connections_ui(const std::vector<Block> &blocks,
                            const std::vector<Connection> &connections, QListWidget *list);

bool add_connection_from_form(const std::vector<Block> &blocks,
                              std::vector<Connection> &connections, QComboBox *conn_from_block,
                              QComboBox *conn_from_port, QComboBox *conn_to_block,
                              QComboBox *conn_to_port);

bool remove_selected_connection(std::vector<Connection> &connections, QListWidget *list);

} // namespace pep::modules::project_design
