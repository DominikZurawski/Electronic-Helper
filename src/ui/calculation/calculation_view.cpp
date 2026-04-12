#include "calculation_view.hpp"

#include "calculation_html_renderer.hpp"

#include <QApplication>
#include <QColor>
#include <QFrame>
#include <QLabel>
#include <QPalette>
#include <QSizePolicy>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTextDocument>
#include <QUrl>
#include <QVBoxLayout>
#include <QShowEvent>

#if defined(PPE_HAS_QT_WEBENGINE)
#include <QWebEngineView>
#endif

namespace pep::ui::calculation {

namespace {

QString to_qstring(const std::string &value) {
  return QString::fromStdString(value);
}

QString build_validation_tooltip(const CalculationRenderResult &result) {
  return QString::fromStdString(format_validation_summary(result.validation_issues));
}

QString build_validation_banner_text(const std::vector<CalculationValidationIssue> &issues) {
  return QString::fromStdString(
      format_validation_summary(issues, "Status dokumentu obliczen"));
}

CalculationTheme theme_for_palette(const QPalette &palette) {
  if (qApp) {
    const QVariant dark_mode = qApp->property("ppe.dark_mode");
    if (dark_mode.isValid()) {
      return dark_mode.toBool() ? CalculationTheme::Dark : CalculationTheme::Light;
    }
  }
  const QColor window = palette.color(QPalette::Window);
  return window.lightness() < 128 ? CalculationTheme::Dark : CalculationTheme::Light;
}

QString build_validation_banner_style(const CalculationValidationSummary &summary) {
  if (summary.error_count > 0) {
    return "QLabel#calculationValidationStatus {"
           " padding: 10px 12px;"
           " border-radius: 10px;"
           " background-color: rgba(120, 35, 35, 0.26);"
           " border: 1px solid rgba(210, 90, 90, 0.60);"
           " color: palette(window-text);"
           " }";
  }

  return "QLabel#calculationValidationStatus {"
         " padding: 10px 12px;"
         " border-radius: 10px;"
         " background-color: rgba(120, 70, 20, 0.22);"
         " border: 1px solid rgba(210, 150, 80, 0.55);"
         " color: palette(window-text);"
         " }";
}

} // namespace

CalculationView::CalculationView(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  auto *status_label = new QLabel(this);
  status_label->setWordWrap(true);
  status_label->setVisible(false);
  status_label->setObjectName("calculationValidationStatus");
  status_label->setStyleSheet(build_validation_banner_style(CalculationValidationSummary{}));
  status_label_ = status_label;
  layout->addWidget(status_label);

#if defined(PPE_HAS_QT_WEBENGINE)
  auto *web_view = new QWebEngineView(this);
  web_view->setContextMenuPolicy(Qt::NoContextMenu);
  web_view->page()->setBackgroundColor(palette().color(QPalette::Window));
  web_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  web_view_ = web_view;
  QObject::connect(web_view, &QWebEngineView::loadFinished, this, [this](bool) {
    if (!web_view_ || !pending_scroll_.has_value()) {
      return;
    }
    const double scroll = *pending_scroll_;
    pending_scroll_.reset();
    web_view_->page()->runJavaScript(
        QString("window.scrollTo(0, %1);").arg(scroll, 0, 'f', 0));
  });
  layout->addWidget(web_view);
#else
  auto *text_browser = new QTextBrowser(this);
  text_browser->setOpenExternalLinks(false);
  text_browser->setReadOnly(true);
  text_browser->setFrameShape(QFrame::NoFrame);
  text_browser->document()->setDocumentMargin(0);
  text_browser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  fallback_view_ = text_browser;
  layout->addWidget(text_browser);
#endif
}

void CalculationView::set_document(const CalculationDocument &document) {
  document_ = document;
  has_document_ = true;
  if (active_section_index_ < 0) {
    active_section_index_ = 0;
  }
  rerender();
}

void CalculationView::set_active_section_index(int index) {
  if (index < 0 || active_section_index_ == index) {
    return;
  }
  active_section_index_ = index;
  if (has_document_) {
    rerender();
  }
}

const std::vector<CalculationValidationIssue> &CalculationView::validation_issues() const {
  return validation_issues_;
}

void CalculationView::showEvent(QShowEvent *event) {
  QWidget::showEvent(event);
  if (has_document_) {
    rerender();
  }
}

void CalculationView::changeEvent(QEvent *event) {
  QWidget::changeEvent(event);
  if (!has_document_) {
    return;
  }

  if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange ||
      event->type() == QEvent::StyleChange) {
    rerender();
  }
}

void CalculationView::set_html(const QString &html) {
#if defined(PPE_HAS_QT_WEBENGINE)
  auto *web_view = findChild<QWebEngineView *>();
  if (web_view) {
    web_view->page()->setBackgroundColor(palette().color(QPalette::Window));
    web_view->setHtml(html, QUrl("qrc:/"));
  }
#else
  if (fallback_view_) {
    fallback_view_->setHtml(html);
  }
#endif
}

void CalculationView::update_validation_status() {
  if (!status_label_) {
    return;
  }

  if (validation_issues_.empty()) {
    status_label_->clear();
    status_label_->setVisible(false);
    return;
  }

  status_label_->setStyleSheet(
      build_validation_banner_style(summarize_validation_issues(validation_issues_)));
  status_label_->setText(build_validation_banner_text(validation_issues_));
  status_label_->setVisible(true);
}

void CalculationView::rerender() {
#if defined(PPE_HAS_QT_WEBENGINE)
  if (web_view_) {
    const int token = ++render_token_;
    web_view_->page()->runJavaScript(QStringLiteral("window.scrollY"),
                                     [this, token](const QVariant &value) {
                                       if (token != render_token_) {
                                         return;
                                       }
                                       const double scroll = value.isValid() ? value.toDouble() : 0.0;
                                       pending_scroll_ = scroll;
                                       const auto rendered = render_calculation_document(
                                           document_, theme_for_palette(palette()),
                                           active_section_index_);
                                       validation_issues_ = rendered.validation_issues;
                                       setToolTip(build_validation_tooltip(rendered));
                                       update_validation_status();
                                       set_html(to_qstring(rendered.html));
                                     });
    return;
  }
#endif
  int scroll_pos = 0;
  if (fallback_view_ && fallback_view_->verticalScrollBar()) {
    scroll_pos = fallback_view_->verticalScrollBar()->value();
  }
  const auto rendered = render_calculation_document(document_, theme_for_palette(palette()),
                                                    active_section_index_);
  validation_issues_ = rendered.validation_issues;
  setToolTip(build_validation_tooltip(rendered));
  update_validation_status();
  set_html(to_qstring(rendered.html));
  if (fallback_view_ && fallback_view_->verticalScrollBar()) {
    fallback_view_->verticalScrollBar()->setValue(scroll_pos);
  }
}

} // namespace pep::ui::calculation
