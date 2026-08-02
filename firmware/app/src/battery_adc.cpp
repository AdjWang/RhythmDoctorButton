#include "battery_adc.h"

#include <algorithm>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

namespace rdb {

BatteryAdc::BatteryAdc(const adc_dt_spec& adc) : adc_(adc) {}

uint8_t BatteryAdc::battery_level() const {
  uint32_t sum = 0;
  for (const uint8_t level : battery_level_samples_) {
    sum += level;
  }
  return static_cast<uint8_t>(sum / battery_level_samples_.size());
}

void BatteryAdc::Begin() {
  if (!adc_is_ready_dt(&adc_)) {
    printk("Battery ADC is not ready\n");
    return;
  }

  int ret = adc_channel_setup_dt(&adc_);
  if (ret != 0) {
    printk("Failed to configure battery ADC channel (%d)\n", ret);
    return;
  }

  initialized_ = true;
  for (uint8_t& sample : battery_level_samples_) {
    sample = ReadBatteryLevel();
  }
}

void BatteryAdc::Update() {
  if (!initialized_) {
    return;
  }

  const uint32_t now_ms = static_cast<uint32_t>(k_uptime_get());
  constexpr uint32_t kSamplePeriodMs = 1000 / kBatterySampleFreqHz;
  if (now_ms - last_sample_time_ms_ <= kSamplePeriodMs) {
    return;
  }

  last_sample_time_ms_ = now_ms;
  battery_level_samples_[sample_index_] = ReadBatteryLevel();
  sample_index_ = (sample_index_ + 1) % battery_level_samples_.size();
}

uint8_t BatteryAdc::ReadBatteryLevel() {
  int16_t raw_value = 0;
  adc_sequence sequence = {
      .buffer = &raw_value,
      .buffer_size = sizeof(raw_value),
  };

  int ret = adc_sequence_init_dt(&adc_, &sequence);
  if (ret != 0) {
    printk("Failed to initialize battery ADC sequence (%d)\n", ret);
    return 0;
  }

  ret = adc_read_dt(&adc_, &sequence);
  if (ret != 0) {
    printk("Battery ADC read failed (%d)\n", ret);
    return 0;
  }

  int32_t adc_mv = raw_value;
  ret = adc_raw_to_millivolts_dt(&adc_, &adc_mv);
  if (ret != 0) {
    printk("Battery ADC conversion failed (%d)\n", ret);
    return 0;
  }

  const auto battery_mv =
      static_cast<uint32_t>(static_cast<float>(adc_mv) / kSampleRatio);
  if (battery_mv < kBatteryMinVoltageMv) {
    return 0;
  }

  float ratio =
      static_cast<float>(battery_mv - kBatteryMinVoltageMv) /
      static_cast<float>(kBatteryMaxVoltageMv - kBatteryMinVoltageMv);
  ratio = std::clamp(ratio, 0.0f, 1.0f);
  return static_cast<uint8_t>(ratio * 100.0f);
}

}  // namespace rdb
