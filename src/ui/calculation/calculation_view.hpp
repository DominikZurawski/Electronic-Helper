#pragma once

#include "calculation_document.hpp"
#include "calculation_validation.hpp"

#include <QEvent>
#include <QWidget>

#include <optional>
#include <vector>

class QLabel;
class QTextBrowser;
#if defined(PPE_HAS_QT_WEBENGINE)
class QWebEngineView;
#endif

namespace pep::ui::calculation {

class CalculationView : public QWidget {
public:
  explicit CalculationView(QWidget *parent = nullptr);

  void set_document(const CalculationDocument &document);
  void set_active_section_index(int index);
  const std::vector<CalculationValidationIssue> &validation_issues() const;

private:
  QLabel *status_label_ = nullptr;
  QTextBrowser *fallback_view_ = nullptr;
#if defined(PPE_HAS_QT_WEBENGINE)
  QWebEngineView *web_view_ = nullptr;
  std::optional<double> pending_scroll_;
  int render_token_ = 0;
#endif
  std::vector<CalculationValidationIssue> validation_issues_;
  CalculationDocument document_;
  bool has_document_ = false;
  int active_section_index_ = 0;

  void showEvent(QShowEvent *event) override;
  void changeEvent(QEvent *event) override;
  void set_html(const QString &html);
  void update_validation_status();
  void rerender();
};

} // namespace pep::ui::calculation
