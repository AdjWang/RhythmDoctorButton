#include "light.h"

#include <algorithm>

#include <zephyr/sys/printk.h>

namespace rdb {

Light::Light(const pwm_dt_spec& pwm, bool flip) : pwm_(pwm), flip_(flip) {}

void Light::Begin() {
  if (!pwm_is_ready_dt(&pwm_)) {
    printk("PWM device is not ready\n");
    return;
  }

  SetBrightness(0.0f);
}

void Light::SetBrightness(float val) {
  brightness_ = val;
  float ratio = std::clamp(val, 0.0f, 1.0f);
  if (flip_) {
    ratio = 1.0f - ratio;
  }

  const auto pulse = static_cast<uint32_t>(pwm_.period * ratio);
  const int ret = pwm_set_pulse_dt(&pwm_, pulse);
  if (ret != 0) {
    printk("Failed to set PWM pulse (%d)\n", ret);
  }
}

void LightFlasher::SetPattern(const std::vector<Pattern>& pattern) {
  pattern_ = pattern;
  Reset();
}

void LightFlasher::Reset() {
  current_pattern_index_ = 0;
  pattern_start_time_ms_ = 0;
  current_brightness_ = 0.0f;
}

bool LightFlasher::Update(uint32_t current_time_ms) {
  if (pattern_.empty()) {
    current_brightness_ = 0.0f;
    return current_brightness_;
  }
  // Start pattern.
  if (pattern_start_time_ms_ == 0) {
    pattern_start_time_ms_ = current_time_ms;
    current_brightness_ = pattern_[0].first;
    return current_brightness_;
  }
  // Progress pattern.
  const uint32_t elapsed_ms = current_time_ms - pattern_start_time_ms_;
  uint32_t accumulated_ms = 0;
  for (size_t i = 0; i < pattern_.size(); ++i) {
    accumulated_ms += pattern_[i].second;
    if (elapsed_ms < accumulated_ms) {
      current_pattern_index_ = i;
      current_brightness_ = pattern_[i].first;
      return current_brightness_;
    }
  }
  pattern_start_time_ms_ = current_time_ms;
  current_pattern_index_ = 0;
  current_brightness_ = pattern_[0].first;
  return current_brightness_;
}

}  // namespace rdb
