#include "waveform_widget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <cmath>

namespace pep::modules::psu_basic {

WaveformWidget::WaveformWidget(QWidget *parent) : QWidget(parent) {
  setMinimumHeight(220);
}

void WaveformWidget::set_params(const WaveformParams &params) {
  params_ = params;
  update();
}

static double value_for_phase(double phase, const WaveformParams &p) {
  const double s = qSin(phase);
  switch (p.type) {
  case WaveformType::Sinus:
    return p.vpeak * s;
  case WaveformType::Square:
    return p.vpeak * (s >= 0.0 ? 1.0 : -1.0);
  case WaveformType::Triangle: {
    const double tri = (2.0 / M_PI) * qAsin(qSin(phase));
    return p.vpeak * tri;
  }
  case WaveformType::HalfWave:
    return s > 0.0 ? p.vpeak * s : 0.0;
  case WaveformType::FullWave:
    return p.vpeak * qAbs(s);
  case WaveformType::Filtered: {
    const double ripple = p.ripple_vpp > 0.0 ? p.ripple_vpp : 0.0;
    const double t = std::fmod(phase, 2.0 * M_PI) / (2.0 * M_PI);
    return p.vrect - ripple * t;
  }
  }
  return 0.0;
}

void WaveformWidget::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  painter.fillRect(rect(), palette().base());

  const double amplitude = params_.vpeak > 0.0 ? params_.vpeak : 1.0;
  const bool bipolar = (params_.type == WaveformType::Sinus) ||
                       (params_.type == WaveformType::Square) ||
                       (params_.type == WaveformType::Triangle);
  const double vmin = bipolar ? -amplitude : 0.0;
  const double vmax = (params_.type == WaveformType::Filtered) ? params_.vrect : amplitude;

  auto fmt_v = [](double v) { return QString::number(v, 'f', (qAbs(v) < 10.0 ? 2 : 1)) + " V"; };
  auto fmt_t = [](double s) {
    if (s >= 1.0) {
      return QString::number(s, 'f', 2) + " s";
    }
    if (s >= 0.01) {
      return QString::number(s * 1000.0, 'f', 1) + " ms";
    }
    return QString::number(s * 1000.0, 'f', 2) + " ms";
  };

  QFontMetrics fm(painter.font());
  const double vmid = 0.5 * (vmax + vmin);
  const QString y_max = fmt_v(vmax);
  const QString y_mid = fmt_v(vmid);
  const QString y_min = fmt_v(vmin);
  const int y_label_w = std::max({fm.horizontalAdvance(y_max), fm.horizontalAdvance(y_mid),
                                  fm.horizontalAdvance(y_min), fm.horizontalAdvance("V")}) +
                        8;

  const double hz = params_.mains_hz > 0.0 ? params_.mains_hz : 1.0;
  const double period = 1.0 / hz;
  const QString t0 = "0";
  const QString t1 = fmt_t(period);
  const QString t2 = fmt_t(2.0 * period);
  const int x_label_w = std::max({fm.horizontalAdvance(t0), fm.horizontalAdvance(t1),
                                  fm.horizontalAdvance(t2), fm.horizontalAdvance("t")}) +
                        8;

  const int left = 8 + y_label_w;
  const int right = width() - 10;
  const int top = 12 + fm.height();
  const int bottom = height() - (fm.height() + 10);

  painter.setPen(palette().mid().color());
  for (int i = 0; i <= 4; ++i) {
    const int y = top + (bottom - top) * i / 4;
    painter.drawLine(left, y, right, y);
  }
  for (int i = 0; i <= 6; ++i) {
    const int x = left + (right - left) * i / 6;
    painter.drawLine(x, top, x, bottom);
  }

  painter.setPen(palette().text().color());
  painter.drawText(QRect(2, top - fm.height(), y_label_w, fm.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, "V");
  painter.drawText(QRect(right - x_label_w + 6, bottom + 2, x_label_w, fm.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, "t");

  auto draw_y = [&](int y, const QString &text) {
    painter.drawText(QRect(2, y - fm.height() / 2, y_label_w, fm.height()),
                     Qt::AlignRight | Qt::AlignVCenter, text);
  };
  draw_y(top, y_max);
  draw_y(top + (bottom - top) / 2, y_mid);
  draw_y(bottom, y_min);

  const int x_label_y = bottom + 2;
  const int span = right - left;
  painter.drawText(QRect(left - x_label_w / 2, x_label_y, x_label_w, fm.height()),
                   Qt::AlignHCenter | Qt::AlignVCenter, t0);
  if (span > (x_label_w * 3)) {
    painter.drawText(QRect(left + span / 2 - x_label_w / 2, x_label_y, x_label_w, fm.height()),
                     Qt::AlignHCenter | Qt::AlignVCenter, t1);
    painter.drawText(QRect(right - x_label_w / 2, x_label_y, x_label_w, fm.height()),
                     Qt::AlignHCenter | Qt::AlignVCenter, t2);
  } else {
    painter.drawText(QRect(right - x_label_w / 2, x_label_y, x_label_w, fm.height()),
                     Qt::AlignHCenter | Qt::AlignVCenter, t1);
  }

  QPainterPath path;
  const int samples = 300;
  for (int i = 0; i <= samples; ++i) {
    const double t = static_cast<double>(i) / samples;
    const double phase = t * 4.0 * M_PI;
    const double v = value_for_phase(phase, params_);
    const double y_norm = (v - vmin) / (vmax - vmin + 1e-9);
    const int x = left + static_cast<int>((right - left) * t);
    const int y = bottom - static_cast<int>((bottom - top) * y_norm);
    if (i == 0) {
      path.moveTo(x, y);
    } else {
      path.lineTo(x, y);
    }
  }

  painter.setPen(QPen(palette().highlight().color(), 2));
  painter.drawPath(path);
}

} // namespace pep::modules::psu_basic
