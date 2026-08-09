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

void LightFlasher::SetPattern(const std::vector<Pattern>& pattern, bool loop) {
  pattern_ = pattern;
  loop_ = loop;
  Reset();
}

void LightFlasher::Reset(int idx) {
  int idx_count = static_cast<int>(pattern_.size());
  if (idx_count == 0) {
    current_pattern_idx_ = 0;
    pattern_start_time_ms_ = 0;
    current_ratio_ = 0.0;
  } else {
    int idx_target = idx;
    if (idx < 0) {
      idx_target = idx_count + idx;
    }
    idx_target = std::clamp(idx_target, 0, idx_count);
    current_pattern_idx_ = static_cast<size_t>(idx_target);
    pattern_start_time_ms_ = 0;
    current_ratio_ = pattern_[current_pattern_idx_].first;
  }
}

void LightFlasher::Update(uint32_t current_time_ms) {
  if (pattern_.empty()) {
    current_ratio_ = 0.0f;
    return;
  }
  // Return the last state if not looping.
  if (!loop_ && current_pattern_idx_ == pattern_.size() - 1) {
    current_ratio_ = pattern_[current_pattern_idx_].first;
    return;
  }
  // Start pattern.
  if (pattern_start_time_ms_ == 0) {
    pattern_start_time_ms_ = current_time_ms;
    current_ratio_ = pattern_[0].first;
    return;
  }
  // Progress pattern.
  const uint32_t elapsed_ms = current_time_ms - pattern_start_time_ms_;
  uint32_t accumulated_ms = 0;
  for (size_t i = 0; i < pattern_.size(); ++i) {
    accumulated_ms += pattern_[i].second;
    if (elapsed_ms < accumulated_ms) {
      current_pattern_idx_ = i;
      current_ratio_ = pattern_[i].first;
      return;
    }
  }
  // Next loop.
  if (loop_) {
    pattern_start_time_ms_ = current_time_ms;
    current_pattern_idx_ = 0;
    current_ratio_ = pattern_[0].first;
  } else {
    current_pattern_idx_ = pattern_.size() - 1;
    current_ratio_ = pattern_[pattern_.size() - 1].first;
  }
}

}  // namespace rdb
