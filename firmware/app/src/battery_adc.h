#ifndef APP_SRC_BATTERY_ADC_H_
#define APP_SRC_BATTERY_ADC_H_

#include <array>
#include <cstdint>

#include <zephyr/drivers/adc.h>

namespace rdb {

class BatteryAdc {
 public:
  explicit BatteryAdc(const adc_dt_spec& adc);

  uint8_t battery_level() const;
  void Begin();
  void Update();

 private:
  static constexpr size_t kBatterySampleCount = 20;
  static constexpr uint32_t kBatteryMinVoltageMv = 3700;
  static constexpr uint32_t kBatteryMaxVoltageMv = 4200;
  static constexpr uint32_t kBatterySampleFreqHz = 3;
  static constexpr float kSampleRatio = 220.0f / (220.0f + 330.0f);

  adc_dt_spec adc_;
  bool initialized_ = false;
  uint32_t last_sample_time_ms_ = 0;
  size_t sample_index_ = 0;
  std::array<uint8_t, kBatterySampleCount> battery_level_samples_{};

  uint8_t ReadBatteryLevel();
};

}  // namespace rdb

#endif  // APP_SRC_BATTERY_ADC_H_
