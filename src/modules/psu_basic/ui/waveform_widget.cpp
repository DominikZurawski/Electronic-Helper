#include "waveform_widget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <cmath>

namespace pep::modules::psu_basic {

WaveformWidget::WaveformWidget(QWidget* parent) : QWidget(parent) {
  setMinimumHeight(220);
}

void WaveformWidget::set_params(const WaveformParams& params) {
  params_ = params;
  update();
}

static double value_for_phase(double phase, const WaveformParams& p) {
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

void WaveformWidget::paintEvent(QPaintEvent*) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  painter.fillRect(rect(), palette().base());

  const int left = 40;
  const int right = width() - 10;
  const int top = 10;
  const int bottom = height() - 30;

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
  painter.drawText(6, top + 12, "V");
  painter.drawText(right - 10, bottom + 20, "t");

  const double amplitude = params_.vpeak > 0.0 ? params_.vpeak : 1.0;
  const bool bipolar = (params_.type == WaveformType::Sinus) || (params_.type == WaveformType::Square) ||
                       (params_.type == WaveformType::Triangle);
  const double vmin = bipolar ? -amplitude : 0.0;
  const double vmax = (params_.type == WaveformType::Filtered) ? params_.vrect : amplitude;

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
