#pragma once

#include <vector>

class QWidget;

namespace pep::modules::project_design {

struct Block;
struct Connection;

void export_ltspice_asc(QWidget *parent, const std::vector<Block> &blocks,
                        const std::vector<Connection> &connections);

} // namespace pep::modules::project_design
