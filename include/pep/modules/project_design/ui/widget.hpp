#pragma once

#include <QWidget>

#include <memory>

namespace pep::modules::project_design {

class Widget : public QWidget {
public:
  explicit Widget(QWidget* parent = nullptr);
  ~Widget() override;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace pep::modules::project_design
