#pragma once

#include <QWidget>

namespace pep::modules::psu_basic {

enum class WaveformType {
  Sinus,
  Square,
  Triangle,
  HalfWave,
  FullWave,
  Filtered
};

struct WaveformParams {
  double vpeak = 0.0;
  double vrect = 0.0;
  double ripple_vpp = 0.0;
  double mains_hz = 50.0;
  WaveformType type = WaveformType::Sinus;
};

class WaveformWidget : public QWidget {
public:
  explicit WaveformWidget(QWidget* parent = nullptr);

  void set_params(const WaveformParams& params);

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  WaveformParams params_;
};

} // namespace pep::modules::psu_basic
