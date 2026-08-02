#ifndef APP_SRC_BUTTON_H_
#define APP_SRC_BUTTON_H_

#include <cstdint>
#include <functional>

#include <zephyr/drivers/gpio.h>

namespace rdb {

class Button {
 public:
  explicit Button(const gpio_dt_spec& gpio);

  bool pressed() const { return state_ == 1; }
  void set_on_press(std::function<void()> cb) { on_press_ = std::move(cb); }
  void set_on_release(std::function<void()> cb) { on_release_ = std::move(cb); }

  void Begin();
  void Update();

 private:
  static constexpr uint32_t kDebounceDelayMs = 10;

  gpio_dt_spec gpio_;
  int state_ = 0;
  int last_state_ = 0;
  uint32_t last_debounce_time_ms_ = 0;
  bool last_pressed_ = false;
  std::function<void()> on_press_;
  std::function<void()> on_release_;
};

}  // namespace rdb

#endif  // APP_SRC_BUTTON_H_
