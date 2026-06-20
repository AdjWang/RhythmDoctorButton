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
  using Pattern = std::pair<bool, uint32_t>;

  bool is_on() const { return current_state_; }
  void SetPattern(const std::vector<Pattern>& pattern);
  void Reset();
  bool Update(uint32_t current_time_ms);

 private:
  std::vector<Pattern> pattern_;
  uint32_t pattern_start_time_ms_ = 0;
  size_t current_pattern_index_ = 0;
  bool current_state_ = false;
};

}  // namespace rdb

#endif  // APP_SRC_LIGHT_H_
