#ifndef APP_SRC_LIGHT_H_
#define APP_SRC_LIGHT_H_

#include <cstdint>
#include <utility>
#include <vector>

#include <zephyr/drivers/pwm.h>

namespace rdb {

class Light {
 public:
  Light(const pwm_dt_spec& pwm, bool flip = false);

  float brightness() const { return brightness_; }
  void Begin();
  void SetBrightness(float val);

 private:
  pwm_dt_spec pwm_;
  bool flip_;
  float brightness_ = 0.0f;
};

class LightFlasher {
 public:
  using Pattern =
      std::pair<float /*brightness_ratio*/, uint32_t /*duration_ms*/>;

  float get_brightness_ratio() const { return current_ratio_; }
  void SetPattern(const std::vector<Pattern>& pattern, bool loop);
  // Input index can be lower than 0 to count from end to start.
  void Reset(int idx = 0);
  // Update frequency controls granularity.
  void Update(uint32_t current_time_ms);

 private:
  std::vector<Pattern> pattern_;
  bool loop_ = true;
  uint32_t pattern_start_time_ms_ = 0;
  size_t current_pattern_idx_ = 0;
  float current_ratio_ = 0.0f;
};

}  // namespace rdb

#endif  // APP_SRC_LIGHT_H_
