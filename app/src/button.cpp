#include "button.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

namespace rdb {

Button::Button(const gpio_dt_spec& gpio) : gpio_(gpio) {}

void Button::Begin() {
  if (!gpio_is_ready_dt(&gpio_)) {
    printk("Button GPIO is not ready\n");
    return;
  }

  const int ret = gpio_pin_configure_dt(&gpio_, GPIO_INPUT);
  if (ret != 0) {
    printk("Failed to configure button GPIO (%d)\n", ret);
    return;
  }

  state_ = gpio_pin_get_dt(&gpio_);
  last_state_ = state_;
  last_pressed_ = pressed();
}

void Button::Update() {
  const int reading = gpio_pin_get_dt(&gpio_);
  if (reading < 0) {
    return;
  }

  const uint32_t now_ms = static_cast<uint32_t>(k_uptime_get());
  if (last_state_ != reading) {
    last_debounce_time_ms_ = now_ms;
  }

  if (now_ms - last_debounce_time_ms_ >= kDebounceDelayMs) {
    state_ = reading;
    const bool is_pressed = pressed();
    if (last_pressed_ != is_pressed) {
      if (is_pressed && on_press_ != nullptr) {
        on_press_();
      }
      if (!is_pressed && on_release_ != nullptr) {
        on_release_();
      }
      last_pressed_ = is_pressed;
    }
  }

  last_state_ = reading;
}

}  // namespace rdb
